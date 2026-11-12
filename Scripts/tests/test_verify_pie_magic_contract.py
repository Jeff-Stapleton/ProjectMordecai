import os
import unittest

try:
    from pie_verify_contracts import combat_hud_layer_checks, player_dead_from_state
except ImportError:
    combat_hud_layer_checks = None
    player_dead_from_state = None


SCRIPTS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


class VerifyPieMagicContractTests(unittest.TestCase):
    def test_combat_hud_layer_contract_covers_alive_and_dead_paths(self):
        self.assertIsNotNone(
            combat_hud_layer_checks,
            "pie_verify_contracts.combat_hud_layer_checks must provide a testable HUD-layer contract",
        )
        if combat_hud_layer_checks is None:
            return

        alive = combat_hud_layer_checks(
            player_dead=False,
            game_layer_contains_hud=True,
            death_screen_is_active_widget=False,
            hud_is_activated=True,
            hud_is_visible=True,
        )
        self.assertEqual(alive, {"hud_visible": True})
        self.assertEqual(
            combat_hud_layer_checks(False, True, False, True, False),
            {"hud_visible": False},
        )

        dead = combat_hud_layer_checks(
            player_dead=True,
            game_layer_contains_hud=True,
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
            combat_hud_layer_checks(True, True, False, False, False)[
                "death_screen_on_game_layer"
            ],
            False,
        )
        self.assertEqual(
            combat_hud_layer_checks(True, True, True, True, False)[
                "hud_inactive_under_death"
            ],
            False,
        )
        self.assertFalse(
            combat_hud_layer_checks(True, False, True, False, False)[
                "death_screen_on_game_layer"
            ],
            "an active death screen on a non-Game stack must fail",
        )

    def test_player_dead_comes_from_health_or_the_asc_tag(self):
        self.assertIsNotNone(player_dead_from_state)
        if player_dead_from_state is None:
            return
        self.assertTrue(player_dead_from_state(0.0, False))
        self.assertTrue(player_dead_from_state(100.0, True))
        self.assertFalse(player_dead_from_state(100.0, False))
        self.assertFalse(player_dead_from_state(None, False))

    def test_magic_smoke_uses_the_contract_without_a_death_skip(self):
        with open(
            os.path.join(SCRIPTS_DIR, "verify_pie_magic.py"),
            "r",
            encoding="utf-8",
        ) as handle:
            source = handle.read()
        self.assertIn("combat_hud_layer_checks", source)
        self.assertNotIn("SKIP: CombatHUDWidget visible", source)
        self.assertIn("unreal.Paths.convert_relative_path_to_full", source)
        self.assertIn("player_dead_from_state", source)
        self.assertIn("get_widget_list()", source)


if __name__ == "__main__":
    unittest.main()
