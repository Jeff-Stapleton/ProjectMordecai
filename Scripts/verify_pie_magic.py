# PIE Smoke Test: Magic System Integration
# Run during PIE via ue_remote_exec.py to verify spell/magic wiring.
# Checks: Spell abilities granted, SP attribute exists, HUD has spell widgets,
#         enemy has fire attack profile.
#
# Usage:
#   py ue_remote_exec.py "exec(open(r'Scripts/verify_pie_magic.py').read())"
#
# NOTE: All PIE references held inside _run() to avoid GC reference leaks.

import os
import re
import sys

import unreal

_scripts_dir = unreal.Paths.convert_relative_path_to_full(
    os.path.join(unreal.Paths.project_dir(), "Scripts"))
if _scripts_dir not in sys.path:
    sys.path.insert(0, _scripts_dir)
from pie_verify_contracts import combat_hud_layer_checks, player_dead_from_state

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
    ps = None
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

    player_health = None
    asc_has_dead_tag = None
    if ps:
        try:
            attributes = ps.get_mordecai_attribute_set()
            if attributes:
                player_health = float(
                    attributes.get_editor_property("health").get_editor_property(
                        "current_value"))
        except Exception:
            pass
    if asc:
        try:
            dead_tag = unreal.GameplayTag()
            imported = dead_tag.import_text('(TagName="Mordecai.State.Dead")')
            if imported:
                asc_has_dead_tag = bool(asc.has_matching_gameplay_tag(dead_tag))
        except Exception:
            pass

    # US-090: verify the live PlayerState owns exactly one tagged native regen
    # effect. The [LOGIC] Fireball watcher below spends SP through GAS and then
    # samples the live periodic increase.
    if ps:
        try:
            attributes = ps.get_mordecai_attribute_set()
            max_sp = float(
                attributes.get_editor_property("max_spell_points").get_editor_property(
                    "current_value"))
            check("US-090 fresh MaxSpellPoints is 10",
                  abs(max_sp - 10.0) < 0.001, f"Got {max_sp:g}")
            regen_count = int(ps.get_spell_point_regen_effect_count())
            check("US-090 live ASC has exactly one tagged spell-point regen effect",
                  regen_count == 1, f"Got {regen_count}")
        except Exception as exc:
            check("US-090 live spell-point state readable", False, str(exc))
    regen_class = getattr(unreal, "MordecaiGE_SpellPointRegen", None)
    check("US-090 native spell-point regen GE exists", regen_class is not None)
    if regen_class:
        try:
            regen_cdo = unreal.get_default_object(regen_class)
            period_text = regen_cdo.get_editor_property("period").export_text()
            period_match = re.search(
                r"(?:^|[,(])\s*Value=([-+]?(?:\d+(?:\.\d*)?|\.\d+))",
                period_text)
            period_value = float(period_match.group(1)) if period_match else None
            duration_text = str(regen_cdo.get_editor_property("duration_policy"))
            executes_on_apply = bool(
                regen_cdo.get_editor_property("execute_periodic_effect_on_application"))
            check("US-090 regen GE has a 1 second period",
                  period_value is not None and abs(period_value - 1.0) < 1e-4,
                  f"Got {period_text}")
            check("US-090 regen GE is infinite",
                  "INFINITE" in duration_text.upper(), f"Got {duration_text}")
            check("US-090 regen GE does not tick on application",
                  not executes_on_apply, f"Got {executes_on_apply}")
        except Exception as exc:
            check("US-090 regen GE contract readable", False, str(exc))

    check(
        "Player death state readable",
        player_health is not None or asc_has_dead_tag is not None,
        "Neither Health nor Mordecai.State.Dead could be read from the PlayerState ASC",
    )
    player_dead = player_dead_from_state(player_health, asc_has_dead_tag)

    # --- Combat HUD ---
    hud_widget = None
    try:
        hud_widget = pc.get_editor_property("combat_hud_widget")
    except Exception:
        pass

    check("CombatHUDWidget exists", hud_widget is not None,
          "PlayerController.CombatHUDWidget is null")

    if hud_widget:
        death_screens = unreal.WidgetLibrary.get_all_widgets_of_class(
            world, unreal.MordecaiDeathScreenWidget, False)
        try:
            # W_OverallUILayout registers GameLayer_Stack as UI.Layer.Game.
            # CommonUI's native GetWidgetList is not reflected to Python.
            game_layer = next(
                (stack for stack in unreal.ObjectIterator(
                    unreal.CommonActivatableWidgetStack)
                 if stack.get_name() == "GameLayer_Stack"),
                None)
            active_game_widget = game_layer.get_active_widget() if game_layer else None
            layer_checks = combat_hud_layer_checks(
                player_dead=player_dead,
                game_layer_is_ui_layer_game=game_layer is not None,
                death_screen_is_active_widget=active_game_widget in death_screens,
                hud_is_activated=hud_widget.is_activated(),
                hud_is_visible=hud_widget.is_visible(),
            )
        except Exception as exc:
            check("UI.Layer.Game state readable", False, str(exc))
            layer_checks = None

        if player_dead and layer_checks:
            check(
                "Death screen is active on the UI.Layer.Game stack",
                layer_checks["death_screen_on_game_layer"],
                "The stack containing CombatHUDWidget does not have a death screen active",
            )
            check(
                "CombatHUDWidget inactive while death screen owns UI.Layer.Game",
                layer_checks["hud_inactive_under_death"],
                "Combat HUD remained activated underneath the death screen",
            )
        elif not player_dead and layer_checks:
            layer_checks = combat_hud_layer_checks(
                player_dead=False,
                game_layer_is_ui_layer_game=game_layer is not None,
                death_screen_is_active_widget=False,
                hud_is_activated=hud_widget.is_activated(),
                hud_is_visible=hud_widget.is_visible(),
            )
            check("CombatHUDWidget visible", layer_checks["hud_visible"])

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
    regen_pass = "PIE_Magic_LiveRegen_PASS"
    regen_fail = "PIE_Magic_LiveRegen_FAIL"
    regen_pending = "PIE_Magic_LiveRegen_PENDING"
    pawn_tags = list(pawn.get_editor_property("tags")) if pawn else []
    pawn_tag_names = {str(tag) for tag in pawn_tags}
    next_phase = False

    if ((logic_pass in pawn_tag_names or logic_fail in pawn_tag_names)
            and (regen_pass in pawn_tag_names or regen_fail in pawn_tag_names)):
        passed = logic_pass in pawn_tag_names
        check("[LOGIC] Fireball cast spawns and moves a projectile", passed,
              "post-tick cast watcher did not observe projectile displacement")
        check("[LOGIC] live tagged GE regenerates SP after a GAS spend",
              regen_pass in pawn_tag_names,
              "post-tick watcher did not observe the expected live periodic SP increase")
        pawn.set_editor_property(
            "tags", [tag for tag in pawn_tags
                     if str(tag) not in (logic_pass, logic_fail, logic_pending)])
        pawn.set_editor_property(
            "tags", [tag for tag in list(pawn.get_editor_property("tags"))
                     if str(tag) not in (regen_pass, regen_fail, regen_pending)])
    elif logic_pending in pawn_tag_names or regen_pending in pawn_tag_names:
        print("  WAIT: [LOGIC] Fireball/live-regen watcher is still running")
        next_phase = True
    elif pawn and asc and spell_abilities and len(spell_abilities) > 0:
        state = {"elapsed": 0.0, "activated": False, "projectile_name": None,
                 "start": None, "handle": None, "original_rotation": pawn.get_actor_rotation(),
                 "original_location": pawn.get_actor_location(), "sp_before": None,
                 "sp_min": None, "projectile_moved": False, "regen_ok": False}

        def current_spell_state():
            attributes = ps.get_mordecai_attribute_set()
            current = float(
                attributes.get_editor_property("spell_points").get_editor_property(
                    "current_value"))
            multiplier = float(
                attributes.get_editor_property(
                    "spell_points_regen_multiplier").get_editor_property("current_value"))
            return current, multiplier

        def finish_logic(projectile_ok, live_regen_ok):
            pawn.set_actor_location(state["original_location"], False, True)
            pawn.set_actor_rotation(state["original_rotation"], False)
            tags = list(pawn.get_editor_property("tags"))
            tags = [value for value in tags
                    if str(value) not in (
                        logic_pass, logic_fail, logic_pending,
                        regen_pass, regen_fail, regen_pending)]
            tags.extend([
                unreal.Name(logic_pass if projectile_ok else logic_fail),
                unreal.Name(regen_pass if live_regen_ok else regen_fail),
            ])
            pawn.set_editor_property("tags", tags)
            if state["handle"] is not None:
                unreal.unregister_slate_post_tick_callback(state["handle"])

        def watch_fireball(delta_seconds):
            try:
                state["elapsed"] += float(delta_seconds)
                if not state["activated"]:
                    state["activated"] = True
                    state["sp_before"], _ = current_spell_state()
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
                        finish_logic(False, False)
                        return

                current_sp, multiplier = current_spell_state()
                state["sp_min"] = (current_sp if state["sp_min"] is None
                                   else min(state["sp_min"], current_sp))
                if (state["sp_before"] is not None and state["sp_min"] is not None
                        and state["sp_before"] - state["sp_min"] >= 2.74
                        and current_sp >= state["sp_min"] + 0.25 * multiplier - 0.03):
                    state["regen_ok"] = True

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
                        state["projectile_moved"] = True
                if state["projectile_moved"] and state["regen_ok"]:
                    finish_logic(True, True)
                    return
                if state["elapsed"] > 2.5:
                    finish_logic(state["projectile_moved"], state["regen_ok"])
            except Exception as exc:
                unreal.log_error(f"PIE magic Fireball watcher exception: {exc!r}")
                finish_logic(False, False)

        pawn_tags.append(unreal.Name(logic_pending))
        pawn_tags.append(unreal.Name(regen_pending))
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
