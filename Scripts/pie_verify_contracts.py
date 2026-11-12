"""Pure contracts shared by PIE smoke scripts and their offline tests."""


def player_dead_from_state(health, asc_has_dead_tag):
    """Death comes from gameplay state, never from whether its UI happened to appear."""
    return bool((health is not None and health <= 0.0) or asc_has_dead_tag)


def combat_hud_layer_checks(
    player_dead,
    game_layer_is_ui_layer_game,
    death_screen_is_active_widget,
    hud_is_activated,
    hud_is_visible,
):
    """Return the hard HUD checks required for the current UI.Layer.Game state."""
    if player_dead:
        return {
            "death_screen_on_game_layer": bool(
                game_layer_is_ui_layer_game and death_screen_is_active_widget
            ),
            "hud_inactive_under_death": not bool(hud_is_activated),
        }
    return {"hud_visible": bool(hud_is_visible)}
