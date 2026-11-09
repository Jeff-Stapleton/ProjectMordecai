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
import playtest_boot as pb  # noqa: E402
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
        self.assertEqual(
            issue092.build_exec_arg(False),
            "-ExecCmds=Mordecai.Debug.DrawMeleeHitShapes")
        off_commands = ("Mordecai.Debug.DrawMeleeHitShapes 0,"
                        "Mordecai.Debug.DrawMeleeHitShapes")
        self.assertEqual(
            issue092.build_exec_arg(True),
            f"-ExecCmds={pb.quote_value(off_commands)}")

        default_plan = issue092.plan_for_mode(False)
        off_plan = issue092.plan_for_mode(True)
        self.assertEqual(next(s for s in default_plan if s.check == "draw_configuration").kind,
                         "infra")
        self.assertEqual(next(s for s in off_plan if s.check == "draw_configuration").proves,
                         ("AC-2",))
        default_acs = {ac for step in default_plan for ac in step.proves}
        off_acs = {ac for step in off_plan for ac in step.proves}
        self.assertEqual(default_acs, {"AC-1", "AC-4", "AC-5", "AC-8"})
        self.assertEqual(off_acs, {"AC-2", "AC-8"})

    def test_plan_prints_without_claiming_us092(self):
        with mock.patch("sys.stdout") as out:
            self.assertEqual(issue092.main(["--plan"]), 0)
        text = "".join(call.args[0] for call in out.write.call_args_list if call.args)
        self.assertIn("`empty_space_swing`", text)
        self.assertIn("`enemy_hits_player`", text)
        self.assertNotIn("US-092", text)


class TestIssue092AgainstFake(unittest.TestCase):
    def play(self, *, draw_off=False, applied=1, **world_kwargs):
        temp = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
        self.addCleanup(temp.cleanup)
        world = CombatWorld(windup_s=0.05, attack_s=0.15, cooldown_s=0.15,
                            **world_kwargs)
        server = fb.FakeBridgeServer(world, token="issue092-token", shot_dir=temp.name).start()
        self.addCleanup(server.stop)
        run_dir = os.path.join(temp.name, "run")
        os.makedirs(run_dir)
        with open(os.path.join(run_dir, "game.log"), "w", encoding="utf-8") as stream:
            stream.write(f'LogConsoleResponse: Display: {issue092.CVAR} = "{applied}"      '
                         'LastSetBy: Console\n')
        argv = ["--attach-port", str(server.port), "--token", "issue092-token",
                "--run-dir", run_dir]
        if draw_off:
            argv.append("--draw-off")
        with mock.patch.object(issue092, "HIT_TIMEOUT_S", 1.0), \
                mock.patch.object(issue092, "ENEMY_TIMEOUT_S", 4.0):
            code = issue092.main(argv)
        with open(os.path.join(run_dir, "report.json"), encoding="utf-8") as stream:
            return code, json.load(stream), server

    @staticmethod
    def checks(report):
        return {check["name"]: check for check in report["checks"]}

    def test_default_run_checks_gameplay_but_leaves_visual_acs_for_screenshot_review(self):
        code, report, server = self.play()
        self.assertEqual(code, 0)
        checks = self.checks(report)
        for name in ("draw_configuration", "empty_space_swing", "player_hits_dummy",
                     "enemy_hits_player"):
            self.assertEqual(checks[name]["status"], "PASS", checks[name]["detail"])
        for name in ("visual_empty_space_overlay", "visual_player_hit_overlay",
                     "visual_enemy_overlay"):
            self.assertEqual(checks[name]["status"], "WARN", checks[name]["detail"])
            self.assertIn("screenshot review", checks[name]["detail"])
        self.assertEqual([shot["label"] for shot in report["screenshots"]],
                         ["miss", "hit", "enemy"])
        acs = {entry["id"]: entry for entry in report["acs"]}
        self.assertEqual(set(acs), {"AC-1", "AC-4", "AC-5", "AC-8"})
        self.assertTrue(all(entry["status"] == "NOT RUN" for entry in acs.values()), acs)
        self.assertTrue(all(entry["accepted"] is False for entry in acs.values()), acs)
        commands = {request.get("cmd") for request in server.received}
        self.assertLessEqual(commands, {"ping", "state", "key", "click", "screenshot", "quit"})
        self.assertIn("click", commands)
        self.assertIn("key", commands)

    def test_off_run_declares_only_off_mode_acs_and_accepts_none(self):
        code, report, _ = self.play(draw_off=True, applied=0)
        self.assertEqual(code, 0)
        acs = {entry["id"]: entry for entry in report["acs"]}
        self.assertEqual(set(acs), {"AC-2", "AC-8"})
        self.assertEqual(acs["AC-2"]["status"], "PASS")
        self.assertEqual(acs["AC-8"]["status"], "NOT RUN")
        self.assertTrue(all(entry["accepted"] is False for entry in acs.values()), acs)

    def test_off_run_fails_when_applied_cvar_is_not_zero(self):
        code, report, _ = self.play(draw_off=True, applied=1)
        self.assertEqual(code, kit.EXIT_INFRA)
        check = self.checks(report)["draw_configuration"]
        self.assertEqual(check["status"], "FAIL")
        self.assertIn("expected 0", check["detail"])
        ac = {entry["id"]: entry for entry in report["acs"]}["AC-2"]
        self.assertEqual(ac["status"], "FAIL")
        self.assertFalse(ac["accepted"])

    def test_empty_space_check_fails_when_no_attack_activates(self):
        code, report, _ = self.play(attack_tag=False)
        self.assertEqual(code, 1)
        check = self.checks(report)["empty_space_swing"]
        self.assertEqual(check["status"], "FAIL")
        self.assertIn("Mordecai.State.Attacking", check["detail"])


if __name__ == "__main__":
    unittest.main()
