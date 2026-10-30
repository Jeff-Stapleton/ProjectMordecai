"""Tests for Scripts/playtest_story.py (the story runner) with stub children.

Story/AC parsing over every real story file, script discovery (owner, COVERS), --lint on temp story
files, --plan, one story run and the --all sweep with stub Tier 3 / Tier 4 child scripts (tiny
Python files that write a report.json into --run-dir and exit 0/1/2, crash or hang), summary
contents, exit-code precedence and --allow-stale as diagnostic.  Nothing here launches the game.
Run: py Scripts/run_python_tests.py -k Story
"""

import contextlib
import io
import json
import os
import re
import sys
import tempfile
import textwrap
import unittest
from unittest import mock

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if SCRIPTS not in sys.path:
    sys.path.insert(0, SCRIPTS)

import playtest_boot as pb  # noqa: E402
import playtest_kit as kit  # noqa: E402
import playtest_story as ps  # noqa: E402

REAL_STORIES = os.path.join(os.path.dirname(SCRIPTS), "stories")
CANDIDATE_EVIDENCE = ("ACCEPTANCE CANDIDATE -- fresh package launched by this script; open every "
                      "screenshot and describe it before citing this run")

# A Tier 4 stub: real PlaytestRun declarations (discovery imports it), and a __main__ that writes
# the report.json its <stem>.behavior.json describes.
T4_STUB = textwrap.dedent('''\
    import json, os, sys, time
    sys.path.insert(0, {scripts!r})
    import playtest_kit as kit
    from playtest_kit import Step

    class Run(kit.PlaytestRun):
        STORY = {story!r}
        COVERS = {covers!r}
        FEATURES = {features!r}
        FRONTEND = {frontend!r}
        MAP = {map!r}
        RUN_SUFFIX = {suffix!r}
        PLAN = (
            Step({check1!r}, "tap Tab -> player.weapon.index==1", proves={proves1!r}),
            Step({check2!r}, "tap BackSlash -> player.weapon.index==0", proves={proves2!r},
                 known_issue={known!r}),
        )
        PENDING = {pending!r}

    if __name__ == "__main__":
        here = os.path.dirname(os.path.abspath(__file__))
        with open(os.path.splitext(os.path.abspath(__file__))[0] + ".behavior.json",
                  encoding="utf-8") as f:
            b = json.load(f)
        argv = sys.argv[1:]
        with open(os.path.join(here, "calls.jsonl"), "a", encoding="utf-8") as f:
            f.write(json.dumps({{"who": os.path.basename(__file__), "argv": argv}}) + "\\n")
        print("[stub] Tier 4 \\u2192 running")
        if b.get("hang"):
            time.sleep(120)
        if b.get("crash"):
            raise RuntimeError("stub crashed before writing a report")
        run_dir = argv[argv.index("--run-dir") + 1]
        rep = b["report"]
        rep["run_dir"] = run_dir
        if "--allow-stale" in argv:
            rep["evidence"] = "DIAGNOSTIC ONLY -- not acceptance evidence: run with --allow-stale"
            for a in rep.get("acs", []):
                a["accepted"] = False
        for s in rep.get("screenshots", []):
            s["path"] = os.path.join(run_dir, s["path"])
            open(s["path"], "wb").close()
        with open(os.path.join(run_dir, "report.json"), "w", encoding="utf-8") as f:
            json.dump(rep, f)
        sys.exit(b["exit"])
''')

BOOT_STUB = textwrap.dedent('''\
    import json, os, sys
    here = os.path.dirname(os.path.abspath(__file__))
    with open(os.path.join(here, "boot.behavior.json"), encoding="utf-8") as f:
        b = json.load(f)
    argv = sys.argv[1:]
    with open(os.path.join(here, "calls.jsonl"), "a", encoding="utf-8") as f:
        f.write(json.dumps({"who": "boot", "argv": argv}) + "\\n")
    path = "frontend" if "--frontend" in argv else "default"
    code = b.get(path, 0)
    if b.get("crash"):
        raise RuntimeError("boot stub crashed")
    run_dir = argv[argv.index("--run-dir") + 1]
    shot = os.path.join(run_dir, "engine_01.png")
    open(shot, "wb").close()
    checks = [{"name": "experience_loaded", "status": "FAIL" if code == 1 else "PASS",
               "detail": "x", "kind": "gameplay"}]
    if code == 2:
        checks.append({"name": "game_window", "status": "FAIL", "detail": "no window",
                       "kind": "infra"})
    rep = {"result": "PASS" if code == 0 else "FAIL", "exit_code": code, "run_dir": run_dir,
           "build": {"fresh": b.get("fresh", True), "build": {"commit": "abc1234567",
                                                              "dirty": False}},
           "options": {"allow_stale": "--allow-stale" in argv}, "checks": checks,
           "engine_screenshots": [{"path": shot, "uniform": False}], "screenshots": []}
    with open(os.path.join(run_dir, "report.json"), "w", encoding="utf-8") as f:
        json.dump(rep, f)
    sys.exit(code)
''')


def t4_report(story, exit_code, acs, new=(), known=(), fixed=(), evidence=None, pending=None,
              covers=(), features=()):
    """A kit-shaped report.json."""
    checks = [{"name": n, "status": "FAIL", "detail": f"{n} failed", "kind": "gameplay"}
              for n in list(new) + list(known)]
    checks += [{"name": "step_ok", "status": "PASS", "detail": "ok", "kind": "gameplay"}]
    if evidence is None:
        evidence = CANDIDATE_EVIDENCE if exit_code == 0 else \
            "FAILED on a fresh package -- the failures below are real findings"
    return {"story": story, "covers": list(covers), "features": list(features),
            "result": "PASS" if exit_code == 0 else "FAIL", "exit_code": exit_code,
            "evidence": evidence, "checks": checks,
            "counts": {"PASS": 1, "WARN": 0, "FAIL": len(checks) - 1, "SKIP": 0, "INFO": 0},
            "acs": acs, "pending": pending or {}, "undeclared_checks": [],
            "known_failures": list(known), "new_failures": list(new),
            "fixed_candidates": list(fixed),
            "screenshots": [{"label": "start", "path": "bridge_01.png", "expect": "the arena"}],
            "build": {"build": {"commit": "abc1234567"}}}


def ac(story, ac_id, status="PASS", accepted=True, checks=("step_one",), pending=None):
    return {"id": ac_id, "story": story, "checks": list(checks), "status": status,
            "accepted": accepted, "known_issue": None, "pending": pending}


STORY_TEMPLATE = """# {story}: Stub story

## Overview
A stub.

## Acceptance Criteria
### Part one
- [ ] {p}.1: the first thing
- [x] **{p}.2** the second thing
- [ ] {p}.3: a logic AC (no step proves it)

## Tests Required
- [ ] `Mordecai.Stub.Thing` (AC-999.9)

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py`
{playtest}

## Definition of Done
- [ ] All ACs checked off
"""


class StoryCase(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
        self.tmp = os.path.join(self._tmp.name, "with space")
        self.addCleanup(self._tmp.cleanup)
        self.pdir = os.path.join(self.tmp, "playtests")
        self.sdir = os.path.join(self.tmp, "stories")
        self.saved = os.path.join(self.tmp, "Saved", "Playtests")
        for d in (self.pdir, self.saved, *(os.path.join(self.sdir, f)
                                           for f in ps.STORY_FOLDERS)):
            os.makedirs(d)
        self.boot = os.path.join(self.pdir, "boot_stub.py")  # not us*.py: never discovered
        with open(self.boot, "w", encoding="utf-8") as f:
            f.write(BOOT_STUB)
        self.boot_behavior({})

    # --- fixtures --------------------------------------------------------------------------
    def boot_behavior(self, behavior):
        with open(os.path.join(os.path.dirname(self.boot), "boot.behavior.json"), "w",
                  encoding="utf-8") as f:
            json.dump(behavior, f)

    def stub(self, name, story, behavior=None, covers=(), features=("weapons",), frontend=False,
             proves1=None, proves2=None, known=None, pending=None, map=None, suffix=None,
             checks=("step_one", "step_two")):
        num = story.split("-")[1]
        proves1 = proves1 if proves1 is not None else (f"AC-{num}.1",)
        proves2 = proves2 if proves2 is not None else (f"AC-{num}.2",)
        path = os.path.join(self.pdir, name)
        with open(path, "w", encoding="utf-8") as f:
            f.write(T4_STUB.format(scripts=SCRIPTS, story=story, covers=tuple(covers),
                                   features=tuple(features), frontend=frontend,
                                   map=map or pb.DEFAULT_MAP, suffix=suffix,
                                   check1=checks[0], check2=checks[1],
                                   proves1=tuple(proves1), proves2=tuple(proves2), known=known,
                                   pending=pending or {}))
        if behavior is not None:
            self.behavior(name, behavior)
        return path

    def behavior(self, name, behavior):
        with open(os.path.join(self.pdir, os.path.splitext(name)[0] + ".behavior.json"), "w",
                  encoding="utf-8") as f:
            json.dump(behavior, f)

    def story_file(self, story, folder="in-progress", playtest="", name=None):
        num = story.split("-")[1]
        path = os.path.join(self.sdir, folder, name or f"{story}-stub-story.md")
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            f.write(STORY_TEMPLATE.format(story=story, p=f"AC-{num}", playtest=playtest))
        return path

    def calls(self):
        path = os.path.join(self.pdir, "calls.jsonl")
        if not os.path.isfile(path):
            return []
        with open(path, encoding="utf-8") as f:
            return [json.loads(line) for line in f if line.strip()]

    def main(self, *argv, skip=True, extra=()):
        args = list(argv) + ["--test-playtests-dir", self.pdir, "--test-stories-dir", self.sdir,
                             "--test-saved-dir", self.saved, "--test-boot-script", self.boot]
        if skip:
            args.append("--test-skip-freshness")
        args += list(extra)
        out = io.StringIO()
        previous = ps.set_log_stream(out)
        try:
            with pb.log_stream(out), contextlib.redirect_stdout(out):
                code = ps.main(args)
        finally:
            ps.set_log_stream(previous)
        return code, out.getvalue()

    def summary(self, kind="story"):
        dirs = [d for d in os.listdir(self.saved) if f"-{kind}" in d]
        self.assertEqual(len(dirs), 1, os.listdir(self.saved))
        d = os.path.join(self.saved, dirs[0])
        with open(os.path.join(d, "summary.json"), encoding="utf-8") as f:
            js = json.load(f)
        with open(os.path.join(d, "summary.md"), encoding="utf-8") as f:
            md = f.read()
        return d, js, md

    def fresh(self, dirty=False):
        return {"fresh": True, "reasons": [], "warnings": [], "git_error": None,
                "build": {"commit": "abc1234567890", "branch": "main", "dirty": dirty},
                "head": "abc1234567890", "branch": "main"}

    @contextlib.contextmanager
    def gate(self, fresh, code=None):
        exe = os.path.join(self.tmp, "LyraGame.exe")
        open(exe, "wb").close()
        with mock.patch.object(ps, "DEFAULT_EXE", exe), \
                mock.patch.object(pb, "gate", return_value=(fresh, code)) as g:
            yield g


# ---------------------------------------------------------------------------
# Parsing
# ---------------------------------------------------------------------------
class TestStoryParsing(unittest.TestCase):
    def test_every_real_story_file_parses_with_ids(self):
        files = ps.all_story_files(REAL_STORIES)
        self.assertGreater(len(files), 40)
        for sid, path in files.items():
            with self.subTest(story=sid):
                st = ps.parse_story(path)
                self.assertEqual(st["id"], sid)
                self.assertTrue(st["acs"], f"{path}: no AC parsed")
                for a in st["acs"]:
                    self.assertRegex(a["id"], kit.RE_AC)
                # every checklist line of an Acceptance Criteria section yields an id
                with open(path, encoding="utf-8-sig") as f:
                    lines = f.read().splitlines()
                n = sum(1 for s, e in ps._sections(lines, lambda h: "acceptance criteria"
                                                   in h.lower())
                        for i in range(s, e) if ps.RE_CHECKLIST.match(lines[i]))
                self.assertEqual(len(st["acs"]), n, f"{path}: a checklist line has no AC id")

    def test_known_real_ids(self):
        files = ps.all_story_files(REAL_STORIES)
        us079 = [a["id"] for a in ps.parse_story(files["US-079"])["acs"]]
        self.assertEqual(us079, [f"AC-079.{i}" for i in range(1, 14)])
        us001 = [a["id"] for a in ps.parse_story(files["US-001"])["acs"]]
        self.assertIn("AC-1.1.1", us001)
        self.assertIn("AC-2.3.1", us001)
        self.assertIn("AC-007.10a", [a["id"] for a in ps.parse_story(files["US-007"])["acs"]])

    def test_formats_and_sections(self):
        text = textwrap.dedent("""\
            # US-500: X
            ## Acceptance Criteria
            - [ ] AC-500.3: plain
            - [x] **AC-054.1** bold
            - [ ] AC3 (stories that create assets)
            * [X] AC-1.2.3: star bullet
              - [ ] AC-500.9: an indented sub-item still counts
            - [ ] Not an AC line
            - [ ] AC-500.4. trailing dot
            ### Sub
            - [ ] AC-500.5: in a subsection
            ## Tests Required
            - [ ] `Mordecai.X` (AC-500.6)
            ## Story 1.2 - Camera
            ### Acceptance Criteria
            - [ ] AC-1.2.1: second block
            ### Tests Required
            - [ ] `Mordecai.Y` (AC-1.2.9)
            """)
        acs = ps.parse_acs(text)
        self.assertEqual([a["id"] for a in acs],
                         ["AC-500.3", "AC-054.1", "AC3", "AC-1.2.3", "AC-500.9", "AC-500.4",
                          "AC-500.5", "AC-1.2.1"])
        self.assertTrue(acs[1]["checked"])
        self.assertFalse(acs[0]["checked"])
        self.assertTrue(acs[3]["checked"])

    def test_normalize_story(self):
        for raw in ("US-078", "us078", "78", "US-78", "us-078"):
            self.assertEqual(ps.normalize_story(raw), "US-078")
        self.assertEqual(ps.normalize_story("US-1"), "US-001")
        self.assertIsNone(ps.normalize_story("AC-078"))

    def test_playtest_section(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "US-500-x.md")
            with open(path, "w", encoding="utf-8") as f:
                f.write(STORY_TEMPLATE.format(
                    story="US-500", p="AC-500",
                    playtest="- **Tier 4 script:** `Scripts\\playtests\\us500_x.py`\n\n"
                             "| # | Check | Step | Proves |\n|---|---|---|---|\n"
                             "| 1 | `a_check` | x | AC-500.1 |\n| 2 | `b_check` | y | - |\n"))
            st = ps.parse_story(path)
        self.assertEqual(st["playtest"]["scripts"], ["us500_x.py"])
        self.assertEqual(st["playtest"]["steps"], ["a_check", "b_check"])
        self.assertEqual(st["playtest"]["script_steps"], {})  # no markers
        self.assertEqual(st["title"], "US-500: Stub story")

    def test_marker_keyed_steps_tables(self):
        lines = textwrap.dedent("""\
            - **Tier 4 scripts:** `Scripts/playtests/us500_a.py`, `Scripts/playtests/us500_b.py`

            <!-- Scripts/playtests/us500_a.py -->
            | # | Check | Step | Proves |
            |---|---|---|---|
            | 1 | `a_one` | x | AC-500.1 |

            Some prose between the tables.

            <!-- C:/some dir/with space/playtests/us500_b.py -->

            | # | Not a steps table |
            |---|---|
            | 1 | x |

            | # | Check | Step | Proves |
            |---|---|---|---|
            | 1 | `b_one` | x | - |
            | 2 | `b_two` | x | - |

            <!-- Scripts/playtests/us500_c.py -->
            <!-- Scripts/playtests/us500_d.py -->
            | # | Check | Step | Proves |
            |---|---|---|---|
            | 1 | `d_one` | x | - |
            <!-- an ordinary comment: not a marker -->
            """).splitlines()
        self.assertEqual(ps.parse_script_steps(lines),
                         {"us500_a.py": ["a_one"], "us500_b.py": ["b_one", "b_two"],
                          "us500_c.py": None, "us500_d.py": ["d_one"]})
        self.assertEqual(ps.parse_steps_table(lines), ["a_one"])  # no markers: the first table
        self.assertEqual(ps.parse_script_steps(["| # | Check |", "|---|---|", "| 1 | `x` |"]), {})


# ---------------------------------------------------------------------------
# Discovery, --plan, --lint, --list
# ---------------------------------------------------------------------------
class TestDiscovery(StoryCase):
    def test_owner_covers_and_broken(self):
        self.stub("us901_one.py", "US-901", covers=("US-902",))
        self.stub("us903_front.py", "US-903", frontend=True)
        with open(os.path.join(self.pdir, "us904_broken.py"), "w", encoding="utf-8") as f:
            f.write("raise SystemExit('broken on import')\n")
        with open(os.path.join(self.pdir, "helper.py"), "w", encoding="utf-8") as f:
            f.write("raise RuntimeError('not a us*.py script: never imported')\n")
        scripts, errors = ps.discover(self.pdir)
        self.assertEqual([s.name for s in scripts], ["us901_one.py", "us903_front.py"])
        self.assertEqual(len(errors), 1)
        self.assertIn("us904_broken.py", errors[0][0])
        self.assertEqual([s.name for s in ps.scripts_for("US-901", scripts)], ["us901_one.py"])
        self.assertEqual([s.name for s in ps.scripts_for("US-902", scripts)], ["us901_one.py"])
        self.assertEqual(ps.scripts_for("US-999", scripts), [])
        front = scripts[1]
        self.assertEqual(front.launch_key(), ("frontend", pb.FRONTEND_EXPERIENCE))
        self.assertEqual(ps.boot_args(front.launch_key()), ["--frontend"])
        self.assertEqual(ps.boot_args(scripts[0].launch_key()), [])
        self.assertEqual(ps.boot_args(("map", "/Game/X", "B_X")),
                         ["--map", "/Game/X", "--expect-experience", "B_X"])

    def test_owner_wins_over_covers(self):
        self.stub("us901_one.py", "US-901", covers=("US-902",))
        self.stub("us902_two.py", "US-902")
        scripts, _ = ps.discover(self.pdir)
        self.assertEqual([s.name for s in ps.scripts_for("US-902", scripts)], ["us902_two.py"])

    def test_class_without_run_attribute(self):
        path = os.path.join(self.pdir, "us905_norun.py")
        with open(path, "w", encoding="utf-8") as f:
            f.write(f"import sys\nsys.path.insert(0, {SCRIPTS!r})\nimport playtest_kit as kit\n"
                    "class Mine(kit.PlaytestRun):\n    STORY = 'US-905'\n"
                    "    PLAN = (kit.Step('a', 'b'),)\n")
        scripts, errors = ps.discover(self.pdir)
        self.assertEqual(errors, [])
        self.assertEqual(scripts[0].run_class.__name__, "Mine")

    def test_real_us079_is_found(self):
        scripts, _ = ps.discover()
        chosen = ps.scripts_for("US-079", scripts)
        self.assertEqual([s.name for s in chosen], ["us079_inventory.py"])
        self.assertEqual(chosen[0].suffix, "us079")

    def test_no_script_is_infra(self):
        code, out = self.main("US-950")
        self.assertEqual(code, ps.EXIT_INFRA)
        self.assertIn("US-950 has no Tier 4 playtest script", out)
        self.assertEqual(os.listdir(self.saved), [])

    def test_list(self):
        self.stub("us901_one.py", "US-901", covers=("US-902",))
        self.story_file("US-901")
        self.story_file("US-906", folder="done")
        old = os.path.join(self.saved, "20260101-000000-us901")
        os.makedirs(old)
        with open(os.path.join(old, "report.json"), "w", encoding="utf-8") as f:
            json.dump({"result": "FAIL", "exit_code": 1, "evidence": "FAILED on a fresh"}, f)
        code, out = self.main("--list")
        self.assertEqual(code, 0, out)
        self.assertIn("US-901 (in-progress)", out)
        self.assertIn("covers US-902", out)
        self.assertIn("20260101-000000-us901 -- FAIL (exit 1)", out)
        self.assertIn("Stories without a Tier 4 script: 1", out)
        self.assertIn("US-906", out)


class TestPlan(StoryCase):
    def test_plan_for_real_us079(self):
        out = io.StringIO()
        previous = ps.set_log_stream(out)
        try:
            code = ps.main(["US-079", "--plan"])
        finally:
            ps.set_log_stream(previous)
        self.assertEqual(code, 0)
        run = ps.load_run_class(os.path.join(ps.PLAYTEST_SCRIPTS_DIR, "us079_inventory.py"))
        self.assertEqual(out.getvalue().strip(), kit.plan_table(run))
        self.assertIn("`menu_closes_escape`", out.getvalue())

    def test_plan_for_a_covered_story_and_no_run_dir(self):
        self.stub("us901_one.py", "US-901", covers=("US-902",))
        code, out = self.main("US-902", "--plan")
        self.assertEqual(code, 0)
        self.assertIn("| 1 | `step_one` |", out)
        self.assertEqual(os.listdir(self.saved), [])


class TestLint(StoryCase):
    def table(self):
        scripts, _ = ps.discover(self.pdir)
        return kit.plan_table(scripts[0].run_class)

    def test_lint_passes(self):
        self.stub("us901_one.py", "US-901")
        self.story_file("US-901", playtest="- **Tier 4 script:** `Scripts/playtests/us901_one.py`"
                        "\n- **Steps:** (generated)\n\n" + self.table())
        code, out = self.main("US-901", "--lint")
        self.assertEqual(code, 0, out)
        self.assertIn("PASS us901_one.py: steps_table", out)
        self.assertIn("INFO uncovered_acs: 1 AC(s) of US-901 no step proves", out)
        self.assertIn("AC-901.3", out)
        self.assertIn("Lint US-901: PASS", out)

    def test_lint_covers_qualified_ids(self):
        self.stub("us901_one.py", "US-901", covers=("US-902",),
                  proves2=("US-902:AC-902.2",))
        ref = "- **Tier 4 script:** `Scripts/playtests/us901_one.py`\n\n" + self.table()
        self.story_file("US-901", playtest=ref)
        self.story_file("US-902", folder="done", playtest=ref)
        code, out = self.main("US-902", "--lint")
        self.assertEqual(code, 0, out)
        self.assertIn("AC-902.1, AC-902.3", out)  # uncovered ACs of US-902

    def test_lint_failures(self):
        self.stub("us901_one.py", "US-901", proves2=("AC-901.7",), covers=("US-902",),
                  pending={"US-902:AC-902.9": "bridge cannot drive: x"})
        table = self.table().splitlines()
        swapped = "\n".join(table[:2] + [table[3], table[2]])
        self.story_file("US-901", playtest="- **Tier 4 script:** `Scripts/playtests/other.py`\n\n"
                        + swapped)
        code, out = self.main("US-901", "--lint")
        self.assertEqual(code, 1, out)
        self.assertIn("FAIL us901_one.py: playtest_section: the Playtest section does not name "
                      "Scripts/playtests/us901_one.py (it names other.py)", out)
        self.assertIn("AC-901.7 (step_two): not an AC of US-901", out)
        self.assertIn("US-902:AC-902.9 (PENDING): no story file for US-902", out)
        self.assertIn("same checks in a different order", out)

    def test_lint_no_story_file_and_no_section(self):
        self.stub("us901_one.py", "US-901")
        code, out = self.main("US-901", "--lint")
        self.assertEqual(code, 1)
        self.assertIn("FAIL story_file", out)
        path = self.story_file("US-901")
        with open(path, encoding="utf-8") as f:
            text = f.read()
        with open(path, "w", encoding="utf-8") as f:
            f.write(re.sub(r"## Playtest.*?## Definition", "## Definition", text, flags=re.S))
        code, out = self.main("US-901", "--lint")
        self.assertEqual(code, 1)
        self.assertIn("US-901 has no Playtest section", out)

    def test_lint_extra_rows(self):
        self.stub("us901_one.py", "US-901")
        self.story_file("US-901", playtest="`Scripts/playtests/us901_one.py`\n\n" + self.table()
                        + "\n| 3 | `ghost` | x | - |")
        code, out = self.main("US-901", "--lint")
        self.assertEqual(code, 1)
        self.assertIn("not in PLAN: ghost", out)

    # --- a story with several scripts (one per test map) --------------------------------------
    def two_scripts(self):
        """US-901 owns two scripts on two maps, with their own checks and RUN_SUFFIX values."""
        self.stub("us901_arena.py", "US-901", checks=("a_one", "a_two"))
        self.stub("us901_combat.py", "US-901", map=pb.COMBAT_TEST_MAP, suffix="us901combat",
                  checks=("c_one", "c_two"), proves1=("AC-901.3",), proves2=())
        scripts, errors = ps.discover(self.pdir)
        self.assertEqual(errors, [])
        return {s.name: kit.plan_table(s.run_class) for s in scripts}

    NAMES = "- **Tier 4 scripts:** `Scripts/playtests/us901_arena.py`, " \
            "`Scripts/playtests/us901_combat.py`\n\n"

    def test_lint_two_scripts_with_marker_keyed_tables(self):
        tables = self.two_scripts()
        self.story_file("US-901", playtest=self.NAMES
                        + "<!-- Scripts/playtests/us901_arena.py -->\n" + tables["us901_arena.py"]
                        + "\n\n<!-- Scripts/playtests/us901_combat.py -->\n"
                        + tables["us901_combat.py"])
        code, out = self.main("US-901", "--lint")
        self.assertEqual(code, 0, out)
        for name in ("us901_arena.py", "us901_combat.py"):
            self.assertIn(f"PASS {name}: steps_table: lists the plan's 2 checks in order (the "
                          "table after its marker)", out)
            self.assertIn(f"PASS {name}: run_suffix", out)
        self.assertIn("Lint US-901: PASS", out)

    def test_lint_accepts_the_plan_output_as_is(self):
        self.two_scripts()
        _, plan = self.main("US-901", "--plan")
        # --plan's output is stable: a marker before each script's table
        self.assertEqual(len(re.findall(r"^<!-- .*us901_(?:arena|combat)\.py -->$", plan,
                                        flags=re.M)), 2, plan)
        self.story_file("US-901", playtest=self.NAMES + plan.strip())
        code, out = self.main("US-901", "--lint")
        self.assertEqual(code, 0, out)

    def test_lint_marker_keyed_table_that_does_not_match_its_script(self):
        tables = self.two_scripts()
        # each marker is followed by the OTHER script's table
        self.story_file("US-901", playtest=self.NAMES
                        + "<!-- Scripts/playtests/us901_arena.py -->\n" + tables["us901_combat.py"]
                        + "\n\n<!-- Scripts/playtests/us901_combat.py -->\n"
                        + tables["us901_arena.py"])
        code, out = self.main("US-901", "--lint")
        self.assertEqual(code, 1, out)
        self.assertIn("FAIL us901_arena.py: steps_table: the Steps table after the <!-- "
                      "Scripts/playtests/us901_arena.py --> marker does not match the PLAN", out)
        self.assertIn("not in PLAN: c_one, c_two; missing: a_one, a_two", out)
        self.assertIn("FAIL us901_combat.py: steps_table", out)

    def test_lint_marker_missing_for_one_script(self):
        tables = self.two_scripts()
        self.story_file("US-901", playtest=self.NAMES
                        + "<!-- Scripts/playtests/us901_arena.py -->\n" + tables["us901_arena.py"]
                        + "\n\n" + tables["us901_combat.py"])
        code, out = self.main("US-901", "--lint")
        self.assertEqual(code, 1, out)
        self.assertIn("PASS us901_arena.py: steps_table", out)
        self.assertIn("FAIL us901_combat.py: steps_table: the Playtest section has one Steps "
                      "table per script (<!-- --> markers for us901_arena.py) but no marker for "
                      "Scripts/playtests/us901_combat.py", out)

    def test_lint_marker_without_a_table(self):
        tables = self.two_scripts()
        self.story_file("US-901", playtest=self.NAMES
                        + "<!-- Scripts/playtests/us901_combat.py -->\n\n"
                        + "<!-- Scripts/playtests/us901_arena.py -->\n" + tables["us901_arena.py"])
        code, out = self.main("US-901", "--lint")
        self.assertEqual(code, 1, out)
        self.assertIn("FAIL us901_combat.py: steps_table: no Steps table after the <!-- "
                      "Scripts/playtests/us901_combat.py --> marker", out)

    def test_lint_two_scripts_without_markers_keeps_the_first_table(self):
        tables = self.two_scripts()
        self.story_file("US-901", playtest=self.NAMES + tables["us901_arena.py"] + "\n\n"
                        + tables["us901_combat.py"])
        code, out = self.main("US-901", "--lint")
        self.assertEqual(code, 1, out)
        self.assertIn("PASS us901_arena.py: steps_table: lists the plan's 2 checks in order\n",
                      out)
        self.assertIn("FAIL us901_combat.py: steps_table: the Steps table does not match the "
                      "PLAN", out)

    def test_lint_fails_on_a_shared_run_suffix(self):
        self.stub("us901_one.py", "US-901")
        self.stub("us902_two.py", "US-902", suffix="US901")  # case-insensitive clash
        self.stub("us903_three.py", "US-903")
        scripts, _ = ps.discover(self.pdir)
        self.story_file("US-901", playtest="`Scripts/playtests/us901_one.py`\n\n"
                        + kit.plan_table(scripts[0].run_class))
        code, out = self.main("US-901", "--lint")
        self.assertEqual(code, 1, out)
        self.assertIn("FAIL us901_one.py: run_suffix: RUN_SUFFIX 'us901' is also the run suffix "
                      "of us902_two.py", out)
        self.assertIn("tier4-us901.log", out)
        self.assertIn("Lint US-901: FAIL (1 problem(s))", out)
        # the other story's script is not a problem of US-903
        self.story_file("US-903", playtest="`Scripts/playtests/us903_three.py`\n\n"
                        + kit.plan_table(scripts[2].run_class))
        code, out = self.main("US-903", "--lint")
        self.assertEqual(code, 0, out)
        self.assertIn("PASS us903_three.py: run_suffix: RUN_SUFFIX 'us903' is unique among the 3 "
                      "discovered script(s)", out)

    def test_the_real_scripts_have_unique_run_suffixes(self):
        scripts, _ = ps.discover()
        suffixes = [s.suffix.lower() for s in scripts]
        self.assertEqual(len(suffixes), len(set(suffixes)),
                         {s.name: s.suffix for s in scripts})

    def test_every_real_script_declares_its_map(self):
        # AC-100.4: each script names its map (or the front end) in its own class, so a change of
        # the kit default (DevTestMap) can never silently move a script to another map
        scripts, errors = ps.discover()
        self.assertEqual(errors, [])
        self.assertTrue(scripts)
        undeclared = [s.name for s in scripts if "MAP" not in s.run_class.__dict__
                      and "FRONTEND" not in s.run_class.__dict__]
        self.assertEqual(undeclared, [], {s.name: s.map for s in scripts})


# ---------------------------------------------------------------------------
# One story run
# ---------------------------------------------------------------------------
class TestStoryRun(StoryCase):
    def setup_pass(self, **kw):
        self.stub("us901_one.py", "US-901", covers=("US-902",), features=("weapons", "weapon_hud"),
                  proves2=("US-902:AC-902.1",), **kw)
        self.behavior("us901_one.py", {"exit": 0, "report": t4_report(
            "US-901", 0, [ac("US-901", "AC-901.1"), ac("US-902", "AC-902.1")],
            features=("weapons", "weapon_hud"))})
        self.story_file("US-901")

    def test_pass_but_skipped_freshness_is_diagnostic(self):
        self.setup_pass()
        code, out = self.main("US-901")
        self.assertEqual(code, 0, out)
        d, js, md = self.summary()
        self.assertTrue(os.path.basename(d).endswith("-story-us901"))
        self.assertEqual(js["exit_code"], 0)
        self.assertTrue(js["evidence"].startswith("DIAGNOSTIC ONLY"))
        self.assertEqual(len(js["tier3"]), 1)
        self.assertEqual(len(js["tier4"]), 1)
        self.assertTrue(os.path.isfile(os.path.join(js["tier3"][0]["run_dir"], "report.json")))
        self.assertTrue(js["tier4"][0]["run_dir"].endswith("-us901"))
        self.assertFalse(any(a["accepted"] for a in js["acs"]))
        for f in js["playable_status"]:
            self.assertNotIn("PLAYABLE", f["line"])
            self.assertIn("DIAGNOSTIC", f["line"])
        self.assertIn("Playable status: weapons", md)
        self.assertIn("console", md.lower())
        self.assertIn("→", "".join(open(js["tier4"][0]["console_log"],
                                              encoding="utf-8").read()))

    def test_fresh_candidate_is_playable_and_accepts(self):
        self.setup_pass()
        with self.gate(self.fresh()):
            code, out = self.main("US-901", skip=False)
        self.assertEqual(code, 0, out)
        d, js, md = self.summary()
        self.assertTrue(js["evidence"].startswith("ACCEPTANCE CANDIDATE"), js["evidence"])
        rows = {a["qualified"]: a for a in js["acs"]}
        self.assertTrue(rows["AC-901.1"]["accepted"])
        self.assertFalse(rows["AC-901.2"]["accepted"])  # no step proves it (step_two: US-902)
        self.assertEqual(rows["AC-901.3"]["status"], "NOT PROVEN BY A STEP")
        self.assertEqual(js["uncovered_acs"], ["AC-901.2", "AC-901.3"])
        others = {a["qualified"]: a for a in js["other_acs"]}
        self.assertTrue(others["US-902:AC-902.1"]["accepted"])
        lines = [f["line"] for f in js["playable_status"]]
        self.assertEqual(len(lines), 2)
        t4 = ps.rel(js["tier4"][0]["run_dir"])
        t3 = ps.rel(js["tier3"][0]["run_dir"])
        self.assertEqual(lines[0], f"- Playable status: weapons — PLAYABLE (input) (Tier 4 {t4}"
                                   f"/report.md + Tier 3 {t3}/report.md @ abc1234567)")
        # every screenshot to open: the engine frame and the bridge frame
        paths = [s["path"] for s in js["screenshots"]]
        self.assertTrue(any(p.endswith("engine_01.png") for p in paths))
        self.assertTrue(any(p.endswith("bridge_01.png") for p in paths))
        for p in paths:
            self.assertIn(p, md)
        self.assertIn("| AC-901.1 | step_one | PASS | yes |", md)
        self.assertIn("US-902:AC-902.1", md)
        self.assertIn("Packaged commit `abc1234567`", md)
        self.assertIn(js["tier4"][0]["run_dir"], md)
        self.assertIn("Summary:", out)

    def test_allow_stale_is_passed_and_diagnostic(self):
        self.setup_pass()
        stale = dict(self.fresh(), fresh=False, reasons=["build commit a != HEAD b"])
        with self.gate(stale) as g:
            code, out = self.main("US-901", "--allow-stale", skip=False)
        g.assert_called_once()
        self.assertTrue(g.call_args[0][1])  # allow_stale reached the gate
        self.assertEqual(code, 0, out)
        for call in self.calls():
            self.assertIn("--allow-stale", call["argv"])
        _, js, md = self.summary()
        self.assertTrue(js["allow_stale"])
        self.assertTrue(js["build"]["allow_stale"])
        self.assertIn("run with --allow-stale", js["evidence"])
        self.assertFalse(any(a["accepted"] for a in js["acs"] + js["other_acs"]))
        for f in js["playable_status"]:
            self.assertNotIn("PLAYABLE", f["line"])
        self.assertIn("STALE BUILD", md)

    def test_stale_without_allow_is_refused_before_any_run(self):
        self.setup_pass()
        stale = dict(self.fresh(), fresh=False, reasons=["x"])
        with self.gate(stale, code=ps.EXIT_INFRA):
            code, out = self.main("US-901", skip=False)
        self.assertEqual(code, ps.EXIT_INFRA)
        self.assertIn("STALE BUILD: refusing to run", out)
        self.assertEqual(os.listdir(self.saved), [])
        self.assertEqual(self.calls(), [])

    def test_dirty_package_is_diagnostic(self):
        self.setup_pass()
        with self.gate(self.fresh(dirty=True)):
            code, _ = self.main("US-901", skip=False)
        _, js, _ = self.summary()
        self.assertIn("dirty working tree", js["evidence"])
        self.assertFalse(any("PLAYABLE" in f["line"] for f in js["playable_status"]))

    def test_tier4_fail_is_not_playable_with_new_first(self):
        self.stub("us901_one.py", "US-901", known="Known issues: B-1")
        self.behavior("us901_one.py", {"exit": 1, "report": t4_report(
            "US-901", 1, [ac("US-901", "AC-901.1", status="FAIL", accepted=False)],
            new=["step_one"], known=["step_two"], features=("weapons",))})
        self.story_file("US-901")
        with self.gate(self.fresh()):
            code, _ = self.main("US-901", skip=False)
        self.assertEqual(code, ps.EXIT_FAIL)
        _, js, md = self.summary()
        line = js["playable_status"][0]["line"]
        self.assertIn("NOT PLAYABLE (playtest FAIL: step_one (NEW), step_two (known); Tier 4", line)
        self.assertTrue(js["new_failures"][0].endswith(":step_one"))
        self.assertTrue(js["known_failures"][0].endswith(":step_two"))
        self.assertIn("**NEW**", md)
        self.assertIn("known `", md)
        self.assertIn("(Known issues: B-1)", md)
        self.assertTrue(js["evidence"].startswith("FAILED on a fresh package"))

    def test_pending_is_playable_boot_only(self):
        self.stub("us901_one.py", "US-901", pending={"AC-901.3": "bridge cannot drive: pad"})
        self.behavior("us901_one.py", {"exit": 0, "report": t4_report(
            "US-901", 0, [ac("US-901", "AC-901.1"),
                          ac("US-901", "AC-901.3", status="PENDING", accepted=False, checks=(),
                             pending="bridge cannot drive: pad")],
            pending={"AC-901.3": "bridge cannot drive: pad"}, features=("weapons",))})
        self.story_file("US-901")
        with self.gate(self.fresh()):
            self.main("US-901", skip=False)
        _, js, _ = self.summary()
        line = js["playable_status"][0]["line"]
        self.assertIn("PLAYABLE (boot) — input not yet accepted (AC-901.3: PENDING TIER 4 "
                      "(bridge cannot drive: pad)", line)

    def test_frontend_script_boots_the_front_end(self):
        self.stub("us903_front.py", "US-903", frontend=True,
                  behavior={"exit": 0, "report": t4_report("US-903", 0, [])})
        self.main("US-903")
        boots = [c for c in self.calls() if c["who"] == "boot"]
        self.assertEqual(len(boots), 1)
        self.assertIn("--frontend", boots[0]["argv"])
        self.assertIn("--run-dir", boots[0]["argv"])

    def test_two_scripts_on_two_maps_and_a_fail_anywhere_wins(self):
        # US-901 owns a DevTestMap script and a CombatTestMap script; both prove AC-901.1.  The
        # first passes (accepted on its own), the second FAILs it: the AC is not accepted.
        self.stub("us901_arena.py", "US-901", checks=("a_one", "a_two"),
                  behavior={"exit": 0, "report": t4_report(
                      "US-901", 0, [ac("US-901", "AC-901.1", checks=("a_one",)),
                                    ac("US-901", "AC-901.2", checks=("a_two",))])})
        self.stub("us901_combat.py", "US-901", map=pb.COMBAT_TEST_MAP, suffix="us901combat",
                  checks=("c_one", "c_two"), proves1=("AC-901.1",), proves2=(),
                  behavior={"exit": 1, "report": t4_report(
                      "US-901", 1, [ac("US-901", "AC-901.1", status="FAIL", accepted=False,
                                       checks=("c_one",))], new=["c_one"])})
        self.story_file("US-901")
        with self.gate(self.fresh()):
            code, out = self.main("US-901", skip=False)
        self.assertEqual(code, ps.EXIT_FAIL, out)
        d, js, md = self.summary()
        # one Tier 3 boot per map, one Tier 4 run per script, distinct console logs
        boots = [c["argv"] for c in self.calls() if c["who"] == "boot"]
        self.assertEqual(len(boots), 2)
        self.assertTrue(any("--map" not in a for a in boots))  # DevTestMap: the default boot
        self.assertTrue(any(a[:4] == ["--map", pb.COMBAT_TEST_MAP, "--expect-experience",
                                      pb.DEFAULT_EXPERIENCE] for a in boots), boots)
        self.assertEqual([t["script"].rsplit("/", 1)[-1] for t in js["tier4"]],
                         ["us901_arena.py", "us901_combat.py"])
        logs = [os.path.basename(t["console_log"]) for t in js["tier4"]]
        self.assertEqual(logs, ["tier4-us901.log", "tier4-us901combat.log"])
        self.assertTrue(js["tier4"][1]["run_dir"].endswith("-us901combat"))
        self.assertTrue(all(os.path.isfile(t["console_log"]) for t in js["tier4"]))
        rows = {a["qualified"]: a for a in js["acs"]}
        self.assertEqual(rows["AC-901.1"]["status"], "FAIL")
        self.assertFalse(rows["AC-901.1"]["accepted"])
        self.assertEqual(rows["AC-901.1"]["checks"], ["c_one", "a_one"])
        self.assertEqual([(r["script"].rsplit("/", 1)[-1], r["status"], r["accepted"])
                          for r in rows["AC-901.1"]["results"]],
                         [("us901_arena.py", "PASS", True), ("us901_combat.py", "FAIL", False)])
        self.assertTrue(rows["AC-901.2"]["accepted"])  # proven by one script only: it passed
        self.assertIn("| AC-901.1 | c_one, a_one | FAIL (per script: us901_arena.py PASS, "
                      "us901_combat.py FAIL) | no |", md)
        self.assertIn("| AC-901.2 | a_two | PASS | yes |", md)

    def test_tier4_only_skips_the_boot(self):
        self.setup_pass()
        with self.gate(self.fresh()):
            code, _ = self.main("US-901", "--tier4-only", skip=False)
        self.assertEqual(code, 0)
        self.assertEqual([c["who"] for c in self.calls()], ["us901_one.py"])
        _, js, md = self.summary()
        self.assertEqual(js["tier3"], [])
        for f in js["playable_status"]:
            self.assertNotIn("PLAYABLE", f["line"])
            self.assertIn("--tier4-only", f["line"])
        self.assertIn("Tier 3 was not run (--tier4-only)", md)

    def test_exit_precedence_infra_wins(self):
        self.setup_pass()
        self.behavior("us901_one.py", {"exit": 1, "report": t4_report(
            "US-901", 1, [], new=["step_one"])})
        self.boot_behavior({"default": 2})
        code, _ = self.main("US-901")
        self.assertEqual(code, ps.EXIT_INFRA)
        _, js, _ = self.summary()
        self.assertEqual(js["tier3"][0]["exit_code"], 2)
        self.assertEqual(js["tier4"][0]["exit_code"], 1)

    def test_boot_fail_and_tier4_pass_is_fail(self):
        self.setup_pass()
        self.boot_behavior({"default": 1})
        with self.gate(self.fresh()):
            code, _ = self.main("US-901", skip=False)
        self.assertEqual(code, ps.EXIT_FAIL)
        _, js, _ = self.summary()
        self.assertIn("NOT PLAYABLE (Tier 3 boot FAIL: experience_loaded",
                      js["playable_status"][0]["line"])

    def test_crashing_child_is_infra(self):
        self.setup_pass()
        self.behavior("us901_one.py", {"exit": 0, "crash": True, "report": {}})
        code, out = self.main("US-901")
        self.assertEqual(code, ps.EXIT_INFRA)
        _, js, _ = self.summary()
        t4 = js["tier4"][0]
        self.assertEqual(t4["exit_code"], 2)
        self.assertEqual(t4["returncode"], 1)  # a traceback exits 1: not a gameplay FAIL
        self.assertIn("no report.json", t4["note"])
        rows = {a["qualified"]: a for a in js["acs"]}
        self.assertEqual(rows["AC-901.1"]["status"], "NO RESULT (not in the Tier 4 report)")
        self.assertEqual(rows["AC-901.1"]["checks"], ["step_one"])
        self.assertFalse(rows["AC-901.1"]["accepted"])
        self.assertEqual(js["uncovered_acs"], ["AC-901.2", "AC-901.3"])
        self.assertTrue(js["evidence"].startswith("DIAGNOSTIC"))  # skipped freshness
        self.assertIn("RuntimeError", open(t4["console_log"], encoding="utf-8").read())

    @unittest.skipUnless(sys.platform == "win32", "taskkill / job objects")
    def test_hung_child_is_killed(self):
        self.setup_pass()
        self.behavior("us901_one.py", {"exit": 0, "hang": True, "report": {}})
        code, _ = self.main("US-901", "--tier4-only", extra=["--tier4-timeout", "3"])
        self.assertEqual(code, ps.EXIT_INFRA)
        _, js, _ = self.summary()
        self.assertTrue(js["tier4"][0]["timed_out"])
        self.assertIn("timed out after 3s", js["tier4"][0]["note"])


class TestAcRollup(unittest.TestCase):
    """story_acs_table over several scripts' acs: a FAIL in any script beats a PASS elsewhere."""

    INFO = {"acs": [{"id": "AC-901.1", "checked": False}, {"id": "AC-901.2", "checked": True}]}

    @staticmethod
    def t4(script, *acs):
        return {"acs": [dict(a, script=f"Scripts/playtests/{script}") for a in acs]}

    def test_a_fail_wins_in_either_order(self):
        passed = self.t4("a.py", ac("US-901", "AC-901.1", checks=("a_one",)))
        failed = self.t4("b.py", ac("US-901", "AC-901.1", status="FAIL", accepted=False,
                                    checks=("b_one",)))
        for order in ((passed, failed), (failed, passed)):
            with self.subTest(first=order[0]["acs"][0]["script"]):
                rows, others = ps.story_acs_table("US-901", self.INFO, list(order))
                row = rows[0]
                self.assertEqual((row["id"], row["status"], row["accepted"]),
                                 ("AC-901.1", "FAIL", False))
                self.assertEqual(row["script"], "Scripts/playtests/b.py")
                self.assertEqual(row["checks"], ["b_one", "a_one"])
                self.assertEqual(len(row["results"]), 2)
                self.assertEqual(rows[1]["status"], "NOT PROVEN BY A STEP")
                self.assertEqual(others, [])
                md = "\n".join(ps.md_ac_rows(rows))
                self.assertIn("| AC-901.1 | b_one, a_one | FAIL (per script: ", md)
                self.assertIn("a.py PASS", md)
                self.assertIn("b.py FAIL", md)
                self.assertIn("| no |", md)

    def test_an_accepted_pass_still_beats_a_not_run_and_a_single_script_is_unchanged(self):
        not_run = self.t4("a.py", ac("US-901", "AC-901.1", status="NOT RUN", accepted=False))
        passed = self.t4("b.py", ac("US-901", "AC-901.1"),
                         ac("US-902", "AC-902.1", checks=("b_two",)))
        rows, others = ps.story_acs_table("US-901", self.INFO, [not_run, passed])
        self.assertEqual((rows[0]["status"], rows[0]["accepted"]), ("PASS", True))
        self.assertEqual(rows[0]["script"], "Scripts/playtests/b.py")
        # one script: the row is the script's entry as it is (no 'results')
        self.assertNotIn("results", others[0])
        self.assertEqual(others[0]["qualified"], "US-902:AC-902.1")
        self.assertTrue(others[0]["accepted"])
        rows, _ = ps.story_acs_table("US-901", self.INFO, [passed])
        self.assertNotIn("results", rows[0])
        self.assertEqual(ps.md_ac_rows(rows)[2], "| AC-901.1 | step_one | PASS | yes |")

    def test_a_fail_in_another_story_ac_wins_too(self):
        rows, others = ps.story_acs_table("US-901", self.INFO, [
            self.t4("a.py", ac("US-902", "AC-902.1")),
            self.t4("b.py", ac("US-902", "AC-902.1", status="FAIL", accepted=False))])
        self.assertEqual((others[0]["status"], others[0]["accepted"]), ("FAIL", False))


# ---------------------------------------------------------------------------
# The sweep
# ---------------------------------------------------------------------------
class TestSweep(StoryCase):
    def setup_three(self):
        self.stub("us901_one.py", "US-901", features=("weapons", "pause_menu"),
                  behavior={"exit": 0, "report": t4_report("US-901", 0, [ac("US-901", "AC-901.1")],
                                                           features=("weapons", "pause_menu"))})
        self.stub("us902_two.py", "US-902", features=("inventory", "pause_menu"),
                  known="Known issues: B-2",
                  behavior={"exit": 1, "report": t4_report(
                      "US-902", 1, [], new=["step_one"], known=["step_two"],
                      features=("inventory", "pause_menu"))})
        self.stub("us903_front.py", "US-903", frontend=True, features=("front_end_menu",),
                  behavior={"exit": 0, "crash": True, "report": {}})

    def test_sweep_runs_everything_and_infra_wins(self):
        self.setup_three()
        with self.gate(self.fresh()):
            code, out = self.main("--all", skip=False)
        self.assertEqual(code, ps.EXIT_INFRA, out)
        calls = self.calls()
        boots = [c for c in calls if c["who"] == "boot"]
        self.assertEqual(len(boots), 2)  # DevTestMap + the front end
        self.assertEqual(sum("--frontend" in c["argv"] for c in boots), 1)
        self.assertEqual([c["who"] for c in calls if c["who"] != "boot"],
                         ["us901_one.py", "us902_two.py", "us903_front.py"])
        d, js, md = self.summary("sweep")
        self.assertEqual(len(js["rows"]), 3)
        rows = {r["story"]: r for r in js["rows"]}
        self.assertEqual(rows["US-902"]["new_failures"], ["step_one"])
        self.assertEqual(rows["US-902"]["known_failures"], ["step_two"])
        self.assertEqual(rows["US-903"]["exit_code"], 2)
        status = {f["feature"]: f for f in js["playable_status"]}
        self.assertEqual(status["weapons"]["kind"], "input")
        self.assertEqual(status["pause_menu"]["kind"], "not")  # worst of us901 + us902
        self.assertIn("NOT PLAYABLE (playtest FAIL: step_one (NEW), step_two (known)",
                      status["pause_menu"]["line"])
        self.assertEqual(status["front_end_menu"]["kind"], "boot")
        self.assertIn("PLAYABLE (boot) — input not yet accepted (Tier 4 infrastructure failure",
                      status["front_end_menu"]["line"])
        self.assertIn("- Playable status: inventory — NOT PLAYABLE", md)
        self.assertIn("| US-902 | inventory, pause_menu |", md)
        self.assertEqual(sum(1 for s in js["screenshots"] if s["path"].endswith("engine_01.png")),
                         2)

    def test_sweep_fail_without_infra_is_1(self):
        self.setup_three()
        os.remove(os.path.join(self.pdir, "us903_front.py"))
        with self.gate(self.fresh()):
            code, _ = self.main("--all", skip=False)
        self.assertEqual(code, ps.EXIT_FAIL)
        boots = [c for c in self.calls() if c["who"] == "boot"]
        self.assertEqual(len(boots), 1)

    def test_only_filters_by_story_or_covers(self):
        self.setup_three()
        code, _ = self.main("--all", "--only", "US-901")
        self.assertEqual(code, 0)
        self.assertEqual([c["who"] for c in self.calls()], ["boot", "us901_one.py"])

    def test_sweep_allow_stale_is_diagnostic(self):
        self.setup_three()
        os.remove(os.path.join(self.pdir, "us903_front.py"))
        stale = dict(self.fresh(), fresh=False, reasons=["x"])
        with self.gate(stale):
            self.main("--all", "--allow-stale", skip=False)
        _, js, md = self.summary("sweep")
        self.assertTrue(js["evidence"].startswith("DIAGNOSTIC ONLY"))
        for f in js["playable_status"]:
            self.assertNotIn("PLAYABLE", f["line"])
        self.assertNotIn("PLAYABLE (", md)

    def test_import_error_is_an_infra_row(self):
        self.stub("us901_one.py", "US-901",
                  behavior={"exit": 0, "report": t4_report("US-901", 0, [])})
        with open(os.path.join(self.pdir, "us904_broken.py"), "w", encoding="utf-8") as f:
            f.write("this is not python\n")
        code, _ = self.main("--all")
        self.assertEqual(code, ps.EXIT_INFRA)
        _, js, md = self.summary("sweep")
        self.assertEqual(len(js["errors"]), 1)
        self.assertIn("us904_broken.py", md)


class TestCli(StoryCase):
    def test_bad_combinations(self):
        for argv in (["US-1", "--all"], ["--plan", "--all"], ["--only", "US-1"], [],
                     ["--list", "US-1"], ["nonsense"]):
            with self.subTest(argv=argv), contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit) as cm:
                    ps.parse_args(argv)
                self.assertEqual(cm.exception.code, 2)

    def test_hidden_options_not_in_help(self):
        out = io.StringIO()
        with contextlib.redirect_stdout(out), self.assertRaises(SystemExit):
            ps.parse_args(["--help"])
        self.assertIn("--tier4-only", out.getvalue())
        self.assertNotIn("--test-", out.getvalue())

    def test_worst_code(self):
        self.assertEqual(ps.worst_code([0, 1, 2]), 2)
        self.assertEqual(ps.worst_code([1, 0]), 1)
        self.assertEqual(ps.worst_code([]), 0)

    def test_import_has_no_side_effects(self):
        import subprocess
        r = subprocess.run([sys.executable, "-c",
                            f"import sys; sys.path.insert(0, {SCRIPTS!r}); import playtest_story"],
                           capture_output=True, text=True, timeout=60)
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(r.stdout + r.stderr, "")


if __name__ == "__main__":
    unittest.main()
