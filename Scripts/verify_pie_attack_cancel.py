# PIE smoke test: US-125 attack-cancel and dodge integration.
# Runtime input is proven only by the packaged Tier 4 scripts; these checks inspect live PIE
# integration and intentionally do not call gameplay functions.

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
    print("PIE SMOKE TEST: ATTACK CANCEL AND DODGE (US-125)")
    print("=" * 60)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    check("PIE world exists", world is not None)
    if not world:
        raise RuntimeError("PIE attack-cancel smoke test failed: no PIE world")

    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    check("player controller exists", pc is not None)
    check("player pawn exists", pawn is not None)
    if pc and pawn:
        capsule = pawn.get_component_by_class(unreal.CapsuleComponent)
        movement = pawn.get_component_by_class(unreal.CharacterMovementComponent)
        check("player capsule exists", capsule is not None)
        check("capsule radius remains 40",
              capsule is not None and abs(capsule.get_unscaled_capsule_radius() - 40.0) < 0.01)
        check("capsule half-height remains 90",
              capsule is not None and abs(capsule.get_unscaled_capsule_half_height() - 90.0) < 0.01)
        check("maximum step height remains 45",
              movement is not None and abs(movement.get_editor_property("max_step_height") - 45.0) < 0.01)

        granted = []
        try:
            asc = pc.player_state.get_lyra_ability_system_component()
            for handle in asc.get_all_abilities():
                ability, _ = unreal.AbilitySystemLibrary.get_gameplay_ability_from_spec_handle(asc, handle)
                if ability:
                    granted.append(ability.get_class().get_name())
        except Exception as exc:
            check("ability specs readable", False, str(exc))
        for name in ("MordecaiGA_MeleeAttack", "MordecaiGA_Dodge",
                     "MordecaiGA_Block", "MordecaiGA_Parry"):
            check(f"ASC holds {name}", name in granted, f"granted: {granted}")

    print("")
    print(f"RESULT: {passed} passed, {failed} failed")
    if failed:
        raise RuntimeError(f"PIE attack-cancel smoke test failed: {failed} failure(s)")
    print("ALL CHECKS PASSED")


_run()
del _run
