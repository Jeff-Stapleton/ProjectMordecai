# PIE Smoke Test: Inventory Widget (US-071)
# Run during PIE via ue_remote_exec.py to verify the inventory widget class is
# registered, can be created, registers into the pause menu tab slot, and
# behaves gracefully while unbound.
#
# Usage:
#   py ue_remote_exec.py "exec(open(r'Scripts/verify_pie_inventory.py').read())"
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

    print("=" * 60)
    print("PIE SMOKE TEST: INVENTORY WIDGET (US-071)")
    print("=" * 60)

    # Get PIE world
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    check("PIE world exists", world is not None, "Start PIE before running this test")

    if not world:
        print(f"\nRESULT: {pass_count} passed, {fail_count} failed")
        raise RuntimeError(f"PIE inventory smoke test failed: {fail_count} failure(s)")

    # Class registration
    has_widget = hasattr(unreal, 'MordecaiInventoryWidget')
    check("MordecaiInventoryWidget class registered", has_widget,
          "Class not found in unreal module")
    has_pause_menu = hasattr(unreal, 'MordecaiPauseMenuWidget')
    check("MordecaiPauseMenuWidget class registered", has_pause_menu,
          "Class not found in unreal module")

    if not (has_widget and has_pause_menu):
        print(f"\nRESULT: {pass_count} passed, {fail_count} failed")
        raise RuntimeError(f"PIE inventory smoke test failed: {fail_count} failure(s)")

    # --- Standalone widget creation + unbound behavior ---
    widget = None
    try:
        widget = unreal.WidgetBlueprintLibrary.create(world, unreal.MordecaiInventoryWidget)
    except Exception as e:
        check("Inventory widget created", False, str(e))

    if widget:
        check("Inventory widget created", True)

        # Unbound state: "--" placeholders, zero rows (AC-071.2 / AC-071.11)
        try:
            list_ph = str(widget.get_list_placeholder_text())
            check("Unbound list placeholder is '--'", list_ph == "--", f"Got '{list_ph}'")
            ledger_ph = str(widget.get_ledger_placeholder_text())
            check("Unbound ledger placeholder is '--'", ledger_ph == "--", f"Got '{ledger_ph}'")
            check("Zero visible rows while unbound", widget.get_visible_row_count() == 0,
                  f"Got {widget.get_visible_row_count()}")
        except Exception as e:
            check("Unbound state queries", False, str(e))

        # Filter changes must not crash while unbound
        try:
            widget.set_filter(unreal.MordecaiInventoryFilter.WEAPONS)
            widget.set_filter(unreal.MordecaiInventoryFilter.ALL)
            check("Filter switching does not crash", True)
        except Exception as e:
            check("Filter switching does not crash", False, str(e))

    # --- Pause menu tab slot integration (AC-071.1) ---
    pause_menu = None
    try:
        pause_menu = unreal.WidgetBlueprintLibrary.create(world, unreal.MordecaiPauseMenuWidget)
    except Exception as e:
        check("Pause menu widget created", False, str(e))

    if pause_menu:
        check("Pause menu widget created", True)
        try:
            tabs_before = pause_menu.get_tab_count()
            unreal.MordecaiInventoryWidget.register_with_pause_menu(pause_menu)
            tabs_after = pause_menu.get_tab_count()
            check("RegisterWithPauseMenu succeeded",
                  tabs_after >= tabs_before,
                  f"Tab count before={tabs_before}, after={tabs_after}")

            # Activating the tab instantiates the inventory widget in the slot
            pause_menu.set_active_tab("inventory")
            active = str(pause_menu.get_active_tab_id())
            check("'inventory' tab is activatable", active == "inventory",
                  f"Active tab is '{active}'")
        except Exception as e:
            check("Pause menu tab registration", False, str(e))

    print("")
    print(f"RESULT: {pass_count} passed, {fail_count} failed")

    if fail_count > 0:
        raise RuntimeError(f"PIE inventory smoke test failed: {fail_count} failure(s)")
    else:
        print("ALL CHECKS PASSED")

_run()
del _run
