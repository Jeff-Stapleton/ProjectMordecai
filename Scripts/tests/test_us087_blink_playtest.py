"""Offline fake-bridge tests for the US-087 Blink owner; never acceptance evidence."""

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
import playtest_kit as kit  # noqa: E402
from tests import fake_bridge as fb  # noqa: E402

PLAYTEST = os.path.join(SCRIPTS, "playtests", "us087_blink.py")
TOKEN = "us087-blink-token"


def load_playtest():
    spec = importlib.util.spec_from_file_location("us087_blink", PLAYTEST)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


us087 = load_playtest()


class BlinkWorld(fb.FakeWorld):
    def __init__(self, faults=()):
        kwargs = dict(fb.MAP_PRESETS[fb.COMBAT_TEST_MAP])
        super().__init__(**kwargs)
        self.faults = set(faults)
        self.status_tags = {us087.STONE_SKIN_TAG} if "stone_skin" in self.faults else set()
        self.replaced = False

    def _dummy(self):
        return next(e for e in self.enemies if e["label"] == "TestDummy")

    def _s_player(self):
        result = super()._s_player()
        result["tags"] = sorted(set(result.get("tags") or []) | self.status_tags)
        return result

    def _actors(self, radius, include_all):
        result = super()._actors(radius, include_all)
        if self.replaced:
            for actor in result:
                if actor.get("label") == "TestDummy":
                    actor["name"] = "ReplacementDummy"
        return result

    def _on_key_pressed(self, key):
        casts_before = len(self.casts)
        super()._on_key_pressed(key)
        if key != "E" or len(self.casts) == casts_before:
            return
        if "wrong_sp" in self.faults:
            self.spell_points = 9.0
        elif "no_sp" in self.faults:
            self.spell_points = 10.0
        if "never_invulnerable" not in self.faults:
            self.status_tags.add(us087.INVULN_TAG)
            if "stuck_invulnerable" not in self.faults:
                self.after(0.05, lambda world: world.status_tags.discard(us087.INVULN_TAG))

        distance = 0.0 if "no_movement" in self.faults else (
            250.0 if "insufficient_movement" in self.faults else 500.0)
        delay = us087.MOVE_WINDOW_S + 0.05 if "late_movement" in self.faults else 0.02
        if distance:
            self.after(delay, lambda world: world.pos.__setitem__(0, world.pos[0] + distance))
        if "target_replacement" in self.faults:
            self.after(0.03, lambda world: setattr(world, "replaced", True))
        if "target_movement" in self.faults:
            self.after(0.03, lambda world: world._dummy()["location"].__setitem__(0, 625.0))
        if "target_health" in self.faults:
            self.after(0.03, lambda world: world._dummy().__setitem__("health", 175.0))
        if "target_death" in self.faults:
            def kill_target(world):
                world._dummy().update(health=0.0, is_dead=True)
            self.after(0.03, kill_target)
        if "player_death" in self.faults:
            self.after(0.03, lambda world: world.kill_player())


class PlaytestCase(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
        self.tmp = self._tmp.name
        self.addCleanup(self._tmp.cleanup)
        self.log = io.StringIO()
        for module in (kit, ab, pb):
            previous = module.set_log_stream(self.log)
            self.addCleanup(module.set_log_stream, previous)
        session = mock.patch.object(ab, "SESSION_FILE", os.path.join(self.tmp, "session.json"))
        session.start()
        self.addCleanup(session.stop)
        for patch in (
            mock.patch.object(us087, "BASELINE_WATCH_S", 0.02),
            mock.patch.object(us087, "MOVE_WINDOW_S", 0.18),
            mock.patch.object(us087, "INVULN_END_WINDOW_S", 0.15),
            mock.patch.object(us087, "POLL_S", 0.005),
        ):
            patch.start()
            self.addCleanup(patch.stop)

    def play(self, faults=()):
        world = BlinkWorld(faults)
        server = fb.FakeBridgeServer(world, token=TOKEN, shot_dir=self.tmp).start()
        self.addCleanup(server.stop)
        run_dir = os.path.join(self.tmp, "run-" + "-".join(faults or ("pass",)))
        code = us087.main(["--attach-port", str(server.port), "--token", TOKEN,
                           "--run-dir", run_dir])
        with open(os.path.join(run_dir, "report.json"), encoding="utf-8") as handle:
            return world, server, code, json.load(handle)

    @staticmethod
    def checks(report):
        return {check["name"]: check for check in report["checks"]}


class TestUs087BlinkOutcomes(PlaytestCase):
    def test_pass_records_motion_cost_invulnerability_and_nonaccepting_attach(self):
        world, server, code, report = self.play()
        checks = self.checks(report)
        self.assertEqual(code, 0, self.log.getvalue()[-4000:])
        self.assertTrue(all(checks[name]["status"] == "PASS" for name in (
            "blink_preconditions", "blink_past_enemy", "blink_integrity",
            "blink_invulnerability_ends")))
        self.assertEqual(report["blink"]["sp_before"], 10.0)
        self.assertEqual(report["blink"]["sp_after"], 8.0)
        self.assertGreater(report["blink"]["final_location"][0] - report["blink"]["origin"][0],
                           us087.MIN_X_MOVEMENT)
        self.assertIsNotNone(report["blink"]["invulnerable_granted_at_wall_s"])
        self.assertIsNotNone(report["blink"]["invulnerable_cleared_at_wall_s"])
        self.assertFalse(any(entry["accepted"] for entry in report["acs"]))
        sent = {request.get("cmd") for request in server.received}
        self.assertLessEqual(sent, {"ping", "state", "key", "screenshot", "wait", "quit"})
        self.assertFalse(any(request.get("route") == "viewport" or request.get("unsafe")
                             for request in server.received))
        e_keys = [request for request in server.received
                  if request.get("cmd") == "key" and request.get("key") == "E"]
        self.assertEqual([request.get("action") for request in e_keys], ["press", "release"])
        self.assertIn((mock.ANY, "up", "E"), world.input_log)

    def test_no_and_insufficient_movement_fail_named_known_check(self):
        for fault in ("no_movement", "insufficient_movement"):
            with self.subTest(fault=fault):
                _, _, code, report = self.play((fault,))
                checks = self.checks(report)
                self.assertEqual(code, 1)
                self.assertEqual(checks["blink_past_enemy"]["status"], "FAIL")
                self.assertIn("blink_past_enemy", report["new_failures"])
                self.assertEqual(checks["blink_integrity"]["status"], "PASS")
                self.assertEqual(checks["blink_invulnerability_ends"]["status"], "PASS")

    def test_wrong_or_missing_sp_cost_fails(self):
        for fault in ("wrong_sp", "no_sp"):
            with self.subTest(fault=fault):
                _, _, code, report = self.play((fault,))
                self.assertEqual(code, 1)
                check = self.checks(report)["blink_past_enemy"]
                self.assertEqual(check["status"], "FAIL")
                self.assertIn("spell_points", check["detail"])

    def test_never_granted_and_stuck_invulnerability_fail_independently(self):
        for fault, text in (("never_invulnerable", "never observed"),
                            ("stuck_invulnerable", "never observed absent")):
            with self.subTest(fault=fault):
                _, _, code, report = self.play((fault,))
                checks = self.checks(report)
                self.assertEqual(code, 1)
                self.assertEqual(checks["blink_past_enemy"]["status"], "PASS")
                self.assertEqual(checks["blink_invulnerability_ends"]["status"], "FAIL")
                self.assertIn(text, checks["blink_invulnerability_ends"]["detail"])

    def test_stone_skin_contamination_sends_no_e(self):
        world, server, code, report = self.play(("stone_skin",))
        checks = self.checks(report)
        self.assertEqual(code, 1)
        self.assertEqual(checks["blink_preconditions"]["status"], "FAIL")
        self.assertIn("StoneSkin", checks["blink_preconditions"]["detail"].replace(" ", ""))
        self.assertTrue(all(checks[name]["status"] == "SKIP" for name in (
            "blink_past_enemy", "blink_integrity", "blink_invulnerability_ends")))
        self.assertNotIn("E", world.casts)
        self.assertFalse(any(request.get("cmd") == "key" and request.get("key") == "E"
                             for request in server.received))

    def test_late_movement_misses_the_two_second_contract(self):
        _, _, code, report = self.play(("late_movement",))
        self.assertEqual(code, 1)
        check = self.checks(report)["blink_past_enemy"]
        self.assertEqual(check["status"], "FAIL")
        self.assertIn("within", check["detail"])


class TestUs087BlinkIntegrity(PlaytestCase):
    def test_target_and_player_faults_fail_integrity_and_motion_acceptance(self):
        faults = ("target_replacement", "target_movement", "target_health",
                  "target_death", "player_death")
        expected = ("replaced", "moved", "health changed", "died", "player died")
        for fault, text in zip(faults, expected):
            with self.subTest(fault=fault):
                _, _, code, report = self.play((fault,))
                checks = self.checks(report)
                self.assertEqual(code, 1)
                self.assertEqual(checks["blink_integrity"]["status"], "FAIL")
                self.assertIn(text, checks["blink_integrity"]["detail"])
                self.assertEqual(checks["blink_past_enemy"]["status"], "FAIL")

    def test_plan_has_no_issue25_marker_after_local_green(self):
        self.assertEqual(kit.plan_problems(us087.Run), [])
        marked = [step.check for step in us087.PLAN if step.known_issue]
        self.assertEqual(marked, [])
        output = io.StringIO()
        previous = kit.set_log_stream(output)
        try:
            code = us087.main(["--plan"])
        finally:
            kit.set_log_stream(previous)
        self.assertEqual(code, 0)
        self.assertIn("`blink_past_enemy`", output.getvalue())
        self.assertIn("AC-087.9", output.getvalue())


if __name__ == "__main__":
    unittest.main()
