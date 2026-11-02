#!/usr/bin/env python3
"""Cursor Config Sync -- generate Cursor's copies of the Claude Code agent config.

The Claude Code config is the source of truth. This script generates Cursor's copies from it:

  .claude/skills/<name>/**     ->  .cursor/skills/<name>/**
      Every file. SKILL.md keeps its frontmatter and body and gains Cursor `metadata` plus a
      one-line "generated" banner; reference files are copied as they are.
  .mcp.json  mcpServers.<name> ->  .cursor/mcp.json  mcpServers.<name>
      A repo-relative path to an existing file in `args` becomes ./<path>. The Cursor desktop app
      starts stdio servers in the user's home folder but resolves ./ arguments against the
      workspace folder; the CLI and the self-hosted worker start them in their own working
      directory (the repo root here) and do not expand ${workspaceFolder} (CLI 2026.10.22), so
      ./<path> is the one form both resolve. Servers that exist only in .cursor/mcp.json are kept.

Edit the Claude originals (a new unreal-tribal-knowledge lesson goes into
.claude/skills/unreal-tribal-knowledge/SKILL.md), run this script, and commit both sides. Never
edit the generated copies: the next run overwrites them. A generated skill whose source is gone
is removed; a .cursor/skills folder without the generated marker is Cursor-only and left alone.
Files are compared with CRLF/LF differences ignored (core.autocrlf), and .cursor/mcp.json by its
parsed content, so reformatting it is not drift.

Usage:
  py Scripts/sync_cursor_config.py           regenerate the Cursor copies
  py Scripts/sync_cursor_config.py --check   report drift and change nothing

Exit codes:
  0 -- in sync (already, or after writing)
  1 -- --check found drift: the listed Cursor files are missing, stale or extra
  2 -- infrastructure problem: a source is missing or cannot be converted (no frontmatter, a
       name that does not match its folder, no description, a metadata key, bad JSON)
"""

import argparse
import json
import os
import re
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)

CLAUDE_SKILLS = ".claude/skills"
CURSOR_SKILLS = ".cursor/skills"
CLAUDE_MCP = ".mcp.json"
CURSOR_MCP = ".cursor/mcp.json"
COMMAND = "py Scripts/sync_cursor_config.py"
GENERATED_MARKER = f"generated-by: {COMMAND}"
SKILL_NAME_RE = re.compile(r"^[a-z0-9]+(-[a-z0-9]+)*$")  # Cursor: lowercase, digits, hyphens
SKIP_DIRS = {"__pycache__"}


class SyncError(Exception):
    """A source this script cannot convert (exit 2)."""


def log(msg):
    print(f"[CursorSync] {msg}", flush=True)


def path_in(root, rel):
    return os.path.join(root, *rel.split("/"))


def read_bytes(path):
    with open(path, "rb") as f:
        return f.read()


def same_text(a, b):
    return a.replace(b"\r\n", b"\n") == b.replace(b"\r\n", b"\n")


# ---------------------------------------------------------------------------
# Skills
# ---------------------------------------------------------------------------

def split_frontmatter(text, where):
    """(frontmatter lines, body) of a SKILL.md: the frontmatter sits between the first two ---."""
    lines = text.splitlines(keepends=True)
    if not lines or lines[0].strip() != "---":
        raise SyncError(f"{where}: no YAML frontmatter (the file must start with a --- line)")
    for i in range(1, len(lines)):
        if lines[i].strip() == "---":
            return lines[1:i], "".join(lines[i + 1:])
    raise SyncError(f"{where}: the frontmatter is not closed by a --- line")


def top_level_keys(frontmatter_lines):
    """{key: raw value} for the unindented `key: value` lines of a YAML frontmatter."""
    keys = {}
    for line in frontmatter_lines:
        if line[:1] in ("", " ", "\t", "#", "\r", "\n") or ":" not in line:
            continue
        key, _, value = line.partition(":")
        keys[key.strip()] = value.strip().strip("'\"")
    return keys


def cursor_skill_md(text, name):
    """The Cursor copy of .claude/skills/<name>/SKILL.md (raises SyncError if it cannot be)."""
    where = f"{CLAUDE_SKILLS}/{name}/SKILL.md"
    newline = "\r\n" if "\r\n" in text else "\n"
    frontmatter, body = split_frontmatter(text, where)
    keys = top_level_keys(frontmatter)
    if keys.get("name") != name:
        raise SyncError(f"{where}: frontmatter name is {keys.get('name')!r}; Cursor requires it to "
                        f"match the folder name {name!r}")
    if not SKILL_NAME_RE.match(name):
        raise SyncError(f"{where}: {name!r} is not a valid Cursor skill name (lowercase letters, "
                        "digits and hyphens)")
    if "description" not in keys:
        raise SyncError(f"{where}: no description (Cursor needs it to pick the skill)")
    if "metadata" in keys:
        raise SyncError(f"{where}: already has a metadata key; this script only adds one")
    metadata = [
        "metadata:",
        f"  generated-from: {where}",
        f"  {GENERATED_MARKER}",
    ]
    banner = (f"> Generated by `{COMMAND}` from `{where}`, the source of truth. Edit that file "
              "(new lessons go there too), then re-run the script; edits to this copy are "
              "overwritten.")
    if not body.startswith(("\n", "\r\n")):
        body = newline + body
    return ("---" + newline + "".join(frontmatter) + newline.join(metadata) + newline + "---"
            + newline + newline + banner + newline + body)


def is_generated_skill(skill_md_path):
    try:
        return GENERATED_MARKER in read_bytes(skill_md_path).decode("utf-8", errors="replace")
    except OSError:
        return False


def expected_skill_files(root):
    """{path under .cursor/skills: bytes} generated from every .claude/skills/<name>/SKILL.md."""
    src_root = path_in(root, CLAUDE_SKILLS)
    if not os.path.isdir(src_root):
        raise SyncError(f"{CLAUDE_SKILLS}/ does not exist")
    expected = {}
    for name in sorted(os.listdir(src_root)):
        src_dir = os.path.join(src_root, name)
        if not os.path.isfile(os.path.join(src_dir, "SKILL.md")):
            continue
        for dirpath, dirnames, filenames in os.walk(src_dir):
            dirnames[:] = sorted(d for d in dirnames if d not in SKIP_DIRS)
            for filename in sorted(filenames):
                rel = os.path.relpath(os.path.join(dirpath, filename), src_root).replace(os.sep, "/")
                data = read_bytes(os.path.join(dirpath, filename))
                if rel == f"{name}/SKILL.md":
                    try:
                        text = data.decode("utf-8")
                    except UnicodeDecodeError as e:
                        raise SyncError(f"{CLAUDE_SKILLS}/{rel}: not UTF-8 ({e})") from e
                    data = cursor_skill_md(text, name).encode("utf-8")
                expected[rel] = data
    return expected


def managed_skill_names(root, expected):
    """Skills this script owns: every Claude skill, plus generated Cursor skills whose source is gone."""
    names = {rel.split("/", 1)[0] for rel in expected}
    dst_root = path_in(root, CURSOR_SKILLS)
    if os.path.isdir(dst_root):
        for name in os.listdir(dst_root):
            if is_generated_skill(os.path.join(dst_root, name, "SKILL.md")):
                names.add(name)
    return names


def actual_skill_files(root, names):
    dst_root = path_in(root, CURSOR_SKILLS)
    actual = set()
    for name in names:
        skill_dir = os.path.join(dst_root, name)
        for dirpath, dirnames, filenames in os.walk(skill_dir):
            dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
            for filename in filenames:
                actual.add(os.path.relpath(os.path.join(dirpath, filename), dst_root)
                           .replace(os.sep, "/"))
    return actual


def skill_drift(root):
    """[(kind, repo path, bytes or None)] needed to bring .cursor/skills in line."""
    expected = expected_skill_files(root)
    actual = actual_skill_files(root, managed_skill_names(root, expected))
    drift = []
    for rel, data in sorted(expected.items()):
        dst = path_in(root, f"{CURSOR_SKILLS}/{rel}")
        if rel not in actual:
            drift.append(("missing", f"{CURSOR_SKILLS}/{rel}", data))
        elif not same_text(read_bytes(dst), data):
            drift.append(("stale", f"{CURSOR_SKILLS}/{rel}", data))
    for rel in sorted(actual - set(expected)):
        drift.append(("extra", f"{CURSOR_SKILLS}/{rel}", None))
    return drift


# ---------------------------------------------------------------------------
# MCP servers
# ---------------------------------------------------------------------------

def load_json(root, rel, required):
    path = path_in(root, rel)
    if not os.path.isfile(path):
        if required:
            raise SyncError(f"{rel} does not exist")
        return None
    try:
        with open(path, encoding="utf-8-sig") as f:
            data = json.load(f)
    except ValueError as e:
        raise SyncError(f"{rel}: invalid JSON ({e})") from e
    if not isinstance(data, dict) or not isinstance(data.get("mcpServers", {}), dict):
        raise SyncError(f"{rel}: expected an object with an mcpServers object")
    return data


def dot_relative_arg(arg, root):
    """A repo-relative path to an existing file, as ./<path>; anything else as is."""
    if (not isinstance(arg, str) or not arg or arg.startswith(("-", "./", ".\\")) or "${" in arg
            or os.path.isabs(arg)):
        return arg
    rel = arg.replace("\\", "/")
    if os.path.isfile(path_in(root, rel)):
        return f"./{rel}"
    return arg


def cursor_mcp_entry(entry, root):
    converted = dict(entry)
    if isinstance(entry.get("args"), list):
        converted["args"] = [dot_relative_arg(a, root) for a in entry["args"]]
    return converted


def expected_cursor_mcp(root):
    """(expected .cursor/mcp.json object, current object or None)."""
    claude = load_json(root, CLAUDE_MCP, required=True)
    current = load_json(root, CURSOR_MCP, required=False)
    expected = json.loads(json.dumps(current)) if current is not None else {}
    servers = expected.setdefault("mcpServers", {})
    for name, entry in claude.get("mcpServers", {}).items():
        servers[name] = cursor_mcp_entry(entry, root)
    return expected, current


def mcp_drift(root):
    expected, current = expected_cursor_mcp(root)
    if current is None:
        return [("missing", CURSOR_MCP, expected)]
    if current != expected:
        return [("stale", CURSOR_MCP, expected)]
    return []


def mcp_newline(root):
    try:
        return "\r\n" if b"\r\n" in read_bytes(path_in(root, CLAUDE_MCP)) else "\n"
    except OSError:
        return "\n"


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def find_drift(root=PROJECT_ROOT):
    """Every Cursor file that differs from what the Claude sources generate (raises SyncError)."""
    return skill_drift(root) + mcp_drift(root)


def apply(root, drift):
    dst_root = path_in(root, CURSOR_SKILLS)
    for kind, rel, data in drift:
        path = path_in(root, rel)
        if kind == "extra":
            os.remove(path)
        elif rel == CURSOR_MCP:
            text = json.dumps(data, indent=2, ensure_ascii=False) + "\n"
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "w", encoding="utf-8", newline="") as f:
                f.write(text.replace("\n", mcp_newline(root)))
        else:
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "wb") as f:
                f.write(data)
        log(f"{'removed' if kind == 'extra' else 'wrote'} {rel}")
    if os.path.isdir(dst_root):
        for dirpath, _, _ in os.walk(dst_root, topdown=False):
            if dirpath != dst_root and not os.listdir(dirpath):
                os.rmdir(dirpath)


def parse_args(argv):
    parser = argparse.ArgumentParser(description="Generate the Cursor copies of the Claude Code "
                                                 "agent config (.cursor/skills, .cursor/mcp.json).")
    parser.add_argument("--check", action="store_true",
                        help="report drift and change nothing (exit 1 on drift)")
    parser.add_argument("--root", default=PROJECT_ROOT, help=argparse.SUPPRESS)
    return parser.parse_args(argv)


def main(argv=None):
    args = parse_args(argv)
    root = os.path.abspath(args.root)
    try:
        drift = find_drift(root)
    except (SyncError, OSError) as e:
        log(f"ERROR: {e}")
        return 2
    if not drift:
        log("in sync: .cursor/skills and .cursor/mcp.json match the Claude originals")
        return 0
    if args.check:
        log(f"OUT OF SYNC: {len(drift)} Cursor file(s) differ from the Claude originals:")
        for kind, rel, _ in drift:
            print(f"  {kind:<7} {rel}", flush=True)
        log(f"fix: {COMMAND}, then commit .cursor/")
        return 1
    try:
        apply(root, drift)
    except OSError as e:
        log(f"ERROR: {e}")
        return 2
    log(f"updated {len(drift)} file(s); commit .cursor/ with the Claude change")
    return 0


if __name__ == "__main__":
    sys.exit(main())
