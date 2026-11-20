"""Offline fake-bridge coverage for Scripts/playtests/us124_player_posture.py."""

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

PLAYTEST = os.path.join(SCRIPTS, "playtests", "us124_player_posture.py")
TOKEN = "us124-token"


def load_playtest():
    spec = importlib.util.spec_from_file_location("us124_player_posture", PLAYTEST)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


us124 = load_playtest()


class PlayerPostureWorld(fb.FakeWorld):
    """Gameplay double for the packaged fixture; never acceptance evidence."""

    def __init__(self, *, break_runtime=True, vulnerability=True,
                 knockdown_runtime=True, recovery=True):
        enemies = [
            {"label": f"PostureTestAttacker_{name}",
             "class": f"BP_MordecaiEnemy_PostureTest{name}_C",
             "location": [0.0, y, 100.0], "max_health": 200.0, "ai_state": "Idle"}
            for name, y in (
                ("Control", -120.0), ("Breaker", -60.0), ("BonusA", 0.0),
                ("BonusB", 60.0), ("Down", 120.0))
        ]
        super().__init__(
            map=us124.PLAYER_POSTURE_TEST_MAP,
            start=(-1200.0, 0.0),
            arena_walls=False,
            obstacles=(),
            pickups=[],
            enemies=enemies,
        )
        self.break_runtime = break_runtime
        self.vulnerability = vulnerability
        self.knockdown_runtime = knockdown_runtime
        self.recovery = recovery
        self.posture = self.max_posture = 100.0
        self.broken_until = -1.0
        self.down_until = -1.0
        self.stagger_damage = 0.0
        self.fixture_started = False

    def tick(self, dt):
        super().tick(dt)
        with self.lock:
            now = self.real_time()
            if self.recovery and self.down_until > 0.0 and now >= self.down_until:
                self.down_until = -1.0
                self.posture = self.max_posture
            elif self.recovery and self.broken_until > 0.0 and now >= self.broken_until:
                self.broken_until = -1.0
                self.stagger_damage = 0.0
                self.posture = self.max_posture
            if not self.fixture_started and self.pos[0] >= us124.FIGHT_SPOT[0] - 25.0:
                self.fixture_started = True
                self.after(0.20, lambda world: world.hit_player(20.0, 0.0))
                self.after(0.50, lambda world: world.hit_player(1.0, 100.0))
                self.after(0.80, lambda world: world.hit_player(20.0, 0.0))
                self.after(1.00, lambda world: world.hit_player(20.0, 0.0))
                self.after(1.30, lambda world: world.hit_player(5.0, 0.0))

    def _move(self, dt):
        now = self.real_time()
        if now < self.broken_until or now < self.down_until:
            self.vel = [0.0, 0.0]
            return
        super()._move(dt)

    def hit_player(self, base_damage, posture_scalar):
        now = self.real_time()
        was_broken = now < self.broken_until
        was_down = now < self.down_until
        multiplier = 2.0 if was_down and self.vulnerability else (
            1.5 if was_broken and self.vulnerability else 1.0)
        actual = base_damage * multiplier
        self.health = max(0.0, self.health - actual)
        if was_broken:
            self.stagger_damage += actual
            if self.stagger_damage >= 40.0 and self.knockdown_runtime:
                self.broken_until = -1.0
                self.down_until = now + 2.0
        self.posture = max(0.0, self.posture - base_damage * posture_scalar)
        if (self.posture <= 0.0 and self.break_runtime
                and self.down_until <= now and self.broken_until <= now):
            self.broken_until = now + 1.5
            self.stagger_damage = 0.0

    def _s_player(self):
        value = super()._s_player()
        now = self.real_time()
        tags = [tag for tag in value["tags"]
                if tag not in (us124.TAG_BROKEN, us124.TAG_DOWN)]
        if now < self.broken_until:
            tags.append(us124.TAG_BROKEN)
        if now < self.down_until:
            tags.append(us124.TAG_DOWN)
        value["tags"] = tags
        value["attributes"]["posture"] = self.posture
        value["attributes"]["max_posture"] = self.max_posture
        value["movement_mode"] = (
            "None" if now < self.broken_until or now < self.down_until else "Walking")
        return value


class Us124PlaytestCase(unittest.TestCase):
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

    def serve(self, **kwargs):
        world = PlayerPostureWorld(**kwargs)
        server = fb.FakeBridgeServer(world, token=TOKEN, shot_dir=self.tmp).start()
        self.addCleanup(server.stop)
        return world, server

    def play(self, server):
        passed_image = {
            "path": "fake.png", "width": 1280, "height": 720,
            "rgb_stddev": [10.0, 10.0, 10.0], "non_blank": True, "passed": True,
        }
        with mock.patch.object(us124, "_image_stats", return_value=passed_image):
            code = us124.main([
                "--attach-port", str(server.port), "--token", TOKEN,
                "--run-dir", self.run_dir,
            ])
        with open(os.path.join(self.run_dir, "report.json"), encoding="utf-8") as stream:
            return code, json.load(stream)

    @staticmethod
    def checks(report):
        return {check["name"]: check for check in report["checks"]}

    def test_plan_contract(self):
        self.assertEqual(kit.plan_problems(us124.Us124Run), [])
        self.assertIs(us124.Run, us124.Us124Run)
        self.assertEqual(us124.Us124Run.launch_map(), us124.PLAYER_POSTURE_TEST_MAP)
        self.assertEqual(
            [step.check for step in us124.PLAN],
            ["setup", "unbroken_control", "break_and_lock", "broken_bonus",
             "knockdown", "down_bonus", "recovery", "image_verification"])

    def test_full_real_input_route_passes_against_fake_bridge(self):
        _, server = self.serve()
        code, report = self.play(server)
        self.assertEqual(code, 0, self.log.getvalue()[-5000:])
        self.assertEqual(
            [name for name, check in self.checks(report).items()
             if check["status"] == "FAIL"], [])
        self.assertEqual(report["damage_route"], [20.0, 30.0, 30.0, 10.0])
        sent = {request.get("cmd") for request in server.received}
        self.assertLessEqual(
            sent, {"ping", "state", "key", "axis", "mouse", "screenshot",
                   "wait", "wait_until"})

    def test_missing_break_is_a_hard_failure(self):
        _, server = self.serve(break_runtime=False)
        code, report = self.play(server)
        check = self.checks(report)["break_and_lock"]
        self.assertEqual(code, 1)
        self.assertEqual(check["status"], "FAIL")
        self.assertIn(us124.TAG_BROKEN, check["detail"])

    def test_missing_bonus_damage_is_a_hard_failure(self):
        _, server = self.serve(vulnerability=False)
        code, report = self.play(server)
        check = self.checks(report)["broken_bonus"]
        self.assertEqual(code, 1)
        self.assertEqual(check["status"], "FAIL")
        self.assertNotEqual(check["status"], "WARN")

    def test_missing_knockdown_is_a_hard_failure(self):
        _, server = self.serve(knockdown_runtime=False)
        code, report = self.play(server)
        check = self.checks(report)["knockdown"]
        self.assertEqual(code, 1)
        self.assertEqual(check["status"], "FAIL")
        self.assertIn(us124.TAG_DOWN, check["detail"])

    def test_missing_recovery_is_a_hard_failure(self):
        _, server = self.serve(recovery=False)
        code, report = self.play(server)
        check = self.checks(report)["recovery"]
        self.assertEqual(code, 1)
        self.assertEqual(check["status"], "FAIL")
        self.assertNotEqual(check["status"], "WARN")


if __name__ == "__main__":
    unittest.main()
