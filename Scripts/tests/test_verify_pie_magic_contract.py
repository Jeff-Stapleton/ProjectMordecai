import os
import unittest

try:
    from verify_pie_contracts import combat_hud_layer_checks
except ImportError:
    combat_hud_layer_checks = None


SCRIPTS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


class VerifyPieMagicContractTests(unittest.TestCase):
    def test_combat_hud_layer_contract_covers_alive_and_dead_paths(self):
        self.assertIsNotNone(
            combat_hud_layer_checks,
            "verify_pie_contracts.combat_hud_layer_checks must provide a testable HUD-layer contract",
        )
        if combat_hud_layer_checks is None:
            return

        alive = combat_hud_layer_checks(
            player_dead=False,
            death_screen_is_active_widget=False,
            hud_is_activated=True,
            hud_is_visible=True,
        )
        self.assertEqual(alive, {"hud_visible": True})
        self.assertEqual(
            combat_hud_layer_checks(False, False, True, False),
            {"hud_visible": False},
        )

        dead = combat_hud_layer_checks(
            player_dead=True,
            death_screen_is_active_widget=True,
            hud_is_activated=False,
            hud_is_visible=False,
        )
        self.assertEqual(
            dead,
            {
                "death_screen_on_game_layer": True,
                "hud_inactive_under_death": True,
            },
        )
        self.assertEqual(
            combat_hud_layer_checks(True, False, False, False)[
                "death_screen_on_game_layer"
            ],
            False,
        )
        self.assertEqual(
            combat_hud_layer_checks(True, True, True, False)[
                "hud_inactive_under_death"
            ],
            False,
        )

    def test_magic_smoke_uses_the_contract_without_a_death_skip(self):
        with open(
            os.path.join(SCRIPTS_DIR, "verify_pie_magic.py"),
            "r",
            encoding="utf-8",
        ) as handle:
            source = handle.read()
        self.assertIn("combat_hud_layer_checks", source)
        self.assertNotIn("SKIP: CombatHUDWidget visible", source)


if __name__ == "__main__":
    unittest.main()
