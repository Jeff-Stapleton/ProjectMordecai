# PIE Smoke Test: Pause Menu Subsystem
# Run during PIE via ue_remote_exec.py to verify pause menu subsystem exists
# and responds to open/close.
#
# US-095 (issues #23, #32): the open menu has registered its CommonUI UI action
# bindings (LB previous tab, RB next tab, I press + I key-up close) with the live
# CommonUI action router, with no duplicates. Tier 1 has no action router, so this is the first tier that
# sees the real registration. Opening the menu and switching tabs here are direct
# calls ([LOGIC]); real LB / RB / I presses are accepted only by the Tier 4 run
# of Scripts/playtests/us069_pause_menu.py.
#
# Usage:
#   py ue_remote_exec.py "exec(open(r'Scripts/verify_pie_pause_menu.py').read())"
#
# NOTE: All PIE references are held inside _run() so they go out of scope
# when the function returns, avoiding GC reference leaks after PIE ends.

import unreal

# MordecaiGameplayTags UIAction_PauseMenu_* (US-095), keyed in Config/DefaultInput.ini -> how many
# bindings the menu registers for each (I: one on the press that arms, one on the key-up that closes)
EXPECTED_UI_ACTIONS = {"UI.Action.Mordecai.PauseMenu.PreviousTab": 1,
                       "UI.Action.Mordecai.PauseMenu.NextTab": 1,
                       "UI.Action.Mordecai.PauseMenu.Inventory": 2}
TAB_ORDER = ("character", "skills", "feats", "inventory", "settings")

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

    def logic(name, condition, detail=""):
        # Direct gameplay/UI call: logic/diagnostic only, never acceptance of a player-facing AC
        check(f"[LOGIC] {name}", condition, detail)

    print("=" * 60)
    print("PIE SMOKE TEST: PAUSE MENU")
    print("=" * 60)

    # Get PIE world
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    check("PIE world exists", world is not None, "Start PIE before running this test")

    if not world:
        print(f"\nRESULT: {pass_count} passed, {fail_count} failed")
        if fail_count > 0:
            raise RuntimeError(f"PIE pause menu smoke test failed: {fail_count} failure(s)")
        return

    # Check that the subsystem class is registered in the engine
    has_class = hasattr(unreal, 'MordecaiPauseMenuSubsystem')
    check("MordecaiPauseMenuSubsystem class registered", has_class,
          "Class not found in unreal module")

    # Check that the widget class is registered
    has_widget = hasattr(unreal, 'MordecaiPauseMenuWidget')
    check("MordecaiPauseMenuWidget class registered", has_widget,
          "Class not found in unreal module")

    # Check placeholder widget class
    has_placeholder = hasattr(unreal, 'MordecaiPauseMenuPlaceholderWidget')
    check("MordecaiPauseMenuPlaceholderWidget class registered", has_placeholder,
          "Class not found in unreal module")

    # Get game instance and try to access subsystem
    gi = unreal.GameplayStatics.get_game_instance(world)
    check("Game instance exists", gi is not None)

    if gi and has_class:
        # GameInstance.get_subsystem isn't exposed to Python; find the PIE game instance's subsystem directly
        sub = next((s for s in unreal.ObjectIterator(unreal.MordecaiPauseMenuSubsystem)
                    if s.get_outer() == gi), None)
        check("PauseMenuSubsystem instance exists", sub is not None,
              "No MordecaiPauseMenuSubsystem on the PIE game instance")

        if sub is not None:

            # Check initial state: menu should be closed
            try:
                is_open = sub.is_menu_open()
                check("Menu initially closed", not is_open,
                      "Menu was open before any input")
            except Exception as e:
                check("is_menu_open callable", False, str(e))

            # Check bCanPauseGame defaults to true
            try:
                # Python drops the 'b' prefix: bCanPauseGame -> can_pause_game
                can_pause = sub.get_editor_property("can_pause_game")
                check("bCanPauseGame defaults to true", can_pause == True,
                      f"Got {can_pause}")
            except Exception as e:
                check("bCanPauseGame property accessible", False, str(e))

            # --- US-095: the open menu's CommonUI UI action bindings ---
            sub.open_pause_menu()
            logic("open_pause_menu opens the menu", sub.is_menu_open())
            menu = sub.get_active_menu_widget()
            logic("the menu widget was pushed", menu is not None,
                  "PrimaryGameLayout missing? Menu widget was not pushed")
            if menu is not None:
                names = [str(n) for n in menu.get_ui_action_binding_names()]
                for tag, count in EXPECTED_UI_ACTIONS.items():
                    check(f"UI action registered with the CommonUI router {count}x (no duplicates): {tag}",
                          names.count(tag) == count, f"Got {names}")

                start = str(menu.get_active_tab_id()).lower()
                if start in TAB_ORDER:
                    i = TAB_ORDER.index(start)
                    menu.select_next_tab()
                    got = str(menu.get_active_tab_id()).lower()
                    want = TAB_ORDER[(i + 1) % len(TAB_ORDER)]
                    logic(f"select_next_tab: {start} -> {want}", got == want, f"Got '{got}'")
                    menu.select_previous_tab()
                    got = str(menu.get_active_tab_id()).lower()
                    logic(f"select_previous_tab: back to {start}", got == start, f"Got '{got}'")
                else:
                    check("the menu opened on a known tab", False, f"Got '{start}'")

            sub.close_pause_menu()
            logic("close_pause_menu closes the menu", not sub.is_menu_open())

    print("")
    print(f"RESULT: {pass_count} passed, {fail_count} failed")

    if fail_count > 0:
        raise RuntimeError(f"PIE pause menu smoke test failed: {fail_count} failure(s)")
    else:
        print("ALL CHECKS PASSED")

_run()
del _run
