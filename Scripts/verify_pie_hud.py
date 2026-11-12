# PIE Smoke Test: HUD Visibility
# Run during PIE via ue_remote_exec.py to verify the combat HUD is visible.
# Exit: prints PASS/FAIL lines. Any FAIL means the HUD is broken.
#
# Usage:
#   py ue_remote_exec.py "exec(open(r'Scripts/verify_pie_hud.py').read())"
#
# NOTE: All PIE references are held inside _run() so they go out of scope
# when the function returns, avoiding GC reference leaks after PIE ends.

import unreal

def _run():
    pass_count = 0
    fail_count = 0

    def check(name, condition, detail=""):
        nonlocal pass_count, fail_count
        if condition:
            pass_count += 1
            print(f"  PASS: {name}")
        else:
            fail_count += 1
            msg = f"  FAIL: {name}"
            if detail:
                msg += f" -- {detail}"
            print(msg)

    def named_widget(owner, name):
        if owner is None:
            return None
        try:
            tree = owner.get_editor_property("widget_tree")
            return tree.find_widget(name) if tree else None
        except Exception:
            return None

    def canvas_box(widget):
        if widget is None:
            return None
        try:
            slot = widget.get_editor_property("slot")
            anchors = slot.get_anchors()
            return {
                "anchor_min": (anchors.minimum.x, anchors.minimum.y),
                "anchor_max": (anchors.maximum.x, anchors.maximum.y),
                "position": (slot.get_position().x, slot.get_position().y),
                "size": (slot.get_size().x, slot.get_size().y),
                "alignment": (slot.get_alignment().x, slot.get_alignment().y),
                "auto_size": slot.get_auto_size(),
            }
        except Exception:
            return None

    def check_box(owner, widget_name, anchor, position, size, alignment):
        widget = named_widget(owner, widget_name)
        box = canvas_box(widget)
        check(f"{widget_name} exists", widget is not None)
        check(
            f"{widget_name} uses its final HUD map box",
            box is not None
            and box["anchor_min"] == anchor
            and box["anchor_max"] == anchor
            and box["position"] == position
            and box["size"] == size
            and box["alignment"] == alignment
            and not box["auto_size"],
            f"Got {box}",
        )
        return widget

    print("=" * 60)
    print("PIE SMOKE TEST: HUD")
    print("=" * 60)

    # Get PIE world
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    check("PIE world exists", world is not None, "Start PIE before running this test")

    if not world:
        print(f"\nRESULT: {pass_count} passed, {fail_count} failed")
        if fail_count > 0:
            raise RuntimeError(f"PIE HUD smoke test failed: {fail_count} failure(s)")
        return

    # Get player controller
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    check("Player controller exists", pc is not None)

    if not pc:
        print(f"\nRESULT: {pass_count} passed, {fail_count} failed")
        if fail_count > 0:
            raise RuntimeError(f"PIE HUD smoke test failed: {fail_count} failure(s)")
        return

    # Check controller class
    pc_class_name = pc.get_class().get_name()
    check("Controller is MordecaiPlayerController",
          "Mordecai" in pc_class_name,
          f"Got {pc_class_name}")

    indicator_manager = pc.get_component_by_class(unreal.LyraIndicatorManagerComponent)
    check("Player controller owns an indicator manager", indicator_manager is not None)

    # Check CombatHUDWidget exists (BlueprintReadOnly property)
    hud_widget = None
    try:
        hud_widget = pc.get_editor_property("combat_hud_widget")
    except Exception:
        pass

    check("CombatHUDWidget is not None", hud_widget is not None,
          "PlayerController.CombatHUDWidget is null -- HUD was never created")

    if hud_widget:
        # Check visibility
        is_visible = hud_widget.is_visible()
        check("CombatHUDWidget is visible", is_visible)

        hud_roots = unreal.WidgetLibrary.get_all_widgets_of_class(
            world, unreal.MordecaiCombatHUDWidget, False)
        check("Exactly one CommonUI combat HUD root", len(hud_roots) == 1,
              f"Found {len(hud_roots)}")

        # Check child widget count via BlueprintCallable accessor
        # (child properties are protected, so we use GetBoundChildCount instead)
        try:
            child_count = hud_widget.get_bound_child_count()
            check(f"All HUD child widgets present ({child_count}/18)",
                  child_count == 18,
                  f"{child_count}/18 -- expected all final-map children plus IndicatorLayer")
        except Exception as e:
            check("GetBoundChildCount accessible", False, str(e))

        gauge = check_box(
            hud_widget, "GaugePlate", (0.0, 0.0), (32.0, 32.0),
            (456.0, 152.0), (0.0, 0.0))
        for name, position in (
            ("SpellCooldownA", (32.0, 216.0)),
            ("SpellCooldownB", (120.0, 216.0)),
            ("SpellCooldownC", (208.0, 216.0)),
            ("SpellCooldownD", (296.0, 216.0)),
        ):
            check_box(
                hud_widget, name, (0.0, 0.0), position,
                (78.0, 78.0), (0.0, 0.0))
        check_box(
            hud_widget, "StatusEffectBar", (0.0, 0.0), (32.0, 318.0),
            (456.0, 112.0), (0.0, 0.0))
        rail = check_box(
            hud_widget, "TallyRailHead", (1.0, 0.0), (-32.0, 32.0),
            (320.0, 56.0), (1.0, 0.0))
        check_box(
            hud_widget, "CombatFeedback", (0.5, 0.5), (0.0, 128.0),
            (520.0, 52.0), (0.5, 0.0))
        check_box(
            hud_widget, "ComboCounter", (0.5, 0.5), (0.0, 184.0),
            (220.0, 36.0), (0.5, 0.0))
        prompt_widget = check_box(
            hud_widget, "PickupPrompt", (0.5, 1.0), (90.0, -36.0),
            (720.0, 60.0), (0.5, 1.0))
        check_box(
            hud_widget, "EquippedWeaponWidget", (1.0, 1.0), (-32.0, -32.0),
            (340.0, 84.0), (1.0, 1.0))

        low_frame = named_widget(hud_widget, "LowHealthFrame")
        check("LowHealthFrame exists", low_frame is not None)
        if low_frame:
            check("LowHealthFrame is HitTestInvisible or hidden",
                  low_frame.get_visibility() in (
                      unreal.SlateVisibility.HIT_TEST_INVISIBLE,
                      unreal.SlateVisibility.HIDDEN,
                      unreal.SlateVisibility.COLLAPSED,
                  ))

        for owner, name in (
            (hud_widget, "HealthBar"),
            (hud_widget, "StaminaBar"),
            (hud_widget, "PostureBar"),
            (hud_widget, "SpellPointsBar"),
            (hud_widget, "KillCounter"),
            (prompt_widget, "PromptText"),
            (prompt_widget, "InventoryHintText"),
        ):
            check(f"{name} binding/fallback exists",
                  named_widget(owner, name) is not None,
                  f"Missing under {owner.get_name() if owner else None}")

        for class_name, names in (
            ("MordecaiHealthBarWidget",
             ("HealthProgressBar", "HealthPercentText", "HealthTrailBar", "HealthLeadBar")),
            ("MordecaiStaminaBarWidget",
             ("StaminaProgressBar", "StaminaTierText", "StaminaDebtBar")),
            ("MordecaiSpellPointsBarWidget",
             ("SpellPointsProgressBar", "SpellPointsText", "SpellPointTokens")),
        ):
            widget_class = getattr(unreal, class_name)
            widgets = unreal.WidgetLibrary.get_all_widgets_of_class(world, widget_class, False)
            check(f"One {class_name}", len(widgets) == 1, f"Found {len(widgets)}")
            if len(widgets) == 1:
                for name in names:
                    check(f"{class_name}.{name} binding/fallback exists",
                          named_widget(widgets[0], name) is not None)

        prompt_text = named_widget(prompt_widget, "PromptText")
        inventory_hint = named_widget(prompt_widget, "InventoryHintText")
        check("Inventory hint keeps shipped string",
              inventory_hint is not None
              and str(inventory_hint.get_text()) == "[I] Inventory")
        check("Inventory hint stays visible during play",
              inventory_hint is not None and inventory_hint.is_visible())

        weapon_widget = None
        try:
            weapon_widget = hud_widget.get_equipped_weapon_widget()
        except Exception as e:
            check("Equipped weapon child readable", False, str(e))
        check("Combat HUD owns the equipped weapon widget", weapon_widget is not None)
        weapon_widgets = unreal.WidgetLibrary.get_all_widgets_of_class(
            world, unreal.MordecaiEquippedWeaponWidget, False)
        check("Exactly one equipped weapon widget", len(weapon_widgets) == 1,
              f"Found {len(weapon_widgets)} -- a standalone viewport duplicate may still exist")
        if weapon_widget:
            check("The one weapon widget is the combat HUD child",
                  len(weapon_widgets) == 1 and weapon_widgets[0] == weapon_widget)
            shown = str(weapon_widget.get_displayed_weapon_name())
            check("Weapon HUD reads Longsword", shown == "Longsword", f"Got {shown!r}")

    # Kill counter (US-086, issue #6): bound to the game mode, whose count it shows (the number the
    # agent bridge reports as player.kills). Read-only: nothing here kills anything (Tier 1 and the
    # Tier 4 us054_combat run kill through the real damage path).
    game_mode = unreal.GameplayStatics.get_game_mode(world)
    is_mordecai = isinstance(game_mode, unreal.MordecaiGameMode)
    check("Game mode is MordecaiGameMode", is_mordecai,
          f"Got {game_mode.get_class().get_name() if game_mode else None}")
    counters = unreal.WidgetLibrary.get_all_widgets_of_class(
        world, unreal.MordecaiKillCounterWidget, False)
    check("One HUD kill counter", len(counters) == 1, f"Found {len(counters)}")
    if is_mordecai and len(counters) == 1:
        bound = counters[0].get_bound_game_mode()
        check("Kill counter is bound to the game mode", bound == game_mode,
              f"bound to {bound.get_name() if bound else None}: it would never change")
        shown = counters[0].get_kill_count()
        count = game_mode.get_enemy_kill_count()
        check(f"Kill counter shows the game mode's count ({shown} == {count})", shown == count,
              "the HUD counter and player.kills disagree")

    # US-092: DevTestMap starts with all three Frontliners inside aggro range. This script runs after
    # PIE readiness, so registration must already have reached the manager-backed indicator path.
    if "DevTestMap" in world.get_path_name():
        enemies = unreal.GameplayStatics.get_all_actors_of_class(
            world, unreal.MordecaiEnemyCharacter)
        indicators = [enemy.get_component_by_class(unreal.MordecaiEnemyIndicatorComponent)
                      for enemy in enemies]
        check("DevTestMap has three enemy indicator components", len(indicators) == 3,
              f"Found {len(indicators)} on {len(enemies)} enemies")
        registered = sum(1 for indicator in indicators
                         if indicator and indicator.is_indicator_registered())
        check("All three living enemies registered always-on indicators from spawn", registered == 3,
              f"{registered}/3 registered")

    print("")
    print(f"RESULT: {pass_count} passed, {fail_count} failed")

    if fail_count > 0:
        raise RuntimeError(f"PIE HUD smoke test failed: {fail_count} failure(s)")
    else:
        print("ALL CHECKS PASSED")

_run()
del _run
