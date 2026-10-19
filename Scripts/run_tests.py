#!/usr/bin/env python3
"""Headless Automation Test Runner — Tier 1

Runs UE automation tests headless (NullRHI) and decides pass/fail by parsing the
log, NOT the process exit code. UnrealEditor-Cmd exits 0 even when tests fail
when it is stopped via -TestExit, so the exit code alone reports false passes.

Usage:
  py Scripts/run_tests.py                         # all Mordecai tests
  py Scripts/run_tests.py Mordecai.GameMode       # one group (name prefix)
  py Scripts/run_tests.py --log path/to/out.log   # choose the log location

Exit codes:
  0 — at least one test ran and every test succeeded
  1 — one or more tests failed
  2 — infrastructure error (no tests matched, editor crashed, queue never finished)
"""

import argparse
import datetime
import os
import re
import subprocess
import sys

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
UPROJECT = os.path.join(PROJECT_ROOT, "ProjectMordecai.uproject")
EDITOR_CMD = os.path.join(
    os.path.dirname(PROJECT_ROOT), "UnrealEngine", "Engine", "Binaries", "Win64", "UnrealEditor-Cmd.exe",
)

RESULT_RE = re.compile(r"Test Completed\. Result=\{(?P<result>[^}]*)\} Name=\{(?P<name>[^}]*)\} Path=\{(?P<path>[^}]*)\}")
QUEUE_EMPTY = "Automation Test Queue Empty"


def log(msg):
    print(f"[Tests] {msg}", flush=True)


def parse_results(lines):
    """Returns (results, details, queue_empty). results: list of (result, path); details: path -> error lines."""
    results = []
    details = {}
    queue_empty = False
    current_fail = None
    for line in lines:
        match = RESULT_RE.search(line)
        if match:
            result, path = match.group("result"), match.group("path")
            results.append((result, path))
            current_fail = path if result != "Success" else None
            continue
        if QUEUE_EMPTY in line:
            queue_empty = True
        # Expectation failures are logged right after their "Test Completed" line
        if current_fail and "LogAutomationController: Error:" in line:
            details.setdefault(current_fail, []).append(line.split("Error:", 1)[1].strip())
    return results, details, queue_empty


def main():
    parser = argparse.ArgumentParser(description="Headless automation test runner")
    parser.add_argument("filter", nargs="?", default="Mordecai",
                        help="Test name prefix (default: Mordecai)")
    parser.add_argument("--log", default=None,
                        help="Log file path (default: Saved/TestLogs/tests-<timestamp>.log)")
    args = parser.parse_args()

    if not os.path.isfile(EDITOR_CMD):
        log(f"ERROR: UnrealEditor-Cmd not found at {EDITOR_CMD}")
        sys.exit(2)

    log_path = args.log or os.path.join(
        PROJECT_ROOT, "Saved", "TestLogs", f"tests-{datetime.datetime.now():%Y%m%d-%H%M%S}.log")
    os.makedirs(os.path.dirname(os.path.abspath(log_path)), exist_ok=True)

    cmd = [
        EDITOR_CMD, UPROJECT,
        f"-ExecCmds=Automation RunTests {args.filter}",
        "-Unattended", "-NullRHI", "-NoSound", "-NoSplash", "-Log",
        "-LogCmds=LogAutomationTest Display",
        f"-TestExit={QUEUE_EMPTY}",
        f"-abslog={log_path}",
    ]
    log(f"Running tests matching '{args.filter}' (log: {log_path})")
    proc = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    if not os.path.isfile(log_path):
        log(f"ERROR: editor produced no log (exit code {proc.returncode})")
        sys.exit(2)
    with open(log_path, encoding="utf-8-sig", errors="replace") as f:
        results, details, queue_empty = parse_results(f.readlines())

    passed = [p for r, p in results if r == "Success"]
    failed = [(r, p) for r, p in results if r != "Success"]

    print("")
    print("=" * 60)
    print(f"AUTOMATION TESTS: {args.filter}")
    print("=" * 60)
    for result, path in failed:
        print(f"  [{result.upper()}] {path}")
        for detail in details.get(path, []):
            print(f"      {detail}")
    print(f"\n{len(passed)} passed, {len(failed)} failed out of {len(results)} test(s)")
    print(f"Log: {log_path}")

    if failed:
        sys.exit(1)
    if not results:
        log(f"ERROR: no tests matched '{args.filter}' (editor exit code {proc.returncode})")
        sys.exit(2)
    if not queue_empty:
        log(f"ERROR: test queue never finished (editor exit code {proc.returncode}) — crash or hang?")
        sys.exit(2)
    log("All tests passed.")
    sys.exit(0)


if __name__ == "__main__":
    main()
