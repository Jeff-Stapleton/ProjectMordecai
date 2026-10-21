"""Tests for Scripts/playtests/us079_inventory.py against tests/fake_bridge.py and fake_game.py.

These prove the playtest's logic and reporting only; they say nothing about the real game.
Nothing here launches the real game.  Run: py Scripts/run_python_tests.py -k Us079
"""

import importlib.util
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
from tests import fake_bridge as fb  # noqa: E402

PLAYTEST = os.path.join(SCRIPTS, "playtests", "us079_inventory.py")
FAKE_GAME = os.path.join(SCRIPTS, "tests", "fake_game.py")
TOKEN = "us079-token"
INPUT_AND_READ_CMDS = {"ping", "state", "key", "click", "screenshot", "wait_until", "wait", "quit"}


def load_playtest():
    spec = importlib.util.spec_from_file_location("us079_inventory", PLAYTEST)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


us079 = load_playtest()


def read_jsonl(path):
    with open(path, encoding="utf-8") as f:
        return [json.loads(line) for line in f if line.strip()]


class PlaytestCase(unittest.TestCase):
    """Temp dir, captured logs, and a fake bridge per run."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
        self.tmp = self._tmp.name
        self.addCleanup(self._tmp.cleanup)
        self.log = io.StringIO()
        for module in (us079, ab, pb):
            previous = module.set_log_stream(self.log)
            self.addCleanup(module.set_log_stream, previous)
        # Never read the real agent_bridge session file.
        patcher = mock.patch.object(ab, "SESSION_FILE", os.path.join(self.tmp, "session.json"))
        patcher.start()
        self.addCleanup(patcher.stop)
        self.run_dir = os.path.join(self.tmp, "run")

    def serve(self, faults=None, **world_kwargs):
        world_kwargs.setdefault("obstacles", fb.DEVTESTMAP_OBSTACLES)
        world = fb.FakeWorld(**world_kwargs)
        server = fb.FakeBridgeServer(world, token=TOKEN, shot_dir=self.tmp, faults=faults).start()
        self.addCleanup(server.stop)
        return world, server

    def play(self, server, *extra):
        code = us079.main(["--attach-port", str(server.port), "--token", TOKEN,
                           "--run-dir", self.run_dir, *extra])
        return code, self.report()

    def report(self):
        with open(os.path.join(self.run_dir, "report.json"), encoding="utf-8") as f:
            return json.load(f)

    @staticmethod
    def by_name(report):
        return {c["name"]: c for c in report["checks"]}

    @staticmethod
    def statuses(report, status):
        return [c["name"] for c in report["checks"] if c["status"] == status]


class TestUs079AgainstTheFake(PlaytestCase):
    def test_passes_when_escape_closes_the_menu(self):
        world, server = self.serve(escape_closes_menu=True)
        code, rep = self.play(server)
        self.assertEqual(code, 0, self.log.getvalue()[-3000:])
        self.assertEqual(self.statuses(rep, "FAIL"), [])
        self.assertEqual(self.statuses(rep, "SKIP"), [])
        checks = self.by_name(rep)
        for name in ("attach", "experience", "hud_visible", "inventory_count", "inventory_items",
                     "ledger_count", "ledger_entries", "menu_opens_inventory", "menu_rows",
                     "menu_ledger_rows", "identify_button", "identify_click",
                     "amulet_identified", "menu_closes_escape", "no_crash"):
            self.assertEqual(checks[name]["status"], "PASS", name)
        for stop in us079.ROUTE:
            for prefix in ("walk", "focus", "prompt", "pickup"):
                self.assertEqual(checks[f"{prefix}_{stop['name']}"]["status"], "PASS",
                                 f"{prefix}_{stop['name']}")
        self.assertEqual(checks["walk_RustySword_waypoint1"]["status"], "PASS")
        # Each pickup is confirmed by what was stored, and both counts are in the detail.
        self.assertIn("inventory.count 0 -> 1, inventory.ledger_count 0 -> 0",
                      checks["pickup_HealthPotion"]["detail"])
        self.assertIn("inventory.count 1 -> 1, inventory.ledger_count 0 -> 1",
                      checks["pickup_IronOre"]["detail"])
        # The game state the fake ended in: all six picked, the amulet identified, menu closed.
        self.assertEqual(sorted(world.picked), sorted(s["label"] for s in us079.ROUTE))
        self.assertEqual(len(world.inventory), 4)
        amulet = [i for i in world.inventory if i["item_id"] == us079.AMULET_ITEM_ID][0]
        self.assertTrue(amulet["identified"])
        self.assertFalse(world.menu_open)
        # Attached to a game it did not launch, and the fake is no fresh package: diagnostic.
        self.assertEqual(checks["build_fresh"]["status"], "WARN")
        self.assertIn("DIAGNOSTIC ONLY", rep["evidence"])
        self.assertEqual(rep["result"], "PASS")

    def test_run_dir_has_the_report_screenshots_and_transcript(self):
        _, server = self.serve()
        code, rep = self.play(server)
        self.assertEqual(code, 0)
        names = sorted(os.listdir(self.run_dir))
        for n in ("report.md", "report.json", "bridge.jsonl", "bridge_01.png", "bridge_02.png",
                  "bridge_03.png", "bridge_04.png"):
            self.assertIn(n, names)
        self.assertEqual([s["label"] for s in rep["screenshots"]],
                         ["start", "menu", "identified", "after_escape"])
        for s in rep["screenshots"]:
            self.assertTrue(ab.file_complete(s["path"]), s["path"])
        md = open(os.path.join(self.run_dir, "report.md"), encoding="utf-8").read()
        self.assertIn("Open every screenshot with the Read tool", md)
        self.assertIn(pb.AGENT_LINE, md)
        for s in rep["screenshots"]:
            self.assertIn(s["path"], md)
        self.assertIn("## Environment (state.focus)", md)
        self.assertIsInstance(rep["focus_start"], dict)
        self.assertIsInstance(rep["focus_end"], dict)
        self.assertIn("commonui_input_mode", rep["focus_start"])
        # The audit trail: only input commands, state reads, waits and screenshots.
        sent = [e["msg"]["cmd"] for e in read_jsonl(os.path.join(self.run_dir, "bridge.jsonl"))
                if e["dir"] == "send"]
        self.assertTrue(sent)
        self.assertLessEqual(set(sent), INPUT_AND_READ_CMDS)
        self.assertFalse([c for c in sent if c.startswith("debug_")])
        keys = [r.get("key") or r.get("keys") for r in server.received if r.get("cmd") == "key"]
        self.assertIn("F", keys)
        self.assertIn("I", keys)
        self.assertIn("Escape", keys)

    def test_escape_regression_fails_step_6_and_nothing_else(self):
        # The game's Escape fix (back handler) regressed: a FAIL that names the regression.
        world, server = self.serve(escape_closes_menu=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(self.statuses(rep, "FAIL"), ["menu_closes_escape"])
        self.assertEqual(self.statuses(rep, "SKIP"), [])
        detail = self.by_name(rep)["menu_closes_escape"]["detail"]
        self.assertIn("still open", detail)
        self.assertIn("REGRESSION of the back-handler fix (AC-069.10), not a known issue", detail)
        self.assertIn("bIsBackHandler", detail)
        self.assertIn("Mordecai.UI.PauseMenu.*", detail)
        self.assertNotIn("Known issue", detail)
        self.assertTrue(world.menu_open)
        # The screenshot after Escape is still taken: it shows the menu stuck open.
        self.assertIn("after_escape", [s["label"] for s in rep["screenshots"]])
        self.assertEqual(rep["result"], "FAIL")

    def test_known_issues_and_the_after_escape_expectation_match_the_fixed_game(self):
        _, server = self.serve()
        code, rep = self.play(server)
        self.assertEqual(code, 0)
        self.assertEqual(rep["known_issues"], [
            "I does not close the pause menu (no UI action binding); Escape closes it via the "
            "CommonUI back handler."])
        after = [s for s in rep["screenshots"] if s["label"] == "after_escape"][0]
        self.assertEqual(after["expect"], "the arena with the HUD (pause menu closed)")
        md = open(os.path.join(self.run_dir, "report.md"), encoding="utf-8").read()
        self.assertNotIn("not a back handler", md)
        self.assertNotIn("predicted", md.lower())

    def test_a_missing_pickup_fails(self):
        labels = [s["label"] for s in fb.PICKUP_SPECS if s["label"] != "TestPickup_RustySword"]
        world, server = self.serve(pickups=labels)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["walk_RustySword"]["status"], "PASS")
        self.assertEqual(checks["focus_RustySword"]["status"], "FAIL")
        self.assertEqual(checks["prompt_RustySword"]["status"], "SKIP")
        self.assertEqual(checks["pickup_RustySword"]["status"], "SKIP")
        self.assertEqual(checks["inventory_count"]["status"], "FAIL")
        self.assertIn("inventory.count=3", checks["inventory_count"]["detail"])
        self.assertEqual(checks["inventory_items"]["status"], "FAIL")
        self.assertIn("TestItem_RustySword missing", checks["inventory_items"]["detail"])
        self.assertEqual(checks["menu_rows"]["status"], "FAIL")
        # Independent steps still ran and passed.
        for name in ("ledger_entries", "identify_button", "amulet_identified",
                     "menu_closes_escape"):
            self.assertEqual(checks[name]["status"], "PASS", name)

    def fast_pickup_timeout(self):
        patcher = mock.patch.object(us079, "PICKUP_TIMEOUT_S", 0.5)
        patcher.start()
        self.addCleanup(patcher.stop)

    def test_an_invalid_pickups_section_after_f_is_a_fail_not_a_pickup(self):
        # The pawn is lost the frame F goes down: pickups (and inventory) turn valid:false and
        # have no 'world' list.  That is no evidence of a pickup (review finding 13).
        self.fast_pickup_timeout()
        world, server = self.serve(lose_pawn_on_f=True)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["pickup_HealthPotion"]
        self.assertEqual(check["status"], "FAIL")
        self.assertIn("pickups section invalid after F (no pawn yet)", check["detail"])
        self.assertIn("inventory.count 0 -> ?, inventory.ledger_count 0 -> ?", check["detail"])
        self.assertFalse(rep["pickups"][0]["picked"])
        self.assertEqual(world.picked, [])
        md = open(os.path.join(self.run_dir, "report.md"), encoding="utf-8").read()
        row = [line for line in md.splitlines() if line.startswith("| TestPickup_HealthPotion |")]
        self.assertTrue(row and row[0].rstrip().endswith("| no |"), row)

    def test_a_pickup_that_stores_nothing_fails(self):
        self.fast_pickup_timeout()
        world, server = self.serve(pickup_adds_nothing=True)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        for stop in us079.ROUTE:
            check = checks[f"pickup_{stop['name']}"]
            self.assertEqual(check["status"], "FAIL", stop["name"])
            self.assertIn("left pickups.world but nothing was stored", check["detail"])
        self.assertIn("inventory.count 0 -> 0, inventory.ledger_count 0 -> 0",
                      checks["pickup_HealthPotion"]["detail"])
        self.assertEqual(sorted(world.picked), sorted(s["label"] for s in us079.ROUTE))

    def test_a_wrong_experience_blocks_the_rest(self):
        _, server = self.serve(experience="B_LyraDefaultExperience")
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["experience"]["status"], "FAIL")
        self.assertIn("B_LyraDefaultExperience", checks["experience"]["detail"])
        self.assertEqual(checks["walk_HealthPotion"]["status"], "SKIP")
        self.assertEqual(checks["menu_closes_escape"]["status"], "SKIP")
        self.assertEqual(self.statuses(rep, "FAIL"), ["experience"])

    def test_a_lost_connection_is_infra_and_skips_the_rest(self):
        _, server = self.serve(faults=fb.Faults(drop_on_cmd="click"))
        code, rep = self.play(server)
        self.assertEqual(code, 2)
        checks = self.by_name(rep)
        self.assertEqual(checks["identify_click"]["status"], "FAIL")
        self.assertEqual(checks["identify_click"]["kind"], "infra")
        for name in ("amulet_identified", "screenshot_identified", "menu_closes_escape",
                     "screenshot_after_escape"):
            self.assertEqual(checks[name]["status"], "SKIP", name)
            self.assertIn("connection was lost", checks[name]["detail"])
        self.assertEqual(checks["menu_opens_inventory"]["status"], "PASS")

    def test_no_bridge_to_attach_is_infra(self):
        code = us079.main(["--attach-port", str(ab.free_port()), "--run-dir", self.run_dir])
        self.assertEqual(code, 2)
        rep = self.report()
        checks = self.by_name(rep)
        self.assertEqual(checks["attach"]["status"], "FAIL")
        self.assertEqual(checks["walk_HealthPotion"]["status"], "SKIP")
        self.assertEqual(self.statuses(rep, "FAIL"), ["attach"])

    def test_attach_snapshots_the_session_game_log(self):
        _, server = self.serve(experience="B_LyraDefaultExperience")  # a quick run
        session_run = os.path.join(self.tmp, "session_run")
        os.makedirs(session_run)
        with open(os.path.join(session_run, "game.log"), "w", encoding="utf-8") as f:
            f.write("LogTemp: Warning: SkipPackage: /MordecaiCore/Missing/Thing\n"
                    "LogTemp: Error: something broke\n")
        ab.write_session(ab.SESSION_FILE, {"pid": os.getpid(), "port": server.port,
                                           "run_dir": session_run})
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertTrue(os.path.isfile(os.path.join(self.run_dir, "game.log")))
        checks = self.by_name(rep)
        self.assertEqual(checks["content_cooked"]["status"], "FAIL")
        self.assertEqual(rep["missing_content"], ["/MordecaiCore/Missing/Thing"])
        self.assertIn("snapshot", rep["game_log_note"])


class TestUs079Launch(PlaytestCase):
    def test_launch_mode_with_the_fake_game(self):
        world_json = os.path.join(self.tmp, "world.json")
        with open(world_json, "w", encoding="utf-8") as f:
            json.dump({"obstacles": [list(b) for b in fb.DEVTESTMAP_OBSTACLES]}, f)
        code = us079.main(["--test-exe-argv", json.dumps([sys.executable, FAKE_GAME]),
                           "--test-skip-freshness", "--run-dir", self.run_dir,
                           f"--extra-arg=-FakeWorldJson={pb.quote_value(world_json)}"])
        rep = self.report()
        self.assertEqual(code, 0, json.dumps([c for c in rep["checks"]
                                              if c["status"] in ("FAIL", "SKIP")]))
        checks = self.by_name(rep)
        self.assertEqual(checks["launch"]["status"], "PASS")
        self.assertIn("kill-on-close job", checks["launch"]["detail"])
        self.assertEqual(checks["build_fresh"]["status"], "WARN")
        self.assertEqual(checks["no_crash"]["status"], "PASS")
        self.assertEqual(checks["content_cooked"]["status"], "PASS")
        self.assertTrue(os.path.isfile(os.path.join(self.run_dir, "game.log")))
        self.assertIn("exit code 0", rep["shutdown"])
        self.assertIn("-AgentBridge", rep["command_line"])
        self.assertIn("DIAGNOSTIC ONLY", rep["evidence"])
        pid = rep["ping"]["pid"]
        self.assertFalse(ab.pid_alive(pid), "the fake game should have quit")

    def test_a_fresh_package_built_from_a_dirty_tree_is_diagnostic_only(self):
        # playtest_boot's gate calls a dirty build fresh when its dirty files still match.
        world_json = os.path.join(self.tmp, "world.json")
        with open(world_json, "w", encoding="utf-8") as f:
            json.dump({"obstacles": [list(b) for b in fb.DEVTESTMAP_OBSTACLES]}, f)
        dirty = {"fresh": True, "reasons": [], "warnings": [], "git_error": None,
                 "build": {"commit": "abc1234def5678", "branch": "x", "dirty": True,
                           "dirty_count": 3},
                 "head": "abc1234def5678", "branch": "x"}
        with mock.patch.object(pb, "gate", return_value=(dirty, None)) as gate:
            code = us079.main(["--test-exe-argv", json.dumps([sys.executable, FAKE_GAME]),
                               "--run-dir", self.run_dir,
                               f"--extra-arg=-FakeWorldJson={pb.quote_value(world_json)}"])
        rep = self.report()
        self.assertEqual(code, 0, json.dumps([c for c in rep["checks"]
                                              if c["status"] in ("FAIL", "SKIP")]))
        self.assertTrue(gate.called)
        self.assertFalse(gate.call_args[0][1], "allow_stale must be False by default")
        check = self.by_name(rep)["build_fresh"]
        self.assertEqual(check["status"], "WARN")
        self.assertIn("DIRTY tree", check["detail"])
        self.assertIn("NOT acceptance evidence", check["detail"])
        self.assertTrue(rep["evidence"].startswith("DIAGNOSTIC ONLY"), rep["evidence"])
        self.assertIn("built from a dirty working tree", rep["evidence"])
        self.assertNotIn("ACCEPTANCE CANDIDATE", rep["evidence"])
        md = open(os.path.join(self.run_dir, "report.md"), encoding="utf-8").read()
        self.assertIn("built from a DIRTY tree (3 uncommitted file(s))", md)
        self.assertNotIn("- Freshness: OK", md)
        self.assertNotIn("--allow-stale", md)

    def test_stale_build_stops_before_a_run_dir(self):
        exe = os.path.join(self.tmp, "LyraGame.exe")
        with open(exe, "wb") as f:
            f.write(b"not a game")
        stale = {"fresh": False, "reasons": ["build commit aaa != HEAD bbb"], "warnings": [],
                 "git_error": None, "build": None, "head": "bbb", "branch": "x"}
        with mock.patch.object(pb, "gate", return_value=(stale, pb.EXIT_INFRA)) as gate, \
                mock.patch.object(ab.Bridge, "launch") as launch:
            code = us079.main(["--exe", exe, "--run-dir", self.run_dir])
        self.assertEqual(code, 2)
        gate.assert_called_once()
        self.assertFalse(gate.call_args[0][1], "allow_stale must be False by default")
        launch.assert_not_called()
        self.assertFalse(os.path.exists(self.run_dir))
        self.assertIn("STALE BUILD", self.log.getvalue())

    def test_missing_exe_is_infra(self):
        code = us079.main(["--exe", os.path.join(self.tmp, "nope.exe"), "--run-dir",
                           self.run_dir])
        self.assertEqual(code, 2)
        self.assertFalse(os.path.exists(self.run_dir))


class TestUs079Evidence(PlaytestCase):
    """evidence() and report.md's Freshness / Evidence lines, without a game."""

    CLEAN = {"fresh": True, "reasons": [], "warnings": [], "git_error": None,
             "build": {"commit": "abc1234def5678", "branch": "x", "dirty": False,
                       "dirty_count": 0}, "head": "abc1234def5678", "branch": "x"}
    STALE = {"fresh": False, "reasons": ["build commit aaa != HEAD bbb"], "warnings": [],
             "git_error": None, "build": None, "head": "bbb", "branch": "x"}

    def written(self, argv, fresh):
        run = us079.Us079Run(us079.parse_args(argv + ["--run-dir", self.run_dir]))
        run.fresh = fresh
        run.run_dir = self.run_dir
        os.makedirs(self.run_dir, exist_ok=True)
        run.exit_code = us079.EXIT_PASS
        run.finished_at = "2026-10-20T00:00:00"
        run.write_reports()
        with open(os.path.join(self.run_dir, "report.md"), encoding="utf-8") as f:
            lines = f.read().splitlines()
        line = {k: [x for x in lines if x.startswith(f"- {k}:")][0]
                for k in ("Evidence", "Freshness")}
        return run.evidence(), line

    def test_allow_stale_is_recorded_and_diagnostic_even_on_a_fresh_clean_build(self):
        evidence, line = self.written(["--allow-stale"], dict(self.CLEAN))
        self.assertTrue(evidence.startswith("DIAGNOSTIC ONLY"), evidence)
        self.assertIn("--allow-stale", line["Evidence"])
        self.assertIn("OK (the package matches the code under test) (run with --allow-stale)",
                      line["Freshness"])

    def test_a_fresh_clean_launch_without_allow_stale_is_a_candidate(self):
        evidence, line = self.written([], dict(self.CLEAN))
        self.assertTrue(evidence.startswith("ACCEPTANCE CANDIDATE"), evidence)
        self.assertNotIn("--allow-stale", line["Evidence"] + line["Freshness"])

    def test_a_stale_build_with_allow_stale(self):
        evidence, line = self.written(["--allow-stale"], dict(self.STALE))
        self.assertIn("STALE or unverified build", evidence)
        self.assertIn("run with --allow-stale", line["Evidence"])
        self.assertIn("STALE BUILD", line["Freshness"])
        self.assertIn("(run with --allow-stale)", line["Freshness"])

    def test_attach_mode_is_always_diagnostic(self):
        evidence, _ = self.written(["--attach-port", "1"], dict(self.CLEAN))
        self.assertTrue(evidence.startswith("DIAGNOSTIC ONLY"), evidence)
        self.assertIn("did not launch", evidence)
        self.assertIn("An attached run is ALWAYS diagnostic only, never acceptance evidence",
                      " ".join(us079.__doc__.split()))


class TestUs079Helpers(unittest.TestCase):
    def test_lit_quotes_and_escapes(self):
        self.assertEqual(us079.lit("[F] Pick up X"), '"[F] Pick up X"')
        self.assertEqual(us079.lit('a"b\\c'), '"a\\"b\\\\c"')
        text = 'say "hi" \\ ok'  # one backslash, two double quotes
        cond = fb.Condition("pickups.prompt.text==" + us079.lit(text))
        ok, values = cond.evaluate({"pickups": {"prompt": {"text": text}}})
        self.assertTrue(ok, values)

    def test_route_matches_the_devtestmap_pickups(self):
        specs = {s["label"]: s for s in fb.PICKUP_SPECS}
        self.assertEqual(len(us079.ROUTE), 6)
        for stop in us079.ROUTE:
            spec = specs[stop["label"]]
            self.assertEqual(list(stop["target"]), spec["location"][:2])
            self.assertEqual(stop["item_id"], spec["item_id"])
            self.assertEqual(stop["display_name"], spec["display_name"])
        inventory = {s["item_id"] for s in fb.PICKUP_SPECS if s["route"] == "inventory"}
        ledger = {s["item_id"] for s in fb.PICKUP_SPECS if s["route"] == "ledger"}
        self.assertEqual(set(us079.EXPECTED_ITEMS), inventory)
        self.assertEqual(set(us079.EXPECTED_LEDGER), ledger)

    def test_waypoint_avoids_box_02(self):
        # Box_02 grown by the capsule radius: X -490..-310, Y -390..-210.
        (wx, wy), = [s for s in us079.ROUTE if s["name"] == "RustySword"][0]["waypoints"]
        self.assertFalse(-490 <= wx <= -310 and -390 <= wy <= -210)


if __name__ == "__main__":
    unittest.main()
