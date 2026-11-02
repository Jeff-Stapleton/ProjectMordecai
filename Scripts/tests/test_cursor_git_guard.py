"""Tests for .cursor/hooks/guard_git.py, the Cursor hook that enforces the git deny list.

The deny list is Jeff's Claude Code one (git push --force*, git push -f*, git reset --hard*,
git clean *). The hook is a beforeShellExecution hook: JSON in on stdin, one JSON answer out.
"""

import importlib.util
import json
import os
import shutil
import subprocess
import sys
import unittest

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROJECT_ROOT = os.path.dirname(SCRIPTS)
HOOK = os.path.join(PROJECT_ROOT, ".cursor", "hooks", "guard_git.py")
HOOKS_JSON = os.path.join(PROJECT_ROOT, ".cursor", "hooks.json")

_spec = importlib.util.spec_from_file_location("guard_git", HOOK)
guard = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(guard)

DENIED = [
    "git push --force",
    "git push -f origin main",
    "git push --force-with-lease",
    "git push --force-with-lease=main:abc123 origin main",
    "git push --force-if-includes --force-with-lease origin x",
    "git push origin main --force",
    "git push -uf origin cursor/x-71f4",
    "git push origin +main",
    "git push origin +HEAD:refs/heads/x",
    "git reset --hard",
    "git reset --hard origin/main",
    "git reset HEAD~1 --hard",
    "git clean -fdx",
    "git clean -n",
    "git clean",
    "git -C C:\\Users\\jeffd\\repo push --force",
    "git -c core.quotepath=off reset --hard",
    "GIT push --force",
    "cd Source; git push -f",
    "git status && git clean -fd",
    "git status\ngit reset --hard",
    "& git push --force",
    "& 'C:\\Program Files\\Git\\cmd\\git.exe' push --force",
    "& \"C:\\Program Files\\Git\\cmd\\git.exe\" clean -fdx",
    "GIT_TRACE=1 git push -f",
    "powershell -NoProfile -Command \"git push --force\"",
    "pwsh -c 'git reset --hard'",
    "cmd /c git clean -fdx",
    "bash -lc \"git push --force\"",
    "Invoke-Expression 'git clean -fdx'",
    "Get-ChildItem | ForEach-Object { git clean -fdx }",
    "Write-Output $(git reset --hard)",
    "git.exe push --force",
]

ALLOWED = [
    "git status",
    "git push",
    "git push -u origin cursor/chore-cursor-agent-config-71f4",
    "git push origin feature-fix",
    "git push --follow-tags",
    "git push origin main:main",
    "git reset --soft HEAD~1",
    "git reset HEAD Scripts/foo.py",
    "git commit -m \"never git push --force; never git reset --hard\"",
    "git commit -m 'git clean is denied'",
    "git log --oneline -15",
    "git checkout -b cursor/x-71f4",
    "git stash",
    "git lfs ls-files",
    "Get-Content .gitignore",
    "py Scripts/run_python_tests.py",
    "gh pr view 73 --json title",
    "echo git push --force",
    "gitk --all",
    "git status 2>&1",
    "",
]


def run_hook(stdin_bytes, argv=None):
    proc = subprocess.run(argv or [sys.executable, "-X", "utf8", "-B", HOOK], input=stdin_bytes,
                          capture_output=True, timeout=60, cwd=PROJECT_ROOT)
    return proc.returncode, proc.stdout.decode("utf-8", errors="replace"), proc.stderr.decode(
        "utf-8", errors="replace")


class TestDecisions(unittest.TestCase):
    def test_deny_list(self):
        for command in DENIED:
            with self.subTest(command=command):
                self.assertIsNotNone(guard.deny_reason(command))
                answer = guard.decide({"command": command, "cwd": PROJECT_ROOT})
                self.assertEqual(answer["permission"], "deny")
                self.assertIn("guard_git.py", answer["user_message"])
                self.assertIn("ask Jeff", answer["agent_message"])

    def test_everything_else_is_allowed(self):
        for command in ALLOWED:
            with self.subTest(command=command):
                self.assertIsNone(guard.deny_reason(command))
                self.assertEqual(guard.decide({"command": command}), {"permission": "allow"})

    def test_unexpected_payloads_are_allowed(self):
        for payload in ({}, {"command": None}, {"command": 5}, [], "git push --force"):
            with self.subTest(payload=payload):
                self.assertEqual(guard.decide(payload), {"permission": "allow"})


class TestProtocol(unittest.TestCase):
    def test_deny_answer_on_stdout(self):
        code, out, err = run_hook(json.dumps({"command": "git push --force",
                                              "cwd": PROJECT_ROOT}).encode("utf-8"))
        self.assertEqual(code, 0, err)
        answer = json.loads(out)
        self.assertEqual(answer["permission"], "deny")
        self.assertEqual(sorted(answer), ["agent_message", "permission", "user_message"])

    def test_allow_answer_with_a_bom_and_non_ascii(self):
        payload = json.dumps({"command": "git commit -m \"caf\u00e9 \u2014 ok\""},
                             ensure_ascii=False)
        code, out, err = run_hook(b"\xef\xbb\xbf" + payload.encode("utf-8"))
        self.assertEqual((code, json.loads(out)), (0, {"permission": "allow"}), err)

    def test_unreadable_input_is_allowed_and_reported(self):
        code, out, err = run_hook(b"{not json")
        self.assertEqual((code, json.loads(out)), (0, {"permission": "allow"}))
        self.assertIn("guard_git: could not check the command", err)

    @unittest.skipUnless(os.name == "nt" and shutil.which("powershell"),
                         "Cursor pipes hook JSON through PowerShell on Windows")
    def test_json_piped_through_powershell_like_cursor_on_windows(self):
        command = f"$input | & '{sys.executable}' -X utf8 -B '{HOOK}'"
        argv = ["powershell", "-NoProfile", "-NonInteractive", "-Command", command]
        for shell_command, permission in (("git push --force", "deny"), ("git push", "allow")):
            with self.subTest(command=shell_command):
                code, out, err = run_hook(json.dumps({"command": shell_command}).encode("utf-8"),
                                          argv)
                self.assertEqual(code, 0, err)
                self.assertEqual(json.loads(out)["permission"], permission, out)


class TestHooksConfig(unittest.TestCase):
    def test_hooks_json_runs_the_guard_before_git_commands(self):
        with open(HOOKS_JSON, encoding="utf-8") as f:
            cfg = json.load(f)
        self.assertEqual(cfg["version"], 1)
        (entry,) = cfg["hooks"]["beforeShellExecution"]
        self.assertEqual(entry["command"], "py -X utf8 .cursor/hooks/guard_git.py")
        self.assertTrue(os.path.isfile(os.path.join(PROJECT_ROOT, *entry["command"].split()[-1]
                                                    .split("/"))))
        self.assertRegex("GIT push --force", entry["matcher"])
        self.assertRegex("git push --force", entry["matcher"])


if __name__ == "__main__":
    unittest.main()
