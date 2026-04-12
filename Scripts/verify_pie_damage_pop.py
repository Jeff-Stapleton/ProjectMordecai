# PIE Smoke Test: Floating Damage Numbers (US-065)
# Run during PIE via ue_remote_exec.py to verify the damage pop component
# is attached to the player controller and has proper defaults.
#
# Usage:
#   py ue_remote_exec.py --file Scripts/verify_pie_damage_pop.py
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
    print("PIE SMOKE TEST: FLOATING DAMAGE NUMBERS (US-065)")
    print("=" * 60)

    # --- PIE World ---
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    check("PIE world exists", world is not None, "Start PIE before running this test")

    if not world:
        print(f"\nRESULT: {pass_count} passed, {fail_count} failed")
        if fail_count > 0:
            raise RuntimeError(f"PIE damage pop smoke test failed: {fail_count} failure(s)")
        return

    # --- Player Controller ---
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    check("Player controller exists", pc is not None)

    if not pc:
        print(f"\nRESULT: {pass_count} passed, {fail_count} failed")
        if fail_count > 0:
            raise RuntimeError(f"PIE damage pop smoke test failed: {fail_count} failure(s)")
        return

    # --- DamagePopComponent on Controller ---
    all_comps = pc.get_components_by_class(unreal.ActorComponent)
    comp_names = [c.get_class().get_name() for c in all_comps]

    has_damage_pop = any("DamagePop" in n for n in comp_names)
    check("DamagePopComponent attached to PlayerController", has_damage_pop,
          f"Components: {', '.join(comp_names)}")

    # Find the actual component for property checks
    pop_comp = None
    for c in all_comps:
        if "DamagePop" in c.get_class().get_name():
            pop_comp = c
            break

    if pop_comp:
        # Check configurable properties exist and have expected defaults
        try:
            lifespan = pop_comp.get_editor_property("pop_lifespan_sec")
            check("PopLifespanSec default is 1.5", abs(lifespan - 1.5) < 0.01,
                  f"Got {lifespan}")
        except Exception as e:
            check("PopLifespanSec accessible", False, str(e))

        try:
            rise_speed = pop_comp.get_editor_property("pop_rise_speed")
            check("PopRiseSpeed default is 100.0", abs(rise_speed - 100.0) < 0.01,
                  f"Got {rise_speed}")
        except Exception as e:
            check("PopRiseSpeed accessible", False, str(e))

        try:
            max_pops = pop_comp.get_editor_property("max_concurrent_pops")
            check("MaxConcurrentPops default is 20", max_pops == 20,
                  f"Got {max_pops}")
        except Exception as e:
            check("MaxConcurrentPops accessible", False, str(e))

        try:
            scatter = pop_comp.get_editor_property("pop_scatter_radius")
            check("PopScatterRadius default is 30.0", abs(scatter - 30.0) < 0.01,
                  f"Got {scatter}")
        except Exception as e:
            check("PopScatterRadius accessible", False, str(e))

        try:
            colors = pop_comp.get_editor_property("damage_type_colors")
            check("DamageTypeColors map populated", len(colors) > 0,
                  f"Map has {len(colors)} entries")
        except Exception as e:
            check("DamageTypeColors accessible", False, str(e))

    print("")
    print(f"RESULT: {pass_count} passed, {fail_count} failed")

    if fail_count > 0:
        raise RuntimeError(f"PIE damage pop smoke test failed: {fail_count} failure(s)")
    else:
        print("ALL CHECKS PASSED")

_run()
del _run
