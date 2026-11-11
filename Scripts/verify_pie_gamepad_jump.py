# PIE smoke: US-119 CommonUI routing for Gamepad A.
#
# This is read-only integration coverage. It does not press A or call Jump; packaged Tier 4
# `us083_combat_input.py` is the only acceptance proof for the player input path.

import unreal


def _run():
    passed = 0
    failed = 0

    def check(name, condition, detail=""):
        nonlocal passed, failed
        if condition:
            passed += 1
            print(f"  PASS: {name}")
        else:
            failed += 1
            print(f"  FAIL: {name}" + (f" -- {detail}" if detail else ""))

    print("=" * 60)
    print("PIE SMOKE TEST: GAMEPAD A JUMP ROUTING (US-119)")
    print("=" * 60)

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    check("PIE world exists", world is not None, "Start PIE before running this test")
    if world:
        controller = unreal.GameplayStatics.get_player_controller(world, 0)
        check("Player controller exists", controller is not None)

        router_type = getattr(unreal, "MordecaiCommonUIActionRouter", None)
        check("Mordecai CommonUI action-router class is registered", router_type is not None,
              "the base CommonUI router still owns the analog cursor")
        router = next(
            (candidate for candidate in unreal.ObjectIterator(router_type)
             if isinstance(candidate.get_outer(), unreal.LocalPlayer)),
            None) if router_type else None
        check("Local player uses MordecaiCommonUIActionRouter", router is not None)
        if router is not None:
            check("Gameplay mode passes virtual gamepad Accept through",
                  not router.should_simulate_virtual_accept_for_active_input_mode(),
                  "Gamepad A would be converted to a CommonUI mouse click")

    print("")
    print(f"RESULT: {passed} passed, {failed} failed")
    if failed:
        raise RuntimeError(f"PIE gamepad jump smoke failed: {failed} failure(s)")
    print("ALL CHECKS PASSED")


_run()
del _run
