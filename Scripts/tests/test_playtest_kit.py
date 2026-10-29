"""Tests for Scripts/playtest_kit.py against tests/fake_bridge.py.

A minimal PlaytestRun subclass plays the fake through --attach-port and --run-dir: plan
enforcement, SKIP propagation, AC acceptance, known/NEW failures, the step helpers, --plan,
reports and exit codes.  These prove the kit's logic only; they say nothing about the real game.
Nothing here launches the real game.  Run: py Scripts/run_python_tests.py -k Kit
"""

import dataclasses
import io
import json
import os
import sys
import tempfile
import unittest
from unittest import mock

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if SCRIPTS not in sys.path:
    sys.path.insert(0, SCRIPTS)

import agent_bridge as ab  # noqa: E402
import playtest_boot as pb  # noqa: E402
import playtest_kit as kit  # noqa: E402
from playtest_kit import Step  # noqa: E402
from tests import fake_bridge as fb  # noqa: E402

TOKEN = "kit-token"
INPUT_AND_READ_CMDS = {"ping", "state", "key", "axis", "mouse", "click", "screenshot",
                       "wait_until", "wait"}

CYCLE_PLAN = (
    Step("cycle_next", "tap Tab -> player.weapon.index==1",
         proves=("AC-900.1", "US-901:AC-901.2")),
    Step("cycle_prev", "tap BackSlash -> player.weapon.index==0", proves=("AC-900.2",)),
)


def make_run(steps, plan=CYCLE_PLAN, **attrs):
    """A PlaytestRun subclass for story US-900 whose steps() is `steps(run)`."""
    body = {"STORY": "US-900", "COVERS": ("US-901",), "FEATURES": ("weapons",),
            "SCREENSHOTS": {"start": "the arena"}, "PLAN": tuple(plan), "steps": steps}
    body.update(attrs)
    return type("KitTestRun", (kit.PlaytestRun,), body)


def cycle_steps(next_cond="player.weapon.index==1", prev_cond="player.weapon.index==0"):
    def steps(run):
        run.shot("start")
        ok, _ = run.press("cycle_next", "Tab", then=next_cond, timeout_s=0.5)
        run.press("cycle_prev", "BackSlash", then=prev_cond, timeout_s=0.5, needs=ok,
                  needs_why="not run: cycle_next failed")
    return steps


def read_jsonl(path):
    with open(path, encoding="utf-8") as f:
        return [json.loads(line) for line in f if line.strip()]


class KitCase(unittest.TestCase):
    """Temp dir, captured logs, and a fake bridge per run."""

    CLEAN = {"fresh": True, "reasons": [], "warnings": [], "git_error": None,
             "build": {"commit": "abc1234def5678", "branch": "x", "dirty": False,
                       "dirty_count": 0}, "head": "abc1234def5678", "branch": "x"}

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
        self.tmp = self._tmp.name
        self.addCleanup(self._tmp.cleanup)
        self.log = io.StringIO()
        for module in (kit, ab, pb):
            previous = module.set_log_stream(self.log)
            self.addCleanup(module.set_log_stream, previous)
        patcher = mock.patch.object(ab, "SESSION_FILE", os.path.join(self.tmp, "session.json"))
        patcher.start()
        self.addCleanup(patcher.stop)
        self.run_dir = os.path.join(self.tmp, "run")

    def serve(self, faults=None, **world_kwargs):
        world = fb.FakeWorld(**world_kwargs)
        server = fb.FakeBridgeServer(world, token=TOKEN, shot_dir=self.tmp, faults=faults).start()
        self.addCleanup(server.stop)
        return world, server

    def play(self, run_class, server, *extra):
        code = kit.main(run_class, ["--attach-port", str(server.port), "--token", TOKEN,
                                    "--run-dir", self.run_dir, *extra])
        return code, self.report()

    def report(self):
        with open(os.path.join(self.run_dir, "report.json"), encoding="utf-8") as f:
            return json.load(f)

    def report_md(self):
        with open(os.path.join(self.run_dir, "report.md"), encoding="utf-8") as f:
            return f.read()

    @staticmethod
    def by_name(report):
        return {c["name"]: c for c in report["checks"]}

    @staticmethod
    def statuses(report, status):
        return [c["name"] for c in report["checks"] if c["status"] == status]

    @staticmethod
    def acs(report):
        return {(a["story"], a["id"]): a for a in report["acs"]}


class TestKitRun(KitCase):
    def test_a_passing_run(self):
        world, server = self.serve()
        code, rep = self.play(make_run(cycle_steps()), server)
        self.assertEqual(code, 0, self.log.getvalue()[-3000:])
        self.assertEqual(self.statuses(rep, "FAIL"), [])
        self.assertEqual(self.statuses(rep, "SKIP"), [])
        checks = self.by_name(rep)
        for name in ("attach", "bridge_ready", "experience", "map", "cycle_next", "cycle_prev",
                     "no_crash"):
            self.assertEqual(checks[name]["status"], "PASS", name)
        self.assertEqual(checks["map"]["detail"], f"loaded {kit.DEFAULT_MAP}")
        self.assertIn(checks["screenshot_start"]["status"], ("PASS", "WARN"))
        self.assertIn("tapped Tab; player.weapon.index==1 after", checks["cycle_next"]["detail"])
        self.assertEqual(world.weapon_index, 0)
        # The report's Phase 3 fields.
        self.assertEqual(rep["story"], "US-900")
        self.assertEqual(rep["covers"], ["US-901"])
        self.assertEqual(rep["features"], ["weapons"])
        self.assertEqual(rep["title"], "US-900 playtest (Tier 4, real input)")
        self.assertEqual([p["check"] for p in rep["plan"]], ["cycle_next", "cycle_prev"])
        self.assertEqual(rep["plan"][0], {"check": "cycle_next",
                                          "does": "tap Tab -> player.weapon.index==1",
                                          "proves": ["AC-900.1", "US-901:AC-901.2"],
                                          "kind": "gameplay", "known_issue": None})
        self.assertEqual(rep["undeclared_checks"], [])
        self.assertEqual(rep["known_failures"], [])
        self.assertEqual(rep["new_failures"], [])
        self.assertEqual(rep["fixed_candidates"], [])
        # Every AC passed, but an attached run accepts nothing.
        acs = self.acs(rep)
        self.assertEqual(sorted(acs), [("US-900", "AC-900.1"), ("US-900", "AC-900.2"),
                                       ("US-901", "AC-901.2")])
        for a in rep["acs"]:
            self.assertEqual(a["status"], "PASS", a)
            self.assertFalse(a["accepted"], a)
        self.assertEqual(acs[("US-901", "AC-901.2")]["checks"], ["cycle_next"])
        self.assertIn("DIAGNOSTIC ONLY", rep["evidence"])
        # The log lines carry the story prefix.
        self.assertIn("[US-900] PASS cycle_next", self.log.getvalue())

    def test_report_md_has_the_acceptance_criteria_table(self):
        _, server = self.serve()
        code, _ = self.play(make_run(cycle_steps()), server)
        self.assertEqual(code, 0)
        md = self.report_md()
        self.assertIn("## Acceptance criteria", md)
        self.assertIn("| AC | Checks | Status | Accepted |", md)
        self.assertIn("| AC-900.1 | cycle_next | PASS | no |", md)
        self.assertIn("| US-901:AC-901.2 | cycle_next | PASS | no |", md)
        self.assertIn("## Checks", md)
        self.assertIn("## Environment (state.focus)", md)
        self.assertIn(pb.AGENT_LINE, md)
        self.assertNotIn("WARN: undeclared", md)

    def test_a_fail_skips_its_dependents(self):
        _, server = self.serve()
        code, rep = self.play(make_run(cycle_steps(next_cond="player.weapon.index==5")), server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["cycle_next"]["status"], "FAIL")
        self.assertIn("but player.weapon.index==5 was not true within 0.5s", checks["cycle_next"]
                      ["detail"])
        self.assertIn('"player.weapon.index":1', checks["cycle_next"]["detail"])
        self.assertEqual(checks["cycle_prev"]["status"], "SKIP")
        self.assertEqual(checks["cycle_prev"]["detail"], "not run: cycle_next failed")
        self.assertEqual(rep["new_failures"], ["cycle_next"])
        acs = self.acs(rep)
        self.assertEqual(acs[("US-900", "AC-900.1")]["status"], "FAIL")
        self.assertEqual(acs[("US-901", "AC-901.2")]["status"], "FAIL")
        self.assertEqual(acs[("US-900", "AC-900.2")]["status"], "NOT RUN")
        self.assertIn("FAIL (NEW)", self.report_md())

    def test_a_lost_connection_skips_the_rest_and_is_infra(self):
        _, server = self.serve(faults=fb.Faults(drop_on_cmd="key"))
        code, rep = self.play(make_run(cycle_steps()), server)
        self.assertEqual(code, 2)
        checks = self.by_name(rep)
        self.assertEqual(checks["cycle_next"]["status"], "FAIL")
        self.assertEqual(checks["cycle_next"]["kind"], "infra")
        self.assertEqual(checks["cycle_prev"]["status"], "SKIP")
        self.assertIn("connection was lost in cycle_next", checks["cycle_prev"]["detail"])

    def test_an_undeclared_check_is_recorded_and_reported(self):
        def steps(run):
            cycle_steps()(run)
            run.expect("surprise", "player.weapon.index==0", timeout_s=1)

        _, server = self.serve()
        code, rep = self.play(make_run(steps), server)
        self.assertEqual(code, 0)
        self.assertEqual(self.by_name(rep)["surprise"]["status"], "PASS")
        self.assertEqual(rep["undeclared_checks"], ["surprise"])
        md = self.report_md()
        self.assertIn("WARN: undeclared check(s), not in PLAN: surprise", md)
        self.assertIn("undeclared check(s), not in PLAN: surprise", self.log.getvalue())

    def test_unreached_plan_checks_are_skipped_as_not_reached(self):
        def steps(run):
            run.press("cycle_next", "Tab", then="player.weapon.index==1")

        _, server = self.serve()
        code, rep = self.play(make_run(steps), server)
        self.assertEqual(code, 0)
        check = self.by_name(rep)["cycle_prev"]
        self.assertEqual(check["status"], "SKIP")
        self.assertEqual(check["detail"], kit.NOT_REACHED)
        self.assertEqual(self.acs(rep)[("US-900", "AC-900.2")]["status"], "NOT RUN")

    def test_a_script_error_records_completed_and_skips_the_rest(self):
        def steps(run):
            run.press("cycle_next", "Tab", then="player.weapon.index==1")
            raise RuntimeError("boom")

        _, server = self.serve()
        with mock.patch("traceback.print_exc"):
            code, rep = self.play(make_run(steps), server)
        self.assertEqual(code, 2)
        checks = self.by_name(rep)
        self.assertEqual(checks["completed"]["status"], "FAIL")
        self.assertIn("RuntimeError: boom", checks["completed"]["detail"])
        self.assertEqual(checks["cycle_prev"]["detail"], kit.NOT_REACHED)
        self.assertEqual(rep["undeclared_checks"], [])

    def test_a_known_issue_fail_is_known_and_still_fails(self):
        plan = (Step("cycle_next", "tap Tab -> index 5", proves=("AC-900.1",),
                     known_issue="Known issues #7: Tab skips a weapon"),
                Step("cycle_prev", "tap BackSlash", proves=("AC-900.2",)))
        _, server = self.serve()
        code, rep = self.play(make_run(cycle_steps(next_cond="player.weapon.index==5"), plan),
                              server)
        self.assertEqual(code, 1)
        self.assertEqual(rep["known_failures"], ["cycle_next"])
        self.assertEqual(rep["new_failures"], [])
        a = self.acs(rep)[("US-900", "AC-900.1")]
        self.assertEqual(a["status"], "FAIL")
        self.assertEqual(a["known_issue"], "Known issues #7: Tab skips a weapon")
        md = self.report_md()
        self.assertIn("FAIL (known)", md)
        self.assertNotIn("FAIL (NEW)", md)
        self.assertIn("Known issue on `cycle_next`", md)

    def test_a_known_issue_pass_is_a_fixed_candidate(self):
        plan = (Step("cycle_next", "tap Tab", proves=("AC-900.1",), known_issue="Known issues #7"),
                Step("cycle_prev", "tap BackSlash", proves=("AC-900.2",)))
        _, server = self.serve()
        code, rep = self.play(make_run(cycle_steps(), plan), server)
        self.assertEqual(code, 0)
        self.assertEqual(rep["fixed_candidates"], ["cycle_next"])
        self.assertEqual(rep["known_failures"], [])
        self.assertIn("PASS (FIXED?)", self.report_md())

    def test_pending_acs_are_reported_and_never_claimed(self):
        pending = {"AC-900.7": "bridge cannot drive: gamepad shoulder buttons are not mapped"}
        _, server = self.serve()
        code, rep = self.play(make_run(cycle_steps(), PENDING=pending), server)
        self.assertEqual(code, 0)
        a = self.acs(rep)[("US-900", "AC-900.7")]
        self.assertEqual(a["status"], "PENDING")
        self.assertFalse(a["accepted"])
        self.assertEqual(a["checks"], [])
        self.assertEqual(a["pending"], pending["AC-900.7"])
        self.assertIn("| AC-900.7 | - | PENDING (bridge cannot drive: gamepad shoulder buttons "
                      "are not mapped) | no |", self.report_md())

    def test_a_wrong_experience_skips_the_plan(self):
        _, server = self.serve(experience="B_LyraDefaultExperience")
        code, rep = self.play(make_run(cycle_steps()), server)
        self.assertEqual(code, 1)
        self.assertEqual(self.statuses(rep, "FAIL"), ["experience"])
        checks = self.by_name(rep)
        for name in ("cycle_next", "cycle_prev", "screenshot_start"):
            self.assertEqual(checks[name]["status"], "SKIP", name)
            self.assertIn("wrong experience", checks[name]["detail"])
        self.assertEqual({a["status"] for a in rep["acs"]}, {"NOT RUN"})
        self.assertNotIn("map", checks)  # the plan is blocked before the map is compared

    def test_no_bridge_to_attach_is_infra(self):
        code = kit.main(make_run(cycle_steps()), ["--attach-port", str(ab.free_port()),
                                                  "--run-dir", self.run_dir])
        self.assertEqual(code, 2)
        rep = self.report()
        self.assertEqual(self.statuses(rep, "FAIL"), ["attach"])
        self.assertEqual(self.by_name(rep)["cycle_next"]["status"], "SKIP")

    def test_the_audit_trail_has_only_input_and_reads(self):
        _, server = self.serve()
        code, _ = self.play(make_run(cycle_steps()), server)
        self.assertEqual(code, 0)
        sent = [e["msg"] for e in read_jsonl(os.path.join(self.run_dir, "bridge.jsonl"))
                if e["dir"] == "send"]
        self.assertLessEqual({m["cmd"] for m in sent}, INPUT_AND_READ_CMDS)
        self.assertFalse([m for m in sent if m.get("route") == "viewport" or m.get("unsafe")])

    def test_the_default_run_suffix_and_title(self):
        run = make_run(cycle_steps())(kit.parse_args(make_run(cycle_steps()), []))
        self.assertEqual(run.run_suffix, "us900")
        self.assertEqual(run.title, "US-900 playtest (Tier 4, real input)")
        front = make_run(cycle_steps(), FRONTEND=True)
        self.assertEqual(front.expected_experience(), kit.FRONTEND_EXPERIENCE)
        self.assertIsNone(front.launch_map())


class TestKitMapCheck(KitCase):
    """The 'map' kit check: ping.map must be the run's launch_map() (the test maps load the same
    experience as DevTestMap, so the experience check cannot tell them apart)."""

    def test_the_map_constants(self):
        self.assertEqual(kit.DEFAULT_MAP, "/MordecaiCore/Game/Maps/DevTestMap")
        self.assertEqual(kit.MOVEMENT_TEST_MAP, "/MordecaiCore/Game/Maps/MovementTestMap")
        self.assertEqual(kit.COMBAT_TEST_MAP, "/MordecaiCore/Game/Maps/CombatTestMap")
        self.assertEqual(kit.TEST_MAPS, (kit.MOVEMENT_TEST_MAP, kit.COMBAT_TEST_MAP))
        for name in ("MOVEMENT_TEST_MAP", "COMBAT_TEST_MAP", "TEST_MAPS"):
            self.assertEqual(getattr(kit, name), getattr(pb, name), name)  # one source: pb
        # the offline fake models the same maps
        self.assertEqual((fb.MOVEMENT_TEST_MAP, fb.COMBAT_TEST_MAP), kit.TEST_MAPS)
        self.assertIn("map", kit.KIT_CHECKS)

    def test_map_key_normalizes_suffix_and_case(self):
        want = "/mordecaicore/game/maps/movementtestmap"
        for value in (kit.MOVEMENT_TEST_MAP, kit.MOVEMENT_TEST_MAP + ".MovementTestMap",
                      "/MORDECAICORE/Game/Maps/movementtestmap", f"  {kit.MOVEMENT_TEST_MAP} "):
            self.assertEqual(kit.map_key(value), want, value)
        self.assertEqual(kit.map_key(None), "")
        self.assertNotEqual(kit.map_key(kit.DEFAULT_MAP), kit.map_key(kit.MOVEMENT_TEST_MAP))

    def test_the_map_the_script_asked_for_passes(self):
        for map_ in kit.TEST_MAPS:
            with self.subTest(map=map_):
                self.run_dir = os.path.join(self.tmp, "run-" + map_.rsplit("/", 1)[-1])
                _, server = self.serve(**fb.MAP_PRESETS[map_])
                code, rep = self.play(make_run(cycle_steps(), MAP=map_), server)
                self.assertEqual(code, 0, self.log.getvalue()[-3000:])
                checks = self.by_name(rep)
                self.assertEqual(checks["map"]["status"], "PASS")
                self.assertEqual(checks["map"]["kind"], "gameplay")
                self.assertEqual(checks["map"]["detail"], f"loaded {map_}")
                names = [c["name"] for c in rep["checks"]]
                self.assertEqual(names.index("map"), names.index("experience") + 1)
                self.assertEqual(rep["map"], map_)
                self.assertEqual(rep["ping"]["map"], map_)
                self.assertIn(f"- Map: {map_}  Expected experience: {kit.DEFAULT_EXPERIENCE}",
                              self.report_md())

    def test_a_wrong_map_fails_and_blocks_the_plan(self):
        # The script asks for CombatTestMap; the game runs DevTestMap (same experience).
        _, server = self.serve()
        code, rep = self.play(make_run(cycle_steps(), MAP=kit.COMBAT_TEST_MAP), server)
        self.assertEqual(code, 1)
        self.assertEqual(self.statuses(rep, "FAIL"), ["map"])
        checks = self.by_name(rep)
        self.assertEqual(checks["experience"]["status"], "PASS")
        self.assertEqual(checks["map"]["kind"], "gameplay")
        self.assertEqual(checks["map"]["detail"],
                         f"loaded {kit.DEFAULT_MAP}, expected {kit.COMBAT_TEST_MAP}")
        for name in ("cycle_next", "cycle_prev", "screenshot_start"):
            self.assertEqual(checks[name]["status"], "SKIP", name)
            self.assertIn("wrong map", checks[name]["detail"])
        self.assertEqual({a["status"] for a in rep["acs"]}, {"NOT RUN"})
        self.assertEqual(rep["new_failures"], ["map"])
        self.assertIn(f"loaded {kit.DEFAULT_MAP}, expected {kit.COMBAT_TEST_MAP}",
                      self.report_md())

    def test_a_default_map_script_on_a_test_map_fails(self):
        _, server = self.serve(**fb.MAP_PRESETS[kit.MOVEMENT_TEST_MAP])
        code, rep = self.play(make_run(cycle_steps()), server)  # MAP = DEFAULT_MAP
        self.assertEqual(code, 1)
        self.assertEqual(self.statuses(rep, "FAIL"), ["map"])
        self.assertEqual(self.by_name(rep)["cycle_next"]["status"], "SKIP")

    def test_object_suffix_and_case_are_normalized(self):
        _, server = self.serve(map=kit.MOVEMENT_TEST_MAP.lower())
        code, rep = self.play(make_run(cycle_steps(),
                                       MAP=kit.MOVEMENT_TEST_MAP + ".MovementTestMap"), server)
        self.assertEqual(code, 0, self.log.getvalue()[-3000:])
        self.assertEqual(self.by_name(rep)["map"]["status"], "PASS")

    def test_a_front_end_run_is_not_compared(self):
        _, server = self.serve(map="/Game/System/FrontEnd/Maps/L_LyraFrontEnd",
                               experience=kit.FRONTEND_EXPERIENCE)
        code, rep = self.play(make_run(cycle_steps(), FRONTEND=True), server)
        self.assertEqual(code, 0, self.log.getvalue()[-3000:])
        check = self.by_name(rep)["map"]
        self.assertEqual(check["status"], "INFO")
        self.assertIn("not compared", check["detail"])
        self.assertIn("L_LyraFrontEnd", check["detail"])
        self.assertEqual(self.by_name(rep)["cycle_next"]["status"], "PASS")
        self.assertIsNone(rep["map"])


class TestKitAcceptance(KitCase):
    """acceptance() without a game: status rules and the ACCEPTANCE CANDIDATE gate."""

    PLAN = (Step("a", "x", proves=("AC-900.1",)),
            Step("b", "x", proves=("AC-900.1", "AC-900.2")),
            Step("c", "x", proves=("AC-900.3",), kind="infra"),
            Step("d", "x", proves=("AC-900.4",)))

    def run_with(self, checks, argv=(), fresh=None, exit_code=0):
        cls = make_run(lambda run: None, self.PLAN)
        run = cls(kit.parse_args(cls, list(argv)))
        run.fresh = dict(fresh or self.CLEAN)
        run.exit_code = exit_code
        for name, status, kind in checks:
            pb.add_check(run.checks, name, status, "detail", kind)
        return {a["id"]: a for a in run.acceptance()}

    def test_a_candidate_run_accepts_gameplay_passes(self):
        acs = self.run_with([("a", "PASS", "gameplay"), ("b", "WARN", "gameplay"),
                             ("c", "PASS", "infra"), ("d", "WARN", "gameplay")])
        self.assertEqual(acs["AC-900.1"]["status"], "PASS")
        self.assertTrue(acs["AC-900.1"]["accepted"])
        self.assertEqual(acs["AC-900.1"]["checks"], ["a", "b"])
        self.assertEqual(acs["AC-900.2"]["status"], "NOT RUN")  # only a WARN: nothing passed
        self.assertFalse(acs["AC-900.2"]["accepted"])
        self.assertEqual(acs["AC-900.3"]["status"], "PASS")
        self.assertFalse(acs["AC-900.3"]["accepted"], "an infra check alone never accepts an AC")

    def test_fail_and_not_run(self):
        acs = self.run_with([("a", "FAIL", "gameplay"), ("b", "SKIP", "gameplay")],
                            exit_code=1)
        self.assertEqual(acs["AC-900.1"]["status"], "FAIL")
        self.assertEqual(acs["AC-900.2"]["status"], "NOT RUN")
        self.assertEqual(acs["AC-900.4"]["status"], "NOT RUN")  # never recorded

    def test_diagnostic_runs_accept_nothing(self):
        passing = [("a", "PASS", "gameplay"), ("b", "PASS", "gameplay")]
        for argv, fresh in ((["--allow-stale"], None), (["--attach-port", "1"], None),
                            ([], {"fresh": False, "reasons": ["stale"]}),
                            ([], {"fresh": None, "skipped": True})):
            acs = self.run_with(passing, argv, fresh)
            self.assertEqual(acs["AC-900.1"]["status"], "PASS")
            self.assertFalse(acs["AC-900.1"]["accepted"], (argv, fresh))
        acs = self.run_with(passing, exit_code=1)  # a fresh run that failed elsewhere
        self.assertFalse(acs["AC-900.1"]["accepted"])


class TestKitHelpers(KitCase):
    """Every step helper's PASS and timeout/FAIL path, in one run against the fake."""

    NAMES = ("expect_pass", "expect_fail", "press_pass", "press_fail", "hold_pass", "hold_fail",
             "axis_pass", "axis_fail", "walk_pass", "walk_fail", "observe_pass", "observe_fail",
             "measure_pass", "measure_fail", "open_menu", "click_button_pass",
             "click_button_fail", "click_button_missing", "click_pass", "click_fail",
             "needs_skip")

    @classmethod
    def setUpClass(cls):
        cls.read_result = {}

    def steps(self, run):
        type(self).read_result = run.read(["player"])
        run.expect("expect_pass", "player.weapon.index==0", timeout_s=2)
        run.expect("expect_fail", "player.weapon.index==7", timeout_s=0.3)
        run.press("press_pass", "Tab", then="player.weapon.index==1")
        run.press("press_fail", "Q", then="player.weapon.index==0", timeout_s=0.3)
        run.hold("hold_pass", "D", 0.3, then="player.location[1]>50")
        run.hold("hold_fail", "Q", 0.1, then="player.weapon.index==0", timeout_s=0.3)
        run.axis("axis_pass", {"Gamepad_LeftY": 1.0}, 0.3, then="player.location[0]>50")
        run.axis("axis_fail", {"Gamepad_LeftY": 0.5}, 0.1, then="player.weapon.index==0",
                 timeout_s=0.3)
        run.walk("walk_pass", 0, 0)
        run.walk("walk_fail", 2000, 0, timeout_s=1.0)
        run.observe("observe_pass", ["player"],
                    lambda st: kit.section(st, "player").get("valid") is True,
                    lambda st: f"weapon {kit.section(st, 'player')['weapon']['display_name']}")
        run.observe("observe_fail", ["player"], lambda st: False, "never true", timeout_s=0.3)
        run.measure("measure_pass", lambda b: ("PASS", f"pid {b.ping().get('pid')}", None))
        run.measure("measure_fail", lambda b: ("FAIL", "measured the wrong thing", None))
        run.press("open_menu", "I", then="pause_menu.inventory_tab.visible==true")
        run.click_button("click_button_pass", "Character",
                         then='pause_menu.active_tab=="character"')
        run.click_button("click_button_fail", "Skills", then='pause_menu.active_tab=="feats"',
                         timeout_s=0.3)
        run.click_button("click_button_missing", "Nope")
        run.click("click_pass", 640, 110, then='pause_menu.active_tab=="feats"')
        run.click("click_fail", 640, 110, then='pause_menu.active_tab=="skills"', timeout_s=0.3)
        run.press("needs_skip", "Tab", needs=False, needs_why="not run: a test prerequisite")
        run.shot("start")

    def test_every_helper(self):
        plan = tuple(Step(n, f"helper {n}") for n in self.NAMES)
        _, server = self.serve(stick_moves=True)
        code, rep = self.play(make_run(lambda run: self.steps(run), plan), server)
        self.assertEqual(code, 1)
        self.assertEqual(rep["undeclared_checks"], [])
        names = [c["name"] for c in rep["checks"]]
        for n in self.NAMES:
            self.assertEqual(names.count(n), 1, f"{n} must record exactly one check")
        checks = self.by_name(rep)
        for n in self.NAMES:
            if n.endswith("_pass") or n == "open_menu":
                self.assertEqual(checks[n]["status"], "PASS", f"{n}: {checks[n]['detail']}")
            elif n == "needs_skip":
                self.assertEqual(checks[n]["status"], "SKIP")
                self.assertEqual(checks[n]["detail"], "not run: a test prerequisite")
            else:
                self.assertEqual(checks[n]["status"], "FAIL", n)
                self.assertEqual(checks[n]["kind"], "gameplay", n)
        for n in ("expect_fail", "press_fail", "hold_fail", "axis_fail", "click_button_fail",
                  "click_fail"):
            self.assertIn("last values", checks[n]["detail"], n)
        self.assertIn('"player.weapon.index":0', checks["expect_fail"]["detail"])
        self.assertIn('"pause_menu.active_tab":"skills"', checks["click_button_fail"]["detail"])
        self.assertIn("tapped Q", checks["press_fail"]["detail"])
        self.assertIn("held D for 0.3s", checks["hold_pass"]["detail"])
        self.assertIn("axis Gamepad_LeftY=1 for 0.3s", checks["axis_pass"]["detail"])
        self.assertIn("reached (", checks["walk_pass"]["detail"])
        self.assertIn("walk_to timed out", checks["walk_fail"]["detail"])
        self.assertEqual(checks["observe_pass"]["detail"], "weapon Staff")
        self.assertEqual(checks["observe_fail"]["detail"], "not met within 0.3s: never true")
        self.assertIn("pid ", checks["measure_pass"]["detail"])
        self.assertIn("clicked 'Character' at (240, 110)", checks["click_button_pass"]["detail"])
        self.assertIn("no visible button with label 'Nope'",
                      checks["click_button_missing"]["detail"])
        self.assertIn("Left click at (640, 110); Slate hit STextBlock",
                      checks["click_pass"]["detail"])
        self.assertIn(checks["screenshot_start"]["status"], ("PASS", "WARN"))
        self.assertTrue(ab.file_complete(rep["screenshots"][0]["path"]))
        self.assertTrue(kit.section(self.read_result, "player").get("valid"))
        # Real input only: no debug command, no viewport route, no unsafe key.
        sent = [e["msg"] for e in read_jsonl(os.path.join(self.run_dir, "bridge.jsonl"))
                if e["dir"] == "send"]
        self.assertLessEqual({m["cmd"] for m in sent}, INPUT_AND_READ_CMDS)
        self.assertFalse([m for m in sent if m.get("route") == "viewport" or m.get("unsafe")])

    def test_a_failed_screenshot_is_an_infra_fail(self):
        _, server = self.serve(faults=fb.Faults(screenshot="failed"))
        code, rep = self.play(make_run(cycle_steps()), server)
        self.assertEqual(code, 2)
        check = self.by_name(rep)["screenshot_start"]
        self.assertEqual(check["status"], "FAIL")
        self.assertEqual(check["kind"], "infra")
        self.assertEqual(self.by_name(rep)["cycle_prev"]["status"], "PASS")


class TestKitPlan(KitCase):
    def test_plan_prints_the_steps_table_without_a_game(self):
        out = io.StringIO()
        previous = kit.set_log_stream(out)
        self.addCleanup(kit.set_log_stream, previous)
        with mock.patch.object(ab.Bridge, "launch") as launch, \
                mock.patch.object(ab.Bridge, "attach") as attach, \
                mock.patch.object(pb, "gate") as gate:
            code = kit.main(make_run(cycle_steps()), ["--plan", "--run-dir", self.run_dir])
        self.assertEqual(code, 0)
        launch.assert_not_called()
        attach.assert_not_called()
        gate.assert_not_called()
        self.assertFalse(os.path.exists(self.run_dir))
        lines = out.getvalue().strip().splitlines()
        self.assertEqual(lines[0], "| # | Check | Player step (bridge calls) → expected | "
                                   "Proves |")
        self.assertEqual(lines[1], "|---|---|---|---|")
        self.assertEqual(lines[2], "| 1 | `cycle_next` | tap Tab -> player.weapon.index==1 | "
                                   "AC-900.1, US-901:AC-901.2 |")
        self.assertEqual(lines[3], "| 2 | `cycle_prev` | tap BackSlash -> "
                                   "player.weapon.index==0 | AC-900.2 |")
        self.assertEqual(len(lines), 4)

    def test_plan_table_escapes_pipes_and_marks_known_issues(self):
        plan = (Step("x", "wait_until a==1 || b==2", known_issue="bug 3"),)
        table = kit.plan_table(make_run(cycle_steps(), plan))
        self.assertIn("| 1 | `x` | wait_until a==1 \\|\\| b==2 _(known issue: bug 3)_ | - |",
                      table)

    def test_a_bad_plan_is_refused(self):
        plan = (Step("cycle_next", "a", proves=("AC-900.1", "US-999:AC-999.1", "nope")),
                Step("cycle_next", "b"),
                Step("no_crash", "c"),
                Step("screenshot_x", "d"),
                Step("bad name", "e"),
                Step("empty", " "),
                Step("weird", "f", kind="other"))
        cls = make_run(cycle_steps(), plan, FEATURES=("weapons", "laser_tag"),
                       PENDING={"AC-900.9": "later"})
        problems = "\n".join(kit.plan_problems(cls))
        for text in ("US-999:AC-999.1 names US-999", "'nope' is not an AC id",
                     "check cycle_next is declared twice", "no_crash is a check the kit records",
                     "screenshot_x is a check the kit records", "bad check name 'bad name'",
                     "'does' is empty", "kind must be gameplay or infra", "'laser_tag'",
                     "PENDING[AC-900.9] must read 'bridge cannot drive: <reason>'"):
            self.assertIn(text, problems)
        code = kit.main(cls, ["--plan"])
        self.assertEqual(code, 2)
        self.assertIn("ERROR: bad declaration", self.log.getvalue())
        self.assertEqual(kit.plan_problems(make_run(cycle_steps())), [])
        self.assertIn("STORY must be 'US-NNN'", "\n".join(
            kit.plan_problems(make_run(cycle_steps(), STORY="079"))))

    def test_step_and_ac_ids(self):
        s = Step("a", "b", proves="AC3")
        self.assertEqual(s.proves, ("AC3",))
        with self.assertRaises(dataclasses.FrozenInstanceError):
            s.check = "z"
        self.assertEqual(kit.split_ac("US-077:AC-077.2", "US-078"), ("US-077", "AC-077.2"))
        self.assertEqual(kit.split_ac("AC-1.2.3", "US-078"), ("US-078", "AC-1.2.3"))
        self.assertEqual(kit.qualified_ac("US-077", "AC-077.2", "US-078"), "US-077:AC-077.2")
        self.assertEqual(kit.qualified_ac("US-078", "AC3", "US-078"), "AC3")


if __name__ == "__main__":
    unittest.main()
