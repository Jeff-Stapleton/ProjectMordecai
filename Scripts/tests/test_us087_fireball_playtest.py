"""Offline logic tests for Scripts/playtests/us087_fireball.py.

These run only against FakeBridgeServer and are not game or acceptance evidence.
"""

import importlib.util
import io
import json
import math
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

PLAYTEST = os.path.join(SCRIPTS, "playtests", "us087_fireball.py")
TOKEN = "us087-fireball-token"


def load_playtest():
    spec = importlib.util.spec_from_file_location("us087_fireball", PLAYTEST)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


us087 = load_playtest()


class FireballWorld(fb.FakeWorld):
    def __init__(self, *, projectile=True, visible=True, damage=True, reset=False,
                 replacement=False, dead_player=False, full_target=True):
        kwargs = dict(fb.MAP_PRESETS[fb.COMBAT_TEST_MAP])
        super().__init__(**kwargs)
        self.projectile_enabled = projectile
        self.projectile_visible = visible
        self.damage_enabled = damage
        self.reset_enabled = reset
        self.replacement_enabled = replacement
        self.dead_player_enabled = dead_player
        self.projectiles = []
        self.replaced = False
        self.cast_position = None
        if not full_target:
            self._dummy()["health"] = 150.0

    def _dummy(self):
        return next(e for e in self.enemies if e["label"] == "TestDummy")

    def _on_key_pressed(self, key):
        casts_before = len(self.casts)
        super()._on_key_pressed(key)
        if key != "Q" or len(self.casts) == casts_before:
            return
        self.cast_position = list(self.pos)
        if self.projectile_enabled:
            self.after(0.03, lambda world: world.projectiles.append(
                {"name": "Fireball_1", "location": [world.pos[0], world.pos[1], 100.0]}))
        if self.damage_enabled:
            self.after(0.09, lambda world: world._damage_dummy())
        if self.dead_player_enabled:
            self.after(0.07, lambda world: world.kill_player())

    def _damage_dummy(self):
        self._dummy()["health"] = 175.0
        if self.reset_enabled:
            self.after(0.06, lambda world: world._dummy().__setitem__("health", 200.0))
        if self.replacement_enabled:
            self.after(0.06, lambda world: setattr(world, "replaced", True))

    def tick(self, dt):
        super().tick(dt)
        with self.lock:
            for projectile in list(self.projectiles):
                projectile["location"][0] += 2000.0 * dt
                if projectile["location"][0] - self.pos[0] > 700.0:
                    self.projectiles.remove(projectile)

    def _actors(self, radius, include_all):
        entries = super()._actors(radius, include_all)
        for entry in entries:
            if entry.get("label") == "TestDummy":
                entry["name"] = "TestDummyReplacement" if self.replaced else "TestDummy"
        px, py = self.pos
        for projectile in self.projectiles:
            x, y, z = projectile["location"]
            distance = math.sqrt((x - px) ** 2 + (y - py) ** 2 + (z - fb.PAWN_Z) ** 2)
            if distance <= radius:
                entries.append({
                    "label": projectile["name"], "name": projectile["name"],
                    "class": "MordecaiProjectile", "native_class": "MordecaiProjectile",
                    "kind": "projectile", "location": [x, y, z], "distance": distance,
                    "hidden": False, "has_visible_mesh": self.projectile_visible,
                    "health": None, "is_dead": None, "ai_state": None, "tags": [],
                    "widgets": [], "text_labels": [],
                })
        entries.sort(key=lambda actor: actor["distance"])
        return entries


class PlaytestCase(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
        self.tmp = self._tmp.name
        self.addCleanup(self._tmp.cleanup)
        self.run_dir = os.path.join(self.tmp, "run")
        self.log = io.StringIO()
        for module in (kit, ab, pb):
            previous = module.set_log_stream(self.log)
            self.addCleanup(module.set_log_stream, previous)
        session = mock.patch.object(ab, "SESSION_FILE", os.path.join(self.tmp, "session.json"))
        session.start()
        self.addCleanup(session.stop)
        patches = (
            mock.patch.object(us087, "SHOT_DISTANCE", 300.0),
            mock.patch.object(us087, "BASELINE_WATCH_S", 0.03),
            mock.patch.object(us087, "PROJECTILE_WINDOW_S", 0.25),
            mock.patch.object(us087, "DAMAGE_WINDOW_S", 0.4),
            mock.patch.object(us087, "POLL_S", 0.01),
        )
        for patch in patches:
            patch.start()
            self.addCleanup(patch.stop)

    def play(self, **world_options):
        world = FireballWorld(**world_options)
        server = fb.FakeBridgeServer(world, token=TOKEN, shot_dir=self.tmp).start()
        self.addCleanup(server.stop)
        code = us087.main(["--attach-port", str(server.port), "--token", TOKEN,
                           "--run-dir", self.run_dir])
        with open(os.path.join(self.run_dir, "report.json"), encoding="utf-8") as handle:
            report = json.load(handle)
        return world, server, code, report

    @staticmethod
    def checks(report):
        return {check["name"]: check for check in report["checks"]}


class TestUs087IndependentOutcomes(PlaytestCase):
    def test_all_projectile_and_damage_outcome_combinations(self):
        combinations = (
            (True, True, "PASS", "PASS"),
            (True, False, "PASS", "FAIL"),
            (False, True, "FAIL", "PASS"),
            (False, False, "FAIL", "FAIL"),
        )
        for index, (projectile, damage, projectile_status, damage_status) in enumerate(combinations):
            with self.subTest(projectile=projectile, damage=damage):
                self.run_dir = os.path.join(self.tmp, f"run-{index}")
                _, _, code, report = self.play(projectile=projectile, damage=damage)
                checks = self.checks(report)
                self.assertEqual(checks["projectile_visible"]["status"], projectile_status)
                self.assertEqual(checks["same_target_damaged"]["status"], damage_status)
                self.assertEqual(checks["fireball_cast"]["status"], "PASS")
                self.assertEqual(checks["target_integrity"]["status"], "PASS")
                self.assertEqual(code, 0 if projectile and damage else 1)

    def test_invisible_projectile_fails_visibility_but_damage_still_passes(self):
        _, _, code, report = self.play(projectile=True, visible=False, damage=True)
        checks = self.checks(report)
        self.assertEqual(code, 1)
        self.assertEqual(checks["projectile_visible"]["status"], "FAIL")
        self.assertIn("has_visible_mesh was never true", checks["projectile_visible"]["detail"])
        self.assertEqual(checks["same_target_damaged"]["status"], "PASS")


class TestUs087IntegrityFailures(PlaytestCase):
    def test_health_reset_invalidates_same_target_damage(self):
        _, _, code, report = self.play(reset=True)
        checks = self.checks(report)
        self.assertEqual(code, 1)
        self.assertEqual(checks["projectile_visible"]["status"], "PASS")
        self.assertEqual(checks["target_integrity"]["status"], "FAIL")
        self.assertIn("health increased/reset", checks["target_integrity"]["detail"])
        self.assertEqual(checks["same_target_damaged"]["status"], "FAIL")

    def test_target_replacement_invalidates_same_target_damage(self):
        _, _, code, report = self.play(replacement=True)
        checks = self.checks(report)
        self.assertEqual(code, 1)
        self.assertEqual(checks["target_integrity"]["status"], "FAIL")
        self.assertIn("replaced", checks["target_integrity"]["detail"])
        self.assertEqual(checks["same_target_damaged"]["status"], "FAIL")

    def test_dead_player_invalidates_the_run(self):
        _, _, code, report = self.play(dead_player=True)
        checks = self.checks(report)
        self.assertEqual(code, 1)
        self.assertEqual(checks["target_integrity"]["status"], "FAIL")
        self.assertIn("player died", checks["target_integrity"]["detail"])
        self.assertEqual(checks["same_target_damaged"]["status"], "FAIL")

    def test_failed_preconditions_send_no_q_and_skip_acceptance_checks(self):
        world, server, code, report = self.play(full_target=False)
        checks = self.checks(report)
        self.assertEqual(code, 1)
        self.assertEqual(checks["target_preconditions"]["status"], "FAIL")
        for name in ("fireball_cast", "projectile_visible", "target_integrity",
                     "same_target_damaged"):
            self.assertEqual(checks[name]["status"], "SKIP")
        self.assertNotIn("Q", world.casts)
        self.assertFalse(any(request.get("cmd") == "key" and request.get("key") == "Q"
                             for request in server.received))


class TestUs087ReportAndAudit(PlaytestCase):
    def test_pass_report_has_exact_target_timing_screenshots_and_safe_commands(self):
        world, server, code, report = self.play()
        self.assertEqual(code, 0, self.log.getvalue()[-4000:])
        self.assertEqual(report["story"], "US-087")
        self.assertEqual(report["features"], ["spells", "projectiles_aim"])
        self.assertEqual(report["fireball"]["target_name"], "TestDummy")
        self.assertIsNotNone(report["fireball"]["projectile_at_wall_s"])
        self.assertIsNotNone(report["fireball"]["damage_at_wall_s"])
        self.assertEqual({shot["label"] for shot in report["screenshots"]},
                         {"projectile_visible", "same_target_damaged"})
        self.assertIsNotNone(world.cast_position)
        self.assertLess(abs(world.cast_position[0] - 300.0), us087.SHOT_TOL + 1)
        self.assertLess(abs(world.cast_position[1] - 600.0), us087.SHOT_TOL + 1)
        sent = {request.get("cmd") for request in server.received}
        self.assertLessEqual(sent, {"ping", "state", "key", "screenshot", "wait", "quit"})
        self.assertFalse(any(request.get("route") == "viewport" or request.get("unsafe")
                             for request in server.received))
        acs = {(entry["story"], entry["id"]): entry for entry in report["acs"]}
        self.assertEqual(acs[("US-087", "AC-087.4")]["status"], "PASS")
        self.assertEqual(acs[("US-087", "AC-087.5")]["status"], "PASS")
        self.assertFalse(any(entry["accepted"] for entry in report["acs"]),
                         "attach-mode fake runs are diagnostic, never acceptance")

    def test_plan_is_valid_and_printable(self):
        output = io.StringIO()
        previous = kit.set_log_stream(output)
        try:
            code = us087.main(["--plan"])
        finally:
            kit.set_log_stream(previous)
        self.assertEqual(code, 0)
        self.assertEqual(kit.plan_problems(us087.Run), [])
        self.assertIn("`projectile_visible`", output.getvalue())
        self.assertIn("`same_target_damaged`", output.getvalue())


if __name__ == "__main__":
    unittest.main()
