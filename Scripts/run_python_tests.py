#!/usr/bin/env python3
"""Python Tooling Test Runner -- offline unit tests for the playtest scripts

Runs unittest discovery over Scripts/tests (test_*.py, top-level dir Scripts/, so tests import
agent_bridge, playtest_boot and tests.fake_bridge directly).  These tests use fakes
(tests/fake_bridge.py, tests/fake_game.py) and never launch the real game or the editor.  They
gate the Python tooling; they are not Tier 1 (UE automation) and not playtest evidence.

Usage:
  py Scripts/run_python_tests.py                    # everything
  py Scripts/run_python_tests.py -k walk_to         # only test names matching *walk_to*
  py Scripts/run_python_tests.py -v                 # one line per test
  py Scripts/run_python_tests.py --pattern "test_agent_*.py"

Exit codes:
  0 -- at least one test ran and every test passed (skips allowed)
  1 -- one or more tests failed or errored
  2 -- infrastructure problem (no tests found, a test module failed to import)
"""

import argparse
import os
import sys
import time
import unittest

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
TESTS_DIR = os.path.join(SCRIPT_DIR, "tests")


def log(msg):
    print(f"[PyTests] {msg}", flush=True)


def main(argv=None):
    parser = argparse.ArgumentParser(description="Run the offline Python tooling tests.")
    parser.add_argument("-k", dest="names", action="append", default=[],
                        help="only run test methods/classes matching this substring (repeatable)")
    parser.add_argument("--pattern", default="test_*.py", help="test file pattern (test_*.py)")
    parser.add_argument("-v", "--verbose", action="store_true", help="one line per test")
    parser.add_argument("--failfast", action="store_true", help="stop at the first failure")
    args = parser.parse_args(argv)

    if not os.path.isdir(TESTS_DIR):
        log(f"ERROR: no test directory {TESTS_DIR}")
        return 2
    sys.dont_write_bytecode = True  # keep Scripts/ free of __pycache__ noise (it is ignored anyway)
    loader = unittest.TestLoader()
    if args.names:
        loader.testNamePatterns = [f"*{n}*" for n in args.names]
    suite = loader.discover(start_dir=TESTS_DIR, pattern=args.pattern, top_level_dir=SCRIPT_DIR)
    total = suite.countTestCases()
    if loader.errors:
        log("ERROR: test discovery failed:")
        for err in loader.errors:
            print(err, flush=True)
        return 2
    if total == 0:
        log(f"ERROR: no tests found in {TESTS_DIR} (pattern {args.pattern}"
            + (f", names {args.names}" if args.names else "") + ")")
        return 2

    log(f"Running {total} test(s) from {TESTS_DIR}")
    t0 = time.time()
    runner = unittest.TextTestRunner(verbosity=2 if args.verbose else 1, failfast=args.failfast,
                                     stream=sys.stdout)
    result = runner.run(suite)
    elapsed = time.time() - t0

    import_errors = [t for t, _ in result.errors
                     if t.__class__.__name__ in ("_FailedTest", "ModuleImportFailure")]
    print("")
    print("=" * 60)
    print("PYTHON TOOLING TESTS")
    print("=" * 60)
    print(f"{result.testsRun} run, {len(result.failures)} failed, {len(result.errors)} errors, "
          f"{len(result.skipped)} skipped in {elapsed:.1f}s")
    if import_errors:
        log("ERROR: a test module failed to import")
        return 2
    if result.failures or result.errors:
        return 1
    if result.testsRun == 0:
        return 2
    log("All tests passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
