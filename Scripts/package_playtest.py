#!/usr/bin/env python3
"""Package Playtest Build — fresh Win64 package for packaged-build playtesting

Runs UAT BuildCookRun to build, cook, stage, pak and archive a Win64 package
into PackagedBuilds/Playtest, then records which commit it was built from in
PackagedBuilds/Playtest/build_info.json (read by playtest_boot.py's
staleness gate).

This script runs OUTSIDE the editor.  It invokes:
  <ENGINE>/Engine/Build/BatchFiles/RunUAT.bat BuildCookRun -project=<uproject>
      -noP4 -platform=Win64 -clientconfig=<config> -build -cook -stage -pak
      -archive -archivedirectory=<ROOT>/PackagedBuilds/Playtest -unattended
      -utf8output [-iterate] -map=<maps> [-CookAll -CookMapsOnly]

Default maps: /Game/System/FrontEnd/Maps/L_LyraFrontEnd (the real player
entry point) + /MordecaiCore/Game/Maps/DevTestMap.

--all-maps adds -CookAll -CookMapsOnly (every map in the project plus what it
references) and still passes the default maps explicitly.  UAT has no -allmaps
switch: BuildCookRun silently ignores unknown switches, and the cooker would
then fall back to the MapsToCook list in DefaultGame.ini, which here is
the same two maps.

-iterate: in UE 5.7 the cooker maps -iterate onto the incremental cook.
--clean leaves it out, so every package is recooked.

Before UAT runs, the script deletes the previous build_info.json and the old
PackagedBuilds/Playtest/Windows folder.  UAT's archive step only copies files
on top of whatever is already there.  Without the delete, binaries and paks
from an earlier package would survive next to a fresh build_info.json, for
example a Development LyraGame.exe left behind by a DebugGame package.

Usage:
  py Scripts/package_playtest.py                   # Development, iterative cook, default maps
  py Scripts/package_playtest.py --clean           # full cook (no -iterate)
  py Scripts/package_playtest.py --all-maps        # cook every map (-CookAll -CookMapsOnly)
  py Scripts/package_playtest.py --dry-run         # print the UAT command and exit 0
  py Scripts/package_playtest.py --config DebugGame  # also Shipping (strips dev features)

Outputs:
  PackagedBuilds/Playtest/Windows/LyraGame.exe   bootstrapper only
  PackagedBuilds/Playtest/Windows/ProjectMordecai/Binaries/Win64/LyraGame.exe   real game binary
  PackagedBuilds/Playtest/build_info.json        written only after a successful package
  Saved/Playtests/package-<YYYYmmdd-HHMMSS>.log  full UAT output

build_info.json fields:
  commit, branch            git HEAD sha and branch ("HEAD" when detached), captured BEFORE UAT
  dirty                     working tree had changes (tracked or untracked, minus .gitignore)
  dirty_files               first 50 paths, exactly as returned by dirty_files_from_git()
                            (git status --porcelain v1, repo-relative, forward slashes,
                            no status code, new path for renames, untracked dirs collapsed)
  dirty_count               full number of dirty paths
  dirty_fingerprint         sha256 of the dirty CONTENT (see compute_dirty_fingerprint());
                            equal fingerprints mean identical uncommitted changes, so a gate
                            should compare it rather than the file list alone
  source_changed_during_build
                            the git state at the end of the build differed from the start:
                            the package may contain changes that are not recorded here,
                            so treat it as STALE
  built_at, config, maps, all_maps, game_exe, uat_log

Exit codes:
  0 — package succeeded (or --dry-run)
  1 — UAT reported success but the expected game binary is missing
  2 — infrastructure error (uproject/RunUAT missing, git unavailable, old archive locked)
  N — UAT failed or was interrupted; exits with UAT's own non-zero exit code (130 on Ctrl+C)
"""

import argparse
import collections
import datetime
import hashlib
import json
import os
import queue
import shutil
import stat
import subprocess
import sys
import threading
import time

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)

ENGINE_DIR = os.path.join(os.path.dirname(PROJECT_ROOT), "UnrealEngine")
RUN_UAT = os.path.join(ENGINE_DIR, "Engine", "Build", "BatchFiles", "RunUAT.bat")
UPROJECT = os.path.join(PROJECT_ROOT, "ProjectMordecai.uproject")

ARCHIVE_DIR = os.path.join(PROJECT_ROOT, "PackagedBuilds", "Playtest")
BUILD_INFO = os.path.join(ARCHIVE_DIR, "build_info.json")
LOG_DIR = os.path.join(PROJECT_ROOT, "Saved", "Playtests")

# Packaged layout: <archive>/Windows/LyraGame.exe is only a bootstrapper; the
# real game binary lives under <archive>/Windows/ProjectMordecai/Binaries/Win64.
ARCHIVE_PLATFORM_DIR = os.path.join(ARCHIVE_DIR, "Windows")
PACKAGED_BIN_DIR = os.path.join(ARCHIVE_PLATFORM_DIR, "ProjectMordecai", "Binaries", "Win64")

DEFAULT_MAPS = [
    "/Game/System/FrontEnd/Maps/L_LyraFrontEnd",
    "/MordecaiCore/Game/Maps/DevTestMap",
]
# Real BuildCookRun switches (ProjectParams.CookAll / CookMapsOnly -> cooker -cookall -mapsonly).
ALL_MAPS_UAT_ARGS = ["-CookAll", "-CookMapsOnly"]
ALL_MAPS_MARKER = "ALL"

CONFIGS = ["Development", "DebugGame", "Shipping"]

FAIL_TAIL_LINES = 60
FAIL_MAX_ERROR_LINES = 100
MAX_DIRTY_FILES = 50

# Some UAT children can keep the output pipe open after RunUAT.bat exits, for
# example dotnet build-server or MSBuild worker nodes.  Once the process has
# exited, stop reading after this many seconds of silence instead of waiting
# for EOF forever.
PIPE_DRAIN_GRACE_SECONDS = 5.0
PIPE_DRAIN_MAX_SECONDS = 30.0


def log(msg):
    print(f"[Package-Playtest] {msg}", flush=True)


# ---------------------------------------------------------------------------
# Git
# ---------------------------------------------------------------------------
def git(*args):
    """Run a git command in the project root. Returns stdout as text. Raises on failure."""
    result = subprocess.run(
        ["git", "-c", "core.quotepath=off", *args],
        cwd=PROJECT_ROOT, capture_output=True, text=True,
        encoding="utf-8", errors="replace",
    )
    if result.returncode != 0:
        raise RuntimeError(f"git {' '.join(args)} failed ({result.returncode}): {result.stderr.strip()}")
    return result.stdout


def git_bytes(*args):
    """Run a git command in the project root. Returns raw stdout bytes. Raises on failure."""
    result = subprocess.run(
        ["git", "-c", "core.quotepath=off", *args],
        cwd=PROJECT_ROOT, capture_output=True,
    )
    if result.returncode != 0:
        raise RuntimeError(f"git {' '.join(args)} failed ({result.returncode}): "
                           f"{result.stderr.decode('utf-8', 'replace').strip()}")
    return result.stdout


def parse_porcelain_z(output):
    """Paths from `git status --porcelain=v1 -z` output.

    -z avoids git's C-style quoting, which applies to paths with spaces and
    other special characters.  For renames and copies, the entry is
    'XY new' followed by a separate 'old' entry.  This returns the new path
    and skips the old one.
    """
    entries = output.split("\0")
    paths = []
    i = 0
    while i < len(entries):
        entry = entries[i]
        i += 1
        if len(entry) < 4:
            continue
        status, path = entry[:2], entry[3:]
        if "R" in status or "C" in status:
            i += 1  # skip the original path of a rename/copy
        paths.append(path)
    return paths


def dirty_files_from_git():
    """Every dirty path (tracked changes + untracked, minus .gitignore'd), in git's order.

    Saved/, Intermediate/, PackagedBuilds/ etc. are ignored by .gitignore, so
    the package itself never makes the tree dirty.
    """
    return parse_porcelain_z(git("status", "--porcelain=v1", "-z"))


def compute_dirty_fingerprint():
    """sha256 over the uncommitted CONTENT of the working tree. Raises on git failure.

    The hash covers these bytes, in this order:
      1. the bytes "diff" and a NUL;
      2. the output of `git diff HEAD --binary --no-color --no-ext-diff --no-textconv`,
         which covers staged and unstaged changes and deletions of tracked files;
      3. for each path from `git ls-files --others --exclude-standard -z`, sorted as bytes:
         "untracked", NUL, the path bytes, NUL, the sha256 hex of the file bytes, NUL.
    The result changes if a file that was already dirty is edited again after
    the package is built.  A file-name comparison cannot catch that.
    To compare, playtest_boot.py should import and call this function rather
    than reimplement it.  Set sys.dont_write_bytecode = True before
    `import package_playtest`, or Scripts/__pycache__/ appears as an untracked
    directory and makes the tree look dirty.
    """
    h = hashlib.sha256()
    h.update(b"diff\0")
    h.update(git_bytes("diff", "HEAD", "--binary", "--no-color", "--no-ext-diff", "--no-textconv"))
    untracked = sorted(p for p in git_bytes("ls-files", "--others", "--exclude-standard", "-z").split(b"\0") if p)
    for rel in untracked:
        h.update(b"untracked\0" + rel + b"\0")
        full = os.path.join(PROJECT_ROOT, rel.decode("utf-8", "surrogateescape"))
        try:
            fh = hashlib.sha256()
            with open(full, "rb") as f:
                for chunk in iter(lambda: f.read(1 << 20), b""):
                    fh.update(chunk)
            h.update(fh.hexdigest().encode("ascii"))
        except OSError:
            h.update(b"<unreadable>")
        h.update(b"\0")
    return h.hexdigest()


def capture_git_state():
    """Return dict with commit, branch, dirty, dirty_files (full list), dirty_count,
    dirty_fingerprint. Raises on git failure."""
    commit = git("rev-parse", "HEAD").strip()
    branch = git("rev-parse", "--abbrev-ref", "HEAD").strip()
    dirty_files = dirty_files_from_git()
    return {
        "commit": commit,
        "branch": branch,
        "dirty": bool(dirty_files),
        "dirty_files": dirty_files,
        "dirty_count": len(dirty_files),
        "dirty_fingerprint": compute_dirty_fingerprint(),
    }


def git_state_changed(before, after):
    return (before["commit"] != after["commit"]
            or before["dirty_files"] != after["dirty_files"]
            or before["dirty_fingerprint"] != after["dirty_fingerprint"])


# ---------------------------------------------------------------------------
# UAT
# ---------------------------------------------------------------------------
def build_uat_command(config, maps, clean):
    cmd = [
        RUN_UAT, "BuildCookRun",
        f"-project={UPROJECT}",
        "-noP4",
        "-platform=Win64",
        f"-clientconfig={config}",
        "-build",
        "-cook",
        "-stage",
        "-pak",
        "-archive",
        f"-archivedirectory={ARCHIVE_DIR}",
        "-unattended",
        "-utf8output",
    ]
    if not clean:
        cmd.append("-iterate")
    # Always name the default maps so they are cooked even if the all-maps
    # depot search misses a plugin mount point.
    cmd.append("-map=" + "+".join(DEFAULT_MAPS if maps == [ALL_MAPS_MARKER] else maps))
    if maps == [ALL_MAPS_MARKER]:
        cmd.extend(ALL_MAPS_UAT_ARGS)
    return cmd


def kill_process_tree(pid):
    """Best effort: kill pid and all its children (cmd -> UAT -> UBT / cook commandlet)."""
    try:
        subprocess.run(["taskkill", "/PID", str(pid), "/T", "/F"],
                       capture_output=True, timeout=60)
    except (OSError, subprocess.SubprocessError):
        pass


def _pump_lines(stream, out_queue):
    """Reader thread: push raw lines from stream onto out_queue, then None at EOF."""
    try:
        for raw in iter(stream.readline, b""):
            out_queue.put(raw)
    except (OSError, ValueError):
        pass
    finally:
        out_queue.put(None)


def run_uat(cmd, log_path):
    """Stream UAT output to the console and log_path.

    Returns (exit_code, tail_lines, first_error_lines, error_line_count).
    The process tree is killed if this function is left early by Ctrl+C or an
    unexpected exception, so no UAT or cooker is left running in the background.
    """
    tail = collections.deque(maxlen=FAIL_TAIL_LINES)
    error_lines = []
    error_count = 0
    console_ok = True
    proc = None
    interrupted = False

    with open(log_path, "w", encoding="utf-8", newline="\n") as log_file:
        log_file.write(f"# {subprocess.list2cmdline(cmd)}\n")
        log_file.flush()
        try:
            # A .bat always runs under cmd.exe.  Start it explicitly with
            # "cmd /d /s /c "<line>"": /s strips only the outer quotes, so quoted
            # paths with spaces survive, and /d skips any AutoRun registry hook.
            comspec = os.environ.get("COMSPEC", "cmd.exe")
            command_line = f'"{comspec}" /d /s /c "{subprocess.list2cmdline(cmd)}"'
            proc = subprocess.Popen(
                command_line, cwd=PROJECT_ROOT,
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                stdin=subprocess.DEVNULL,
            )
            lines = queue.Queue()
            reader = threading.Thread(target=_pump_lines, args=(proc.stdout, lines), daemon=True)
            reader.start()

            exited_at = None
            last_line_at = time.monotonic()
            while True:
                try:
                    raw = lines.get(timeout=0.5)
                except queue.Empty:
                    if proc.poll() is not None:
                        now = time.monotonic()
                        if exited_at is None:
                            exited_at = now
                        if (now - last_line_at > PIPE_DRAIN_GRACE_SECONDS
                                or now - exited_at > PIPE_DRAIN_MAX_SECONDS):
                            log("UAT exited but a leftover child process still holds its output "
                                "pipe; stopped reading.")
                            break
                    continue
                if raw is None:
                    break
                last_line_at = time.monotonic()
                line = raw.decode("utf-8", errors="replace").rstrip("\r\n")
                if console_ok:
                    try:
                        print(line, flush=True)
                    except (OSError, ValueError):
                        console_ok = False  # e.g. the reading end of a pipe closed; keep logging
                log_file.write(line + "\n")
                tail.append(line)
                if "Error" in line:
                    error_count += 1
                    if len(error_lines) < FAIL_MAX_ERROR_LINES:
                        error_lines.append(line)
            code = proc.wait()
        except KeyboardInterrupt:
            interrupted = True
            log_file.write("# INTERRUPTED\n")
            try:
                log("Interrupted - stopping UAT...")
                code = proc.wait(timeout=10) if proc is not None else 130
            except (subprocess.TimeoutExpired, KeyboardInterrupt):
                code = 130
        finally:
            if proc is not None and proc.poll() is None:
                kill_process_tree(proc.pid)

    if interrupted:
        # An interrupted package is never a success, whatever UAT returned.
        code = code or 130
    return code, list(tail), error_lines, error_count


def expected_game_exe(config):
    exe_name = "LyraGame.exe" if config == "Development" else f"LyraGame-Win64-{config}.exe"
    return os.path.join(PACKAGED_BIN_DIR, exe_name)


def find_game_exes():
    found = []
    windows_dir = os.path.join(ARCHIVE_DIR, "Windows")
    for dirpath, _dirnames, filenames in os.walk(windows_dir):
        for name in filenames:
            if name.lower().startswith("lyragame") and name.lower().endswith(".exe"):
                found.append(os.path.join(dirpath, name))
    return found


def _rmtree_onexc(func, path, _exc):
    """rmtree error handler: clear the read-only bit and retry once (re-raises on failure)."""
    os.chmod(path, stat.S_IWRITE)
    func(path)


def clean_archive_platform_dir():
    """Delete <archive>/Windows so that no file from an earlier package survives into this one.

    UAT's Archive step (ApplyArchiveManifest) only copies files on top of the
    archive directory and never cleans it.  Raises OSError if a file is locked,
    for example when the packaged game is still running.
    """
    if not os.path.isdir(ARCHIVE_PLATFORM_DIR):
        return False
    if sys.version_info >= (3, 12):
        shutil.rmtree(ARCHIVE_PLATFORM_DIR, onexc=_rmtree_onexc)
    else:
        shutil.rmtree(ARCHIVE_PLATFORM_DIR, onerror=_rmtree_onexc)
    return True


FAILURE_HINTS = [
    ("Live Coding", "Live Coding is active in a running editor. Close the editor or press "
                    "Ctrl+Alt+F11 there and retry."),
    ("conflicting instance", "Another UnrealBuildTool is running, for example an editor build. "
                             "Wait for it to finish and retry."),
    ("Failed to delete staging directory", "A staged file is locked. Is a packaged game still running?"),
]


def print_failure(code, tail, error_lines, error_count, log_path):
    print("")
    print("=" * 60)
    print(f"PACKAGING FAILED (UAT exit code {code})")
    print("=" * 60)
    print(f"--- last {len(tail)} lines ---")
    for line in tail:
        print(f"    {line}")
    print(f"--- lines containing 'Error' ({error_count}) ---")
    for line in error_lines:
        print(f"    {line}")
    if error_count > len(error_lines):
        print(f"    ... {error_count - len(error_lines)} more in the log")
    seen_text = "\n".join(list(tail) + error_lines)
    for needle, hint in FAILURE_HINTS:
        if needle in seen_text:
            print(f"HINT: {hint}")
    print("")
    log(f"Full UAT log: {log_path}")
    log("build_info.json was NOT written, so playtest_boot.py will report STALE BUILD.")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    try:
        sys.stdout.reconfigure(errors="replace")
    except (AttributeError, ValueError):
        pass

    parser = argparse.ArgumentParser(description="Package a fresh Win64 playtest build")
    parser.add_argument("--clean", action="store_true",
                        help="Full cook (omit -iterate)")
    parser.add_argument("--all-maps", action="store_true",
                        help="Cook every map (-CookAll -CookMapsOnly) in addition to the front end + DevTestMap")
    parser.add_argument("--dry-run", action="store_true",
                        help="Print the UAT command and exit 0")
    parser.add_argument("--config", choices=CONFIGS, default="Development",
                        type=lambda s: {c.lower(): c for c in CONFIGS}.get(s.lower(), s),
                        help="Client config (default: Development)")
    args = parser.parse_args()

    maps = [ALL_MAPS_MARKER] if args.all_maps else list(DEFAULT_MAPS)
    cmd = build_uat_command(args.config, maps, args.clean)

    if args.config == "Shipping":
        log("WARNING: Shipping strips dev features (console, most logging, cheats, the "
            "agent bridge). playtest_boot.py expects a Development build.")
    elif args.config != "Development":
        log(f"WARNING: {args.config} binary is LyraGame-Win64-{args.config}.exe; "
            "pass it to playtest_boot.py with --exe.")

    # --- Step 1: Record the commit the package is built from (BEFORE UAT) ---
    try:
        git_state = capture_git_state()
    except (OSError, RuntimeError) as e:
        if args.dry_run:
            log(f"WARNING: could not read git state: {e}")
            git_state = None
        else:
            log(f"ERROR: could not read git state: {e}")
            sys.exit(2)

    if git_state:
        log(f"Commit: {git_state['commit']} ({git_state['branch']})")
        if git_state["dirty"]:
            log(f"WARNING: working tree is dirty ({git_state['dirty_count']} file(s)); the package "
                "includes uncommitted changes; playtest_boot.py will require the same dirty state "
                "or --allow-stale.")
            for path in git_state["dirty_files"][:10]:
                print(f"    {path}")
            if git_state["dirty_count"] > 10:
                print(f"    ... {git_state['dirty_count'] - 10} more")

    maps_desc = ("ALL (-CookAll -CookMapsOnly) + " + "+".join(DEFAULT_MAPS)) if args.all_maps else "+".join(maps)
    log(f"Config: {args.config}   Maps: {maps_desc}   "
        f"Cook: {'clean' if args.clean else 'iterative'}")
    log(f"Archive: {ARCHIVE_DIR}")

    if args.dry_run:
        if not os.path.isfile(RUN_UAT):
            log(f"WARNING: RunUAT.bat not found: {RUN_UAT}")
        if not os.path.isfile(UPROJECT):
            log(f"WARNING: uproject not found: {UPROJECT}")
        log(f"A real run first deletes {BUILD_INFO} and {ARCHIVE_PLATFORM_DIR}.")
        log("Dry run - UAT command:")
        print(subprocess.list2cmdline(cmd))
        sys.exit(0)

    # --- Step 2: Preflight ---
    if not os.path.isfile(UPROJECT):
        log(f"ERROR: uproject not found: {UPROJECT}")
        sys.exit(2)
    if not os.path.isfile(RUN_UAT):
        log(f"ERROR: RunUAT.bat not found: {RUN_UAT}")
        sys.exit(2)

    os.makedirs(LOG_DIR, exist_ok=True)
    os.makedirs(ARCHIVE_DIR, exist_ok=True)
    timestamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    log_path = os.path.join(LOG_DIR, f"package-{timestamp}.log")

    # A failed/partial package must not leave an old build_info.json describing
    # half-overwritten binaries; playtest_boot.py treats a missing file as STALE.
    try:
        if os.path.isfile(BUILD_INFO):
            os.remove(BUILD_INFO)
            log("Removed previous build_info.json (rewritten on success).")
        # UAT's archive step never cleans the archive dir, so binaries and paks from
        # an earlier package (another config, another chunk layout) would survive.
        if clean_archive_platform_dir():
            log(f"Removed previous archive: {ARCHIVE_PLATFORM_DIR}")
    except OSError as e:
        log(f"ERROR: could not clear the previous package: {e}")
        log("Is the packaged game still running (playtest_boot.py --keep-running)? Close it and retry.")
        sys.exit(2)

    # --- Step 3: Run UAT ---
    log("Running UAT BuildCookRun:")
    print(subprocess.list2cmdline(cmd))
    log(f"Log: {log_path}")
    print("")

    start = time.time()
    try:
        code, tail, error_lines, error_count = run_uat(cmd, log_path)
    except OSError as e:
        log(f"ERROR: running UAT failed: {e}")
        sys.exit(2)
    elapsed = time.time() - start

    if code != 0:
        print_failure(code, tail, error_lines, error_count, log_path)
        sys.exit(code)

    # --- Step 4: Verify the real game binary exists BEFORE vouching for the build ---
    game_exe = expected_game_exe(args.config)
    if not os.path.isfile(game_exe):
        print("")
        log(f"ERROR: UAT succeeded but the game binary is missing: {game_exe}")
        found = find_game_exes()
        if found:
            log("LyraGame*.exe found under the archive:")
            for path in found:
                print(f"    {path}")
        log("build_info.json was NOT written.")
        log(f"Full UAT log: {log_path}")
        sys.exit(1)

    # --- Step 5: Detect source changes made while UAT was running ---
    try:
        post_state = capture_git_state()
        changed_during_build = git_state_changed(git_state, post_state)
    except (OSError, RuntimeError) as e:
        log(f"WARNING: could not re-read git state after the build: {e}")
        changed_during_build = True
    if changed_during_build:
        log("WARNING: the git state changed while UAT was running (commit, dirty files or their "
            "content). The package may contain changes that build_info.json cannot describe. "
            "It is marked source_changed_during_build=true and dirty=true; re-run for a "
            "trustworthy build.")

    # --- Step 6: Record build_info.json (atomically) ---
    build_info = {
        "commit": git_state["commit"],
        "branch": git_state["branch"],
        "dirty": git_state["dirty"] or changed_during_build,
        "dirty_files": git_state["dirty_files"][:MAX_DIRTY_FILES],
        "dirty_count": git_state["dirty_count"],
        "dirty_fingerprint": git_state["dirty_fingerprint"],
        "source_changed_during_build": changed_during_build,
        "built_at": datetime.datetime.now().astimezone().isoformat(timespec="seconds"),
        "config": args.config,
        "maps": maps,
        "all_maps": bool(args.all_maps),
        "game_exe": game_exe,
        "uat_log": log_path,
    }
    tmp_path = BUILD_INFO + ".tmp"
    with open(tmp_path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(build_info, f, indent=2)
        f.write("\n")
    os.replace(tmp_path, BUILD_INFO)

    # --- Step 7: Report ---
    print("")
    print("=" * 60)
    print("PACKAGE PLAYTEST SUMMARY")
    print("=" * 60)
    print(f"  Config:     {args.config}")
    print(f"  Commit:     {git_state['commit']} ({git_state['branch']}){' DIRTY' if build_info['dirty'] else ''}")
    print(f"  Duration:   {elapsed / 60:.1f} min")
    print(f"  Archive:    {ARCHIVE_DIR}")
    print(f"  Game exe:   {game_exe}")
    print(f"  Build info: {BUILD_INFO}")
    print(f"  UAT log:    {log_path}")

    log("Package succeeded. Next: py Scripts/playtest_boot.py")
    sys.exit(0)


if __name__ == "__main__":
    main()
