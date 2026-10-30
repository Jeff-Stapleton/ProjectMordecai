#!/usr/bin/env python3
"""PIE Verification Pipeline — Tier 2 Orchestrator

Launches the editor (if needed), starts PIE, runs all verify_pie_*.py smoke
tests, stops PIE, and exits with 0 (all pass) or 1 (any failure).

This script runs OUTSIDE the editor.  It uses:
  - check_editor.py  — wait for editor remote-exec readiness
  - ue_remote_exec.py — send Python scripts to the running editor

Usage:
  py Scripts/run_pie_verify.py                  # full pipeline
  py Scripts/run_pie_verify.py --no-launch      # skip editor launch (already running)
  py Scripts/run_pie_verify.py --no-pie-stop    # leave PIE running after tests
  py Scripts/run_pie_verify.py --tests hud      # run only the HUD smoke test
  py Scripts/run_pie_verify.py --tests hud,spawn  # run specific tests
  py Scripts/run_pie_verify.py --map /MordecaiCore/Game/Maps/CombatTestMap   # PIE a test map (US-100)

PIE plays the map the editor has open. Without --map that is whatever is open
(DevTestMap, the EditorStartupMap: the default suite needs its Frontliners and
pickups). --map <package path> opens that map first and reopens DevTestMap
after PIE stops; it refuses (exit 2) when a map has unsaved changes, because
opening another map discards them. For a test map (MovementTestMap,
CombatTestMap) without --tests, only the smoke tests that apply to any map run:
spawn, hud, test_maps.

Exit codes:
  0 — all smoke tests passed
  1 — one or more smoke tests failed
  2 — infrastructure error (editor won't start, PIE won't launch, map won't open, etc.)

Multi-phase smoke tests: a verify script that prints a line starting with
PIE_NEXT_PHASE is run again (up to MAX_PHASES runs) after the editor has
ticked for PHASE_DELAY_S. One remote exec runs inside a single editor frame,
so this is how a script checks something that only happens on a later frame
(e.g. CommonUI releasing a closed menu's Slate tree, US-094). The script
derives its phase from PIE state; it keeps nothing between runs.
"""

import argparse
import glob
import os
import subprocess
import sys
import time

from playtest_boot import DEFAULT_MAP, TEST_MAPS  # the map paths' single source (US-100)

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)

# The smoke tests that hold on any map; the rest of the default suite needs DevTestMap's Frontliners and pickups
ANY_MAP_TESTS = ["spawn", "hud", "test_maps"]

CHECK_EDITOR = os.path.join(SCRIPT_DIR, "check_editor.py")
UE_REMOTE_EXEC = os.path.join(SCRIPT_DIR, "ue_remote_exec.py")
EDITOR_EXE = os.path.join(
    os.path.expanduser("~"),
    "Documents", "Gamedev", "UnrealEngine", "Engine", "Binaries", "Win64", "UnrealEditor.exe",
)
UPROJECT = os.path.join(PROJECT_ROOT, "ProjectMordecai.uproject")

PHASE_MARKER = "PIE_NEXT_PHASE"  # printed by a verify script that must run again on a later editor frame
MAX_PHASES = 8
PHASE_DELAY_S = 2.0


def log(msg):
    print(f"[PIE-Verify] {msg}", flush=True)


def run_check_editor(wait=False, timeout=300):
    """Returns True if editor is ready."""
    cmd = [sys.executable, CHECK_EDITOR]
    if wait:
        cmd += ["--wait", "--timeout", str(timeout)]
    result = subprocess.run(cmd, capture_output=True, text=True)
    return result.returncode == 0


def launch_editor():
    """Launch the Unreal Editor as a detached process."""
    log(f"Launching editor: {EDITOR_EXE}")
    subprocess.Popen(
        [EDITOR_EXE, UPROJECT],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        creationflags=0x00000008,  # DETACHED_PROCESS on Windows
    )


def remote_exec(script_path, timeout=60):
    """Run a Python script in the editor via remote exec. Returns (success, output)."""
    # Use exec(open(...).read()) instead of --file flag, because UE's ExecuteFile
    # mode expects a file path on the remote machine, not file contents.
    escaped_path = script_path.replace("\\", "/")
    command = f"exec(open(r'{escaped_path}').read())"
    cmd = [sys.executable, UE_REMOTE_EXEC, command, "--timeout", str(timeout)]
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout + 10)
    output = result.stdout + result.stderr
    return result.returncode == 0, output


def remote_exec_cmd(command, timeout=30):
    """Run a single Python command in the editor. Returns (success, output)."""
    cmd = [sys.executable, UE_REMOTE_EXEC, command, "--timeout", str(timeout)]
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout + 10)
    output = result.stdout + result.stderr
    return result.returncode == 0, output


def start_pie():
    """Start Play In Editor (play mode, not simulate)."""
    log("Starting PIE...")
    # Use editor_request_begin_play — NOT editor_play_simulate (which starts spectator mode)
    success, output = remote_exec_cmd(
        "import unreal; unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()"
    )
    return success


def stop_pie():
    """Stop Play In Editor."""
    log("Stopping PIE...")
    remote_exec_cmd(
        "import unreal; unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()"
    )


def wait_for_pie_end(timeout=30):
    """Wait until the PIE world is gone (end_play is only a request; a map can't be opened during PIE)."""
    check_cmd = (
        "import unreal; print('PIE_ENDED' if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)"
        ".get_game_world() is None else 'PIE_RUNNING')"
    )
    start = time.time()
    while time.time() - start < timeout:
        success, output = remote_exec_cmd(check_cmd, timeout=10)
        if success and "PIE_ENDED" in output:
            return True
        time.sleep(1)
    return False


def normalize_map(map_path):
    """'/MordecaiCore/Game/Maps/X' from a package path, with or without a '.X' object suffix."""
    return map_path.strip().split(".")[0].rstrip("/")


def is_test_map(map_path):
    return map_path is not None and map_path.lower() in (m.lower() for m in TEST_MAPS)


# Runs inside the editor. Drops the editor-Python globals that still reference a world's objects and returns their
# names. Every smoke script runs in the editor's __main__ namespace, so one that keeps a PIE actor or world at module
# level (verify_pie_weapon.py does) keeps that world alive: ending PIE then hitches on a stale-reference ensure, and
# the next map load (EditorDestroyWorld -> CheckForWorldGCLeaks) dies with the fatal "World Memory Leaks".
DROP_WORLD_REFS_CODE = """
def _run_pie_verify_drop_world_refs():
    import gc
    import unreal

    def world_bound(value):
        if isinstance(value, (unreal.World, unreal.Level, unreal.Actor, unreal.ActorComponent, unreal.UserWidget,
                              unreal.GameInstance, unreal.Subsystem, unreal.Player)):
            return True
        if isinstance(value, unreal.Object):
            name = value.get_path_name()
            return 'UEDPIE_' in name or ':PersistentLevel' in name or name.startswith('/Temp/')
        if isinstance(value, dict):
            return any(world_bound(v) for v in value.values())
        if isinstance(value, (list, tuple, set, unreal.Array)):
            return any(world_bound(v) for v in value)
        return False

    names = globals()
    stale = sorted(n for n, v in list(names.items()) if not n.startswith('__') and world_bound(v))
    for n in stale:
        del names[n]
    gc.collect()
    return stale
"""

# Runs inside the editor (one exec, all names local to functions)
OPEN_MAP_CODE = DROP_WORLD_REFS_CODE + """
def _run_pie_verify_open_map(path):
    import unreal

    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    if editor.get_game_world() is not None:
        return 'MAP_REFUSED: PIE is running'
    world = editor.get_editor_world()
    current = world.get_path_name().split('.')[0] if world else ''
    del world  # a live reference to the old world would itself be the leak
    if current.lower() == path.lower():
        return 'MAP_OPEN: ' + path
    dirty = [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()]
    if dirty:
        return 'MAP_REFUSED: unsaved changes in ' + ', '.join(dirty) + ' (save or revert them first)'
    stale = _run_pie_verify_drop_world_refs()
    if stale:
        print('DROPPED_WORLD_REFS: ' + ', '.join(stale))
    if unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(path):
        return 'MAP_OPEN: ' + path
    return 'MAP_FAILED: could not load ' + path
"""


def open_map(map_path, timeout=120):
    """Open map_path in the editor unless it already is the open map. Returns (ok, output).

    Refuses while PIE runs or while any map has unsaved changes: opening another map discards them."""
    code = OPEN_MAP_CODE + (f"print(_run_pie_verify_open_map({map_path!r}))\n"
                            "del _run_pie_verify_open_map, _run_pie_verify_drop_world_refs\n")
    success, output = remote_exec_cmd(f"exec({code!r})", timeout=timeout)
    return success and f"MAP_OPEN: {map_path}" in output, output


def drop_world_refs():
    """Drop the editor-Python globals the smoke scripts left holding PIE objects (see DROP_WORLD_REFS_CODE)."""
    code = DROP_WORLD_REFS_CODE + ("print('DROPPED_WORLD_REFS: ' + (', '.join(_run_pie_verify_drop_world_refs()) or 'none'))\n"
                                   "del _run_pie_verify_drop_world_refs\n")
    success, output = remote_exec_cmd(f"exec({code!r})", timeout=30)
    dropped = [line.split("DROPPED_WORLD_REFS:", 1)[1].strip() for line in output.splitlines() if "DROPPED_WORLD_REFS:" in line]
    if not success or not dropped:
        log(f"WARNING: could not clear the smoke scripts' Python references: {output.strip()}")
    elif dropped[0] != "none":
        log(f"Dropped editor Python globals that held PIE objects (a smoke script left them): {dropped[0]}")


def finish_session(args, opened_map):
    """Stop PIE (unless --no-pie-stop) and, after a --map run, reopen DevTestMap. Returns False on a failed reopen.

    Waits until PIE has really ended (end_play is a request; the editor can be busy for seconds tearing PIE down,
    e.g. reporting a world a smoke script still references), so a run started right after this one finds it idle."""
    if args.no_pie_stop:
        if opened_map:
            log(f"PIE left running on {opened_map}: {DEFAULT_MAP} not reopened")
        return True
    drop_world_refs()
    stop_pie()
    ended = wait_for_pie_end(timeout=60)
    if not opened_map:
        if not ended:
            log("WARNING: PIE has not ended yet")
        return True
    if not ended:
        log(f"ERROR: PIE did not end: {DEFAULT_MAP} not reopened")
        return False
    ok, output = open_map(DEFAULT_MAP)
    if ok:
        log(f"Reopened {DEFAULT_MAP}.")
    else:
        log(f"ERROR: could not reopen {DEFAULT_MAP}: {output.strip()}")
    return ok


def wait_for_pie_world(timeout=30):
    """Wait for the PIE world and player pawn to be ready."""
    log("Waiting for PIE world and player spawn...")
    # Single-statement form (ExecuteStatement mode requires one expression/statement). The world and pawn stay
    # local to a function: a global would keep the PIE world alive after PIE ends.
    check_cmd = (
        "exec(\"import unreal\\n"
        "def _run_pie_verify_ready():\\n"
        "    w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()\\n"
        "    if not w:\\n"
        "        return 'NO_PIE_WORLD'\\n"
        "    return 'PIE_READY' if unreal.GameplayStatics.get_player_pawn(w, 0) else 'NO_PAWN'\\n"
        "print(_run_pie_verify_ready())\\n"
        "del _run_pie_verify_ready\")"
    )

    start = time.time()
    while time.time() - start < timeout:
        success, output = remote_exec_cmd(check_cmd, timeout=10)
        if success and "PIE_READY" in output:
            log("PIE world ready, player spawned.")
            return True
        time.sleep(2)

    log(f"PIE world not ready after {timeout}s")
    return False


def discover_smoke_tests(filter_names=None):
    """Find all verify_pie_*.py scripts."""
    pattern = os.path.join(SCRIPT_DIR, "verify_pie_*.py")
    tests = sorted(glob.glob(pattern))

    if filter_names:
        filtered = []
        for t in tests:
            basename = os.path.basename(t)
            # verify_pie_hud.py -> "hud"
            test_name = basename.replace("verify_pie_", "").replace(".py", "")
            if test_name in filter_names:
                filtered.append(t)
        tests = filtered

    return tests


def main():
    parser = argparse.ArgumentParser(description="PIE Verification Pipeline")
    parser.add_argument("--no-launch", action="store_true",
                        help="Skip editor launch (assume already running)")
    parser.add_argument("--no-pie-stop", action="store_true",
                        help="Leave PIE running after tests")
    parser.add_argument("--tests", type=str, default=None,
                        help="Comma-separated list of test names (e.g., hud,spawn)")
    parser.add_argument("--editor-timeout", type=int, default=300,
                        help="Max seconds to wait for editor (default: 300)")
    parser.add_argument("--pie-timeout", type=int, default=30,
                        help="Max seconds to wait for PIE ready (default: 30)")
    parser.add_argument("--map", type=str, default=None,
                        help="Package path of the map to PIE (e.g. /MordecaiCore/Game/Maps/MovementTestMap). "
                             f"Opened before PIE; {DEFAULT_MAP} is reopened after PIE stops. Test maps: "
                             + ", ".join(TEST_MAPS) + f" (default tests there: {','.join(ANY_MAP_TESTS)}). "
                             "Default: the map the editor has open.")
    args = parser.parse_args()

    map_path = normalize_map(args.map) if args.map else None
    if map_path is not None and not map_path.startswith("/"):
        # Git Bash rewrites a leading-slash argument into a Windows path (C:/Program Files/Git/MordecaiCore/...)
        log(f"ERROR: --map wants a package path such as {TEST_MAPS[0]}, got {args.map!r}. From Git Bash, run with "
            "MSYS_NO_PATHCONV=1 (or from PowerShell): Git Bash rewrites arguments that start with '/'.")
        sys.exit(2)
    filter_names = None
    if args.tests:
        filter_names = [t.strip() for t in args.tests.split(",")]
    elif is_test_map(map_path):
        filter_names = list(ANY_MAP_TESTS)
        log(f"{map_path} is a test map: running the smoke tests for any map ({','.join(filter_names)})")

    # --- Step 1: Ensure editor is running ---
    log("Checking editor status...")
    if run_check_editor():
        log("Editor already running and ready.")
    elif args.no_launch:
        log("ERROR: Editor not running and --no-launch specified.")
        sys.exit(2)
    else:
        launch_editor()
        log(f"Waiting up to {args.editor_timeout}s for editor to be ready...")
        if not run_check_editor(wait=True, timeout=args.editor_timeout):
            log("ERROR: Editor did not become ready in time.")
            sys.exit(2)

    # --- Step 1b: Open the requested map (PIE plays the open map) ---
    opened_map = None
    if map_path:
        log(f"Opening {map_path}...")
        ok, output = open_map(map_path)
        if not ok:
            log(f"ERROR: could not open {map_path}: {output.strip()}")
            sys.exit(2)
        opened_map = map_path if map_path.lower() != DEFAULT_MAP.lower() else None
        log(f"{map_path} is open.")

    # --- Step 2: Start PIE ---
    if not start_pie():
        log("WARNING: PIE start command may have failed, continuing anyway...")

    # Give PIE a moment to initialize
    time.sleep(3)

    if not wait_for_pie_world(timeout=args.pie_timeout):
        log("ERROR: PIE world never became ready.")
        finish_session(args, opened_map)
        sys.exit(2)

    # --- Step 3: Run smoke tests ---
    tests = discover_smoke_tests(filter_names)
    if not tests:
        log("WARNING: No smoke tests found!")
        finish_session(args, opened_map)
        sys.exit(0)

    log(f"Running {len(tests)} smoke test(s)...")
    print("")

    results = {}
    for test_path in tests:
        test_name = os.path.basename(test_path)
        success = True
        phase = 1
        while True:
            log(f"Running: {test_name}" + (f" (phase {phase})" if phase > 1 else ""))
            ok, output = remote_exec(test_path, timeout=30)
            success = success and ok

            # Print the test output indented
            for line in output.strip().splitlines():
                print(f"    {line}")
            print("")

            if not ok or PHASE_MARKER not in output:
                break
            if phase >= MAX_PHASES:
                log(f"ERROR: {test_name} still asks for another phase after {MAX_PHASES} runs")
                success = False
                break
            phase += 1
            time.sleep(PHASE_DELAY_S)  # let the editor tick before the next phase
        results[test_name] = success

    # --- Step 4: Stop PIE (and reopen DevTestMap after a --map run) ---
    restored = finish_session(args, opened_map)

    # --- Step 5: Report ---
    print("=" * 60)
    print("PIE VERIFICATION SUMMARY")
    print("=" * 60)

    passed = sum(1 for v in results.values() if v)
    failed = sum(1 for v in results.values() if not v)

    for test_name, success in results.items():
        status = "PASS" if success else "FAIL"
        print(f"  [{status}] {test_name}")

    print(f"\n{passed} passed, {failed} failed out of {len(results)} test(s)"
          + (f" on {map_path}" if map_path else ""))

    if failed > 0:
        sys.exit(1)
    elif not restored:
        log(f"All PIE smoke tests passed, but the editor is not back on {DEFAULT_MAP}.")
        sys.exit(2)
    else:
        log("All PIE smoke tests passed.")
        sys.exit(0)


if __name__ == "__main__":
    main()
