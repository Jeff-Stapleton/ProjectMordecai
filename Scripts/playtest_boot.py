#!/usr/bin/env python3
"""Packaged-Build Boot Playtest -- Phase 1 (observation only, no input yet)

Launches the PACKAGED game (not PIE, not the editor), waits for the Lyra
experience to finish loading, takes screenshots of the real game window, then
shuts the game down and writes a report.  This is the first thing that proves
a feature can reach an actual player: if the Mordecai experience does not load
in the packaged build, nothing built on top of it is playable.

Observation = game.log markers + window screenshots.  Real input (keys, mouse,
Enhanced Input mappings) arrives in Phase 2 via the in-game agent bridge.

STALENESS GATE: the package must have been built from the code being tested.
PackagedBuilds/Playtest/build_info.json (written by package_playtest.py) is
compared with git HEAD and the working tree; a mismatch is a STALE BUILD
(exit 2) unless --allow-stale is passed, which records it as a warning.
STALE when: build_info.json is missing/unreadable; its commit != HEAD; the
tree changed while UAT ran (source_changed_during_build); packaged binaries or
paks are newer than built_at (re-packaged by something else); a clean build now
has uncommitted changes under Source/ Plugins/ Content/ Config/ or the
.uproject; or a dirty build's dirty-file list differs, or its uncommitted
content (dirty_fingerprint) changed in a packaged file.

Usage:
  py Scripts/playtest_boot.py                     # DevTestMap, expect B_MordecaiArenaExperience
  py Scripts/playtest_boot.py --frontend          # no map arg: boot to L_LyraFrontEnd like a player
  py Scripts/playtest_boot.py --map /Game/Foo --expect-experience B_FooExperience
  py Scripts/playtest_boot.py --allow-stale       # run an out-of-date package (warning in report)
  py Scripts/playtest_boot.py --keep-running      # leave the game open afterwards
  py Scripts/playtest_boot.py --fail-on-errors    # any 'LogXxx: Error:' line fails the run

Prereq: py Scripts/package_playtest.py  (writes PackagedBuilds/Playtest + build_info.json)

Output: Saved/Playtests/<YYYYmmdd-HHMMSS>-boot/
  game.log, shot_01.png .., report.json, report.md

Exit codes:
  0 -- all checks passed
  1 -- a gameplay check failed (wrong experience, crash, timeout, ...)
  2 -- infrastructure problem (exe missing, stale build, window not found, git error)
"""

import argparse
import codecs
import collections
import datetime
import json
import os
import re
import subprocess
import sys
import time

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)

PLAYTEST_ARCHIVE = os.path.join(PROJECT_ROOT, "PackagedBuilds", "Playtest")
BUILD_INFO = os.path.join(PLAYTEST_ARCHIVE, "build_info.json")
# <archive>/Windows/LyraGame.exe is only a bootstrapper; this is the real game binary.
DEFAULT_EXE = os.path.join(
    PLAYTEST_ARCHIVE, "Windows", "ProjectMordecai", "Binaries", "Win64", "LyraGame.exe",
)
PLAYTESTS_DIR = os.path.join(PROJECT_ROOT, "Saved", "Playtests")

# ---------------------------------------------------------------------------
# Defaults
# ---------------------------------------------------------------------------
DEFAULT_MAP = "/MordecaiCore/Game/Maps/DevTestMap"
DEFAULT_EXPERIENCE = "B_MordecaiArenaExperience"
FRONTEND_EXPERIENCE = "B_LyraFrontEnd_Experience"
RES_X, RES_Y = 1280, 720

# Inputs that, when changed, make a package stale.
RELEVANT_DIRS = ("Source", "Plugins", "Content", "Config")
# Root files that also feed the package (plugin enable list, target platforms).
UPROJECT_NAME = "ProjectMordecai.uproject"
RELEVANT_DESC = "Source/ Plugins/ Content/ Config/ " + UPROJECT_NAME

# ---------------------------------------------------------------------------
# Log markers (from Lyra source: LyraGameMode.cpp, LyraExperienceManagerComponent.cpp)
# ---------------------------------------------------------------------------
RE_TIMESTAMP = re.compile(r"^\[[^\]]*\]\[\s*\d+\]")
RE_IDENTIFIED = re.compile(r"Identified experience (\S+) \(Source: ([^)]*)\)")
RE_LOAD_COMPLETE = re.compile(r"EXPERIENCE: OnExperienceLoadComplete\(CurrentExperience = ([^,)]+)")
RE_SEVERITY = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*): (Error|Warning): ")

MARKER_FALLBACK = "falling back to the default"
MARKER_FAILED_IDENTIFY = "Failed to identify experience"
CRASH_MARKERS = ("Fatal error", "Unhandled Exception", "Assertion failed", "=== Critical error: ===")

# After a known experience failure, keep watching this long for load-complete so the
# screenshots show what DID load (e.g. the Lyra shooter HUD) as evidence.
EVIDENCE_GRACE_S = 60
# After a crash marker, let the game finish writing its callstack to game.log and hand
# the crash to CrashReportClient (minidump) before the process tree is killed.
CRASH_GRACE_S = 30

# Screenshot heuristics (8-bit grayscale).
UNIFORM_RANGE = 12      # max-min luminance at or below this = flat image
UNIFORM_STDDEV = 3.0    # luminance stddev below this = flat image
BLACK_MAX = 16          # brightest pixel at or below this = black frame

AGENT_LINE = ("AGENT: open each screenshot with the Read tool and describe what is "
              "actually on screen before declaring the story done.")

EXIT_PASS, EXIT_FAIL, EXIT_INFRA = 0, 1, 2


def log(msg):
    print(f"[Boot-Playtest] {msg}", flush=True)


# ---------------------------------------------------------------------------
# Git / staleness gate
# ---------------------------------------------------------------------------
class GitError(Exception):
    pass


def git(*args):
    cmd = ["git", "-C", PROJECT_ROOT, "-c", "core.quotepath=off"] + list(args)
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8",
                           errors="replace", timeout=60)
    except (OSError, subprocess.TimeoutExpired) as e:
        raise GitError(f"git {' '.join(args)}: {e}")
    if r.returncode != 0:
        raise GitError(f"git {' '.join(args)} exited {r.returncode}: {r.stderr.strip()}")
    return r.stdout


def parse_porcelain_z(output):
    """Paths from `git status --porcelain=v1 -z` output.

    Exact mirror of package_playtest.parse_porcelain_z(), which produced the recorded
    dirty_files: -z output is never C-quoted (spaces, non-ASCII) and renames/copies
    are 'XY new' followed by a separate 'old' entry, which is skipped.
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


def current_dirty_files():
    """Same command and parsing as package_playtest.dirty_files_from_git()."""
    return parse_porcelain_z(git("status", "--porcelain=v1", "-z"))


def current_dirty_fingerprint():
    """package_playtest.compute_dirty_fingerprint(), imported so both sides hash identically."""
    import importlib.util
    path = os.path.join(SCRIPT_DIR, "package_playtest.py")
    spec = importlib.util.spec_from_file_location("_package_playtest_for_boot", path)
    if spec is None or spec.loader is None:
        raise GitError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    # Importing would write Scripts/__pycache__/package_playtest.*.pyc, an untracked file that
    # is itself part of the fingerprint -- the check must not change what it measures.
    saved_flag = sys.dont_write_bytecode
    sys.dont_write_bytecode = True
    try:
        spec.loader.exec_module(module)
    finally:
        sys.dont_write_bytecode = saved_flag
    try:
        return module.compute_dirty_fingerprint()
    except (OSError, RuntimeError) as e:
        raise GitError(f"dirty fingerprint: {e}")


def is_relevant(path):
    parts = path.rstrip("/").split("/")
    if len(parts) == 1:
        return parts[0].lower().endswith(".uproject")
    return parts[0].lower() in {d.lower() for d in RELEVANT_DIRS}


def files_modified_after(rel_paths, ts):
    """Relative paths whose mtime is newer than ts.

    Untracked directories (git lists them collapsed, e.g. 'Plugins/X/Content/Items/') are
    walked; their directory mtimes are included too, because adding a copied file (which
    keeps its old mtime) or deleting one only changes the containing directory's mtime.
    """
    newer = []
    for rel in rel_paths:
        full = os.path.join(PROJECT_ROOT, rel.rstrip("/"))
        candidates = []
        if os.path.isdir(full):
            for dirpath, _dirs, files in os.walk(full):
                candidates.append(dirpath)
                candidates.extend(os.path.join(dirpath, f) for f in files)
                if len(candidates) > 20000:
                    break
        elif os.path.isfile(full):
            candidates.append(full)
        for c in candidates:
            try:
                if os.path.getmtime(c) > ts + 1.0:
                    newer.append(os.path.relpath(c, PROJECT_ROOT).replace("\\", "/"))
            except OSError:
                pass
    return newer


def packaged_files_newer_than(archive_root, ts):
    """Game binaries / paks in the archive written after build_info.json's built_at.

    UAT's archive copy keeps the staged files' timestamps, which are all older than
    built_at (stamped after UAT finished).  Anything newer means the folder was
    re-packaged by something other than package_playtest.py (editor 'Package Project',
    a direct RunUAT call) and build_info.json no longer describes it.
    """
    newer = []
    platform_dir = os.path.join(archive_root, "Windows")
    for dirpath, _dirs, files in os.walk(platform_dir):
        for name in files:
            low = name.lower()
            if not (low.endswith((".exe", ".pak", ".ucas", ".utoc"))):
                continue
            full = os.path.join(dirpath, name)
            try:
                if os.path.getmtime(full) > ts + 120.0:
                    newer.append(os.path.relpath(full, archive_root).replace("\\", "/"))
            except OSError:
                pass
    return newer


def find_build_info(exe):
    """build_info.json sits at the archive root: <archive>/Windows/ProjectMordecai/Binaries/Win64/<exe>."""
    d = os.path.dirname(os.path.abspath(exe))
    for _ in range(5):
        candidate = os.path.join(d, "build_info.json")
        if os.path.isfile(candidate):
            return candidate
        parent = os.path.dirname(d)
        if parent == d:
            break
        d = parent
    return None


def short(sha):
    return (sha or "?")[:10]


def check_build_freshness(build_info_path):
    """Returns a dict: fresh (bool), reasons (stale reasons), warnings, git_error, details."""
    res = {
        "build_info_path": build_info_path or BUILD_INFO,
        "fresh": False,
        "reasons": [],
        "warnings": [],
        "git_error": None,
        "build": None,
        "head": None,
        "branch": None,
        "current_dirty_files": [],
    }

    try:
        head = git("rev-parse", "HEAD").strip()
        branch = git("rev-parse", "--abbrev-ref", "HEAD").strip()
        current_dirty = current_dirty_files()
    except GitError as e:
        res["git_error"] = str(e)
        res["reasons"].append(f"git error: {e}")
        return res
    res["head"], res["branch"] = head, branch
    res["current_dirty_files"] = current_dirty[:50]

    if not build_info_path or not os.path.isfile(build_info_path):
        res["reasons"].append(
            "build_info.json missing (looked in the packaged build's archive root, e.g. "
            f"{BUILD_INFO}) -- the package was not made by package_playtest.py, so the "
            "commit it was built from is unknown")
        return res
    try:
        with open(build_info_path, "r", encoding="utf-8-sig") as f:
            info = json.load(f)
        if not isinstance(info, dict):
            raise ValueError("top-level JSON is not an object")
    except (OSError, ValueError) as e:
        res["reasons"].append(f"build_info.json unreadable: {e}")
        return res
    res["build"] = info

    build_commit = str(info.get("commit") or "").strip()
    if not build_commit:
        res["reasons"].append("build_info.json has no 'commit' field")
        return res

    def sample_of(paths):
        return ", ".join(paths[:8]) + (" ..." if len(paths) > 8 else "")

    built_at = info.get("built_at")
    try:
        built_ts = datetime.datetime.fromisoformat(str(built_at)).timestamp()
    except (TypeError, ValueError, OverflowError):
        built_ts = None
        res["warnings"].append(f"could not parse built_at {built_at!r}; mtime checks skipped")

    if str(info.get("config") or "").lower() == "shipping":
        res["warnings"].append("Shipping package: logging is compiled out, so game.log will not "
                               "show the experience markers (expect a timeout)")

    # --- The archive must still be the one build_info.json describes ---
    if built_ts is not None:
        repackaged = packaged_files_newer_than(os.path.dirname(build_info_path), built_ts)
        if repackaged:
            res["reasons"].append(
                f"{len(repackaged)} packaged file(s) were written after build_info.json "
                f"({built_at}) -- re-packaged outside package_playtest.py?: {sample_of(repackaged)}")

    # --- Commit match ---
    if build_commit != head:
        res["reasons"].append(f"build commit {short(build_commit)} != HEAD {short(head)}")
        try:
            git("cat-file", "-e", build_commit + "^{commit}")
            changed = [p for p in git("diff", "--name-only", build_commit, "HEAD", "--",
                                      *RELEVANT_DIRS, UPROJECT_NAME).splitlines() if p.strip()]
            if changed:
                res["reasons"].append(
                    f"{len(changed)} file(s) under {RELEVANT_DESC} changed between the "
                    f"build commit and HEAD: {sample_of(changed)}")
        except GitError:
            res["reasons"].append(f"build commit {short(build_commit)} is not in this repository")

    # --- Edits made while UAT was running (package_playtest.py compares git state before/after) ---
    if info.get("source_changed_during_build"):
        res["reasons"].append(
            "the working tree changed while the package was being built "
            "(source_changed_during_build=true): the package may contain changes that "
            "build_info.json does not describe")

    # --- Working tree ---
    relevant_now = [p for p in current_dirty if is_relevant(p)]
    built_dirty = info.get("dirty")
    if built_dirty is None:
        res["warnings"].append("build_info.json has no 'dirty' field; assuming a clean build")
        built_dirty = False

    if built_dirty:
        res["warnings"].append(
            "package was built from a DIRTY working tree (uncommitted changes were packaged)")
        recorded = [str(p) for p in (info.get("dirty_files") or [])]
        recorded_count = info.get("dirty_count")
        count_differs = isinstance(recorded_count, int) and recorded_count != len(current_dirty)
        if set(current_dirty[:50]) != set(recorded[:50]) or count_differs:
            only_now = sorted(set(current_dirty[:50]) - set(recorded))
            only_then = sorted(set(recorded) - set(current_dirty[:50]))
            res["reasons"].append(
                "build was dirty and the working tree no longer matches it "
                f"(new dirty: {', '.join(only_now[:6]) or '-'}; "
                f"no longer dirty: {', '.join(only_then[:6]) or '-'}"
                + (f"; dirty count {recorded_count} -> {len(current_dirty)}" if count_differs else "")
                + ")")
        else:
            # Same file list -- but a dirty file may have been edited again since the build.
            # dirty_fingerprint (sha256 of the uncommitted CONTENT) settles it when it matches.
            recorded_fp = info.get("dirty_fingerprint")
            fp_match = None
            if recorded_fp:
                try:
                    fp_match = current_dirty_fingerprint() == recorded_fp
                except Exception as e:  # GitError, import problems: fall back to mtimes
                    res["warnings"].append(f"could not compute the dirty fingerprint ({e}); "
                                           "falling back to file modification times")
            if fp_match:
                pass  # identical uncommitted content: the package matches the tree
            elif built_ts is None:
                if fp_match is False:
                    res["reasons"].append("uncommitted content changed since the build "
                                          "(dirty_fingerprint differs) and built_at is unreadable")
            else:
                newer = files_modified_after(relevant_now, built_ts)
                if newer:
                    res["reasons"].append(
                        f"{len(newer)} dirty file(s) under {RELEVANT_DESC} edited after the "
                        f"build ({built_at}): {sample_of(newer)}")
                elif fp_match is False:
                    other = files_modified_after(
                        [p for p in current_dirty if not is_relevant(p)], built_ts)
                    if other:
                        res["warnings"].append(
                            "uncommitted content changed since the build, but only in files "
                            f"that are not packaged: {sample_of(other)}")
                    else:
                        res["reasons"].append(
                            "uncommitted content changed since the build (dirty_fingerprint "
                            "differs) and no edited non-packaged file explains it")
    elif relevant_now:
        res["reasons"].append(
            f"{len(relevant_now)} uncommitted change(s) under {RELEVANT_DESC} "
            f"are not in the (clean) build: {sample_of(relevant_now)}")

    res["fresh"] = not res["reasons"]
    return res


def print_stale_banner(fresh, allowed):
    bar = "!" * 78
    print(bar, flush=True)
    title = "STALE BUILD" + (" (continuing: --allow-stale)" if allowed else "")
    print(f"!!  {title}", flush=True)
    print("!!  The packaged game does not match the code you are testing:", flush=True)
    for r in fresh["reasons"]:
        print(f"!!   - {r}", flush=True)
    if not allowed:
        print("!!  Re-package:  py Scripts/package_playtest.py", flush=True)
        print("!!  (or pass --allow-stale to run anyway; it is recorded in the report)", flush=True)
    print(bar, flush=True)


# ---------------------------------------------------------------------------
# Experience name helpers
# ---------------------------------------------------------------------------
def experience_name(value):
    """'LyraExperienceDefinition:B_X', '/Path/B_X.B_X_C', 'B_X_C' -> 'B_X'."""
    s = str(value or "").strip()
    s = s.split(":")[-1]
    s = s.rsplit("/", 1)[-1]
    if "." in s:
        s = s.split(".", 1)[0]
    if s.endswith("_C"):
        s = s[:-2]
    return s


def strip_timestamp(line):
    return RE_TIMESTAMP.sub("", line, count=1).strip()


# ---------------------------------------------------------------------------
# Log tailing
# ---------------------------------------------------------------------------
class LogTail:
    """Incrementally reads a UTF-8 log the game is still writing (shared read)."""

    def __init__(self, path):
        self.path = path
        self.fh = None
        self.decoder = codecs.getincrementaldecoder("utf-8")(errors="replace")
        self.buf = ""
        self.started = False
        self.line_count = 0

    def poll(self):
        if self.fh is None:
            if not os.path.isfile(self.path):
                return []
            try:
                self.fh = open(self.path, "rb")
            except OSError:
                return []
        try:
            data = self.fh.read()
        except OSError:
            return []
        if not data:
            return []
        text = self.decoder.decode(data)
        if not self.started and text:
            text = text.lstrip("\ufeff")
            self.started = True
        self.buf += text
        parts = self.buf.split("\n")
        self.buf = parts.pop()
        lines = [p.rstrip("\r") for p in parts]
        self.line_count += len(lines)
        return lines

    def flush(self):
        lines = self.poll()
        if self.buf:
            lines.append(self.buf.rstrip("\r"))
            self.line_count += 1
            self.buf = ""
        return lines

    def close(self):
        if self.fh:
            try:
                self.fh.close()
            except OSError:
                pass
            self.fh = None


class BootState:
    def __init__(self, expected):
        self.expected = expected
        self.identified = []          # [{id, name, source, line, t}]
        self.load_complete = None     # first {experience, name, line, t}
        self.load_completes = []      # every OnExperienceLoadComplete seen
        self.fallback_lines = []
        self.other_fallback_lines = []  # 'falling back to the default' from non-experience code
        self.failed_identify_line = None
        self.crash_lines = []

    def scan(self, lines, t):
        for raw in lines:
            line = strip_timestamp(raw)
            if not line:
                continue
            if MARKER_FALLBACK in line:
                # Lyra: "LogLyraExperience: Error: EXPERIENCE: Wanted to use X but couldn't
                # find it, falling back to the default)".  The same phrase is logged by unrelated
                # engine code (virtualization, geometry collections); those are only warnings.
                if "experience" in line.lower():
                    self.fallback_lines.append(line[:400])
                    log(f"!! {line[:200]}")
                elif len(self.other_fallback_lines) < 20:
                    self.other_fallback_lines.append(line[:400])
            if MARKER_FAILED_IDENTIFY in line and self.failed_identify_line is None:
                self.failed_identify_line = line[:400]
                log(f"!! {line[:200]}")
            for marker in CRASH_MARKERS:
                if marker in line:
                    self.crash_lines.append(line[:400])
                    log(f"!! CRASH MARKER: {line[:200]}")
                    break
            m = RE_IDENTIFIED.search(line)
            if m:
                entry = {"id": m.group(1), "name": experience_name(m.group(1)),
                         "source": m.group(2), "line": line[:400], "t": round(t, 1)}
                self.identified.append(entry)
                tag = "" if entry["name"] == self.expected else f"  <-- EXPECTED {self.expected}"
                log(f"Identified experience {entry['id']} (Source: {entry['source']}) "
                    f"at {t:.1f}s{tag}")
            m = RE_LOAD_COMPLETE.search(line)
            if m:
                entry = {"experience": m.group(1).strip(), "name": experience_name(m.group(1)),
                         "line": line[:400], "t": round(t, 1)}
                self.load_completes.append(entry)
                if self.load_complete is None:
                    self.load_complete = entry
                log(f"Experience load complete ({entry['experience']}) at {t:.1f}s")

    @property
    def first_identified(self):
        return self.identified[0] if self.identified else None

    def experience_failure(self):
        """Non-crash reason the expected experience did not load (or did not stay loaded)."""
        first = self.first_identified
        if first and first["name"] != self.expected:
            msg = (f"wrong experience identified: {first['id']} (Source: {first['source']}), "
                   f"expected {self.expected}")
            if first["name"] == "B_LyraDefaultExperience":
                msg += (" -- Lyra fell back to its default shooter experience; the Mordecai "
                        "experience did NOT load")
            return msg
        if self.fallback_lines:
            return f"experience fallback: {self.fallback_lines[0]}"
        if self.failed_identify_line:
            return f"no experience: {self.failed_identify_line}"
        # A later map travel (e.g. an error that returns to the front end) re-identifies.
        switched = next((e for e in self.identified[1:] if e["name"] != self.expected), None)
        if switched:
            return (f"experience changed mid-run to {switched['id']} (Source: "
                    f"{switched['source']}) at {switched['t']}s -- the game travelled away from "
                    f"{self.expected}; later screenshots may not show it")
        wrong_load = next((e for e in self.load_completes if e["name"] != self.expected), None)
        if wrong_load:
            return (f"wrong experience loaded: {wrong_load['experience']} at {wrong_load['t']}s, "
                    f"expected {self.expected}")
        return None


# ---------------------------------------------------------------------------
# Win32 window helpers (ctypes, loaded lazily so --help works anywhere)
# ---------------------------------------------------------------------------
CONSOLE_CLASSES = {"ConsoleWindowClass", "PseudoConsoleWindow", "CASCADIA_HOSTING_WINDOW_CLASS"}
GW_OWNER = 4
GA_ROOT = 2
SW_RESTORE = 9
HWND_TOPMOST, HWND_NOTOPMOST = -1, -2
SWP_NOSIZE, SWP_NOMOVE, SWP_NOACTIVATE = 0x0001, 0x0002, 0x0010


def make_dpi_aware():
    import ctypes
    try:
        hr = ctypes.windll.shcore.SetProcessDpiAwareness(2)  # PROCESS_PER_MONITOR_DPI_AWARE
        if hr == 0:
            return "per-monitor"
        if hr & 0xFFFFFFFF == 0x80070005:  # E_ACCESSDENIED: awareness already set
            return "already-set"
    except (AttributeError, OSError):
        pass
    try:
        if ctypes.windll.user32.SetProcessDPIAware():
            return "system"
    except (AttributeError, OSError):
        pass
    return "unaware"


class Win32:
    def __init__(self):
        import ctypes
        from ctypes import wintypes
        self.ctypes = ctypes
        self.wt = wintypes
        u = ctypes.WinDLL("user32", use_last_error=True)
        self.WNDENUMPROC = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
        u.EnumWindows.argtypes = [self.WNDENUMPROC, wintypes.LPARAM]
        u.EnumWindows.restype = wintypes.BOOL
        u.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
        u.GetWindowThreadProcessId.restype = wintypes.DWORD
        for name in ("IsWindowVisible", "IsIconic", "IsWindow", "SetForegroundWindow",
                     "BringWindowToTop"):
            fn = getattr(u, name)
            fn.argtypes = [wintypes.HWND]
            fn.restype = wintypes.BOOL
        u.GetWindow.argtypes = [wintypes.HWND, wintypes.UINT]
        u.GetWindow.restype = wintypes.HWND
        u.GetClassNameW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
        u.GetClassNameW.restype = ctypes.c_int
        u.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
        u.GetWindowTextW.restype = ctypes.c_int
        u.GetClientRect.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.RECT)]
        u.GetClientRect.restype = wintypes.BOOL
        u.ClientToScreen.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.POINT)]
        u.ClientToScreen.restype = wintypes.BOOL
        u.ShowWindow.argtypes = [wintypes.HWND, ctypes.c_int]
        u.ShowWindow.restype = wintypes.BOOL
        u.GetForegroundWindow.argtypes = []
        u.GetForegroundWindow.restype = wintypes.HWND
        u.AttachThreadInput.argtypes = [wintypes.DWORD, wintypes.DWORD, wintypes.BOOL]
        u.AttachThreadInput.restype = wintypes.BOOL
        u.SetWindowPos.argtypes = [wintypes.HWND, wintypes.HWND, ctypes.c_int, ctypes.c_int,
                                   ctypes.c_int, ctypes.c_int, wintypes.UINT]
        u.SetWindowPos.restype = wintypes.BOOL
        u.WindowFromPoint.argtypes = [wintypes.POINT]
        u.WindowFromPoint.restype = wintypes.HWND
        u.GetAncestor.argtypes = [wintypes.HWND, wintypes.UINT]
        u.GetAncestor.restype = wintypes.HWND
        k = ctypes.WinDLL("kernel32", use_last_error=True)
        k.GetCurrentThreadId.argtypes = []
        k.GetCurrentThreadId.restype = wintypes.DWORD
        self.u = u
        self.k = k

    def client_rect_screen(self, hwnd):
        rc = self.wt.RECT()
        if not self.u.GetClientRect(hwnd, self.ctypes.byref(rc)):
            return None
        pt = self.wt.POINT(0, 0)
        if not self.u.ClientToScreen(hwnd, self.ctypes.byref(pt)):
            return None
        return (pt.x, pt.y, pt.x + (rc.right - rc.left), pt.y + (rc.bottom - rc.top))

    def windows_for_pid(self, pid):
        found = []
        buf_len = 256

        def callback(hwnd, _lparam):
            try:
                if not hwnd or not self.u.IsWindowVisible(hwnd):
                    return True
                owner_pid = self.wt.DWORD(0)
                self.u.GetWindowThreadProcessId(hwnd, self.ctypes.byref(owner_pid))
                if owner_pid.value != pid or self.u.GetWindow(hwnd, GW_OWNER):
                    return True
                cls = self.ctypes.create_unicode_buffer(buf_len)
                self.u.GetClassNameW(hwnd, cls, buf_len)
                title = self.ctypes.create_unicode_buffer(buf_len)
                self.u.GetWindowTextW(hwnd, title, buf_len)
                rect = self.client_rect_screen(hwnd)
                found.append({"hwnd": int(hwnd), "class": cls.value, "title": title.value,
                              "client_rect": rect})
            except Exception:  # never let an exception cross the C callback boundary
                pass
            return True

        proc = self.WNDENUMPROC(callback)  # keep a reference for the duration of the call
        self.u.EnumWindows(proc, 0)
        return found

    def find_game_window(self, pid):
        """Top-level visible window owned by pid; prefers UnrealWindow, skips -log consoles."""
        best, best_score = None, None
        for w in self.windows_for_pid(pid):
            if w["class"] in CONSOLE_CLASSES:
                continue
            rect = w["client_rect"]
            area = (rect[2] - rect[0]) * (rect[3] - rect[1]) if rect else 0
            score = (w["class"] == "UnrealWindow", area)
            if best_score is None or score > best_score:
                best, best_score = w, score
        return best

    def focus(self, hwnd):
        """Bring the window to the foreground. Returns True if it is now foreground.

        A background process is normally refused SetForegroundWindow (foreground lock), so
        this attaches to the current foreground thread's input first and, if that still
        fails, at least raises the window above other non-topmost windows.
        """
        attached = None
        try:
            if self.u.IsIconic(hwnd):
                self.u.ShowWindow(hwnd, SW_RESTORE)
            fg = self.u.GetForegroundWindow()
            me = self.k.GetCurrentThreadId()
            fg_tid = self.u.GetWindowThreadProcessId(fg, None) if fg else 0
            if fg_tid and fg_tid != me and self.u.AttachThreadInput(me, fg_tid, True):
                attached = fg_tid
            self.u.BringWindowToTop(hwnd)
            self.u.SetForegroundWindow(hwnd)
        except Exception:
            pass
        finally:
            if attached:
                try:
                    self.u.AttachThreadInput(self.k.GetCurrentThreadId(), attached, False)
                except Exception:
                    pass
        time.sleep(0.3)
        try:
            ok = int(self.u.GetForegroundWindow() or 0) == int(hwnd)
        except Exception:
            ok = False
        if not ok:
            try:  # z-order bump without activation
                flags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
                self.u.SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, flags)
                self.u.SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, flags)
                time.sleep(0.2)
            except Exception:
                pass
        return ok

    def occluded_points(self, hwnd, bbox):
        """Sample points of bbox whose top-level window is not hwnd (something covers the game)."""
        left, top, right, bottom = bbox
        w, h = right - left, bottom - top
        samples = [(0.5, 0.5), (0.1, 0.1), (0.9, 0.1), (0.1, 0.9), (0.9, 0.9),
                   (0.5, 0.1), (0.5, 0.9), (0.1, 0.5), (0.9, 0.5)]
        covered = []
        for fx, fy in samples:
            x, y = left + int(w * fx), top + int(h * fy)
            try:
                at = self.u.WindowFromPoint(self.wt.POINT(x, y))
                root = self.u.GetAncestor(at, GA_ROOT) if at else None
            except Exception:
                continue
            if int(root or 0) != int(hwnd):
                covered.append((x, y))
        return covered


class KillOnCloseJob:
    """Win32 job object with JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE.

    The game (and anything it spawns, e.g. CrashReportClient) is put in the job, so it is
    killed even if this script itself is killed -- for example when an agent's tool call
    times out in the middle of the run.  terminate() also clears children that outlive the
    game.  Best effort: if the job cannot be created/assigned, .ok is False.
    """

    def __init__(self, pid):
        import ctypes
        from ctypes import wintypes
        self.ctypes = ctypes
        self.handle = None
        self.ok = False
        self.error = None

        class IO_COUNTERS(ctypes.Structure):
            _fields_ = [(n, ctypes.c_ulonglong) for n in (
                "ReadOperationCount", "WriteOperationCount", "OtherOperationCount",
                "ReadTransferCount", "WriteTransferCount", "OtherTransferCount")]

        class BASIC_LIMIT(ctypes.Structure):
            _fields_ = [("PerProcessUserTimeLimit", ctypes.c_int64),
                        ("PerJobUserTimeLimit", ctypes.c_int64),
                        ("LimitFlags", wintypes.DWORD),
                        ("MinimumWorkingSetSize", ctypes.c_size_t),
                        ("MaximumWorkingSetSize", ctypes.c_size_t),
                        ("ActiveProcessLimit", wintypes.DWORD),
                        ("Affinity", ctypes.c_size_t),
                        ("PriorityClass", wintypes.DWORD),
                        ("SchedulingClass", wintypes.DWORD)]

        class EXTENDED_LIMIT(ctypes.Structure):
            _fields_ = [("BasicLimitInformation", BASIC_LIMIT),
                        ("IoInfo", IO_COUNTERS),
                        ("ProcessMemoryLimit", ctypes.c_size_t),
                        ("JobMemoryLimit", ctypes.c_size_t),
                        ("PeakProcessMemoryUsed", ctypes.c_size_t),
                        ("PeakJobMemoryUsed", ctypes.c_size_t)]

        class BASIC_ACCOUNTING(ctypes.Structure):
            _fields_ = [("TotalUserTime", ctypes.c_int64), ("TotalKernelTime", ctypes.c_int64),
                        ("ThisPeriodTotalUserTime", ctypes.c_int64),
                        ("ThisPeriodTotalKernelTime", ctypes.c_int64),
                        ("TotalPageFaultCount", wintypes.DWORD),
                        ("TotalProcesses", wintypes.DWORD),
                        ("ActiveProcesses", wintypes.DWORD),
                        ("TotalTerminatedProcesses", wintypes.DWORD)]

        self.BASIC_ACCOUNTING = BASIC_ACCOUNTING
        k = ctypes.WinDLL("kernel32", use_last_error=True)
        k.CreateJobObjectW.argtypes = [ctypes.c_void_p, wintypes.LPCWSTR]
        k.CreateJobObjectW.restype = wintypes.HANDLE
        k.SetInformationJobObject.argtypes = [wintypes.HANDLE, ctypes.c_int, ctypes.c_void_p,
                                              wintypes.DWORD]
        k.SetInformationJobObject.restype = wintypes.BOOL
        k.QueryInformationJobObject.argtypes = [wintypes.HANDLE, ctypes.c_int, ctypes.c_void_p,
                                                wintypes.DWORD, ctypes.c_void_p]
        k.QueryInformationJobObject.restype = wintypes.BOOL
        k.AssignProcessToJobObject.argtypes = [wintypes.HANDLE, wintypes.HANDLE]
        k.AssignProcessToJobObject.restype = wintypes.BOOL
        k.TerminateJobObject.argtypes = [wintypes.HANDLE, wintypes.UINT]
        k.TerminateJobObject.restype = wintypes.BOOL
        k.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
        k.OpenProcess.restype = wintypes.HANDLE
        k.CloseHandle.argtypes = [wintypes.HANDLE]
        k.CloseHandle.restype = wintypes.BOOL
        self.k = k

        job = k.CreateJobObjectW(None, None)
        if not job:
            self.error = f"CreateJobObject failed ({ctypes.get_last_error()})"
            return
        self.handle = job
        info = EXTENDED_LIMIT()
        info.BasicLimitInformation.LimitFlags = 0x2000  # JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
        if not k.SetInformationJobObject(job, 9, ctypes.byref(info), ctypes.sizeof(info)):
            self.error = f"SetInformationJobObject failed ({ctypes.get_last_error()})"
            return
        hproc = k.OpenProcess(0x0100 | 0x0001, False, pid)  # PROCESS_SET_QUOTA | TERMINATE
        if not hproc:
            self.error = f"OpenProcess({pid}) failed ({ctypes.get_last_error()})"
            return
        try:
            if not k.AssignProcessToJobObject(job, hproc):
                self.error = f"AssignProcessToJobObject failed ({ctypes.get_last_error()})"
                return
        finally:
            k.CloseHandle(hproc)
        self.ok = True

    def active_processes(self):
        if not self.ok:
            return None
        acc = self.BASIC_ACCOUNTING()
        if not self.k.QueryInformationJobObject(self.handle, 1, self.ctypes.byref(acc),
                                                self.ctypes.sizeof(acc), None):
            return None
        return int(acc.ActiveProcesses)

    def terminate(self):
        """Kill every process still in the job. Returns how many were running."""
        if not self.ok:
            return 0
        n = self.active_processes() or 0
        if n:
            self.k.TerminateJobObject(self.handle, 1)
        return n


def analyze_image(img):
    from PIL import ImageStat
    gray = img.convert("L")
    lo, hi = gray.getextrema()
    stat = ImageStat.Stat(gray)
    mean, std = stat.mean[0], stat.stddev[0]
    uniform = (hi - lo) <= UNIFORM_RANGE or std < UNIFORM_STDDEV
    return {"min": lo, "max": hi, "mean": round(mean, 1), "stddev": round(std, 1),
            "uniform": bool(uniform), "black": bool(hi <= BLACK_MAX)}


# ---------------------------------------------------------------------------
# Process helpers
# ---------------------------------------------------------------------------
def kill_tree(proc, job=None):
    """taskkill /PID <pid> /T /F, then reap; then clear leftovers (e.g. CrashReportClient).

    Never raises: it runs in a finally block and must not hide the report.
    """
    notes = []
    if proc.poll() is not None:
        notes.append(f"already exited (code {proc.returncode})")
    else:
        try:
            r = subprocess.run(["taskkill", "/PID", str(proc.pid), "/T", "/F"],
                               capture_output=True, text=True, errors="replace", timeout=60)
            tk_ok = r.returncode == 0
            tk_msg = (r.stdout + r.stderr).strip()
        except (OSError, subprocess.SubprocessError) as e:
            tk_ok, tk_msg = False, str(e)
        try:
            proc.wait(timeout=30)
            if tk_ok:
                notes.append(f"terminated process tree (taskkill /PID {proc.pid} /T /F)")
            else:
                # Raced with the game exiting by itself (e.g. right after a crash).
                notes.append(f"exited (code {proc.returncode}) before taskkill ({tk_msg})")
        except subprocess.TimeoutExpired:
            try:
                proc.kill()
                proc.wait(timeout=10)
                notes.append(f"taskkill did not stop pid {proc.pid} ({tk_msg}); TerminateProcess did")
            except (OSError, subprocess.TimeoutExpired) as e:
                notes.append(f"COULD NOT STOP pid {proc.pid}: {tk_msg}; {e}")
    if job is not None:
        try:
            leftovers = job.terminate()
            if leftovers:
                notes.append(f"killed {leftovers} leftover child process(es) via job object")
        except Exception as e:
            notes.append(f"job cleanup failed: {e}")
    return "; ".join(notes)


def quote_value(value):
    # UE parses -abslog="C:/a b/game.log"; quoting the whole token would hide it from FParse.
    return f'"{value}"' if any(c in value for c in " \t") else value


def resolve_exe(exe):
    """If given the packaging bootstrapper, switch to the real game binary next to it."""
    exe = os.path.abspath(exe)
    real = os.path.join(os.path.dirname(exe), "ProjectMordecai", "Binaries", "Win64",
                        os.path.basename(exe))
    if os.path.isfile(real):
        log(f"NOTE: {exe} is the packaging bootstrapper; using the real game binary {real}")
        return real
    return exe


def new_run_dir():
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    run_dir = os.path.join(PLAYTESTS_DIR, f"{stamp}-boot")
    n = 2
    while os.path.exists(run_dir):
        run_dir = os.path.join(PLAYTESTS_DIR, f"{stamp}-boot-{n}")
        n += 1
    os.makedirs(run_dir)
    return run_dir


# ---------------------------------------------------------------------------
# Log summary
# ---------------------------------------------------------------------------
def summarize_log(path, top_n=15, tail_n=40):
    out = {"path": path, "exists": os.path.isfile(path), "lines": 0, "errors": 0,
           "warnings": 0, "top_errors": [], "tail": []}
    if not out["exists"]:
        return out
    errors = collections.Counter()
    tail = collections.deque(maxlen=tail_n)
    try:
        with open(path, "r", encoding="utf-8-sig", errors="replace") as f:
            for raw in f:
                out["lines"] += 1
                line = strip_timestamp(raw.rstrip("\r\n"))
                tail.append(line)
                m = RE_SEVERITY.match(line)
                if not m:
                    continue
                if m.group(2) == "Error":
                    out["errors"] += 1
                    errors[line[:300]] += 1
                else:
                    out["warnings"] += 1
    except OSError as e:
        out["read_error"] = str(e)
    out["top_errors"] = [{"line": l, "count": c} for l, c in errors.most_common(top_n)]
    out["tail"] = list(tail)
    return out


# ---------------------------------------------------------------------------
# Reports
# ---------------------------------------------------------------------------
CHECK_ORDER = ("build_fresh", "launch", "experience_identified", "experience_loaded", "no_crash",
               "game_window", "screenshots", "log_errors", "completed")


def add_check(checks, name, status, detail, kind="gameplay"):
    checks.append({"name": name, "status": status, "detail": detail, "kind": kind})


def sort_checks(checks):
    rank = {n: i for i, n in enumerate(CHECK_ORDER)}
    checks.sort(key=lambda c: rank.get(c["name"], len(CHECK_ORDER)))


def exit_code_for(checks):
    if any(c["status"] == "FAIL" and c["kind"] == "gameplay" for c in checks):
        return EXIT_FAIL
    if any(c["status"] == "FAIL" and c["kind"] == "infra" for c in checks):
        return EXIT_INFRA
    return EXIT_PASS


def md_cell(text):
    return str(text).replace("|", "\\|").replace("\n", " ")


def write_reports(run_dir, report):
    json_path = os.path.join(run_dir, "report.json")
    with open(json_path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(report, f, indent=2, ensure_ascii=False)
        f.write("\n")

    exp = report["experience"]
    build = report["build"]
    logsum = report["log"]
    lines = [
        f"# Boot playtest: {report['result']}",
        "",
        f"- Run dir: `{report['run_dir']}`",
        f"- Started: {report['started_at']}  Finished: {report['finished_at']}",
        f"- Exe: `{report['exe']}`",
        f"- Command: `{report['command_line']}`",
        f"- Map: {report['map'] or '(none: --frontend, GameDefaultMap)'}",
        f"- Exit code: {report['exit_code']}",
        "",
        "## Build",
        "",
    ]
    b = build.get("build") or {}
    if b:
        lines.append(f"- Built from commit `{short(b.get('commit'))}` on `{b.get('branch')}`, "
                     f"dirty={b.get('dirty')}, built_at={b.get('built_at')}, config={b.get('config')}")
    lines.append(f"- Current HEAD `{short(build.get('head'))}` on `{build.get('branch')}`")
    if build.get("fresh"):
        lines.append("- Freshness: OK (package matches the code under test)")
    else:
        lines.append("- Freshness: **STALE BUILD (allowed by --allow-stale)**")
        for r in build.get("reasons", []):
            lines.append(f"  - {r}")
    for w in build.get("warnings", []):
        lines.append(f"- Warning: {w}")

    lines += ["", "## Checks", "", "| Check | Result | Detail |", "|---|---|---|"]
    for c in report["checks"]:
        lines.append(f"| {md_cell(c['name'])} | {c['status']} | {md_cell(c['detail'])} |")

    first = exp.get("identified")
    lines += [
        "",
        "## Experience",
        "",
        f"- Expected: {exp['expected']}",
        f"- Identified: {first['id'] + ' (Source: ' + first['source'] + ')' if first else 'none'}",
        f"- Load complete: {exp['load_complete']['experience'] if exp.get('load_complete') else 'no'}",
        f"- Load time: {str(exp['load_time_s']) + 's after launch' if exp.get('load_time_s') is not None else 'n/a'}",
    ]
    if exp.get("all_identified") and len(exp["all_identified"]) > 1:
        lines.append("- All identified: " + ", ".join(
            f"{e['id']} ({e['source']})" for e in exp["all_identified"]))
    for l in report.get("fatal_lines", []):
        lines.append(f"- Fatal marker: `{l}`")

    lines += [
        "",
        "## Log",
        "",
        f"- game.log: `{logsum['path']}` ({logsum['lines']} lines)",
        f"- Errors: {logsum['errors']}  Warnings: {logsum['warnings']}"
        + ("" if report["fail_on_errors"] else "  (informational; --fail-on-errors to gate)"),
    ]
    if logsum["top_errors"]:
        lines += ["", "### Top errors", ""]
        for i, e in enumerate(logsum["top_errors"], 1):
            lines.append(f"{i}. (x{e['count']}) `{e['line']}`")
    if report["result"] != "PASS" and logsum.get("tail"):
        lines += ["", "### Log tail", "", "```"] + logsum["tail"] + ["```"]

    lines += ["", "## Screenshots", ""]
    if report["screenshots"]:
        for s in report["screenshots"]:
            flag = ""
            if s.get("uniform"):
                flag += (f"  **WARNING: near-uniform{' black' if s.get('black') else ''} frame "
                         f"(mean {s['mean']}, stddev {s['stddev']})**")
            if s.get("occluded"):
                flag += "  **WARNING: another window covered part of the game window**"
            lines.append(f"- {s['path']}{flag}")
    else:
        lines.append("- (none captured)")
    if report["warnings"]:
        lines += ["", "## Warnings", ""] + [f"- {w}" for w in report["warnings"]]
    lines += ["", AGENT_LINE, ""]

    md_path = os.path.join(run_dir, "report.md")
    with open(md_path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))
    return json_path, md_path


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def parse_args():
    parser = argparse.ArgumentParser(
        description="Phase 1 packaged-build boot playtest: launch the packaged game, verify the "
                    "experience loads, screenshot the window, report.")
    parser.add_argument("--exe", default=DEFAULT_EXE,
                        help=f"Game binary (default: {DEFAULT_EXE})")
    where = parser.add_mutually_exclusive_group()
    where.add_argument("--map", default=None,
                       help=f"Map to launch (default: {DEFAULT_MAP})")
    where.add_argument("--frontend", action="store_true",
                       help="Launch with NO map argument (boots to L_LyraFrontEnd like a player); "
                            f"expects {FRONTEND_EXPERIENCE} unless --expect-experience is given")
    parser.add_argument("--expect-experience", default=None,
                        help=f"Experience that must load (default: {DEFAULT_EXPERIENCE}, "
                             f"or {FRONTEND_EXPERIENCE} with --frontend)")
    parser.add_argument("--timeout", type=float, default=240,
                        help="Max seconds to wait for the experience to load (default: 240)")
    parser.add_argument("--settle", type=float, default=6,
                        help="Seconds to wait after load before screenshots (default: 6)")
    parser.add_argument("--shots", type=int, default=3,
                        help="Number of screenshots (default: 3)")
    parser.add_argument("--shot-interval", type=float, default=2.0,
                        help="Seconds between screenshots (default: 2.0)")
    parser.add_argument("--allow-stale", action="store_true",
                        help="Run even if the package does not match HEAD/working tree "
                             "(recorded as a warning in the report)")
    parser.add_argument("--fail-on-errors", action="store_true",
                        help="Fail (exit 1) if game.log contains any 'LogXxx: Error:' lines")
    parser.add_argument("--keep-running", action="store_true",
                        help="Leave the game running after the test")
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error("--timeout must be > 0")
    if args.settle < 0 or args.shot_interval < 0:
        parser.error("--settle and --shot-interval must be >= 0")
    if args.shots < 0:
        parser.error("--shots must be >= 0")
    return args


def main():
    # Game log lines can contain characters the console/pipe encoding (cp1252 when captured)
    # cannot encode; a UnicodeEncodeError mid-run would otherwise abort without a report.
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(errors="replace")
        except (AttributeError, ValueError):
            pass
    args = parse_args()
    expected = experience_name(
        args.expect_experience or (FRONTEND_EXPERIENCE if args.frontend else DEFAULT_EXPERIENCE))
    map_arg = None if args.frontend else (args.map or DEFAULT_MAP)

    # --- Step 1: Preflight (infra) ---
    if sys.platform != "win32":
        log("ERROR: this playtest drives a Win64 packaged build and must run on Windows.")
        return EXIT_INFRA

    exe = os.path.abspath(args.exe)
    if not os.path.isfile(exe):
        log(f"ERROR: game exe not found: {exe}")
        log("Package first:  py Scripts/package_playtest.py")
        return EXIT_INFRA
    exe = resolve_exe(exe)

    # --- Step 2: Staleness gate ---
    build_info_path = find_build_info(exe)
    fresh = check_build_freshness(build_info_path)
    if fresh["git_error"]:
        log(f"{'WARNING' if args.allow_stale else 'ERROR'}: cannot verify build freshness: "
            f"{fresh['git_error']}")
        if not args.allow_stale:
            return EXIT_INFRA
    elif not fresh["fresh"]:
        print_stale_banner(fresh, args.allow_stale)
        if not args.allow_stale:
            return EXIT_INFRA
    else:
        log(f"Build is fresh: commit {short(fresh['head'])} ({fresh['branch']})")
    recorded_exe = (fresh.get("build") or {}).get("game_exe")
    if recorded_exe and os.path.normcase(os.path.abspath(recorded_exe)) != os.path.normcase(exe):
        fresh["warnings"].append(f"running {exe}, but build_info.json describes {recorded_exe}")
    for w in fresh["warnings"]:
        log(f"WARNING: {w}")

    # --- Step 3: Launch ---
    dpi = make_dpi_aware()
    run_dir = new_run_dir()
    game_log = os.path.join(run_dir, "game.log")
    game_args = ([map_arg] if map_arg else []) + [
        "-windowed", f"-ResX={RES_X}", f"-ResY={RES_Y}", "-log",
        f"-abslog={quote_value(game_log)}", "-nosplash", "-nosound",
    ]
    command_line = subprocess.list2cmdline([exe]) + " " + " ".join(game_args)

    started_at = datetime.datetime.now().isoformat(timespec="seconds")
    checks, warnings, screenshots = [], [], []
    warnings.extend(fresh["warnings"])
    if not fresh["fresh"]:
        warnings.append("STALE BUILD allowed by --allow-stale: " + "; ".join(fresh["reasons"]))
    add_check(checks, "build_fresh", "PASS" if fresh["fresh"] else "WARN",
              "package matches HEAD/working tree" if fresh["fresh"]
              else "STALE (allowed): " + "; ".join(fresh["reasons"]), kind="infra")

    state = BootState(expected)
    tail = LogTail(game_log)
    proc = None
    window = None
    shutdown_note = None
    outcome = "not_launched"
    wait_elapsed = 0.0
    exited_on_own = False

    log(f"Run dir: {run_dir}")
    log(f"Launching: {command_line}")
    t0 = time.time()
    job = None
    try:
        try:
            proc = subprocess.Popen(command_line, cwd=os.path.dirname(exe),
                                    stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                                    stderr=subprocess.DEVNULL)
        except OSError as e:
            add_check(checks, "launch", "FAIL", f"could not start the game: {e}", kind="infra")
        else:
            add_check(checks, "launch", "PASS", f"pid {proc.pid}", kind="infra")
            if not args.keep_running:
                # Kills the game even if this script is killed (e.g. a tool-call timeout).
                try:
                    job = KillOnCloseJob(proc.pid)
                    if not job.ok:
                        log(f"NOTE: no kill-on-exit job for the game ({job.error}); "
                            "relying on taskkill at shutdown")
                        job = None
                except Exception as e:
                    log(f"NOTE: no kill-on-exit job for the game ({e})")
                    job = None

        if proc is not None:
            # --- Step 4: Wait for the experience (tail game.log) ---
            deadline = t0 + args.timeout
            outcome = None
            next_progress = t0 + 15
            log(f"Waiting up to {args.timeout:.0f}s for experience {expected}...")
            while True:
                now = time.time()
                state.scan(tail.poll(), now - t0)
                if state.crash_lines:
                    outcome = "crash"
                    break
                if state.failed_identify_line:
                    outcome = "no_experience"
                    break
                if state.load_complete:
                    outcome = "loaded"
                    break
                if state.experience_failure() and deadline > now + EVIDENCE_GRACE_S:
                    # Known failure: only wait a little longer for evidence screenshots.
                    deadline = now + EVIDENCE_GRACE_S
                if proc.poll() is not None:
                    time.sleep(0.5)
                    state.scan(tail.flush(), time.time() - t0)
                    outcome = "exited"
                    break
                if now >= deadline:
                    outcome = "timeout"
                    break
                if now >= next_progress:
                    log(f"  ...{now - t0:.0f}s, game.log "
                        f"{'lines: ' + str(tail.line_count) if tail.fh else 'not created yet'}")
                    next_progress = now + 15
                time.sleep(0.5)
            wait_elapsed = time.time() - t0

            if outcome == "crash":
                # Let the game finish logging the callstack and hand off to CrashReportClient
                # (minidump) instead of killing it mid-write.
                log(f"Crash detected; waiting up to {CRASH_GRACE_S}s for the game to finish "
                    "logging and exit...")
                crash_end = time.time() + CRASH_GRACE_S
                while proc.poll() is None and time.time() < crash_end:
                    state.scan(tail.poll(), time.time() - t0)
                    time.sleep(0.5)

            # --- Step 5: Settle, then screenshots (also on failure, as evidence) ---
            def watch(seconds):
                end = time.time() + seconds
                while True:
                    state.scan(tail.poll(), time.time() - t0)
                    if proc.poll() is not None:
                        return False
                    remaining = end - time.time()
                    if remaining <= 0:
                        return True
                    time.sleep(min(0.5, remaining))

            alive = proc.poll() is None and outcome != "crash"
            if alive and args.shots > 0:
                log(f"Settling {args.settle:.1f}s before screenshots...")
                alive = watch(args.settle)
            if not alive:
                add_check(checks, "game_window", "SKIP", "game process is not running",
                          kind="infra")
            elif args.shots == 0:
                add_check(checks, "game_window", "SKIP", "--shots 0", kind="infra")
            else:
                win = Win32()
                find_end = time.time() + 15
                while window is None and time.time() < find_end and proc.poll() is None:
                    window = win.find_game_window(proc.pid)
                    if window is None:
                        time.sleep(0.5)
                if window is None:
                    add_check(checks, "game_window", "FAIL",
                              f"no visible top-level window owned by pid {proc.pid}", kind="infra")
                else:
                    add_check(checks, "game_window", "PASS",
                              f"hwnd {window['hwnd']} class={window['class']} "
                              f"title={window['title']!r} client={window['client_rect']}",
                              kind="infra")
                    shot_problem = None
                    for i in range(args.shots):
                        if i > 0 and not watch(args.shot_interval):
                            break
                        hwnd = window["hwnd"]
                        if not win.focus(hwnd):
                            msg = ("could not bring the game window to the foreground; "
                                   "screenshots may include overlapping windows")
                            if msg not in warnings:
                                warnings.append(msg)
                                log(f"WARNING: {msg}")
                        bbox = win.client_rect_screen(hwnd)
                        if not bbox or bbox[2] <= bbox[0] or bbox[3] <= bbox[1]:
                            shot_problem = f"game window has no client area (rect {bbox})"
                            break
                        try:
                            from PIL import ImageGrab
                            img = ImageGrab.grab(bbox=bbox, all_screens=True)
                            path = os.path.join(run_dir, f"shot_{i + 1:02d}.png")
                            img.save(path)
                        except Exception as e:  # ImportError, OSError from the grabber
                            shot_problem = f"screenshot failed: {e}"
                            break
                        info = analyze_image(img)
                        covered = win.occluded_points(hwnd, bbox)
                        info.update({"path": path, "bbox": list(bbox),
                                     "width": img.width, "height": img.height,
                                     "occluded": bool(covered),
                                     "t": round(time.time() - t0, 1)})
                        screenshots.append(info)
                        log(f"Screenshot {path} ({img.width}x{img.height}, mean {info['mean']}, "
                            f"stddev {info['stddev']})")
                        if info["uniform"]:
                            msg = (f"{os.path.basename(path)} is near-uniform"
                                   f"{' BLACK' if info['black'] else ''} (mean {info['mean']}, "
                                   f"stddev {info['stddev']}): window occluded/minimized, "
                                   "screen locked, or nothing rendered")
                            warnings.append(msg)
                            log(f"WARNING: {msg}")
                        if covered:
                            msg = (f"{os.path.basename(path)}: another window covers part of the "
                                   f"game window ({len(covered)}/9 sample points, e.g. "
                                   f"{covered[0]}); the screenshot may not show the game")
                            warnings.append(msg)
                            log(f"WARNING: {msg}")
                    if shot_problem:
                        add_check(checks, "screenshots", "FAIL", shot_problem, kind="infra")
                    elif screenshots:
                        n_uniform = sum(1 for s in screenshots if s["uniform"])
                        n_occluded = sum(1 for s in screenshots if s["occluded"])
                        add_check(checks, "screenshots",
                                  "WARN" if (n_uniform or n_occluded) else "PASS",
                                  f"{len(screenshots)}/{args.shots} captured"
                                  + (f", {n_uniform} near-uniform" if n_uniform else "")
                                  + (f", {n_occluded} partly covered by other windows"
                                     if n_occluded else ""),
                                  kind="infra")
                    else:
                        add_check(checks, "screenshots", "SKIP",
                                  "game process exited before the first screenshot", kind="infra")
    except KeyboardInterrupt:
        outcome = "interrupted"
        add_check(checks, "completed", "FAIL", "interrupted by user", kind="infra")
        log("Interrupted.")
    except Exception as e:  # report what we have instead of dying without a report
        outcome = "script_error"
        add_check(checks, "completed", "FAIL", f"playtest script error: {type(e).__name__}: {e}",
                  kind="infra")
        import traceback
        traceback.print_exc()
    finally:
        if proc is not None:
            exited_on_own = proc.poll() is not None
            if args.keep_running and not exited_on_own:
                shutdown_note = f"left running (--keep-running), pid {proc.pid}"
            else:
                shutdown_note = kill_tree(proc, job)
            log(f"Shutdown: {shutdown_note}")
        state.scan(tail.flush(), time.time() - t0)
        tail.close()

    for line in state.other_fallback_lines[:5]:
        warnings.append(f"non-experience fallback logged (not treated as failure): {line[:200]}")

    # --- Step 6: Evaluate ---
    if proc is not None and outcome not in ("interrupted", "script_error"):
        first = state.first_identified
        exp_fail = state.experience_failure()
        if first and first["name"] == expected and not exp_fail:
            add_check(checks, "experience_identified", "PASS",
                      f"{first['id']} (Source: {first['source']}) at {first['t']}s")
        elif exp_fail:
            add_check(checks, "experience_identified", "FAIL", exp_fail)
        else:
            add_check(checks, "experience_identified", "FAIL",
                      "no 'Identified experience' line in game.log")

        lc = state.load_complete
        wrong_loads = [e for e in state.load_completes if e["name"] != expected]
        if lc and lc["name"] == expected and not wrong_loads and not exp_fail:
            add_check(checks, "experience_loaded", "PASS",
                      f"OnExperienceLoadComplete({lc['experience']}) {lc['t']}s after launch")
        elif wrong_loads:
            add_check(checks, "experience_loaded", "FAIL",
                      f"loaded {wrong_loads[0]['experience']} at {wrong_loads[0]['t']}s "
                      f"instead of {expected}")
        elif lc:
            add_check(checks, "experience_loaded", "FAIL",
                      f"OnExperienceLoadComplete({lc['experience']}) was logged, but: {exp_fail}")
        elif outcome == "timeout":
            stage = ("experience identified but OnExperienceLoadComplete never logged"
                     if state.identified else
                     "no 'Identified experience' line" if tail.line_count else
                     "game.log was never written")
            add_check(checks, "experience_loaded", "FAIL",
                      f"timed out after {wait_elapsed:.0f}s: {stage}")
        else:
            add_check(checks, "experience_loaded", "FAIL",
                      f"OnExperienceLoadComplete not seen (outcome: {outcome})")

        if state.crash_lines:
            add_check(checks, "no_crash", "FAIL", f"crash marker: {state.crash_lines[0]}")
        elif exited_on_own:
            add_check(checks, "no_crash", "FAIL",
                      f"game process exited on its own (code {proc.returncode}) "
                      "before the test finished")
        else:
            add_check(checks, "no_crash", "PASS", "no crash markers; process alive until shutdown")
    elif proc is not None:
        for name in ("experience_identified", "experience_loaded", "no_crash"):
            add_check(checks, name, "SKIP", f"not evaluated ({outcome})")

    logsum = summarize_log(game_log)
    if args.fail_on_errors:
        add_check(checks, "log_errors", "FAIL" if logsum["errors"] else "PASS",
                  f"{logsum['errors']} error(s), {logsum['warnings']} warning(s)")
    else:
        add_check(checks, "log_errors", "INFO",
                  f"{logsum['errors']} error(s), {logsum['warnings']} warning(s) (informational)")

    sort_checks(checks)
    code = exit_code_for(checks)
    result = {EXIT_PASS: "PASS", EXIT_FAIL: "FAIL", EXIT_INFRA: "ERROR"}[code]
    lc = state.load_complete
    report = {
        "result": result,
        "exit_code": code,
        "started_at": started_at,
        "finished_at": datetime.datetime.now().isoformat(timespec="seconds"),
        "run_dir": run_dir,
        "exe": exe,
        "command_line": command_line,
        "map": map_arg,
        "frontend": args.frontend,
        "pid": proc.pid if proc else None,
        "shutdown": shutdown_note,
        "dpi_awareness": dpi,
        "options": {"timeout": args.timeout, "settle": args.settle, "shots": args.shots,
                    "shot_interval": args.shot_interval, "allow_stale": args.allow_stale,
                    "fail_on_errors": args.fail_on_errors, "keep_running": args.keep_running},
        "fail_on_errors": args.fail_on_errors,
        "build": fresh,
        "experience": {
            "expected": expected,
            "identified": state.first_identified,
            "all_identified": state.identified,
            "load_complete": lc,
            "load_time_s": lc["t"] if lc else None,
            "outcome": outcome,
        },
        "fatal_lines": state.crash_lines + state.fallback_lines
        + ([state.failed_identify_line] if state.failed_identify_line else []),
        "window": window,
        "screenshots": screenshots,
        "log": logsum,
        "checks": checks,
        "warnings": warnings,
        "agent_note": AGENT_LINE,
    }
    _json_path, md_path = write_reports(run_dir, report)

    # --- Step 7: Console summary ---
    print("")
    print("=" * 60)
    print(f"BOOT PLAYTEST: {result}")
    print("=" * 60)
    for c in checks:
        print(f"  [{c['status']}] {c['name']}: {c['detail']}")
    if logsum["top_errors"]:
        print(f"\n  Top errors ({logsum['errors']} total):")
        for e in logsum["top_errors"][:5]:
            print(f"    (x{e['count']}) {e['line'][:160]}")
    print(f"\nRun dir: {run_dir}")
    print(f"Report:  {md_path}")
    for s in screenshots:
        print(f"Screenshot: {s['path']}")
    if screenshots:
        print(AGENT_LINE)
    return code


if __name__ == "__main__":
    sys.exit(main())
