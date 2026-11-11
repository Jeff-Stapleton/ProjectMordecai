"""Offline tests for the US-097 gamepad Tier 4 playtest."""

import importlib.util
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

PLAYTEST = os.path.join(SCRIPTS, "playtests", "us097_gamepad.py")
TOKEN = "us097-token"


def load_playtest():
    spec = importlib.util.spec_from_file_location("us097_gamepad", PLAYTEST)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


us097 = load_playtest()


class GamepadWorld(fb.FakeWorld):
    def __init__(self, **kw):
        kw.setdefault("stick_moves", True)
        kw.setdefault("map", fb.MOVEMENT_TEST_MAP)
        kw.setdefault("enemies", False)
        super().__init__(**kw)
        self.base_speed = self.speed
        self.sprinting = False

    def _on_key_pressed(self, key):
        super()._on_key_pressed(key)
        if self.ready and not self.menu_open and not self.is_dead and key == "Gamepad_LeftThumbstick":
            self.sprinting = not self.sprinting
            self.speed = self.base_speed * (1.5 if self.sprinting else 1.0)


class PlaytestCase(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
        self.tmp = self._tmp.name
        self.addCleanup(self._tmp.cleanup)
        patcher = mock.patch.object(ab, "SESSION_FILE", os.path.join(self.tmp, "session.json"))
        patcher.start()
        self.addCleanup(patcher.stop)
        self.run_dir = os.path.join(self.tmp, "run")

    def serve(self, world_class=GamepadWorld, **world_kwargs):
        for key, value in fb.MAP_PRESETS[fb.MOVEMENT_TEST_MAP].items():
            world_kwargs.setdefault(key, value)
        world = world_class(**world_kwargs)
        server = fb.FakeBridgeServer(world, token=TOKEN, shot_dir=self.tmp).start()
        self.addCleanup(server.stop)
        return world, server

    def play(self, server):
        code = us097.main(["--attach-port", str(server.port), "--token", TOKEN,
                           "--run-dir", self.run_dir])
        with open(os.path.join(self.run_dir, "report.json"), encoding="utf-8") as f:
            return code, json.load(f)


class TestUs097Playtest(PlaytestCase):
    def test_fixed_gamepad_mapping_passes_move_and_sprint(self):
        world, server = self.serve()
        code, report = self.play(server)
        self.assertEqual(code, 0)
        checks = {c["name"]: c for c in report["checks"]}
        for name in ("spawn_ready", "imc_active", "gamepad_move", "sprint_l3"):
            self.assertEqual(checks[name]["status"], "PASS", checks[name]["detail"])
        self.assertFalse(world.sprinting, "the second L3 tap toggles sprint off")
        self.assertEqual(report["new_failures"], [])
        self.assertEqual(report["known_failures"], [])

    def test_dead_stick_mapping_fails_honestly(self):
        _, server = self.serve(fb.FakeWorld, stick_moves=False)
        code, report = self.play(server)
        self.assertEqual(code, 1)
        checks = {c["name"]: c for c in report["checks"]}
        self.assertEqual(checks["gamepad_move"]["status"], "FAIL")
        self.assertIn("gamepad_move", report["new_failures"])

    def test_plan_and_route(self):
        self.assertEqual(kit.plan_problems(us097.Us097Run), [])
        self.assertEqual(us097.Us097Run.STORY, "US-097")
        self.assertEqual(us097.Us097Run.MAP, kit.MOVEMENT_TEST_MAP)
        self.assertEqual([step.check for step in us097.PLAN],
                         ["spawn_ready", "imc_active", "gamepad_move", "sprint_l3"])


if __name__ == "__main__":
    unittest.main()
