"""Offline failure-path tests for the US-125 Tier 4 scripts.

These use the fake bridge only; they do not prove game behavior.
"""

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
from tests import fake_bridge as fb  # noqa: E402

TOKEN = "us125-token"


def _load(name):
    path = os.path.join(SCRIPTS, "playtests", name + ".py")
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


attack_cancel = _load("us125_attack_cancel")
perfect_combo = _load("us125_perfect_dodge_combo")


class TestUs125PlaytestsAgainstFakeBridge(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
        self.addCleanup(self.temp.cleanup)
        patcher = mock.patch.object(ab, "SESSION_FILE",
                                    os.path.join(self.temp.name, "session.json"))
        patcher.start()
        self.addCleanup(patcher.stop)

    def _run(self, module, world, name):
        server = fb.FakeBridgeServer(world, token=TOKEN, shot_dir=self.temp.name).start()
        self.addCleanup(server.stop)
        run_dir = os.path.join(self.temp.name, name)
        code = module.main(["--attach-port", str(server.port), "--token", TOKEN,
                            "--run-dir", run_dir])
        with open(os.path.join(run_dir, "report.json"), encoding="utf-8") as handle:
            return code, json.load(handle)

    @staticmethod
    def _checks(report):
        return {check["name"]: check for check in report["checks"]}

    def test_attack_cancel_fails_when_real_attack_tags_never_appear(self):
        world = fb.FakeWorld(**fb.MAP_PRESETS[fb.MOVEMENT_TEST_MAP])
        code, report = self._run(attack_cancel, world, "attack-cancel")
        checks = self._checks(report)
        self.assertEqual(code, 1)
        self.assertEqual(checks["setup_yellow"]["status"], "FAIL")
        self.assertIn("Attacking", checks["setup_yellow"]["detail"])
        self.assertEqual(checks["recovery_dodge_cancel"]["status"], "SKIP")

    def test_attack_cancel_accepts_zero_as_a_settled_speed(self):
        state = {"player": {"valid": True, "speed": 0}}
        self.assertEqual(attack_cancel._speed(state), 0.0)

    def test_perfect_combo_skips_when_test_attacker_is_absent(self):
        options = dict(fb.MAP_PRESETS[fb.COMBAT_TEST_MAP])
        options["enemies"] = []
        world = fb.FakeWorld(**options)
        code, report = self._run(perfect_combo, world, "perfect-combo")
        checks = self._checks(report)
        self.assertEqual(code, 1)
        self.assertEqual(checks["setup"]["status"], "FAIL")
        self.assertEqual(checks["perfect_dodge"]["status"], "SKIP")
        self.assertEqual(checks["swap_during_combo"]["status"], "SKIP")


if __name__ == "__main__":
    unittest.main()
