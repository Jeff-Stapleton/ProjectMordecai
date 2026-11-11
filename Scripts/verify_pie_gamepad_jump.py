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
            detail = detail() if callable(detail) else detail
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
        game_instance = unreal.GameplayStatics.get_game_instance(world)
        router_candidates = list(unreal.ObjectIterator(router_type)) if router_type else []
        # PIE LocalPlayers are outered to LyraEditorEngine, not the PIE GameInstance. Resolve each
        # candidate's live GameInstance through its LocalPlayer world context before comparing it.
        router = next(
            (candidate for candidate in router_candidates
             if (isinstance(candidate.get_outer(), unreal.LocalPlayer)
                 and unreal.GameplayStatics.get_game_instance(candidate.get_outer())
                 == game_instance)),
            None) if router_type else None
        candidate_owners = [
            f"{candidate.get_path_name()} -> {candidate.get_outer().get_path_name()}, "
            f"game instance="
            f"{unreal.GameplayStatics.get_game_instance(candidate.get_outer()).get_path_name()}"
            for candidate in router_candidates
            if (isinstance(candidate.get_outer(), unreal.LocalPlayer)
                and unreal.GameplayStatics.get_game_instance(candidate.get_outer()))
        ]
        check("Local player uses MordecaiCommonUIActionRouter", router is not None,
              f"PIE game instance={game_instance.get_path_name()}; "
              f"router ownership chains={candidate_owners}")
        if router is not None:
            def router_failure_detail():
                get_mode = getattr(router, "get_active_input_mode", None)
                mode = get_mode() if get_mode else "<GetActiveInputMode is not Python-reflected>"
                return (f"active input mode={mode}; "
                        "Gamepad A would be converted to a CommonUI mouse click")

            check("Gameplay mode passes virtual gamepad Accept through",
                  not router.should_simulate_virtual_accept_for_active_input_mode(),
                  router_failure_detail)

    print("")
    print(f"RESULT: {passed} passed, {failed} failed")
    if failed:
        raise RuntimeError(f"PIE gamepad jump smoke failed: {failed} failure(s)")
    print("ALL CHECKS PASSED")


_run()
del _run
