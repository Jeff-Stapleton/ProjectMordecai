# PIE Smoke Test: Death Screen (US-086, issue #7)
# Run during PIE via ue_remote_exec.py to verify the player controller's death screen
# component exists, listens to the player's ASC, and agrees with the player's state:
# no death screen while alive; an activated, visible one while dead.
#
# Read-only: nothing here kills the player or calls gameplay functions. DevTestMap's
# Frontliners may kill the player during the suite, so either state can be observed;
# the check for that state runs. The death screen's texts and its removal on respawn
# are Tier 1 (Mordecai.Recovery.DeathScreen.*) and Tier 4 (us054_combat.py
# death_screen, death_screen_removed).
#
# Usage:
#   py Scripts/run_pie_verify.py --tests death_screen
#   py ue_remote_exec.py "exec(open(r'Scripts/verify_pie_death_screen.py').read())"
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

    def finish():
        print("")
        print(f"RESULT: {pass_count} passed, {fail_count} failed")
        if fail_count > 0:
            raise RuntimeError(f"PIE death screen smoke test failed: {fail_count} failure(s)")
        print("ALL CHECKS PASSED")

    print("=" * 60)
    print("PIE SMOKE TEST: DEATH SCREEN")
    print("=" * 60)

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    check("PIE world exists", world is not None, "Start PIE before running this test")
    if not world:
        finish()
        return

    for cls_name in ('MordecaiDeathScreenComponent', 'MordecaiDeathScreenWidget'):
        check(f"{cls_name} class registered", hasattr(unreal, cls_name),
              "Class not found in unreal module")

    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    check("Player controller exists", pc is not None)
    if not pc:
        finish()
        return

    component = None
    try:
        component = pc.get_editor_property("death_screen_component")
    except Exception as e:
        check("DeathScreenComponent readable", False, str(e))
    check("Controller has a DeathScreenComponent", component is not None,
          f"{pc.get_class().get_name()} has none")
    if not component:
        finish()
        return

    check("DeathScreenComponent listens to the PlayerState ASC (bound in OnPossess)",
          component.is_bound_to_player_state())

    pawn = pc.get_controlled_pawn()
    check("Player pawn exists", pawn is not None)
    if not pawn:
        finish()
        return

    dead = pawn.is_dead()
    screen = component.get_death_screen_widget()
    print(f"  INFO: the player is {'dead' if dead else 'alive'}")
    if dead:
        check("Dead: a death screen is shown", screen is not None)
        if screen:
            check("Dead: the death screen is activated (on its layer stack)", screen.is_activated())
            check("Dead: the death screen is visible", screen.is_visible())
    else:
        check("Alive: no death screen", screen is None,
              f"{screen.get_name() if screen else ''} is still shown")

    finish()

_run()
del _run
