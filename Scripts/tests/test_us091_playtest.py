"""Declaration tests for US-091's dedicated Tier 4 owner runs.

The owner runs reuse the already-fake-tested US-054 death flow and US-078 weapon helpers while
excluding unrelated acceptance criteria and US-078's known Shift+Tab failure. Nothing here
launches the game.
Run: py Scripts/run_python_tests.py -k us091_playtest
"""

import importlib.util
import json
import os
import sys
import unittest

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PLAYTESTS = os.path.join(SCRIPTS, "playtests")
if SCRIPTS not in sys.path:
    sys.path.insert(0, SCRIPTS)

import playtest_kit as kit  # noqa: E402
from tests import test_us054_combat_playtest as t54  # noqa: E402
from tests import test_us078_weapons_playtest as t78  # noqa: E402


def load(name):
    spec = importlib.util.spec_from_file_location(name, os.path.join(PLAYTESTS, name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


death = load("us091_death_layering")
weapons = load("us091_weapon_hud")


class TestUs091DeathLayeringPlan(unittest.TestCase):
    def test_declares_a_dedicated_owner_run(self):
        run = death.Us091DeathLayeringRun
        self.assertIs(death.Run, run)
        self.assertEqual(run.STORY, "US-091")
        self.assertEqual(run.COVERS, ())
        self.assertEqual(run.RUN_SUFFIX, "us091death")
        self.assertEqual(run.launch_map(), kit.COMBAT_TEST_MAP)
        self.assertEqual(kit.plan_problems(run), [])

    def test_only_the_distinct_layering_steps_prove_us091(self):
        proves = {step.check: step.proves for step in death.Us091DeathLayeringRun.PLAN
                  if step.proves}
        self.assertEqual(proves, {
            "weapon_hud_hidden_under_death": ("AC-091.C",),
            "weapon_hud_restored_after_respawn": ("AC-091.C",),
        })
        self.assertFalse(any(step.known_issue for step in death.Us091DeathLayeringRun.PLAN))


class TestUs091WeaponHudPlan(unittest.TestCase):
    def test_declares_a_dedicated_owner_run(self):
        run = weapons.Us091WeaponHudRun
        self.assertIs(weapons.Run, run)
        self.assertEqual(run.STORY, "US-091")
        self.assertEqual(run.COVERS, ())
        self.assertEqual(run.RUN_SUFFIX, "us091weapons")
        self.assertEqual(run.launch_map(), kit.COMBAT_TEST_MAP)
        self.assertEqual(kit.plan_problems(run), [])

    def test_only_text_and_four_real_tabs_prove_us091(self):
        self.assertEqual([step.check for step in weapons.Us091WeaponHudRun.PLAN], [
            "starting_weapon", "weapon_hud_text", "cycle_next_1", "cycle_next_2",
            "cycle_next_3", "weapon_hud_updates",
        ])
        proves = {step.check: step.proves for step in weapons.Us091WeaponHudRun.PLAN
                  if step.proves}
        self.assertEqual(proves, {
            "starting_weapon": ("AC-091.D",),
            "weapon_hud_text": ("AC-091.D",),
            "weapon_hud_updates": ("AC-091.D",),
        })
        self.assertFalse(any(step.known_issue for step in weapons.Us091WeaponHudRun.PLAN))


class Us091DeathCase(t54.Us054CombatCase):
    def play(self, server):
        code = death.main(["--attach-port", str(server.port), "--token", t54.TOKEN,
                           "--run-dir", self.run_dir])
        with open(os.path.join(self.run_dir, "report.json"), encoding="utf-8") as f:
            return code, json.load(f)


class TestUs091DeathLayeringAgainstFake(Us091DeathCase):
    def test_passes_the_dedicated_layering_checks(self):
        _, server = self.serve(**t54.game_today())
        code, rep = self.play(server)
        self.assertEqual(code, 0, self.log.getvalue()[-3000:])
        checks = self.by_name(rep)
        self.assertEqual(checks["weapon_hud_hidden_under_death"]["status"], "PASS")
        self.assertEqual(checks["weapon_hud_restored_after_respawn"]["status"], "PASS")
        ac = self.acs(rep)[("US-091", "AC-091.C")]
        self.assertEqual(ac["status"], "PASS")
        self.assertFalse(ac["accepted"], "attached fake runs are diagnostic")


class Us091WeaponCase(t78.Us078Case):
    def play(self, server):
        code = weapons.main(["--attach-port", str(server.port), "--token", t78.TOKEN,
                             "--run-dir", self.run_dir])
        with open(os.path.join(self.run_dir, "report.json"), encoding="utf-8") as f:
            return code, json.load(f)


class TestUs091WeaponHudAgainstFake(Us091WeaponCase):
    def test_passes_without_exercising_the_unrelated_shift_tab_bug(self):
        _, server = self.serve(shift_tab_prev=False)
        code, rep = self.play(server)
        self.assertEqual(code, 0, self.log.getvalue()[-3000:])
        checks = self.by_name(rep)
        self.assertNotIn("cycle_prev_shift_tab", checks)
        self.assertEqual(checks["weapon_hud_text"]["status"], "PASS")
        self.assertEqual(checks["weapon_hud_updates"]["status"], "PASS")
        self.assertIn("Mace", checks["weapon_hud_updates"]["detail"])
        ac = {(a["story"], a["id"]): a for a in rep["acs"]}[("US-091", "AC-091.D")]
        self.assertEqual(ac["status"], "PASS")
        self.assertFalse(ac["accepted"], "attached fake runs are diagnostic")


if __name__ == "__main__":
    unittest.main()
