#!/usr/bin/env python3
"""Content Commit Guard -- catch "the C++ landed but the content never did".

US-079 ("playable inventory arena") shipped its C++ and its asset-authoring
editor scripts (create_test_pickups.py, place_test_pickups.py) with ZERO
.uasset/.umap files: the scripts were never successfully run + saved, so the
map had nothing to pick up. This guard fails loudly when that happens again.

Check 1 -- uncommitted content
  git status --porcelain=v1 -uall -- '*.uasset' '*.umap'
  Any modified / added / untracked / deleted / renamed asset = FAIL
  ("saved on disk but not committed").

Check 2 -- asset scripts without assets
  Range = merge-base(--base, HEAD)..HEAD   (--base defaults to origin/main)
  An asset-writing script is a .py file under Scripts/ that was added or
  modified in the range -- OR is modified/untracked in the working tree, so
  running this before `git commit` cannot miss a brand-new script -- and
  whose CURRENT content contains any of:
    save_asset(  save_loaded_asset  save_current_level  save_directory(
    save_packages  create_asset(  save_dirty_packages
    save_all_dirty_levels  save_map(  import_asset_tasks(
  If there is >= 1 such script but 0 .uasset/.umap files were added or
  modified (in the range, or on disk awaiting commit -- which check 1 then
  fails) = FAIL, naming the scripts. Asset deletions do not count as content.
  (This file is excluded from its own scan.)
  [no-content] in a commit message exempts the scripts that commit touched
  (with a printed note). A script is exempt only if EVERY commit in the range
  that touched it carries the tag and it has no uncommitted changes; if all
  asset-writing scripts are exempt, check 2 is SKIPPED. Use the tag only when
  the scripts genuinely produce no assets.

Read-only and fast: a handful of git commands, no network (it never fetches;
run `git fetch` first if --base is a stale remote-tracking ref), no index
lock taken. Works from any cwd -- the repo root is resolved from this
script's location (PROJECT_ROOT = parent of Scripts/).

Usage:
  py Scripts/check_content_committed.py                  # both checks vs origin/main
  py Scripts/check_content_committed.py --base main      # compare against local main
  py Scripts/check_content_committed.py --json           # machine-readable output only

Exit codes:
  0 -- all checks passed (check 2 may be skipped via [no-content])
  1 -- a check failed (uncommitted assets, or asset scripts without assets)
  2 -- git error / bad --base ref / not a git repository / internal error
"""

import argparse
import json
import os
import subprocess
import sys

# ---------------------------------------------------------------------------
# Paths / constants
# ---------------------------------------------------------------------------
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)

ASSET_PATHSPECS = ["*.uasset", "*.umap"]
ASSET_EXTS = (".uasset", ".umap")
SCRIPTS_PREFIX = "Scripts/"
SAVE_MARKERS = (
    "save_asset(",
    "save_loaded_asset",
    "save_current_level",
    "save_directory(",
    "save_packages",
    "create_asset(",
    "save_dirty_packages",
    # Other editor-Python calls that write .uasset/.umap files (taught in the
    # unreal-python-editor skill); without them a level script that saves via
    # LevelEditorSubsystem.save_all_dirty_levels() would slip through.
    "save_all_dirty_levels",
    "save_map(",
    "import_asset_tasks(",
)
SKIP_TAG = "[no-content]"
GIT_TIMEOUT_S = 300  # a wedged git must become exit 2, never a hang
MAX_LISTED = 50  # cap for long file lists in console / JSON

_QUIET = False  # set by --json: only the JSON document goes to stdout


def log(msg):
    if not _QUIET:
        print(f"[ContentCheck] {msg}", flush=True)


def out(msg=""):
    if not _QUIET:
        print(msg, flush=True)


class GitError(Exception):
    pass


# ---------------------------------------------------------------------------
# git helpers
# ---------------------------------------------------------------------------
def git_rc(args, root):
    """Run git in root. Returns (returncode, stdout_text, stderr_text)."""
    # --no-optional-locks: `git status` must not grab index.lock (read-only check).
    cmd = ["git", "--no-optional-locks", "-C", root] + args
    try:
        result = subprocess.run(cmd, capture_output=True, timeout=GIT_TIMEOUT_S)
    except FileNotFoundError:
        raise GitError("git executable not found on PATH")
    except subprocess.TimeoutExpired:
        raise GitError(f"`git {' '.join(args)}` timed out after {GIT_TIMEOUT_S}s")
    except OSError as e:
        raise GitError(f"could not run git: {e}")
    stdout = result.stdout.decode("utf-8", errors="replace")
    stderr = result.stderr.decode("utf-8", errors="replace").strip()
    return result.returncode, stdout, stderr


def git(args, root):
    """Run git in root and return stdout; raise GitError on non-zero exit."""
    rc, stdout, stderr = git_rc(args, root)
    if rc != 0:
        raise GitError(f"`git {' '.join(args)}` failed (exit {rc}): {stderr}")
    return stdout


def resolve_repo_root():
    top = git(["rev-parse", "--show-toplevel"], PROJECT_ROOT).strip()
    if not top:
        raise GitError(f"{PROJECT_ROOT} is not inside a git repository")
    return os.path.normpath(top)


def self_relpath(root):
    """This script's path relative to the repo root, posix style, LOWER-CASED.

    Compared case-insensitively: on Windows, invoking it via a lower-case
    "scripts" folder gives a lower-case __file__, which must still match git's
    "Scripts/..." path (otherwise the guard flags itself as an asset script).
    """
    try:
        rel = os.path.relpath(os.path.abspath(__file__), root).replace(os.sep, "/")
    except ValueError:  # different drive
        rel = SCRIPTS_PREFIX + os.path.basename(__file__)
    return rel.lower()


def is_asset(path):
    return path.lower().endswith(ASSET_EXTS)


# ---------------------------------------------------------------------------
# Check 1 -- uncommitted content
# ---------------------------------------------------------------------------
def describe_xy(xy):
    if xy == "??":
        return "untracked"
    if "U" in xy or xy in ("AA", "DD"):
        return "unmerged"
    if "D" in xy:
        return "deleted"
    if "R" in xy:
        return "renamed"
    if "C" in xy:
        return "copied"
    if xy[0] == "A":
        return "added (staged, not committed)"
    return "modified"


def parse_status_z(text):
    """Parse `git status --porcelain=v1 -z` output into entry dicts."""
    tokens = text.split("\0")
    entries = []
    i = 0
    while i < len(tokens):
        tok = tokens[i]
        i += 1
        if len(tok) < 4:
            continue
        xy, path = tok[:2], tok[3:]
        entry = {"xy": xy, "path": path, "state": describe_xy(xy)}
        if "R" in xy or "C" in xy:
            # In -z mode the rename/copy source follows as its own token.
            if i < len(tokens) and tokens[i]:
                entry["from"] = tokens[i]
            i += 1
        entries.append(entry)
    return entries


def check_uncommitted_content(root):
    text = git(["status", "--porcelain=v1", "-uall", "-z", "--"] + ASSET_PATHSPECS, root)
    entries = [e for e in parse_status_z(text) if is_asset(e["path"]) or is_asset(e.get("from", ""))]
    if entries:
        return {
            "status": "FAIL",
            "message": f"{len(entries)} .uasset/.umap file(s) saved on disk but not committed",
            "files": entries,
        }
    return {
        "status": "PASS",
        "message": "no uncommitted .uasset/.umap changes",
        "files": [],
    }


def print_check1(c1):
    log(f"Check 1 (uncommitted content): {c1['status']} -- {c1['message']}")
    files = c1["files"]
    for e in files[:MAX_LISTED]:
        src = f"  (from {e['from']})" if e.get("from") else ""
        out(f"    {e['xy']} {e['path']}  [{e['state']}]{src}")
    if len(files) > MAX_LISTED:
        out(f"    ... and {len(files) - MAX_LISTED} more")
    if files:
        log("  -> commit them (git add <files>), or revert/delete them if they were not meant to change.")


# ---------------------------------------------------------------------------
# Check 2 -- asset scripts without assets
# ---------------------------------------------------------------------------
def read_current_content(root, rel):
    """Current content of a repo file: working tree first, then HEAD. None if gone."""
    full = os.path.join(root, *rel.split("/"))
    try:
        with open(full, "r", encoding="utf-8", errors="replace") as f:
            return f.read()
    except OSError:
        pass
    rc, stdout, _ = git_rc(["show", f"HEAD:{rel}"], root)
    return stdout if rc == 0 else None


def parse_name_status_z(text):
    """Parse `git diff --name-status --no-renames -z` into (status, path) pairs."""
    tokens = text.split("\0")
    pairs = []
    i = 0
    while i + 1 < len(tokens):
        status, path = tokens[i], tokens[i + 1]
        i += 2
        if not status:
            break
        pairs.append((status, path))
    return pairs


def parse_log_z(text):
    """Parse `git log -z --name-only --format=%x1e%H%x1f%h%x1f%B%x1f` into commit dicts.

    Each record is: RS sha US short US body US NUL LF file NUL file NUL ...
    (merge commits list no files).
    """
    commits = []
    for rec in text.split("\x1e"):
        parts = rec.split("\x1f")
        if len(parts) < 4:
            continue
        files = []
        for tok in parts[3].split("\0"):
            tok = tok.lstrip("\r\n")
            if tok:
                files.append(tok)
        commits.append({
            "sha": parts[0].strip(),
            "short": parts[1].strip(),
            "tagged": SKIP_TAG in parts[2].lower(),
            "files": files,
        })
    return commits


def uncommitted_script_entries(root):
    """Scripts/**.py entries that are modified/staged/untracked/deleted in the working tree."""
    text = git(["status", "--porcelain=v1", "-uall", "-z", "--", SCRIPTS_PREFIX.rstrip("/")], root)
    return [e for e in parse_status_z(text)
            if e["path"].startswith(SCRIPTS_PREFIX) and e["path"].lower().endswith(".py")]


def file_exists(root, rel):
    return os.path.isfile(os.path.join(root, *rel.split("/")))


def check_scripts_without_assets(root, base, report, uncommitted_assets):
    if not base or base.startswith("-"):
        raise GitError(f"bad --base ref '{base}'")
    rc, base_out, _ = git_rc(["rev-parse", "--verify", "--quiet", f"{base}^{{commit}}"], root)
    base_sha = base_out.strip()
    if rc != 0 or not base_sha:
        raise GitError(
            f"bad --base ref '{base}' (does not resolve to a commit; "
            f"run `git fetch` or pass e.g. --base main)"
        )
    head = git(["rev-parse", "--verify", "HEAD^{commit}"], root).strip()
    rc, mb_out, mb_err = git_rc(["merge-base", base_sha, head], root)
    merge_base = mb_out.strip()
    if rc != 0 or not merge_base:
        raise GitError(f"no merge-base between '{base}' and HEAD (unrelated histories?) {mb_err}".rstrip())
    report["head"] = head
    report["merge_base"] = merge_base
    rng = f"{merge_base}..{head}"

    result = {
        "status": "PASS",
        "message": "",
        "range": f"{merge_base[:10]}..{head[:10]}",
        "commits": 0,
        "no_content_commits": [],
        "asset_writing_scripts": [],
        "asset_changes": 0,          # .uasset/.umap added/modified in the committed range
        "asset_files": [],
        "asset_deletions": 0,        # deletions are reported but never count as content
        "uncommitted_assets": 0,     # on disk, awaiting commit (check 1 fails on these)
    }

    # Commits in range, their messages ([no-content]) and the files each touched.
    # rev-list/log over the full range (no pathspec) so no history simplification hides commits.
    log_text = git(["log", "--no-show-signature", "--no-renames", "--name-only", "-z",
                    "--format=%x1e%H%x1f%h%x1f%B%x1f", rng], root)
    commits = parse_log_z(log_text)
    result["commits"] = len(commits)
    result["no_content_commits"] = [c["short"] for c in commits if c["tagged"]]
    touched_by = {}
    for c in commits:
        for f in c["files"]:
            touched_by.setdefault(f, []).append(c)

    # Net tree diff of the committed range.
    diff_text = git(["diff", "--name-status", "--no-renames", "-z", merge_base, head], root)
    changes = parse_name_status_z(diff_text)

    written = [p for s, p in changes if is_asset(p) and s[:1] in ("A", "M", "T")]
    result["asset_changes"] = len(written)
    result["asset_files"] = written[:MAX_LISTED]
    result["asset_deletions"] = sum(1 for s, p in changes if is_asset(p) and s[:1] == "D")
    on_disk = [e["path"] for e in uncommitted_assets if file_exists(root, e["path"])]
    result["uncommitted_assets"] = len(on_disk)

    # Candidate scripts: changed in the committed range, plus uncommitted working-tree changes.
    candidates = {}  # path -> change label
    for status, path in changes:
        if status[:1] in ("A", "M", "T") and path.startswith(SCRIPTS_PREFIX) and path.lower().endswith(".py"):
            candidates[path] = status[:1]
    uncommitted = set()
    for e in uncommitted_script_entries(root):
        if "D" in e["xy"] and e["path"] not in candidates:
            continue  # deleting a script needs no assets
        candidates[e["path"]] = f"uncommitted ({e['state']})"
        uncommitted.add(e["path"])

    me = self_relpath(root)
    for path in sorted(candidates):
        if path.lower() == me:
            continue
        content = read_current_content(root, path)
        if content is None:
            continue
        hits = [m for m in SAVE_MARKERS if m in content]
        if not hits:
            continue
        touching = touched_by.get(path, [])
        exempt = (path not in uncommitted) and bool(touching) and all(c["tagged"] for c in touching)
        result["asset_writing_scripts"].append(
            {"path": path, "change": candidates[path], "markers": hits, "exempt": exempt})

    scripts = result["asset_writing_scripts"]
    active = [s for s in scripts if not s["exempt"]]
    n_exempt = len(scripts) - len(active)
    n_unc = sum(1 for s in active if s["path"] in uncommitted)
    n_assets = result["asset_changes"] + result["uncommitted_assets"]
    scope = f"{result['commits']} commit(s)" + (", range empty: HEAD is at the merge-base" if not commits else "")
    if not scripts:
        result["message"] = (
            f"no asset-writing scripts added/modified ({scope}, "
            f"{result['asset_changes']} asset(s) added/modified)"
        )
    elif not active:
        result["status"] = "SKIP"
        result["message"] = (
            f"skipped: all {len(scripts)} asset-writing script(s) were only touched by {SKIP_TAG} "
            f"commit(s) {', '.join(result['no_content_commits'])}"
        )
    elif n_assets == 0:
        result["status"] = "FAIL"
        result["message"] = (
            f"{len(active)} asset-writing script(s) added/modified ({scope}; {n_unc} uncommitted) "
            f"but 0 .uasset/.umap files added/modified in the range or on disk"
        )
    else:
        unc_note = (f" + {result['uncommitted_assets']} uncommitted (see check 1)"
                    if result["uncommitted_assets"] else "")
        result["message"] = (
            f"{len(active)} asset-writing script(s) and {result['asset_changes']} .uasset/.umap "
            f"file(s) added/modified in range{unc_note}"
        )
    if n_exempt and active:
        result["message"] += f" ({n_exempt} more exempt via {SKIP_TAG})"
    return result


def print_check2(c2, base):
    log(f"Check 2 (asset scripts without assets) vs {base} [{c2['range']}]: {c2['status']} -- {c2['message']}")
    if c2["no_content_commits"]:
        log(f"  NOTE: {SKIP_TAG} in commit(s) {', '.join(c2['no_content_commits'])}: exempts only scripts "
            f"whose every commit in the range carries the tag (and that have no uncommitted changes).")
    for s in c2["asset_writing_scripts"]:
        ex = f"  [exempt: {SKIP_TAG}]" if s["exempt"] else ""
        out(f"    {s['change']} {s['path']}  (writes assets: {', '.join(s['markers'])}){ex}")
    if c2["status"] == "FAIL":
        log("  -> run these scripts in the editor, confirm the .uasset/.umap files are on disk, and COMMIT them;")
        log(f"     or put {SKIP_TAG} in the message of the commit(s) adding the scripts if they genuinely")
        log("     produce no assets.")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    global _QUIET
    parser = argparse.ArgumentParser(
        description="Content commit guard: fail on uncommitted .uasset/.umap files, "
                    "or on asset-writing Scripts/*.py changes with no committed assets.")
    parser.add_argument("--base", type=str, default="origin/main",
                        help="Base ref; check 2 inspects merge-base(BASE, HEAD)..HEAD (default: origin/main)")
    parser.add_argument("--json", action="store_true",
                        help="Print only a machine-readable JSON report to stdout")
    args = parser.parse_args()
    _QUIET = args.json

    try:
        sys.stdout.reconfigure(errors="replace")
    except (AttributeError, ValueError):
        pass

    report = {
        "result": None,
        "exit_code": None,
        "repo_root": None,
        "base": args.base,
        "merge_base": None,
        "head": None,
        "checks": {},
        "error": None,
    }

    try:
        root = resolve_repo_root()
        report["repo_root"] = root
        log(f"Repo: {root}")

        c1 = check_uncommitted_content(root)
        report["checks"]["uncommitted_content"] = c1
        print_check1(c1)

        c2 = check_scripts_without_assets(root, args.base, report, c1["files"])
        report["checks"]["scripts_without_assets"] = c2
        print_check2(c2, args.base)
    except Exception as e:  # GitError, or anything unexpected: exit 2, never a traceback's exit 1 (= FAIL)
        report["result"] = "ERROR"
        report["exit_code"] = 2
        report["error"] = str(e) if isinstance(e, GitError) else f"{type(e).__name__}: {e}"
        if _QUIET:
            print(json.dumps(report, indent=2))
        else:
            log(f"ERROR: {e}")
        sys.exit(2)

    failed = [name for name, c in report["checks"].items() if c["status"] == "FAIL"]
    report["result"] = "FAIL" if failed else "PASS"
    report["exit_code"] = 1 if failed else 0

    if _QUIET:
        print(json.dumps(report, indent=2))
    else:
        out("")
        out("=" * 60)
        out("CONTENT COMMIT CHECK")
        out("=" * 60)
        for name, c in report["checks"].items():
            out(f"  [{c['status']}] {name}: {c['message']}")
        out(f"\nRESULT: {report['result']}")

    sys.exit(report["exit_code"])


if __name__ == "__main__":
    main()
