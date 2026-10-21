"""Exit codes of Scripts/run_python_tests.py, run in isolated interpreters on throwaway suites."""

import os
import subprocess
import sys
import tempfile
import unittest

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

PASSING = "import unittest\nclass T(unittest.TestCase):\n    def test_ok(self):\n        pass\n"
FAILING = ("import unittest\nclass T(unittest.TestCase):\n    def test_bad(self):\n"
           "        self.assertEqual(1, 2)\n")
BROKEN = "import this_module_does_not_exist_anywhere\n"


class TestRunnerExitCodes(unittest.TestCase):
    def run_suite(self, files, *args):
        with tempfile.TemporaryDirectory() as tmp:
            pkg = os.path.join(tmp, "pyrunner_suite")
            os.makedirs(pkg)
            with open(os.path.join(pkg, "__init__.py"), "w", encoding="utf-8"):
                pass
            for name, text in files.items():
                with open(os.path.join(pkg, name), "w", encoding="utf-8") as f:
                    f.write(text)
            code = ("import sys\n"
                    f"sys.path.insert(0, {SCRIPTS!r})\n"
                    "import run_python_tests as r\n"
                    f"r.SCRIPT_DIR = {tmp!r}\n"
                    f"r.TESTS_DIR = {pkg!r}\n"
                    f"sys.exit(r.main({list(args)!r}))\n")
            proc = subprocess.run([sys.executable, "-c", code], capture_output=True, text=True,
                                  timeout=120)
            return proc.returncode, proc.stdout + proc.stderr

    def test_all_pass(self):
        code, out = self.run_suite({"test_a.py": PASSING})
        self.assertEqual(code, 0, out)
        self.assertIn("1 run, 0 failed", out)

    def test_failure(self):
        code, out = self.run_suite({"test_a.py": PASSING, "test_b.py": FAILING})
        self.assertEqual(code, 1, out)

    def test_import_error(self):
        code, out = self.run_suite({"test_a.py": PASSING, "test_c.py": BROKEN})
        self.assertEqual(code, 2, out)

    def test_nothing_found(self):
        code, out = self.run_suite({"helper.py": PASSING})
        self.assertEqual(code, 2, out)
        code, out = self.run_suite({"test_a.py": PASSING}, "-k", "no_such_test")
        self.assertEqual(code, 2, out)

    def test_name_filter(self):
        code, out = self.run_suite({"test_a.py": PASSING, "test_b.py": FAILING}, "-k", "test_ok")
        self.assertEqual(code, 0, out)


if __name__ == "__main__":
    unittest.main()
