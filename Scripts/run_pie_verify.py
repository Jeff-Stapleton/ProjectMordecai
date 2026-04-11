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

Exit codes:
  0 — all smoke tests passed
  1 — one or more smoke tests failed
  2 — infrastructure error (editor won't start, PIE won't launch, etc.)
"""

import argparse
import glob
import os
import subprocess
import sys
import time

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)

CHECK_EDITOR = os.path.join(
    os.path.expanduser("~"),
    ".openclaw", "workspace", "projects", "mordecai", "scripts", "check_editor.py",
)
UE_REMOTE_EXEC = os.path.join(
    os.path.expanduser("~"),
    ".openclaw", "workspace", "skills", "unreal-python-editor", "scripts", "ue_remote_exec.py",
)
EDITOR_EXE = os.path.join(
    os.path.expanduser("~"),
    "Documents", "Gamedev", "UnrealEngine", "Engine", "Binaries", "Win64", "UnrealEditor.exe",
)
UPROJECT = os.path.join(PROJECT_ROOT, "ProjectMordecai.uproject")


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


def wait_for_pie_world(timeout=30):
    """Wait for the PIE world and player pawn to be ready."""
    log("Waiting for PIE world and player spawn...")
    check_script = (
        "import unreal\n"
        "w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()\n"
        "if not w:\n"
        "    raise RuntimeError('NO_PIE_WORLD')\n"
        "p = unreal.GameplayStatics.get_player_pawn(w, 0)\n"
        "if not p:\n"
        "    raise RuntimeError('NO_PAWN')\n"
        "print('PIE_READY')\n"
    )

    start = time.time()
    while time.time() - start < timeout:
        success, output = remote_exec_cmd(check_script, timeout=10)
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
    args = parser.parse_args()

    filter_names = None
    if args.tests:
        filter_names = [t.strip() for t in args.tests.split(",")]

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

    # --- Step 2: Start PIE ---
    if not start_pie():
        log("WARNING: PIE start command may have failed, continuing anyway...")

    # Give PIE a moment to initialize
    time.sleep(3)

    if not wait_for_pie_world(timeout=args.pie_timeout):
        log("ERROR: PIE world never became ready.")
        if not args.no_pie_stop:
            stop_pie()
        sys.exit(2)

    # --- Step 3: Run smoke tests ---
    tests = discover_smoke_tests(filter_names)
    if not tests:
        log("WARNING: No smoke tests found!")
        if not args.no_pie_stop:
            stop_pie()
        sys.exit(0)

    log(f"Running {len(tests)} smoke test(s)...")
    print("")

    results = {}
    for test_path in tests:
        test_name = os.path.basename(test_path)
        log(f"Running: {test_name}")
        success, output = remote_exec(test_path, timeout=30)
        results[test_name] = success

        # Print the test output indented
        for line in output.strip().splitlines():
            print(f"    {line}")
        print("")

    # --- Step 4: Stop PIE ---
    if not args.no_pie_stop:
        stop_pie()

    # --- Step 5: Report ---
    print("=" * 60)
    print("PIE VERIFICATION SUMMARY")
    print("=" * 60)

    passed = sum(1 for v in results.values() if v)
    failed = sum(1 for v in results.values() if not v)

    for test_name, success in results.items():
        status = "PASS" if success else "FAIL"
        print(f"  [{status}] {test_name}")

    print(f"\n{passed} passed, {failed} failed out of {len(results)} test(s)")

    if failed > 0:
        sys.exit(1)
    else:
        log("All PIE smoke tests passed.")
        sys.exit(0)


if __name__ == "__main__":
    main()
