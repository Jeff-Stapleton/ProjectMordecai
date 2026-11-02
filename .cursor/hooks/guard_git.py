#!/usr/bin/env python3
"""Cursor beforeShellExecution hook: deny the git commands on Project Mordecai's deny list.

Jeff's Claude Code settings (.claude/settings.local.json) deny
    git push --force*    git push -f*    git reset --hard*    git clean *
Cursor imports Claude Code hooks but not Claude Code permissions, and Cloud Agents (the
self-hosted worker included) never ask before running a command, so this hook enforces the same
list for every Cursor agent. It also catches the flag later in the command
(git push origin x --force), a +refspec force push, git -C <dir> ..., every command of a chain
(a; b && c | d), PowerShell script blocks and subexpressions, and a command nested in
powershell -Command, cmd /c, bash -c or Invoke-Expression.

Protocol (.cursor/hooks.json): the hook JSON arrives on stdin ({"command": ..., "cwd": ...});
one JSON object goes to stdout: {"permission": "allow"}, or "deny" with a user_message and an
agent_message. It always exits 0 with valid JSON, because a permission hook whose output is not
valid JSON blocks the command; input it cannot read is allowed (Cursor's own fail-open default)
and reported on stderr, which the Hooks output channel shows.
"""

import json
import re
import shlex
import sys

SEPARATORS = set(";&|(){}\r\n")
PREFIX_WORDS = {"&", ".", "call", "command", "exec", "env", "sudo", "nohup", "time"}
GIT_OPTIONS_WITH_VALUE = {"-C", "-c", "--git-dir", "--work-tree", "--namespace", "--config-env",
                          "--exec-path", "--super-prefix", "--attr-source"}
SHORT_FORCE = re.compile(r"-[A-Za-z]*f[A-Za-z]*")
SH_COMMAND_FLAG = re.compile(r"-[a-z]*c[a-z]*")
ENV_ASSIGNMENT = re.compile(r"[A-Za-z_][A-Za-z0-9_]*=.*")
MAX_NESTING = 3

AGENT_MESSAGE = (
    "Denied by the project hook .cursor/hooks/guard_git.py: force pushes (--force, "
    "--force-with-lease, -f, +refspec), git reset --hard and git clean are on Project "
    "Mordecai's deny list, the same list as Jeff's Claude Code settings. Do not retry it in "
    "another form. Do not rewrite history or delete local files: add a new commit or use "
    "git revert, and ask Jeff if you think one of these is needed.")


def split_commands(command):
    """Simple commands of a command line, split at unquoted ; & | && || ( ) { } and line breaks."""
    parts, current, quote, i = [], [], None, 0
    while i < len(command):
        ch = command[i]
        if quote:
            current.append(ch)
            if quote == '"' and ch in ("\\", "`") and i + 1 < len(command):
                current.append(command[i + 1])
                i += 2
                continue
            if ch == quote:
                quote = None
        elif ch in ("'", '"'):
            quote = ch
            current.append(ch)
        elif ch in SEPARATORS:
            parts.append("".join(current))
            current = []
        else:
            current.append(ch)
        i += 1
    parts.append("".join(current))
    return [p.strip() for p in parts if p.strip()]


def words(simple_command):
    try:
        return shlex.split(simple_command, posix=True)
    except ValueError:
        return simple_command.split()


def program_name(word):
    name = word.replace("\\", "/").rsplit("/", 1)[-1].lower()
    for ext in (".exe", ".cmd", ".bat"):
        if name.endswith(ext):
            return name[:-len(ext)]
    return name


def git_reason(args):
    """Why `git <args>` is denied, or None. args: the words after git."""
    i = 0
    while i < len(args) and args[i].startswith("-"):
        i += 2 if args[i] in GIT_OPTIONS_WITH_VALUE else 1
    if i >= len(args):
        return None
    sub, rest = args[i], args[i + 1:]
    if sub == "push":
        for arg in rest:
            if arg.startswith("--force") or SHORT_FORCE.fullmatch(arg):
                return f"git push {arg} (force push)"
            if arg.startswith("+") and len(arg) > 1:
                return f"git push {arg} (a + refspec is a force push)"
    elif sub == "reset" and "--hard" in rest:
        return "git reset --hard"
    elif sub == "clean":
        return "git clean"
    return None


def nested_command(program, argv):
    """The command string a shell wrapper runs (powershell -Command x, cmd /c x, bash -c x, iex x)."""
    if program in ("invoke-expression", "iex"):
        return " ".join(a for a in argv[1:] if a.lower() != "-command")
    for i, arg in enumerate(argv[1:], start=1):
        flag = arg.lower()
        if ((program in ("powershell", "pwsh") and flag in ("-c", "-command"))
                or (program == "cmd" and flag in ("/c", "/k"))
                or (program in ("bash", "sh", "zsh") and SH_COMMAND_FLAG.fullmatch(flag))):
            return " ".join(argv[i + 1:])
    return None


def deny_reason(command, depth=0):
    """Why the command line is denied, or None if it may run."""
    for simple in split_commands(command):
        argv = words(simple)
        while argv and (argv[0].lower() in PREFIX_WORDS or ENV_ASSIGNMENT.fullmatch(argv[0])):
            argv = argv[1:]
        if not argv:
            continue
        program = program_name(argv[0])
        if program == "git":
            reason = git_reason(argv[1:])
        elif depth < MAX_NESTING:
            inner = nested_command(program, argv)
            reason = deny_reason(inner, depth + 1) if inner else None
        else:
            reason = None
        if reason:
            return reason
    return None


def decide(payload):
    command = payload.get("command") if isinstance(payload, dict) else None
    if not isinstance(command, str):
        return {"permission": "allow"}
    reason = deny_reason(command)
    if not reason:
        return {"permission": "allow"}
    return {"permission": "deny",
            "user_message": f"Blocked by .cursor/hooks/guard_git.py: {reason} is on Project "
                            "Mordecai's git deny list (the same list as Claude Code's).",
            "agent_message": AGENT_MESSAGE}


def main():
    try:
        raw = sys.stdin.buffer.read().decode("utf-8", errors="replace").lstrip("\ufeff")
        response = decide(json.loads(raw) if raw.strip() else {})
    except Exception as e:  # a crash must still print valid JSON, or Cursor blocks the command
        sys.stderr.write(f"guard_git: could not check the command ({e!r}); allowing it\n")
        response = {"permission": "allow"}
    sys.stdout.write(json.dumps(response) + "\n")
    sys.stdout.flush()
    return 0


if __name__ == "__main__":
    sys.exit(main())
