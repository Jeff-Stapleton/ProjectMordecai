"""Pure contracts shared by PIE smoke scripts and their offline tests."""


def combat_hud_layer_checks(
    player_dead,
    death_screen_is_active_widget,
    hud_is_activated,
    hud_is_visible,
):
    """Return the hard HUD checks required for the current UI.Layer.Game state."""
    if player_dead:
        return {
            "death_screen_on_game_layer": bool(death_screen_is_active_widget),
            "hud_inactive_under_death": not bool(hud_is_activated),
        }
    return {"hud_visible": bool(hud_is_visible)}
