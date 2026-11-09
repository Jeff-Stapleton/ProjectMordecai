"""Offline tests for issue092_melee_debug_draw.py; no game or editor is launched."""

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

import playtest_kit as kit  # noqa: E402
from tests import fake_bridge as fb  # noqa: E402
from tests.test_us054_combat_playtest import CombatWorld  # noqa: E402

SCRIPT = os.path.join(SCRIPTS, "playtests", "issue092_melee_debug_draw.py")
spec = importlib.util.spec_from_file_location("issue092_melee_debug_draw", SCRIPT)
issue092 = importlib.util.module_from_spec(spec)
spec.loader.exec_module(issue092)


class TestIssue092Declarations(unittest.TestCase):
    def test_uses_issue_identifier_and_both_launch_modes(self):
        self.assertEqual(issue092.Issue092Run.STORY, "ISSUE-92")
        self.assertNotEqual(issue092.Issue092Run.STORY, "US-092")
        self.assertEqual(issue092.Issue092Run.launch_map(), kit.COMBAT_TEST_MAP)
        self.assertFalse(issue092.is_off_launch([]))
        self.assertTrue(issue092.is_off_launch(
            ['-ExecCmds="Mordecai.Debug.DrawMeleeHitShapes 0"']))
        self.assertEqual([step.check for step in issue092.PLAN],
                         ["draw_configuration", "empty_space_swing", "walk_to_dummy",
                          "player_hits_dummy", "walk_to_attacker", "enemy_hits_player"])

    def test_plan_prints_without_claiming_us092(self):
        with mock.patch("sys.stdout") as out:
            self.assertEqual(issue092.main(["--plan"]), 0)
        text = "".join(call.args[0] for call in out.write.call_args_list if call.args)
        self.assertIn("`empty_space_swing`", text)
        self.assertIn("`enemy_hits_player`", text)
        self.assertNotIn("US-092", text)


class TestIssue092AgainstFake(unittest.TestCase):
    def test_hit_miss_and_enemy_steps_pass_with_real_input_commands(self):
        with tempfile.TemporaryDirectory(ignore_cleanup_errors=True) as temp:
            world = CombatWorld(windup_s=0.05, attack_s=0.15, cooldown_s=0.15)
            server = fb.FakeBridgeServer(world, token="issue092-token", shot_dir=temp).start()
            self.addCleanup(server.stop)
            run_dir = os.path.join(temp, "run")
            with mock.patch.object(issue092, "HIT_TIMEOUT_S", 1.0), \
                    mock.patch.object(issue092, "ENEMY_TIMEOUT_S", 4.0):
                code = issue092.main([
                    "--attach-port", str(server.port),
                    "--token", "issue092-token",
                    "--run-dir", run_dir,
                ])
            with open(os.path.join(run_dir, "report.json"), encoding="utf-8") as stream:
                report = json.load(stream)

            self.assertEqual(code, 0)
            checks = {check["name"]: check for check in report["checks"]}
            for name in ("empty_space_swing", "player_hits_dummy", "enemy_hits_player"):
                self.assertEqual(checks[name]["status"], "PASS", checks[name]["detail"])
            self.assertEqual([shot["label"] for shot in report["screenshots"]],
                             ["miss", "hit", "enemy"])
            commands = {request.get("cmd") for request in server.received}
            self.assertLessEqual(commands,
                                 {"ping", "state", "key", "click", "screenshot", "quit"})
            self.assertIn("click", commands)
            self.assertIn("key", commands)


if __name__ == "__main__":
    unittest.main()
