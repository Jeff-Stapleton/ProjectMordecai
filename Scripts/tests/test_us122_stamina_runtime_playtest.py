"""Offline contract tests for US-122's Tier 4 script.

Fake physics and mocked pixels are never acceptance evidence. These tests only
pin the real-input route, state assertions, reporting, and failure behavior.
"""

import importlib.util
import json
import math
import os
import sys
import tempfile
import time
import unittest
from unittest import mock

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if SCRIPTS not in sys.path:
    sys.path.insert(0, SCRIPTS)

import agent_bridge as ab  # noqa: E402
import playtest_kit as kit  # noqa: E402
from tests import fake_bridge as fb  # noqa: E402

PLAYTEST = os.path.join(SCRIPTS, "playtests", "us122_stamina_runtime.py")
TOKEN = "us122-offline-token"
INPUT_AND_READ = {"ping", "state", "key", "axis", "wait", "screenshot"}


def load_playtest():
    spec = importlib.util.spec_from_file_location("us122_stamina_runtime", PLAYTEST)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


us122 = load_playtest()


class StaminaWorld(fb.FakeWorld):
    """Fast fake-only stamina profile matching the production state contract."""

    def __init__(self, *, ignore_sprint=False, **kwargs):
        super().__init__(**kwargs)
        self.ignore_sprint = ignore_sprint
        self.sprinting = False
        self.blocking = False
        self.sprint_has_moved = False
        self.sprint_toggle_s = 0.0
        self.stamina = 100.0
        self.last_spend_at = None
        self.stationary_s = 0.0
        self.delay_s = 0.5
        self.drain_per_s = 200.0
        self.regen_per_s = 300.0

    def _on_key_pressed(self, key):
        super()._on_key_pressed(key)
        if key == us122.SPRINT_KEY and not self.ignore_sprint and self.game_input_allowed:
            if self.blocking:
                return
            self.sprinting = not self.sprinting
            self.stationary_s = 0.0
            self.sprint_has_moved = False
            self.sprint_toggle_s = 0.0
            self.speed = 900.0 if self.sprinting else 600.0

    def set_axis(self, conn_id, key, value):
        super().set_axis(conn_id, key, value)
        if key == us122.BLOCK_AXIS:
            self.blocking = float(value) > 0.0

    def tick(self, dt):
        super().tick(dt)
        with self.lock:
            moving = math.hypot(*self.vel) > 60.0
            if self.sprinting:
                self.sprint_toggle_s += dt
            if self.sprinting and moving:
                self.sprint_has_moved = True
                self.stationary_s = 0.0
                self.stamina = max(0.0, self.stamina - self.drain_per_s * dt)
                self.last_spend_at = self.world_time
            else:
                if self.sprinting and (
                    self.sprint_has_moved or self.sprint_toggle_s >= 1.0
                ):
                    self.stationary_s += dt
                    if self.stationary_s >= 0.25:
                        self.sprinting = False
                        self.speed = 600.0
                if (
                self.last_spend_at is not None
                and self.world_time - self.last_spend_at >= self.delay_s
                ):
                    self.stamina = min(100.0, self.stamina + self.regen_per_s * dt)

    def _s_player(self):
        state = super()._s_player()
        state["attributes"]["stamina"] = round(self.stamina, 2)
        tags = list(state["tags"])
        if self.blocking:
            tags.append(us122.TAG_BLOCKING)
        if self.sprinting:
            tags.append(us122.TAG_SPRINTING)
        if self.stamina <= 0.0:
            tags.append(us122.TAG_EXHAUSTED)
        elif self.stamina <= 25.0:
            tags.append(us122.TAG_RED)
        elif self.stamina >= 75.0:
            tags.append(us122.TAG_GREEN)
        else:
            tags.append("Mordecai.Stamina.Tier.Yellow")
        state["tags"] = tags
        return state


def fake_analysis(path, expected):
    return {
        "path": os.path.abspath(path),
        "expected": expected,
        "non_uniform": True,
        "rgb_stddev": [20.0, 20.0, 20.0],
        "hud_green_pixels": 30 if expected == "green" else 0,
        "hud_red_pixels": 30 if expected == "red" else 0,
        "passed": True,
    }


class Us122FakeCase(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
        self.tmp = self._tmp.name
        self.addCleanup(self._tmp.cleanup)
        session_patch = mock.patch.object(
            ab, "SESSION_FILE", os.path.join(self.tmp, "session.json"))
        session_patch.start()
        self.addCleanup(session_patch.stop)
        self.run_dir = os.path.join(self.tmp, "run")

    def serve(self, *, ignore_sprint=False):
        world = StaminaWorld(
            map=us122.Us122Run.MAP,
            experience=us122.Us122Run.EXPERIENCE,
            start=(0.0, 0.0),
            arena_walls=False,
            obstacles=(),
            pickups=[],
            enemies=False,
            ignore_sprint=ignore_sprint,
        )
        server = fb.FakeBridgeServer(world, token=TOKEN, shot_dir=self.tmp).start()
        self.addCleanup(server.stop)
        return server

    def play(self, server):
        patches = (
            mock.patch.object(us122, "LANE_WAYPOINTS", ((0.0, 0.0),)),
            mock.patch.object(us122, "PULSE_S", 0.05),
            mock.patch.object(us122, "DELAY_OBSERVE_S", 0.075),
            mock.patch.object(us122, "LOW_TIER_SPEED_HOLD_S", 0.05),
            mock.patch.object(us122, "analyze_hud_image", fake_analysis),
        )
        with (
            patches[0], patches[1], patches[2],
            patches[3], patches[4]
        ):
            code = us122.main([
                "--attach-port", str(server.port),
                "--token", TOKEN,
                "--run-dir", self.run_dir,
            ])
        with open(os.path.join(self.run_dir, "report.json"), encoding="utf-8") as stream:
            return code, json.load(stream)


class TestUs122Playtest(Us122FakeCase):
    def test_compliant_runtime_passes_and_uses_only_input_and_reads(self):
        server = self.serve()
        code, report = self.play(server)
        self.assertEqual(code, 0)
        checks = {check["name"]: check for check in report["checks"]}
        for name in (
            "idle_sprint_auto_clears", "block_rejects_sprint", "sprint_drains",
            "low_tier_authoritative",
            "low_tier_sprint_speed", "regen_delay", "refills_green", "image_verification",
        ):
            self.assertEqual(checks[name]["status"], "PASS", checks[name]["detail"])
        with open(os.path.join(self.run_dir, "image-verification.json"), encoding="utf-8") as stream:
            self.assertEqual(json.load(stream)["result"], "PASS")
        with open(os.path.join(self.run_dir, "bridge.jsonl"), encoding="utf-8") as stream:
            entries = [json.loads(line) for line in stream if line.strip()]
        commands = {
            entry["msg"]["cmd"] for entry in entries if entry.get("dir") == "send"
        }
        self.assertLessEqual(commands, INPUT_AND_READ)
        stop_taps = [
            entry["msg"] for entry in entries
            if entry.get("dir") == "send"
            and entry.get("msg", {}).get("cmd") == "key"
            and entry["msg"].get("action") == "tap"
            and entry["msg"].get("key") == us122.SPRINT_KEY
            and entry["msg"].get("hold_s") == us122.STOP_TAP_HOLD_S
        ]
        self.assertEqual(len(stop_taps), 1, "sprint stop must use the one-frame tap")

    def test_missing_sprint_runtime_fails_honestly(self):
        server = self.serve(ignore_sprint=True)
        code, report = self.play(server)
        self.assertEqual(code, 1)
        checks = {check["name"]: check for check in report["checks"]}
        self.assertEqual(checks["sprint_drains"]["status"], "FAIL")
        self.assertIn("sprint_drains", report["new_failures"])

    def test_plan_contract(self):
        self.assertEqual(kit.plan_problems(us122.Us122Run), [])
        self.assertEqual(us122.Us122Run.MAP, kit.MOVEMENT_TEST_MAP)
        self.assertGreaterEqual(
            us122.LOW_TIER_SPEED_HOLD_S, 1.0,
            "the speed sample must outlast acceleration after the drain loop changes direction")
        self.assertEqual(
            [step.check for step in us122.PLAN],
            [
                "walk_lane", "idle_sprint_auto_clears", "block_rejects_sprint", "sprint_drains",
                "low_tier_authoritative", "low_tier_sprint_speed", "regen_delay",
                "refills_green", "image_verification",
            ],
        )


if __name__ == "__main__":
    unittest.main()
