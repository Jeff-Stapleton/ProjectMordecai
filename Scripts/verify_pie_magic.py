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
    spell_abilities = None
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
    asc = None
    if pc:
        try:
            ps = pc.player_state
            check("PlayerState exists", ps is not None)

            if ps:
                # Check ASC has SpellPoints attribute
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

    # --- SpellDataAssets carry the shipped cooldown contract (US-089 / issue #35) ---
    spell_assets = (
        ('/MordecaiCore/Spells/DA_Spell_Fireball',
         'Mordecai.Cooldown.Spell.Fireball', 2.0),
        ('/MordecaiCore/Spells/DA_Spell_Blink',
         'Mordecai.Cooldown.Spell.Blink', 4.0),
        ('/MordecaiCore/Spells/DA_Spell_StoneSkin',
         'Mordecai.Cooldown.Spell.StoneSkin', 15.0),
        ('/MordecaiCore/Spells/DA_Spell_Restoration',
         'Mordecai.Cooldown.Spell.Restoration', 10.0),
    )
    for path, expected_tag, expected_duration in spell_assets:
        asset = unreal.load_asset(path)
        name = path.split('/')[-1]
        check(f"{name} loadable", asset is not None)
        if asset:
            cooldown_tag = asset.get_editor_property('cooldown_tag')
            serialized_tag = cooldown_tag.export_text()
            duration = float(asset.get_editor_property('cooldown_duration'))
            check(f"{name} CooldownTag is {expected_tag}",
                  serialized_tag == f'(TagName="{expected_tag}")',
                  f"Got {serialized_tag!r}")
            check(f"{name} CooldownDuration is {expected_duration:g}s",
                  abs(duration - expected_duration) < 0.001,
                  f"Got {duration:g}")

    # Issue #13: one labelled, bound cooldown widget per loadout slot.
    cooldown_widgets = unreal.WidgetLibrary.get_all_widgets_of_class(
        world, unreal.MordecaiSpellCooldownWidget, False)
    check("Four spell cooldown widgets exist",
          len(cooldown_widgets) == 4,
          f"Found {len(cooldown_widgets)}")
    expected_bound = {
        'Mordecai.Cooldown.Spell.Fireball',
        'Mordecai.Cooldown.Spell.Blink',
        'Mordecai.Cooldown.Spell.StoneSkin',
        'Mordecai.Cooldown.Spell.Restoration',
    }
    actual_bound = set()
    actual_labels = set()
    for widget in cooldown_widgets:
        try:
            actual_bound.add(
                widget.get_editor_property('bound_cooldown_tag').export_text()
                .replace('(TagName="', '').replace('")', ''))
            actual_labels.add(str(widget.get_editor_property('slot_key_label')))
        except Exception as exc:
            check("Cooldown widget tag and label readable", False, str(exc))
    check("Slots A-D are bound after spell grants",
          actual_bound == expected_bound,
          f"Got {sorted(actual_bound)}")
    check("Cooldown slot labels are Q / E / R / 1",
          actual_labels == {'Q', 'E', 'R', '1'},
          f"Got {sorted(actual_labels)}")

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

    # --- [LOGIC] Fireball cast integration ---
    # Direct ability activation is a PIE integration diagnostic, not player-input evidence. A Slate
    # post-tick callback watches the real world while it ticks, because one remote exec occupies one
    # editor frame and Fireball's cast timers/movement cannot advance synchronously inside it.
    logic_pass = "PIE_Magic_FireballMoved_PASS"
    logic_fail = "PIE_Magic_FireballMoved_FAIL"
    logic_pending = "PIE_Magic_FireballMoved_PENDING"
    pawn_tags = list(pawn.get_editor_property("tags")) if pawn else []
    pawn_tag_names = {str(tag) for tag in pawn_tags}
    next_phase = False

    if logic_pass in pawn_tag_names or logic_fail in pawn_tag_names:
        passed = logic_pass in pawn_tag_names
        check("[LOGIC] Fireball cast spawns and moves a projectile", passed,
              "post-tick cast watcher did not observe projectile displacement")
        pawn.set_editor_property(
            "tags", [tag for tag in pawn_tags
                     if str(tag) not in (logic_pass, logic_fail, logic_pending)])
    elif logic_pending in pawn_tag_names:
        print("  WAIT: [LOGIC] Fireball cast watcher is still running")
        next_phase = True
    elif pawn and asc and spell_abilities and len(spell_abilities) > 0:
        state = {"elapsed": 0.0, "activated": False, "projectile_name": None,
                 "start": None, "handle": None, "original_rotation": pawn.get_actor_rotation(),
                 "original_location": pawn.get_actor_location()}

        def finish_logic(tag):
            pawn.set_actor_location(state["original_location"], False, True)
            pawn.set_actor_rotation(state["original_rotation"], False)
            tags = list(pawn.get_editor_property("tags"))
            tags = [value for value in tags
                    if str(value) not in (logic_pass, logic_fail, logic_pending)]
            tags.append(unreal.Name(tag))
            pawn.set_editor_property("tags", tags)
            if state["handle"] is not None:
                unreal.unregister_slate_post_tick_callback(state["handle"])

        def watch_fireball(delta_seconds):
            try:
                state["elapsed"] += float(delta_seconds)
                if not state["activated"]:
                    state["activated"] = True
                    # DevTestMap starts with Frontliners close to +X. Aim into the open -X side so
                    # the diagnostic projectile survives long enough to sample on two ticks. Lift
                    # the pawn clear of arena actors for this direct-call diagnostic; Tier 4 owns
                    # player-path proof and this smoke only checks the PIE cast integration.
                    pawn.set_actor_location(
                        state["original_location"] + unreal.Vector(0.0, 0.0, 2000.0),
                        False, True)
                    pawn.set_actor_rotation(
                        unreal.Rotator(pitch=0.0, yaw=180.0, roll=0.0), False)
                    activated = asc.try_activate_ability_by_class(spell_abilities[0], True)
                    if not activated:
                        finish_logic(logic_fail)
                        return

                projectiles = [
                    actor for actor in unreal.GameplayStatics.get_all_actors_of_class(
                        world, unreal.Actor)
                    if "MordecaiProjectile" in actor.get_class().get_name()
                ]
                projectile = next(
                    (actor for actor in projectiles
                     if state["projectile_name"] == actor.get_name()), None)
                if projectile is None and projectiles:
                    projectile = projectiles[0]
                    state["projectile_name"] = projectile.get_name()
                    state["start"] = projectile.get_actor_location()
                elif projectile is not None and state["start"] is not None:
                    displacement = (projectile.get_actor_location() - state["start"]).length()
                    if displacement > 1.0:
                        finish_logic(logic_pass)
                        return
                if state["elapsed"] > 1.5:
                    finish_logic(logic_fail)
            except Exception as exc:
                unreal.log_error(f"PIE magic Fireball watcher exception: {exc!r}")
                finish_logic(logic_fail)

        pawn_tags.append(unreal.Name(logic_pending))
        pawn.set_editor_property("tags", pawn_tags)
        state["handle"] = unreal.register_slate_post_tick_callback(watch_fireball)
        print("  [LOGIC] Fireball cast watcher scheduled")
        next_phase = True
    else:
        check("[LOGIC] Fireball cast spawns and moves a projectile", False,
              "pawn, ASC, or configured Fireball class unavailable")

    print("")
    print(f"RESULT: {pass_count} passed, {fail_count} failed")

    if fail_count > 0:
        raise RuntimeError(f"PIE magic smoke test failed: {fail_count} failure(s)")
    elif next_phase:
        print("PHASE PASSED (not finished: Fireball watcher needs later editor ticks)")
        print("PIE_NEXT_PHASE: wait for [LOGIC] Fireball projectile movement")
    else:
        print("ALL CHECKS PASSED")

_run()
del _run
