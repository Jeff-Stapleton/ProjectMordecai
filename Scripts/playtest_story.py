#!/usr/bin/env python3
"""Story Playtest Runner -- a story's Tier 3 boot + Tier 4 real-input playtest, one summary

The contract is Specs/story_playtests_v1.md sections 3, 4 and 6.  A story id maps to its Tier 4
script (Scripts/playtests/us*.py: the script whose Run.STORY is the story, else the script(s)
whose Run.COVERS contains it).  One run = the STALE gate once, the Tier 3 boot playtest
(Scripts/playtest_boot.py) on the script's launch path, then the Tier 4 script, both on the same
package, each in a subprocess with a timeout and its own run dir.  The summary gives per-AC
acceptance for the story (from the Tier 4 report's acs), every screenshot to open, known vs NEW
failures, and a Playable Status line per feature.  --all is the Playability Recovery sweep.

Evidence: a feature is PLAYABLE only from a fresh-package run.  Anything diagnostic (STALE,
--allow-stale, a dirty-tree package, a skipped freshness gate, a child run that reports itself
DIAGNOSTIC) accepts no AC and yields only DIAGNOSTIC status lines.

Usage:
  py Scripts/playtest_story.py US-078             Tier 3 boot (the script's launch path) + Tier 4
  py Scripts/playtest_story.py US-078 --tier4-only
  py Scripts/playtest_story.py US-078 --plan      the Steps table for the story's Playtest section
  py Scripts/playtest_story.py US-078 --lint      story file <-> script consistency (offline)
  py Scripts/playtest_story.py --all              the sweep: every Tier 4 script, one Tier 3 boot
                                                  per launch path
  py Scripts/playtest_story.py --all --only US-054,US-078
  py Scripts/playtest_story.py --list             stories, their scripts, the newest run of each
  Options: --allow-stale (diagnostic only; passed to every child and recorded)
           --boot-timeout S, --tier4-timeout S (a hung child is killed with its game)

Output:
  Saved/Playtests/<YYYYmmdd-HHMMSS>-story-usXXX/ summary.md, summary.json, tier3-<path>.log,
      tier4-<suffix>.log (the children's console output); the children write their own run dirs
      Saved/Playtests/<ts>-boot/ and Saved/Playtests/<ts>-usXXX/ (report.md, screenshots)
  Saved/Playtests/<YYYYmmdd-HHMMSS>-sweep/ summary.md, summary.json and the console logs
  --plan, --lint and --list print to the console only (no game, no run dir).

Exit codes:
  0 -- every run passed (--plan printed, --lint clean, --list printed)
  1 -- a gameplay FAIL in a run (or --lint found a problem)
  2 -- infrastructure: STALE BUILD (without --allow-stale), no package, no Tier 4 script for the
       story, a child that crashed, timed out or wrote no report.  Infra wins over FAIL.
"""

import argparse
import datetime
import glob
import importlib.util
import json
import os
import re
import subprocess
import sys
import threading
import time

SCRIPTS_DIR = os.path.dirname(os.path.abspath(__file__))
if SCRIPTS_DIR not in sys.path:
    sys.path.insert(0, SCRIPTS_DIR)

import playtest_boot as pb  # noqa: E402
import playtest_kit as kit  # noqa: E402

PROJECT_ROOT = pb.PROJECT_ROOT
PLAYTEST_SCRIPTS_DIR = os.path.join(SCRIPTS_DIR, "playtests")
STORIES_DIR = os.path.join(PROJECT_ROOT, "stories")
STORY_FOLDERS = ("in-progress", "backlog", "done")
SAVED_DIR = pb.PLAYTESTS_DIR
BOOT_SCRIPT = os.path.join(SCRIPTS_DIR, "playtest_boot.py")
DEFAULT_EXE = pb.DEFAULT_EXE

EXIT_PASS, EXIT_FAIL, EXIT_INFRA = pb.EXIT_PASS, pb.EXIT_FAIL, pb.EXIT_INFRA
# A boot waits up to 240 s for the experience; a Tier 4 script plays for minutes.  Past these the
# child (and the game it launched) is killed and the run is an infra failure.
BOOT_TIMEOUT_S = 900.0
TIER4_TIMEOUT_S = 1800.0

NOT_PROVEN = "NOT PROVEN BY A STEP"
CANDIDATE = "ACCEPTANCE CANDIDATE"
DIAGNOSTIC = "DIAGNOSTIC ONLY"
RE_STORY_ARG = re.compile(r"^(?:US-?)?0*(\d+)$", re.IGNORECASE)
RE_HEADING = re.compile(r"^(#{1,6})\s+(.*?)\s*#*\s*$")
RE_CHECKLIST = re.compile(r"^\s*[-*+]\s+\[([ xX])\]\s*(.*)$")
# The first AC token of a checklist line: AC-079.3, AC-1.2.3, AC3, AC-006.12a (also in **...**).
RE_AC_TOKEN = re.compile(r"(?<![A-Za-z0-9_])(AC-?\d[A-Za-z0-9.\-]*)")
RE_SCRIPT_REF = re.compile(r"Scripts[/\\]playtests[/\\]([A-Za-z0-9_.\-]+\.py)")
RE_TABLE_SEP = re.compile(r"^\|?\s*:?-{3,}")
KIND_RANK = {"diag": 0, "not": 1, "none": 2, "boot": 3, "input": 4}

_LOG_STREAM = None


def set_log_stream(stream):
    """Send this module's console output to `stream` (None = sys.stdout).  Returns the previous."""
    global _LOG_STREAM
    previous = _LOG_STREAM
    _LOG_STREAM = stream
    return previous


def _out():
    return _LOG_STREAM if _LOG_STREAM is not None else sys.stdout


def log(msg):
    print(f"[Story-Playtest] {msg}", file=_out(), flush=True)


def say(text=""):
    print(text, file=_out(), flush=True)


def now_iso():
    return datetime.datetime.now().isoformat(timespec="seconds")


def rel(path):
    """A project-relative path with forward slashes (absolute when outside the project)."""
    if not path:
        return path
    try:
        r = os.path.relpath(path, PROJECT_ROOT)
    except ValueError:  # another drive
        return os.path.abspath(path).replace("\\", "/")
    if r.startswith(".."):
        return os.path.abspath(path).replace("\\", "/")
    return r.replace("\\", "/")


def write_text(path, text):
    with open(path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(text)


def write_json(path, obj):
    with open(path, "w", encoding="utf-8", newline="\n") as fh:
        json.dump(obj, fh, indent=2, ensure_ascii=False, default=str)
        fh.write("\n")


def read_json(path):
    try:
        with open(path, "r", encoding="utf-8-sig") as fh:
            data = json.load(fh)
        return data if isinstance(data, dict) else None
    except (OSError, ValueError):
        return None


def worst_code(codes):
    """Exit-code precedence for the runner: infra (2) wins over FAIL (1) wins over PASS (0)."""
    codes = list(codes)
    if any(c == EXIT_INFRA for c in codes):
        return EXIT_INFRA
    if any(c == EXIT_FAIL for c in codes):
        return EXIT_FAIL
    return EXIT_PASS


def result_word(code):
    return {EXIT_PASS: "PASS", EXIT_FAIL: "FAIL"}.get(code, "ERROR")


# ---------------------------------------------------------------------------
# Story files
# ---------------------------------------------------------------------------
def normalize_story(text):
    """'US-078', 'us078', '78', 'US-78' -> 'US-078'.  None when it is not a story id."""
    m = RE_STORY_ARG.match(str(text or "").strip())
    return f"US-{int(m.group(1)):03d}" if m else None


def story_suffix(story):
    return "us" + re.sub(r"\D", "", str(story or ""))


def story_number(story):
    digits = re.sub(r"\D", "", str(story or ""))
    return int(digits) if digits else 0


def find_story_file(story, stories_dir=None):
    """stories/{in-progress,backlog,done}/US-XXX-*.md (or US-XXX.md).  None when missing."""
    stories_dir = stories_dir or STORIES_DIR
    for folder in STORY_FOLDERS:
        hits = sorted(glob.glob(os.path.join(glob.escape(os.path.join(stories_dir, folder)),
                                             f"{story}-*.md")))
        exact = os.path.join(stories_dir, folder, f"{story}.md")
        if os.path.isfile(exact):
            hits.insert(0, exact)
        if hits:
            return hits[0]
    return None


def all_story_files(stories_dir=None):
    """{story id: path} for every stories/<folder>/US-NNN*.md."""
    stories_dir = stories_dir or STORIES_DIR
    found = {}
    for folder in STORY_FOLDERS:
        for path in sorted(glob.glob(os.path.join(glob.escape(os.path.join(stories_dir, folder)),
                                                  "US-*.md"))):
            m = re.match(r"^(US-\d+)", os.path.basename(path))
            if m and m.group(1) not in found:
                found[m.group(1)] = path
    return found


def _sections(lines, predicate):
    """(start, end) line ranges of every section whose heading text satisfies predicate.

    A section runs to the next heading of the same or a higher level (### Acceptance Criteria
    under ## Story 1.1 ends at the next ### or ##).
    """
    out = []
    for i, line in enumerate(lines):
        m = RE_HEADING.match(line)
        if not m or not predicate(m.group(2)):
            continue
        level = len(m.group(1))
        end = len(lines)
        for j in range(i + 1, len(lines)):
            n = RE_HEADING.match(lines[j])
            if n and len(n.group(1)) <= level:
                end = j
                break
        out.append((i + 1, end))
    return out


def ac_token(text):
    """The first AC id in a checklist line's text, or None."""
    m = RE_AC_TOKEN.search(text)
    if not m:
        return None
    token = m.group(1).rstrip(".-")
    return token if kit.RE_AC.match(token) else None


def parse_acs(text):
    """[{id, checked, text, line}] from the checklist lines of every Acceptance Criteria section."""
    lines = text.splitlines()
    acs, seen = [], set()
    for start, end in _sections(lines, lambda h: "acceptance criteria" in h.lower()):
        for n in range(start, end):
            m = RE_CHECKLIST.match(lines[n])
            if not m:
                continue
            ac = ac_token(m.group(2))
            if ac and ac not in seen:
                seen.add(ac)
                acs.append({"id": ac, "checked": m.group(1) in "xX",
                            "text": m.group(2).strip(), "line": n + 1})
    return acs


def parse_steps_table(lines):
    """Check names (in order) of the first markdown table with a 'Check' column, or None."""
    i = 0
    while i < len(lines):
        line = lines[i].strip()
        if line.startswith("|") and i + 1 < len(lines) and RE_TABLE_SEP.match(
                lines[i + 1].strip()):
            header = [c.strip().lower() for c in line.strip("|").split("|")]
            if "check" in header:
                col = header.index("check")
                checks = []
                j = i + 2
                while j < len(lines) and lines[j].strip().startswith("|"):
                    cells = [c.strip() for c in lines[j].strip().strip("|").split("|")]
                    if col < len(cells):
                        checks.append(cells[col].strip("`* "))
                    j += 1
                return checks
            i += 2
            continue
        i += 1
    return None


def parse_story(path):
    """A story file: id, path, folder, title, acs, and its Playtest section (scripts, steps)."""
    with open(path, "r", encoding="utf-8-sig", errors="replace") as fh:
        text = fh.read()
    lines = text.splitlines()
    title = next((RE_HEADING.match(l).group(2) for l in lines
                  if RE_HEADING.match(l) and RE_HEADING.match(l).group(1) == "#"), None)
    m = re.match(r"^(US-\d+)", os.path.basename(path))
    playtest = {"present": False, "scripts": [], "steps": None}
    sections = _sections(lines, lambda h: h.strip().lower().startswith("playtest"))
    if sections:
        start, end = sections[0]
        body = lines[start:end]
        playtest = {"present": True,
                    "scripts": sorted({s for s in RE_SCRIPT_REF.findall("\n".join(body))}),
                    "steps": parse_steps_table(body)}
    return {"id": m.group(1) if m else None, "path": path,
            "folder": os.path.basename(os.path.dirname(path)), "title": title,
            "acs": parse_acs(text), "playtest": playtest}


# ---------------------------------------------------------------------------
# Script discovery
# ---------------------------------------------------------------------------
def load_run_class(path):
    """Import a story script by path (no side effects: main is guarded); its PlaytestRun class."""
    stem = os.path.splitext(os.path.basename(path))[0]
    name = f"_playtest_story_{stem}_{abs(hash(os.path.normcase(os.path.abspath(path)))):x}"
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise ImportError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    saved_flag = sys.dont_write_bytecode
    sys.dont_write_bytecode = True
    try:
        spec.loader.exec_module(module)
    except BaseException:
        sys.modules.pop(name, None)
        raise
    finally:
        sys.dont_write_bytecode = saved_flag
    run = getattr(module, "Run", None)
    if isinstance(run, type) and issubclass(run, kit.PlaytestRun):
        return run
    found = [v for v in vars(module).values()
             if isinstance(v, type) and issubclass(v, kit.PlaytestRun) and v is not kit.PlaytestRun
             and v.__module__ == module.__name__]
    if len(found) != 1:
        raise ImportError(f"{os.path.basename(path)}: expected one PlaytestRun subclass (or a "
                          f"'Run' attribute), found {len(found)}")
    return found[0]


class Script:
    """One Tier 4 story script and its declarations."""

    def __init__(self, path, run_class):
        self.path = os.path.abspath(path)
        self.run_class = run_class
        self.story = run_class.STORY
        self.covers = tuple(run_class.COVERS or ())
        self.features = tuple(run_class.FEATURES or ())
        self.suffix = run_class.RUN_SUFFIX or story_suffix(run_class.STORY)
        self.frontend = bool(run_class.FRONTEND)
        self.map = run_class.launch_map()
        self.experience = run_class.expected_experience()

    @property
    def name(self):
        return os.path.basename(self.path)

    @property
    def rel_path(self):
        return rel(self.path)

    def launch_key(self):
        return ("frontend", self.experience) if self.frontend else (
            "map", self.map, self.experience)

    def stories(self):
        return (self.story,) + self.covers


def discover(playtests_dir=None):
    """(scripts sorted by story, [(path, error)]) for every Scripts/playtests/us*.py."""
    playtests_dir = playtests_dir or PLAYTEST_SCRIPTS_DIR
    scripts, errors = [], []
    for path in sorted(glob.glob(os.path.join(glob.escape(playtests_dir), "us*.py"))):
        try:
            with kit_quiet():
                scripts.append(Script(path, load_run_class(path)))
        except BaseException as e:  # SystemExit / syntax errors in a script must not stop us
            if isinstance(e, KeyboardInterrupt):
                raise
            errors.append((path, f"{type(e).__name__}: {e}"))
    scripts.sort(key=lambda s: (story_number(s.story), s.name))
    return scripts, errors


class kit_quiet:
    """Silence anything a script prints while it is imported."""

    def __enter__(self):
        import io
        self._sink = io.StringIO()
        self._prev = sys.stdout
        sys.stdout = self._sink
        return self

    def __exit__(self, *exc):
        sys.stdout = self._prev
        return False


def scripts_for(story, scripts):
    """The owner script, else every script that COVERS the story."""
    owners = [s for s in scripts if s.story == story]
    if owners:
        return owners
    return [s for s in scripts if story in s.covers]


# ---------------------------------------------------------------------------
# --plan and --lint
# ---------------------------------------------------------------------------
def plan_text(scripts):
    parts = []
    for s in scripts:
        if len(scripts) > 1:
            parts.append(f"<!-- {s.rel_path} -->")
        parts.append(kit.plan_table(s.run_class))
    return "\n\n".join(parts)


def lint(story, scripts, stories_dir=None):
    """[(status, rule, detail)] for the story file <-> script consistency (section 3)."""
    out = []
    path = find_story_file(story, stories_dir)
    if not path:
        out.append(("FAIL", "story_file", f"no stories/{{{','.join(STORY_FOLDERS)}}}/"
                                          f"{story}-*.md"))
        return out
    st = parse_story(path)
    out.append(("PASS", "story_file", f"{rel(path)} ({len(st['acs'])} AC(s))"))
    cache = {story: st}

    def story_acs(sid):
        if sid not in cache:
            p = find_story_file(sid, stories_dir)
            cache[sid] = parse_story(p) if p else None
        return cache[sid]

    for s in scripts:
        tag = s.name
        problems = kit.plan_problems(s.run_class)
        out.append(("FAIL" if problems else "PASS", f"{tag}: declarations",
                    "; ".join(problems) if problems else "PLAN and class attributes are valid"))
        pt = st["playtest"]
        if not pt["present"]:
            out.append(("FAIL", f"{tag}: playtest_section",
                        f"{story} has no Playtest section (stories/PLANNER.md section 7)"))
        elif s.name not in pt["scripts"]:
            out.append(("FAIL", f"{tag}: playtest_section",
                        f"the Playtest section does not name Scripts/playtests/{s.name}"
                        + (f" (it names {', '.join(pt['scripts'])})" if pt["scripts"] else "")))
        else:
            out.append(("PASS", f"{tag}: playtest_section",
                        f"names Scripts/playtests/{s.name}"))
        named = [(ac, step.check) for step in s.run_class.PLAN if isinstance(step, kit.Step)
                 for ac in step.proves]
        named += [(ac, "PENDING") for ac in (s.run_class.PENDING or {})]
        missing = []
        for ac, where in named:
            sid, ac_id = kit.split_ac(ac, s.story)
            target = story_acs(sid)
            if target is None:
                missing.append(f"{ac} ({where}): no story file for {sid}")
            elif ac_id not in {a["id"] for a in target["acs"]}:
                missing.append(f"{ac} ({where}): not an AC of {sid}")
        out.append(("FAIL" if missing else "PASS", f"{tag}: ac_ids",
                    "; ".join(missing) if missing else
                    f"all {len(named)} proves/PENDING id(s) exist in the story files"))
        plan_checks = [step.check for step in s.run_class.PLAN if isinstance(step, kit.Step)]
        steps = pt["steps"] if pt["present"] else None
        if steps is None:
            out.append(("INFO", f"{tag}: steps_table", "no Steps table in the Playtest section "
                        f"(generate it: py Scripts/playtest_story.py {story} --plan)"))
        elif steps == plan_checks:
            out.append(("PASS", f"{tag}: steps_table",
                        f"lists the plan's {len(plan_checks)} checks in order"))
        else:
            extra = [c for c in steps if c not in plan_checks]
            absent = [c for c in plan_checks if c not in steps]
            detail = (f"the Steps table does not match the PLAN (regenerate with --plan): "
                      f"{len(steps)} row(s) vs {len(plan_checks)} check(s)")
            if extra:
                detail += f"; not in PLAN: {', '.join(extra[:8])}"
            if absent:
                detail += f"; missing: {', '.join(absent[:8])}"
            if not extra and not absent:
                detail += "; same checks in a different order"
            out.append(("FAIL", f"{tag}: steps_table", detail))
    proven = set()
    for s in scripts:
        for step in s.run_class.PLAN:
            for ac in getattr(step, "proves", ()):
                sid, ac_id = kit.split_ac(ac, s.story)
                if sid == story:
                    proven.add(ac_id)
        for ac in s.run_class.PENDING or {}:
            sid, ac_id = kit.split_ac(ac, s.story)
            if sid == story:
                proven.add(ac_id)
    uncovered = [a["id"] for a in st["acs"] if a["id"] not in proven]
    if uncovered:
        out.append(("INFO", "uncovered_acs", f"{len(uncovered)} AC(s) of {story} no step proves "
                    f"(logic ACs are expected here): {', '.join(uncovered)}"))
    return out


# ---------------------------------------------------------------------------
# Children
# ---------------------------------------------------------------------------
def run_child(argv, log_path, timeout_s):
    """Run a child (streaming its output to the console and log_path).

    Returns {returncode, timed_out, elapsed_s, note}.  The child is put in a kill-on-close job
    (Windows), so a runner that dies takes the child and its game with it; on a timeout the whole
    process tree is killed.
    """
    env = dict(os.environ)
    env.update({"PYTHONIOENCODING": "utf-8", "PYTHONUNBUFFERED": "1",
                "PYTHONDONTWRITEBYTECODE": "1"})
    t0 = time.time()
    out = _out()
    say(f"$ {subprocess.list2cmdline(argv)}")
    try:
        proc = subprocess.Popen(argv, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, cwd=PROJECT_ROOT, env=env)
    except OSError as e:
        write_text(log_path, f"could not start: {e}\n")
        return {"returncode": None, "timed_out": False, "elapsed_s": 0.0,
                "note": f"could not start the child: {e}"}
    job = None
    if sys.platform == "win32":
        try:
            job = pb.KillOnCloseJob(proc.pid)
            if not job.ok:
                job = None
        except Exception:
            job = None

    def pump():
        with open(log_path, "w", encoding="utf-8", newline="\n") as fh:
            for raw in iter(proc.stdout.readline, b""):
                line = raw.decode("utf-8", errors="replace").rstrip("\r\n")
                fh.write(line + "\n")
                try:
                    print(line, file=out, flush=True)
                except (OSError, ValueError):
                    pass

    reader = threading.Thread(target=pump, daemon=True)
    reader.start()
    timed_out = False
    note = None
    try:
        proc.wait(timeout=timeout_s)
    except subprocess.TimeoutExpired:
        timed_out = True
        note = f"timed out after {timeout_s:g}s; " + pb.kill_tree(proc, job)
    reader.join(timeout=30)
    if job is not None:
        try:
            job.close()
        except Exception:
            pass
    return {"returncode": proc.returncode, "timed_out": timed_out,
            "elapsed_s": round(time.time() - t0, 1), "note": note}


def classify(child, report):
    """The child's exit code as the runner counts it, and why (None when it is its own code)."""
    if child["timed_out"]:
        return EXIT_INFRA, child["note"]
    rc = child["returncode"]
    if rc is None:
        return EXIT_INFRA, child["note"] or "the child did not start"
    if report is None:
        return EXIT_INFRA, (f"no report.json (the child crashed or refused to run; exit {rc})")
    if rc not in (EXIT_PASS, EXIT_FAIL, EXIT_INFRA):
        return EXIT_INFRA, f"unexpected exit code {rc}"
    if report.get("exit_code") is not None and report.get("exit_code") != rc:
        return worst_code([rc, report.get("exit_code")]), (
            f"exit {rc} but report.json says {report.get('exit_code')}")
    return rc, None


def boot_evidence(report, code):
    """Tier 3 evidence, as the kit words it for Tier 4 (playtest_boot.py does not write one)."""
    if not report:
        return "no report"
    build = report.get("build") or {}
    reasons = []
    if not build.get("fresh"):
        reasons.append("STALE or unverified build")
    elif (build.get("build") or {}).get("dirty"):
        reasons.append("the package was built from a dirty working tree")
    if (report.get("options") or {}).get("allow_stale"):
        reasons.append("run with --allow-stale")
    if reasons:
        return f"{DIAGNOSTIC} -- not acceptance evidence: " + "; ".join(reasons)
    if code == EXIT_PASS:
        return f"{CANDIDATE} -- fresh package; open every engine_NN.png before citing this run"
    return "FAILED on a fresh package -- the failures are real findings"


def boot_args(key):
    """playtest_boot.py arguments for a launch path key."""
    if key[0] == "frontend":
        return ["--frontend"] + ([] if key[1] == pb.FRONTEND_EXPERIENCE
                                 else ["--expect-experience", key[1]])
    _, map_, exp = key
    if map_ == pb.DEFAULT_MAP and exp == pb.DEFAULT_EXPERIENCE:
        return []
    return ["--map", map_, "--expect-experience", exp]


def launch_label(key):
    if key[0] == "frontend":
        return "front end" + ("" if key[1] == pb.FRONTEND_EXPERIENCE else f" ({key[1]})")
    _, map_, exp = key
    return f"{map_.rsplit('/', 1)[-1]} ({exp})"


def launch_slug(key):
    return "frontend" if key[0] == "frontend" else re.sub(
        r"[^A-Za-z0-9]+", "_", key[1].rsplit("/", 1)[-1]).strip("_").lower()


# ---------------------------------------------------------------------------
# The runner
# ---------------------------------------------------------------------------
class Runner:
    def __init__(self, args):
        self.args = args
        self.saved_dir = os.path.abspath(args.test_saved_dir or SAVED_DIR)
        self.prefix = json.loads(args.test_child_prefix) if args.test_child_prefix else [
            sys.executable]
        self.boot_script = os.path.abspath(args.test_boot_script or BOOT_SCRIPT)
        self.fresh = None
        self.started_at = now_iso()

    # --- the gate -------------------------------------------------------------------------
    def gate(self):
        """The STALE gate once.  Returns None to continue, else the exit code."""
        if self.args.test_skip_freshness:
            self.fresh = {"fresh": None, "skipped": True, "reasons": [],
                          "warnings": ["staleness gate skipped (test run)"], "build": None,
                          "head": None, "branch": None}
            return None
        exe = os.path.abspath(DEFAULT_EXE)
        if not os.path.isfile(exe):
            log(f"ERROR: game exe not found: {exe} -- package first: py "
                "Scripts/package_playtest.py")
            return EXIT_INFRA
        with pb.log_stream(_out()):
            self.fresh, code = pb.gate(exe, self.args.allow_stale)
        if code is not None:
            log("STALE BUILD: refusing to run (no run dir written).  Re-package with py "
                "Scripts/package_playtest.py; --allow-stale is diagnostic only.")
        return code

    def diagnostic_reasons(self):
        f = self.fresh or {}
        reasons = []
        if f.get("skipped"):
            reasons.append("the staleness gate was skipped (test run)")
        elif not f.get("fresh"):
            reasons.append("STALE or unverified build")
        elif (f.get("build") or {}).get("dirty"):
            reasons.append("the package was built from a dirty working tree")
        if self.args.allow_stale:
            reasons.append("run with --allow-stale")
        return reasons

    def build_info(self):
        f = self.fresh or {}
        b = f.get("build") or {}
        return {"commit": b.get("commit"), "branch": b.get("branch"), "dirty": b.get("dirty"),
                "built_at": b.get("built_at"), "head": f.get("head"),
                "head_branch": f.get("branch"), "fresh": f.get("fresh"),
                "skipped": bool(f.get("skipped")), "allow_stale": bool(self.args.allow_stale),
                "reasons": list(f.get("reasons") or []), "warnings": list(f.get("warnings") or [])}

    # --- children -------------------------------------------------------------------------
    def common_child_args(self):
        return ["--allow-stale"] if self.args.allow_stale else []

    def run_boot(self, key, summary_dir):
        run_dir = pb.new_run_dir("boot", base=self.saved_dir)
        argv = self.prefix + [self.boot_script] + boot_args(key) + ["--run-dir", run_dir] \
            + self.common_child_args()
        log(f"Tier 3 boot playtest ({launch_label(key)}) -> {run_dir}")
        child = run_child(argv, os.path.join(summary_dir, f"tier3-{launch_slug(key)}.log"),
                          self.args.boot_timeout)
        report = read_json(os.path.join(run_dir, "report.json"))
        code, note = classify(child, report)
        engine = [s.get("path") for s in (report or {}).get("engine_screenshots") or []
                  if s.get("path")]
        desktop = [s.get("path") for s in (report or {}).get("screenshots") or []
                   if s.get("path") and not s.get("uniform")]
        fails = [c for c in (report or {}).get("checks") or [] if c.get("status") == "FAIL"]
        return {"tier": 3, "launch": launch_label(key), "launch_key": list(key),
                "args": boot_args(key), "run_dir": run_dir, "report_md":
                os.path.join(run_dir, "report.md"), "exit_code": code,
                "returncode": child["returncode"], "timed_out": child["timed_out"],
                "elapsed_s": child["elapsed_s"], "note": note, "command": subprocess.list2cmdline(
                    argv), "console_log": os.path.join(summary_dir,
                                                       f"tier3-{launch_slug(key)}.log"),
                "result": result_word(code), "evidence": boot_evidence(report, code),
                "failed_checks": [{"name": c.get("name"), "detail": c.get("detail")}
                                  for c in fails],
                "engine_screenshots": engine, "desktop_screenshots": desktop,
                "report_found": report is not None}

    def run_tier4(self, script, summary_dir):
        run_dir = pb.new_run_dir(script.suffix, base=self.saved_dir)
        argv = self.prefix + [script.path, "--run-dir", run_dir] + self.common_child_args()
        log(f"Tier 4 {script.rel_path} ({script.story}) -> {run_dir}")
        console = os.path.join(summary_dir, f"tier4-{script.suffix}.log")
        child = run_child(argv, console, self.args.tier4_timeout)
        report = read_json(os.path.join(run_dir, "report.json"))
        code, note = classify(child, report)
        rep = report or {}
        checks = {c.get("name"): c for c in rep.get("checks") or [] if isinstance(c, dict)}
        evidence = rep.get("evidence") or "no report"
        candidate = str(evidence).startswith(CANDIDATE) and code == EXIT_PASS \
            and not self.diagnostic_reasons()
        acs = []
        for a in rep.get("acs") or []:
            if not isinstance(a, dict):
                continue
            a = dict(a)
            # Never more than the child claims, and never from a diagnostic run.
            a["accepted"] = bool(a.get("accepted")) and candidate
            a["script"] = script.rel_path
            acs.append(a)
        counts = rep.get("counts") or {}
        if not counts and checks:
            counts = {s: sum(1 for c in checks.values() if c.get("status") == s)
                      for s in ("PASS", "WARN", "FAIL", "SKIP", "INFO")}

        def detail(name):
            return (checks.get(name) or {}).get("detail")

        return {"tier": 4, "script": script.rel_path, "story": script.story,
                "covers": list(script.covers), "features": list(script.features),
                "launch": launch_label(script.launch_key()), "run_dir": run_dir,
                "report_md": os.path.join(run_dir, "report.md"), "exit_code": code,
                "returncode": child["returncode"], "timed_out": child["timed_out"],
                "elapsed_s": child["elapsed_s"], "note": note,
                "command": subprocess.list2cmdline(argv), "console_log": console,
                "result": result_word(code), "evidence": evidence, "candidate": candidate,
                "counts": counts,
                "new_failures": [{"name": n, "detail": detail(n)}
                                 for n in rep.get("new_failures") or []],
                "known_failures": [{"name": n, "detail": detail(n),
                                    "known_issue": _known(script, n)}
                                   for n in rep.get("known_failures") or []],
                "fixed_candidates": list(rep.get("fixed_candidates") or []),
                "undeclared_checks": list(rep.get("undeclared_checks") or []),
                "pending": dict(rep.get("pending") or {}),
                "screenshots": [{"label": s.get("label"), "path": s.get("path"),
                                 "expect": s.get("expect"), "uniform": bool(s.get("uniform"))}
                                for s in rep.get("screenshots") or [] if isinstance(s, dict)],
                "acs": acs, "report_found": report is not None,
                "report_commit": ((rep.get("build") or {}).get("build") or {}).get("commit")}

    # --- Playable Status (section 6.1) ------------------------------------------------------
    def commit(self, *runs):
        c = (self.build_info() or {}).get("commit")
        for r in runs:
            if not c and r and r.get("report_commit"):
                c = r.get("report_commit")
        return pb.short(c) if c else "?"

    def script_status(self, t4, boot, tier4_only):
        """(kind, status text) for one script's features: input | boot | not | none | diag."""
        commit = self.commit(t4)
        t4_ref = f"{rel(t4['run_dir'])}/report.md"
        reasons = list(self.diagnostic_reasons())
        if t4["report_found"] and str(t4["evidence"]).startswith(DIAGNOSTIC):
            reasons.append("the Tier 4 run reports itself DIAGNOSTIC")
        if boot is not None and boot["report_found"] and str(boot["evidence"]).startswith(
                DIAGNOSTIC):
            reasons.append("the Tier 3 run reports itself DIAGNOSTIC")
        if reasons:
            boot_part = (f"; Tier 3 {boot['result']} exit {boot['exit_code']} "
                         f"{rel(boot['run_dir'])}" if boot is not None else "")
            return "diag", (f"DIAGNOSTIC ({'; '.join(reasons)}): Tier 4 {t4['result']} exit "
                            f"{t4['exit_code']} {t4_ref}{boot_part} @ {commit} -- not a "
                            "Playable Status; re-run on a fresh package")
        if t4["exit_code"] == EXIT_FAIL:
            fails = [f["name"] + " (NEW)" for f in t4["new_failures"]]
            fails += [f["name"] + " (known)" for f in t4["known_failures"]]
            return "not", (f"NOT PLAYABLE (playtest FAIL: {', '.join(fails) or 'see report'}; "
                           f"Tier 4 {rel(t4['run_dir'])} @ {commit})")
        if boot is not None and boot["exit_code"] == EXIT_FAIL:
            names = ", ".join(c["name"] for c in boot["failed_checks"]) or "see report"
            return "not", (f"NOT PLAYABLE (Tier 3 boot FAIL: {names}; Tier 3 "
                           f"{rel(boot['run_dir'])} @ {commit})")
        boot_ok = boot is not None and boot["exit_code"] == EXIT_PASS and str(
            boot["evidence"]).startswith(CANDIDATE)
        boot_ref = f"{rel(boot['run_dir'])}/report.md" if boot is not None else None
        if t4["exit_code"] != EXIT_PASS:
            why = f"Tier 4 infrastructure failure, exit {t4['exit_code']}" + (
                f": {t4['note']}" if t4["note"] else "") + f"; {t4_ref}"
            if boot_ok:
                return "boot", (f"PLAYABLE (boot) — input not yet accepted ({why}; Tier 3 "
                                f"{boot_ref} @ {commit})")
            return "none", f"no Playable Status ({why}; no passing Tier 3 run) @ {commit}"
        if boot is None:
            how = "--tier4-only" if tier4_only else "not run"
            return "none", (f"no Playable Status yet: Tier 4 PASS {t4_ref} @ {commit}, but Tier 3 "
                            f"was {how}; run py Scripts/playtest_story.py {t4['story']}")
        if not boot_ok:
            why = f"Tier 3 infrastructure failure, exit {boot['exit_code']}" + (
                f": {boot['note']}" if boot["note"] else "")
            return "none", f"no Playable Status ({why}; {boot_ref}) @ {commit}"
        if not t4["candidate"]:
            return "boot", (f"PLAYABLE (boot) — input not yet accepted (the Tier 4 run is not an "
                            f"acceptance candidate: {t4['evidence']}; {t4_ref} @ {commit})")
        if t4["pending"]:
            pend = "; ".join(f"{ac}: PENDING TIER 4 ({why})" for ac, why in t4["pending"].items())
            return "boot", (f"PLAYABLE (boot) — input not yet accepted ({pend}; Tier 4 {t4_ref} "
                            f"+ Tier 3 {boot_ref} @ {commit})")
        return "input", f"PLAYABLE (input) (Tier 4 {t4_ref} + Tier 3 {boot_ref} @ {commit})"

    def feature_lines(self, entries):
        """entries: [(t4, boot)] -> [{feature, kind, status, line, scripts}] per feature."""
        tier4_only = self.args.tier4_only
        per = {}
        order = []
        for t4, boot in entries:
            kind, text = self.script_status(t4, boot, tier4_only)
            for f in t4["features"] or ["(no FEATURES declared)"]:
                if f not in per:
                    per[f] = []
                    order.append(f)
                per[f].append((kind, text, t4, boot))
        out = []
        for f in order:
            items = per[f]
            worst = min(KIND_RANK[k] for k, _, _, _ in items)
            kind = next(k for k, _, _, _ in items if KIND_RANK[k] == worst)
            if kind == "input" and len(items) > 1:
                t4s = ", ".join(f"{rel(t['run_dir'])}/report.md" for _, _, t, _ in items)
                boots = []
                for _, _, _, b in items:
                    ref = f"{rel(b['run_dir'])}/report.md"
                    if ref not in boots:
                        boots.append(ref)
                status = (f"PLAYABLE (input) (Tier 4 {t4s} + Tier 3 {', '.join(boots)} @ "
                          f"{self.commit(items[0][2])})")
            else:
                texts = []
                for k, text, _, _ in items:
                    if k == kind and text not in texts:
                        texts.append(text)
                status = "; ".join(texts)
            out.append({"feature": f, "kind": kind, "status": status,
                        "line": f"- Playable status: {f} — {status}",
                        "scripts": [t["script"] for _, _, t, _ in items]})
        return out

    def evidence(self, code, entries):
        reasons = list(self.diagnostic_reasons())
        for t4, boot in entries:
            if t4["report_found"] and str(t4["evidence"]).startswith(DIAGNOSTIC):
                reasons.append(f"{t4['script']} reports DIAGNOSTIC")
            if boot is not None and boot["report_found"] and str(boot["evidence"]).startswith(
                    DIAGNOSTIC):
                reasons.append(f"Tier 3 {boot['launch']} reports DIAGNOSTIC")
        reasons = list(dict.fromkeys(reasons))
        if reasons:
            return f"{DIAGNOSTIC} -- not acceptance evidence: " + "; ".join(reasons)
        if code == EXIT_PASS and all(t4["candidate"] for t4, _ in entries) and all(
                b is None or str(b["evidence"]).startswith(CANDIDATE) for _, b in entries):
            return (f"{CANDIDATE} -- fresh package; open every screenshot and describe it before "
                    "citing these runs")
        if code == EXIT_PASS:
            return "PASSED, but not every run is an acceptance candidate (see the runs below)"
        if code == EXIT_INFRA:
            return ("INCOMPLETE -- an infrastructure failure (see the runs below): no acceptance "
                    "evidence from the failed run(s)")
        return "FAILED on a fresh package -- the failures below are real findings"


def _known(script, check):
    for step in script.run_class.PLAN:
        if isinstance(step, kit.Step) and step.check == check:
            return step.known_issue
    return None


def screenshot_list(boots, t4s):
    shots = []
    for b in boots:
        if b is None:
            continue
        for p in b["engine_screenshots"]:
            shots.append({"path": p, "tier": 3, "run": b["launch"],
                          "expect": "the game's own frame after boot (engine_NN.png)"})
        for p in b["desktop_screenshots"]:
            shots.append({"path": p, "tier": 3, "run": b["launch"],
                          "expect": "desktop capture of the game window (shot_NN.png)"})
    for t in t4s:
        for s in t["screenshots"]:
            shots.append({"path": s["path"], "tier": 4, "run": t["script"],
                          "expect": s.get("expect") or s.get("label") or "",
                          "uniform": s.get("uniform")})
    return shots


def md_failures(t4s):
    lines = []
    for t in t4s:
        for f in t["new_failures"]:
            lines.append(f"- **NEW** `{t['script']}` `{f['name']}`: "
                         f"{pb.md_cell(f['detail'] or '')}")
    for t in t4s:
        for f in t["known_failures"]:
            lines.append(f"- known `{t['script']}` `{f['name']}`"
                         + (f" ({f['known_issue']})" if f.get("known_issue") else "")
                         + f": {pb.md_cell(f['detail'] or '')}")
    for t in t4s:
        for n in t["fixed_candidates"]:
            lines.append(f"- FIXED? `{t['script']}` `{n}` passed but cites a known issue: "
                         "re-check the backlog")
        if t["undeclared_checks"]:
            lines.append(f"- WARN `{t['script']}` undeclared checks: "
                         f"{', '.join(t['undeclared_checks'])}")
        if t["exit_code"] == EXIT_INFRA:
            lines.append(f"- INFRA `{t['script']}`: exit {t['exit_code']}"
                         + (f" ({t['note']})" if t["note"] else ""))
    return lines or ["- none"]


def md_runs(boots, t4s):
    lines = ["| Tier | Run | Exit | Result | Evidence | Report |", "|---|---|---|---|---|---|"]
    for b in boots:
        if b is None:
            continue
        note = f" ({b['note']})" if b["note"] else ""
        lines.append(f"| 3 | boot: {pb.md_cell(b['launch'])} | {b['exit_code']} | "
                     f"{b['result']}{pb.md_cell(note)} | {pb.md_cell(b['evidence'])} | "
                     f"`{b['report_md']}` |")
    for t in t4s:
        note = f" ({t['note']})" if t["note"] else ""
        lines.append(f"| 4 | `{t['script']}` | {t['exit_code']} | {t['result']}"
                     f"{pb.md_cell(note)} | {pb.md_cell(t['evidence'])} | `{t['report_md']}` |")
    return lines


def md_build(build):
    lines = []
    if build.get("commit"):
        lines.append(f"- Packaged commit `{pb.short(build['commit'])}` on `{build.get('branch')}`, "
                     f"dirty={build.get('dirty')}, built_at={build.get('built_at')}")
    if build.get("head"):
        lines.append(f"- HEAD `{pb.short(build['head'])}` on `{build.get('head_branch')}`")
    allow = " (--allow-stale)" if build.get("allow_stale") else ""
    if build.get("skipped"):
        lines.append("- Freshness: **not checked (test run)**" + allow)
    elif build.get("fresh") and build.get("dirty"):
        lines.append("- Freshness: matches the tree, but **built from a DIRTY tree -- diagnostic "
                     "only**" + allow)
    elif build.get("fresh"):
        lines.append("- Freshness: OK (the package matches the code under test)" + allow)
    else:
        lines.append("- Freshness: **STALE BUILD / unverified -- not acceptance evidence**" + allow)
        lines += [f"  - {r}" for r in build.get("reasons") or []]
    lines += [f"- Warning: {w}" for w in build.get("warnings") or []]
    return lines


def md_screens(shots):
    lines = ["Open EVERY screenshot with the Read tool and state what is actually on screen. A "
             "screenshot nobody looked at is not evidence.", ""]
    if not shots:
        return lines + ["- (none captured)"]
    for s in shots:
        flag = "  **WARNING: near-uniform frame**" if s.get("uniform") else ""
        lines.append(f"- `{s['path']}` (Tier {s['tier']}, {s['run']}): {s['expect']}{flag}")
    return lines


# ---------------------------------------------------------------------------
# One story
# ---------------------------------------------------------------------------
def planned_checks(scripts):
    """{(story, ac id): [check, ...]} for every AC a script's PLAN proves or PENDING names."""
    out = {}
    for s in scripts:
        for step in s.run_class.PLAN:
            for a in getattr(step, "proves", ()):
                out.setdefault(kit.split_ac(a, s.story), []).append(step.check)
        for a in s.run_class.PENDING or {}:
            out.setdefault(kit.split_ac(a, s.story), [])
    return out


def story_acs_table(story, story_info, t4s, scripts=()):
    """Per-AC rows for the requested story, plus the other ACs the run proved."""
    planned = planned_checks(scripts)
    entries = {}
    for t in t4s:
        for a in t["acs"]:
            key = (a.get("story"), a.get("id"))
            prev = entries.get(key)
            if prev is None or (a.get("accepted") and not prev.get("accepted")):
                entries[key] = a
    rows = []
    listed = set()
    for ac in (story_info or {}).get("acs") or []:
        a = entries.get((story, ac["id"]))
        listed.add((story, ac["id"]))
        if a is None:
            checks = planned.get((story, ac["id"]))
            status = NOT_PROVEN if checks is None else "NO RESULT (not in the Tier 4 report)"
            rows.append({"id": ac["id"], "qualified": ac["id"], "story": story,
                         "checks": list(checks or []), "status": status, "accepted": False,
                         "known_issue": None, "pending": None, "script": None,
                         "checked_in_story": ac["checked"]})
        else:
            rows.append(dict(a, qualified=ac["id"], checked_in_story=ac["checked"]))
    others = []
    for (sid, ac_id), a in entries.items():
        if (sid, ac_id) in listed:
            continue
        q = kit.qualified_ac(sid, ac_id, story)
        row = dict(a, qualified=q, checked_in_story=None)
        if sid == story:
            row["status"] = f"{a.get('status')} (not in the story file)"
            rows.append(row)
        else:
            others.append(row)
    return rows, others


def md_ac_rows(rows):
    lines = ["| AC | Checks | Status | Accepted |", "|---|---|---|---|"]
    for a in rows:
        status = str(a.get("status"))
        if a.get("pending"):
            status += f" ({a['pending']})"
        elif a.get("known_issue") and a.get("status") == "FAIL":
            status += f" (known: {a['known_issue']})"
        checks = ", ".join(a.get("checks") or []) or "-"
        lines.append(f"| {pb.md_cell(a['qualified'])} | {pb.md_cell(checks)} "
                     f"| {pb.md_cell(status)} | {'yes' if a.get('accepted') else 'no'} |")
    return lines


def run_story(args, story, scripts):
    runner = Runner(args)
    code = runner.gate()
    if code is not None:
        return code
    summary_dir = pb.new_run_dir(f"story-{story_suffix(story)}", base=runner.saved_dir)
    log(f"Summary dir: {summary_dir}")
    boots = {}
    if not args.tier4_only:
        for s in scripts:
            key = s.launch_key()
            if key not in boots:
                boots[key] = runner.run_boot(key, summary_dir)
    t4s, entries = [], []
    for s in scripts:
        t = runner.run_tier4(s, summary_dir)
        t4s.append(t)
        entries.append((t, boots.get(s.launch_key())))
    code = worst_code([b["exit_code"] for b in boots.values()] + [t["exit_code"] for t in t4s])
    path = find_story_file(story, args.test_stories_dir)
    info = parse_story(path) if path else None
    rows, others = story_acs_table(story, info, t4s, scripts)
    features = runner.feature_lines(entries)
    shots = screenshot_list(list(boots.values()), t4s)
    summary = {
        "kind": "story", "story": story, "story_file": rel(path) if path else None,
        "story_folder": info["folder"] if info else None,
        "scripts": [s.rel_path for s in scripts], "summary_dir": summary_dir,
        "started_at": runner.started_at, "finished_at": now_iso(),
        "result": result_word(code), "exit_code": code,
        "evidence": runner.evidence(code, entries), "build": runner.build_info(),
        "tier4_only": bool(args.tier4_only), "allow_stale": bool(args.allow_stale),
        "tier3": list(boots.values()), "tier4": t4s, "acs": rows, "other_acs": others,
        "uncovered_acs": [r["id"] for r in rows if r["status"] == NOT_PROVEN],
        "playable_status": features, "screenshots": shots,
        "known_failures": [f"{t['script']}:{f['name']}" for t in t4s for f in t["known_failures"]],
        "new_failures": [f"{t['script']}:{f['name']}" for t in t4s for f in t["new_failures"]],
    }
    write_json(os.path.join(summary_dir, "summary.json"), summary)
    lines = [
        f"# Story playtest {story}: {summary['result']}", "",
        f"- Result: **{summary['result']}** (exit {code})",
        f"- Evidence: **{summary['evidence']}**",
        f"- Story: {story}" + (f" ({info['title']}; `{rel(path)}`, {info['folder']})" if info
                               else " (no story file found)"),
        f"- Tier 4 script(s): " + ", ".join(f"`{s.rel_path}`" for s in scripts),
        f"- Summary dir: `{summary_dir}`",
        f"- Started: {summary['started_at']}  Finished: {summary['finished_at']}",
    ]
    if args.tier4_only:
        lines.append("- Tier 3 was not run (--tier4-only): no Playable Status can be PLAYABLE")
    lines += ["", "## Build", ""] + md_build(summary["build"])
    lines += ["", "## Runs", ""] + md_runs(list(boots.values()), t4s)
    lines += ["", f"## Acceptance criteria of {story}", "",
              "From the Tier 4 report's acs. An AC is accepted only by a PASS in an ACCEPTANCE "
              "CANDIDATE run; a diagnostic run accepts nothing, PENDING ACs are never claimed, "
              "and 'NOT PROVEN BY A STEP' ACs (logic ACs) are accepted by Tier 1/2 instead.", ""]
    lines += md_ac_rows(rows) if rows else ["- (no story file / no ACs parsed)"]
    if others:
        lines += ["", "### Other stories' ACs this run proves", ""] + md_ac_rows(others)
    lines += ["", "## Failures (known vs NEW)", ""] + md_failures(t4s)
    lines += ["", "## Playable Status", "",
              "Paste into the planner log (stories/PLANNER.md section 6):", ""]
    lines += [f["line"] for f in features] or ["- (no features)"]
    lines += ["", "## Screenshots", ""] + md_screens(shots)
    lines += ["", "## Console logs", ""]
    lines += [f"- `{r['console_log']}`" for r in list(boots.values()) + t4s]
    lines += ["", pb.AGENT_LINE, ""]
    write_text(os.path.join(summary_dir, "summary.md"), "\n".join(lines))
    print_end(summary, os.path.join(summary_dir, "summary.md"))
    return code


def print_end(summary, md_path):
    bar = "=" * 60
    say(bar)
    title = f"STORY PLAYTEST {summary['story']}" if summary["kind"] == "story" else "SWEEP"
    say(f"{title}: {summary['result']} (exit {summary['exit_code']})")
    say(bar)
    for b in summary["tier3"]:
        say(f"  Tier 3 {b['launch']}: {b['result']} (exit {b['exit_code']}) {b['run_dir']}")
    for t in summary["tier4"]:
        say(f"  Tier 4 {t['script']}: {t['result']} (exit {t['exit_code']}) {t['run_dir']}")
    for f in summary["new_failures"]:
        say(f"  NEW failure: {f}")
    for f in summary["known_failures"]:
        say(f"  known failure: {f}")
    say(f"Evidence: {summary['evidence']}")
    for f in summary["playable_status"]:
        say(f["line"])
    for s in summary["screenshots"]:
        say(f"Screenshot: {s['path']}")
    say(f"Summary: {md_path}")
    say(pb.AGENT_LINE)


# ---------------------------------------------------------------------------
# The sweep
# ---------------------------------------------------------------------------
def run_sweep(args, scripts, import_errors):
    runner = Runner(args)
    code = runner.gate()
    if code is not None:
        return code
    summary_dir = pb.new_run_dir("sweep", base=runner.saved_dir)
    log(f"Sweep summary dir: {summary_dir} ({len(scripts)} script(s))")
    boots = {}
    if not args.tier4_only:
        keys = [] if args.only else [("map", pb.DEFAULT_MAP, pb.DEFAULT_EXPERIENCE)]
        for s in scripts:
            if s.launch_key() not in keys:
                keys.append(s.launch_key())
        for key in keys:
            try:
                boots[key] = runner.run_boot(key, summary_dir)
            except Exception as e:  # a runner bug must not stop the sweep
                log(f"ERROR: Tier 3 {launch_label(key)}: {type(e).__name__}: {e}")
    t4s, entries, crashed = [], [], []
    for s in scripts:
        try:
            t = runner.run_tier4(s, summary_dir)
        except Exception as e:
            log(f"ERROR: {s.rel_path}: {type(e).__name__}: {e}")
            crashed.append({"script": s.rel_path, "error": f"{type(e).__name__}: {e}"})
            continue
        t4s.append(t)
        entries.append((t, boots.get(s.launch_key())))
    errors = [{"script": rel(p), "error": e} for p, e in import_errors] + crashed
    code = worst_code([b["exit_code"] for b in boots.values()] + [t["exit_code"] for t in t4s]
                      + [EXIT_INFRA for _ in errors])
    features = runner.feature_lines(entries)
    shots = screenshot_list(list(boots.values()), t4s)
    rows = []
    for t in t4s:
        c = t["counts"] or {}
        rows.append({"story": t["story"], "covers": t["covers"], "features": t["features"],
                     "script": t["script"], "exit_code": t["exit_code"], "result": t["result"],
                     "pass": c.get("PASS", 0), "fail": c.get("FAIL", 0),
                     "new_failures": [f["name"] for f in t["new_failures"]],
                     "known_failures": [f["name"] for f in t["known_failures"]],
                     "run_dir": t["run_dir"], "evidence": t["evidence"], "note": t["note"]})
    summary = {
        "kind": "sweep", "story": None, "only": args.only, "summary_dir": summary_dir,
        "started_at": runner.started_at, "finished_at": now_iso(),
        "result": result_word(code), "exit_code": code,
        "evidence": runner.evidence(code, entries), "build": runner.build_info(),
        "tier4_only": bool(args.tier4_only), "allow_stale": bool(args.allow_stale),
        "tier3": list(boots.values()), "tier4": t4s, "rows": rows, "errors": errors,
        "playable_status": features, "screenshots": shots,
        "known_failures": [f"{t['script']}:{f['name']}" for t in t4s for f in t["known_failures"]],
        "new_failures": [f"{t['script']}:{f['name']}" for t in t4s for f in t["new_failures"]],
    }
    write_json(os.path.join(summary_dir, "summary.json"), summary)
    lines = [
        f"# Playability sweep: {summary['result']}", "",
        f"- Result: **{summary['result']}** (exit {code}): {len(t4s)} Tier 4 script(s), "
        f"{len(boots)} Tier 3 boot(s)" + (f", only {', '.join(args.only)}" if args.only else ""),
        f"- Evidence: **{summary['evidence']}**",
        f"- Summary dir: `{summary_dir}`",
        f"- Started: {summary['started_at']}  Finished: {summary['finished_at']}",
    ]
    lines += ["", "## Build", ""] + md_build(summary["build"])
    lines += ["", "## Scripts", "",
              "| Story | Features | Script | Exit | PASS | FAIL | NEW failures | Known failures "
              "| Run dir |", "|---|---|---|---|---|---|---|---|---|"]
    for r in rows:
        story = r["story"] + (f" (+{', '.join(r['covers'])})" if r["covers"] else "")
        lines.append(f"| {story} | {', '.join(r['features']) or '-'} | `{r['script']}` | "
                     f"{r['exit_code']} {r['result']} | {r['pass']} | {r['fail']} | "
                     f"{pb.md_cell(', '.join(r['new_failures']) or '-')} | "
                     f"{pb.md_cell(', '.join(r['known_failures']) or '-')} | `{r['run_dir']}` |")
    for e in errors:
        lines.append(f"| ? | - | `{e['script']}` | 2 ERROR | - | - | {pb.md_cell(e['error'])} | - "
                     "| - |")
    lines += ["", "## Runs", ""] + md_runs(list(boots.values()), t4s)
    lines += ["", "## Failures (known vs NEW)", "",
              "Every NEW failure is a game bug: a recovery story or a Known issues entry "
              "(Specs/story_playtests_v1.md section 6.3).", ""] + md_failures(t4s)
    lines += ["", "## Playable Status", "", "Paste into stories/PLANNER_LOG.md:", ""]
    lines += [f["line"] for f in features] or ["- (no features)"]
    lines += ["", "## Screenshots", ""] + md_screens(shots)
    lines += ["", "## Console logs", ""]
    lines += [f"- `{r['console_log']}`" for r in list(boots.values()) + t4s]
    lines += ["", pb.AGENT_LINE, ""]
    write_text(os.path.join(summary_dir, "summary.md"), "\n".join(lines))
    print_end(summary, os.path.join(summary_dir, "summary.md"))
    return code


# ---------------------------------------------------------------------------
# --list
# ---------------------------------------------------------------------------
def newest_run(saved_dir, suffix):
    """(run_dir, report) of the newest Saved/Playtests/<ts>-<suffix>[-N] run with a report."""
    pattern = re.compile(r"^\d{8}-\d{6}-" + re.escape(suffix) + r"(?:-\d+)?$")
    try:
        names = sorted((n for n in os.listdir(saved_dir) if pattern.match(n)), reverse=True)
    except OSError:
        return None, None
    for n in names:
        report = read_json(os.path.join(saved_dir, n, "report.json"))
        if report is not None:
            return os.path.join(saved_dir, n), report
    return None, None


def list_stories(args, scripts, import_errors):
    stories = all_story_files(args.test_stories_dir)
    saved = os.path.abspath(args.test_saved_dir or SAVED_DIR)
    say("Tier 4 story playtests (Scripts/playtests/us*.py):")
    if not scripts:
        say("  (none)")
    for s in scripts:
        path = stories.get(s.story)
        folder = os.path.basename(os.path.dirname(path)) if path else "no story file"
        say(f"  {s.story} ({folder}) {s.rel_path}"
            + (f" covers {', '.join(s.covers)}" if s.covers else "")
            + (f"; features {', '.join(s.features)}" if s.features else "")
            + f"; launch {launch_label(s.launch_key())}")
        run_dir, report = newest_run(saved, s.suffix)
        if run_dir:
            say(f"      newest run: {run_dir} -- {report.get('result')} (exit "
                f"{report.get('exit_code')}); {str(report.get('evidence') or '')[:80]}")
        else:
            say("      newest run: none")
    for p, e in import_errors:
        say(f"  ERROR {rel(p)}: {e}")
    mapped = {sid for s in scripts for sid in s.stories()}
    without = [sid for sid in sorted(stories, key=story_number) if sid not in mapped]
    say(f"Stories without a Tier 4 script: {len(without)} (a story with no input-driven AC needs "
        "none; run py Scripts/playtest_boot.py for its Tier 3)")
    if without:
        say("  " + ", ".join(without))
    return EXIT_PASS


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        description="A story's Tier 3 boot + Tier 4 real-input playtest on the same package, or "
                    "the --all sweep. Exit 0 pass, 1 gameplay FAIL, 2 infra.")
    parser.add_argument("story", nargs="?", help="story id (US-078)")
    parser.add_argument("--all", action="store_true",
                        help="the sweep: every Tier 4 script, one Tier 3 boot per launch path")
    parser.add_argument("--only", default=None,
                        help="with --all: only the scripts owning or covering these stories "
                             "(US-054,US-078)")
    parser.add_argument("--tier4-only", action="store_true", help="skip the Tier 3 boot")
    parser.add_argument("--plan", action="store_true",
                        help="print the story script's PLAN as its Steps table (offline)")
    parser.add_argument("--lint", action="store_true",
                        help="check the story file against its script (offline)")
    parser.add_argument("--list", action="store_true",
                        help="stories, their scripts and the newest run of each (offline)")
    parser.add_argument("--allow-stale", action="store_true",
                        help="run a STALE package (diagnostic only; passed to every child)")
    parser.add_argument("--boot-timeout", type=float, default=BOOT_TIMEOUT_S,
                        help=f"kill a Tier 3 child after this many seconds ({BOOT_TIMEOUT_S:g})")
    parser.add_argument("--tier4-timeout", type=float, default=TIER4_TIMEOUT_S,
                        help=f"kill a Tier 4 child after this many seconds ({TIER4_TIMEOUT_S:g})")
    # Test-only switches: stub children and temp directories (Scripts/tests/test_playtest_story.py).
    parser.add_argument("--test-playtests-dir", default=None, help=argparse.SUPPRESS)
    parser.add_argument("--test-stories-dir", default=None, help=argparse.SUPPRESS)
    parser.add_argument("--test-saved-dir", default=None, help=argparse.SUPPRESS)
    parser.add_argument("--test-child-prefix", default=None, help=argparse.SUPPRESS)
    parser.add_argument("--test-boot-script", default=None, help=argparse.SUPPRESS)
    parser.add_argument("--test-skip-freshness", action="store_true", help=argparse.SUPPRESS)
    args = parser.parse_args(argv)
    modes = sum(bool(x) for x in (args.plan, args.lint, args.list))
    if modes > 1:
        parser.error("use one of --plan, --lint, --list")
    if args.list:
        if args.story or args.all:
            parser.error("--list takes no story and no --all")
    elif args.all:
        if args.story:
            parser.error("give a story id or --all, not both")
        if args.plan or args.lint:
            parser.error("--plan and --lint take a story id, not --all")
    elif not args.story:
        parser.error("give a story id (US-078), --all or --list")
    if args.only and not args.all:
        parser.error("--only goes with --all")
    if args.story:
        sid = normalize_story(args.story)
        if not sid:
            parser.error(f"not a story id: {args.story!r} (US-078)")
        args.story = sid
    if args.only:
        only = [normalize_story(x) for x in args.only.split(",") if x.strip()]
        if not only or None in only:
            parser.error(f"--only wants story ids: {args.only!r}")
        args.only = only
    if args.boot_timeout <= 0 or args.tier4_timeout <= 0:
        parser.error("timeouts must be > 0")
    return args


def main(argv=None):
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(errors="replace")
        except (AttributeError, ValueError):
            pass
    args = parse_args(argv)
    scripts, import_errors = discover(args.test_playtests_dir)
    for p, e in import_errors:
        log(f"WARNING: could not import {rel(p)}: {e}")
    if args.list:
        return list_stories(args, scripts, import_errors)
    if args.all:
        chosen = scripts
        if args.only:
            chosen = [s for s in scripts if set(s.stories()) & set(args.only)]
            missing = [sid for sid in args.only if not any(sid in s.stories() for s in chosen)]
            for sid in missing:
                log(f"WARNING: {sid} has no Tier 4 playtest script")
        if not chosen:
            log("ERROR: no Tier 4 playtest scripts to run")
            return EXIT_INFRA
        return run_sweep(args, chosen, [] if args.only else import_errors)
    story = args.story
    chosen = scripts_for(story, scripts)
    if not chosen:
        log(f"ERROR: {story} has no Tier 4 playtest script (no Scripts/playtests/us*.py has "
            f"STORY = {story!r} or COVERS it).  A story with no input-driven AC needs none: run "
            "py Scripts/playtest_boot.py alone.")
        return EXIT_INFRA
    if args.plan:
        if _LOG_STREAM is None:
            try:  # the table's arrow is UTF-8, also through a pipe to a cp1252 parent
                sys.stdout.reconfigure(encoding="utf-8")
            except (AttributeError, ValueError):
                pass
        problems = [p for s in chosen for p in kit.plan_problems(s.run_class)]
        if problems:
            for p in problems:
                log(f"ERROR: bad declaration: {p}")
            return EXIT_INFRA
        print(plan_text(chosen), file=_out(), flush=True)
        return EXIT_PASS
    if args.lint:
        results = lint(story, chosen, args.test_stories_dir)
        for status, rule, detail in results:
            say(f"  {status:<4} {rule}: {detail}")
        failed = [r for r in results if r[0] == "FAIL"]
        say(f"Lint {story}: {'FAIL' if failed else 'PASS'} ({len(failed)} problem(s))")
        return EXIT_FAIL if failed else EXIT_PASS
    return run_story(args, story, chosen)


if __name__ == "__main__":
    sys.exit(main())
