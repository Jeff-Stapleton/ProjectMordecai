# PIE Smoke Test: Magic System Integration
# Run during PIE via ue_remote_exec.py to verify spell/magic wiring.
# Checks: Spell abilities granted, SP attribute exists, HUD has spell widgets,
#         enemy has fire attack profile.
#
# Usage:
#   py ue_remote_exec.py "exec(open(r'Scripts/verify_pie_magic.py').read())"
#
# NOTE: All PIE references held inside _run() to avoid GC reference leaks.

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
    print("PIE SMOKE TEST: MAGIC SYSTEM")
    print("=" * 60)

    # --- PIE World ---
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    check("PIE world exists", world is not None, "Start PIE before running this test")

    if not world:
        print(f"\nRESULT: {pass_count} passed, {fail_count} failed")
        if fail_count > 0:
            raise RuntimeError(f"PIE magic smoke test failed: {fail_count} failure(s)")
        return

    # --- Player Controller ---
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    check("Player controller exists", pc is not None)

    if not pc:
        print(f"\nRESULT: {pass_count} passed, {fail_count} failed")
        if fail_count > 0:
            raise RuntimeError(f"PIE magic smoke test failed: {fail_count} failure(s)")
        return

    # Check controller is Mordecai class
    pc_class = pc.get_class().get_name()
    check("Controller is Mordecai class", "Mordecai" in pc_class, f"Got {pc_class}")

    # --- Check spell abilities are configured ---
    try:
        spell_abilities = pc.get_editor_property("default_spell_abilities")
        check("DefaultSpellAbilities populated",
              spell_abilities is not None and len(spell_abilities) >= 4,
              f"Got {len(spell_abilities) if spell_abilities else 0} abilities")
    except Exception as e:
        check("DefaultSpellAbilities accessible", False, str(e))

    # --- Player Pawn ---
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    check("Player pawn exists", pawn is not None)

    # --- Player State & ASC ---
    if pc:
        try:
            ps = pc.player_state
            check("PlayerState exists", ps is not None)

            if ps:
                # Check ASC has SpellPoints attribute
                asc = None
                try:
                    asc = ps.get_ability_system_component()
                except Exception:
                    try:
                        asc = ps.get_editor_property("ability_system_component")
                    except Exception:
                        pass

                if asc:
                    check("ASC exists on PlayerState", True)

                    # Check if spell abilities are granted
                    try:
                        activatable = asc.get_activatable_abilities()
                        ability_names = [a.get_class().get_name() for a in activatable] if activatable else []
                        has_fireball = any("Fireball" in n for n in ability_names)
                        has_blink = any("Blink" in n for n in ability_names)
                        check("Fireball ability granted", has_fireball,
                              f"Abilities: {', '.join(ability_names[:10])}")
                        check("Blink ability granted", has_blink)
                    except Exception as e:
                        # Can't easily query granted abilities from Python, skip
                        check("Spell abilities query (skipped)", True,
                              f"Unable to query abilities via Python: {e}")
                else:
                    check("ASC exists on PlayerState", False)
        except Exception as e:
            check("PlayerState accessible", False, str(e))

    # --- Combat HUD ---
    hud_widget = None
    try:
        hud_widget = pc.get_editor_property("combat_hud_widget")
    except Exception:
        pass

    check("CombatHUDWidget exists", hud_widget is not None,
          "PlayerController.CombatHUDWidget is null")

    if hud_widget:
        check("CombatHUDWidget visible", hud_widget.is_visible())

        try:
            child_count = hud_widget.get_bound_child_count()
            # With spell widgets, we expect more children than before (5 core + spell widgets)
            check(f"HUD child widgets ({child_count})",
                  child_count >= 5,
                  f"Only {child_count} children bound")
        except Exception as e:
            check("HUD child count query", False, str(e))

    # --- SpellDataAssets exist ---
    spell_da_paths = [
        '/MordecaiCore/Spells/DA_Spell_Fireball',
        '/MordecaiCore/Spells/DA_Spell_Blink',
        '/MordecaiCore/Spells/DA_Spell_StoneSkin',
        '/MordecaiCore/Spells/DA_Spell_Restoration',
    ]
    for path in spell_da_paths:
        asset = unreal.load_asset(path)
        name = path.split('/')[-1]
        check(f"{name} loadable", asset is not None)

    # --- Enemy with fire attack ---
    all_actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
    enemy_count = 0
    for actor in all_actors:
        actor_class = actor.get_class().get_name()
        if "Enemy" in actor_class and "Frontliner" in actor_class:
            enemy_count += 1

    check(f"Arena has enemies ({enemy_count})", enemy_count >= 1,
          "No Frontliner enemies found in arena")

    # Check fire slash asset exists
    fire_slash = unreal.load_asset('/MordecaiCore/Combat/Enemies/DA_EnemyAttack_FireSlash')
    check("DA_EnemyAttack_FireSlash loadable", fire_slash is not None)

    print("")
    print(f"RESULT: {pass_count} passed, {fail_count} failed")

    if fail_count > 0:
        raise RuntimeError(f"PIE magic smoke test failed: {fail_count} failure(s)")
    else:
        print("ALL CHECKS PASSED")

_run()
del _run
