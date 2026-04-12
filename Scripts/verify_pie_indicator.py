# PIE Smoke Test: Enemy Indicator System (US-064)
# Run during PIE via ue_remote_exec.py to verify enemy indicator component exists
# on enemy characters and is properly configured.
#
# Usage:
#   py ue_remote_exec.py --file Scripts/verify_pie_indicator.py
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
    print("PIE SMOKE TEST: ENEMY INDICATOR SYSTEM")
    print("=" * 60)

    # --- PIE World ---
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    check("PIE world exists", world is not None, "Start PIE before running this test")

    if not world:
        print(f"\nRESULT: {pass_count} passed, {fail_count} failed")
        if fail_count > 0:
            raise RuntimeError(f"PIE indicator smoke test failed: {fail_count} failure(s)")
        return

    # --- Find enemy character actors (filter for Characters only, not controllers) ---
    all_actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Character)
    enemy_actors = [a for a in all_actors if "EnemyCharacter" in a.get_class().get_name()]

    # If no strict match, try broader filter on Characters with "Enemy" in name
    if not enemy_actors:
        enemy_actors = [a for a in all_actors if "Enemy" in a.get_class().get_name()]

    check("Enemy character actors found", len(enemy_actors) > 0, f"Found {len(enemy_actors)} enemies")

    if not enemy_actors:
        print(f"\nRESULT: {pass_count} passed, {fail_count} failed")
        if fail_count > 0:
            raise RuntimeError(f"PIE indicator smoke test failed: {fail_count} failure(s)")
        return

    # --- Check first enemy for indicator component ---
    enemy = enemy_actors[0]
    comps = enemy.get_components_by_class(unreal.ActorComponent)
    comp_names = [c.get_class().get_name() for c in comps]

    has_indicator = any("EnemyIndicatorComponent" in n or "IndicatorComponent" in n for n in comp_names)
    check("Enemy has indicator component", has_indicator,
          f"Components: {', '.join(comp_names)}")

    # --- Check indicator component properties ---
    indicator_comps = [c for c in comps if "IndicatorComponent" in c.get_class().get_name()]
    if indicator_comps:
        ic = indicator_comps[0]
        try:
            height = ic.get_editor_property("indicator_height_offset")
            check("IndicatorHeightOffset is set", height is not None and height > 0,
                  f"Got {height}")
        except Exception as e:
            check("IndicatorHeightOffset accessible", False, str(e))

        try:
            vis_range = ic.get_editor_property("indicator_visibility_range")
            check("IndicatorVisibilityRange is set", vis_range is not None and vis_range > 0,
                  f"Got {vis_range}")
        except Exception as e:
            check("IndicatorVisibilityRange accessible", False, str(e))

    # --- Check enemy ASC exists (needed for indicator binding) ---
    has_asc = any("AbilitySystemComponent" in n for n in comp_names)
    check("Enemy has ASC for indicator binding", has_asc)

    print("")
    print(f"RESULT: {pass_count} passed, {fail_count} failed")

    if fail_count > 0:
        raise RuntimeError(f"PIE indicator smoke test failed: {fail_count} failure(s)")
    else:
        print("ALL CHECKS PASSED")

_run()
del _run
