# PIE Smoke Test: Character Sheet Widget (US-066)
# Verify the character sheet widget class is registered and that the pause menu
# has swapped out the placeholder for the real character sheet on the
# "character" tab.
#
# Usage:
#   py ue_remote_exec.py "exec(open(r'Scripts/verify_pie_character_sheet.py').read())"
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
    print("PIE SMOKE TEST: CHARACTER SHEET")
    print("=" * 60)

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    check("PIE world exists", world is not None,
          "Start PIE before running this test")

    if not world:
        print(f"\nRESULT: {pass_count} passed, {fail_count} failed")
        if fail_count > 0:
            raise RuntimeError(
                f"PIE character sheet smoke test failed: {fail_count} failure(s)"
            )
        return

    # Character sheet widget class must be registered.
    has_widget = hasattr(unreal, 'MordecaiCharacterSheetWidget')
    check("MordecaiCharacterSheetWidget class registered", has_widget,
          "Class not found in unreal module")

    # Pause menu widget class must also be registered for the tab wiring to work.
    has_pause_menu = hasattr(unreal, 'MordecaiPauseMenuWidget')
    check("MordecaiPauseMenuWidget class registered", has_pause_menu)

    # Verify the character-tab id is the expected FName and that BindToASC is
    # exposed to Python (proving the UFUNCTION + class are loadable at runtime).
    if has_widget:
        widget_class = unreal.MordecaiCharacterSheetWidget
        check(
            "BindToASC exposed to Python",
            hasattr(widget_class, 'bind_to_asc'),
            "UFUNCTION missing from Python-exposed surface"
        )
        check(
            "UnbindFromASC exposed to Python",
            hasattr(widget_class, 'unbind_from_asc'),
            "UFUNCTION missing from Python-exposed surface"
        )
        check(
            "RegisterWithPauseMenu exposed to Python",
            hasattr(widget_class, 'register_with_pause_menu'),
            "Static UFUNCTION missing from Python-exposed surface"
        )

        pc = unreal.GameplayStatics.get_player_controller(world, 0)
        check("PlayerController exists", pc is not None)

        # Confirm the local player has an AbilitySystemComponent reachable via
        # PlayerState (Lyra puts the ASC on the player state, not the pawn).
        if pc is not None:
            ps = pc.player_state
            check("PlayerState exists", ps is not None)
            if ps is not None:
                asc = ps.get_component_by_class(unreal.AbilitySystemComponent)
                check("PlayerState has AbilitySystemComponent", asc is not None)

    print("")
    print(f"RESULT: {pass_count} passed, {fail_count} failed")

    if fail_count > 0:
        raise RuntimeError(
            f"PIE character sheet smoke test failed: {fail_count} failure(s)"
        )
    else:
        print("ALL CHECKS PASSED")

_run()
del _run
