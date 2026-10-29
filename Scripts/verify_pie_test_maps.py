# PIE Smoke Test: the playtest test maps (US-100, issue #46 / T1)
# Run during PIE via ue_remote_exec.py to verify what the Tier 4 scripts rely on when they launch a test map:
#   MovementTestMap: the Mordecai game mode, the pawn spawned near (0, 0), no enemy and no pickup.
#   CombatTestMap: the same, plus TestDummy (BP_MordecaiEnemy_TestDummy_C, AI BP_MordecaiEnemyAI_TestDummy_C) and
#   TestAttacker (BP_MordecaiEnemy_TestAttacker_C, the native MordecaiEnemyAIController), both alive at 200/200,
#   Idle, with MaxWalkSpeed 0; the attacker farther than its AggroRange from the pawn; navigation data present.
#   Any other map (DevTestMap, the default suite): SKIP lines only, and the test passes.
# PIE applies the PIE-only ExperienceOverride to every map, so this cannot prove the packaged route to the
# experience (World Settings GameMode Override): Mordecai.Level.TestMaps.* and the Tier 3 boot do.
# Reads state only: no gameplay function is called, so no check here is [LOGIC].
#
# Usage:
#   py Scripts/run_pie_verify.py --map /MordecaiCore/Game/Maps/CombatTestMap --tests spawn,hud,test_maps
#
# NOTE: All PIE references are held inside _run() so they go out of scope
# when the function returns, avoiding GC reference leaks after PIE ends.

import math

import unreal

MOVEMENT_TEST_MAP = "MovementTestMap"
COMBAT_TEST_MAP = "CombatTestMap"
SPAWN = (0.0, 0.0)
SPAWN_TOLERANCE_CM = 100.0
EXPECTED_MAX_HEALTH = 200.0
# (label, enemy class, AI controller class, spawn)
COMBAT_ENEMIES = (
    ("TestDummy", "BP_MordecaiEnemy_TestDummy_C", "BP_MordecaiEnemyAI_TestDummy_C", (600.0, 600.0)),
    ("TestAttacker", "BP_MordecaiEnemy_TestAttacker_C", "MordecaiEnemyAIController", (2100.0, 0.0)),
)


def _run():
    pass_count = 0
    fail_count = 0

    def check(name, condition, detail=""):
        nonlocal pass_count, fail_count
        if condition:
            pass_count += 1
            print(f"  PASS: {name}" + (f" -- {detail}" if detail else ""))
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
            raise RuntimeError(f"PIE test maps smoke test failed: {fail_count} failure(s)")
        print("ALL CHECKS PASSED")

    def attribute(enemy, name):
        """Current value of a UMordecaiAttributeSet attribute (None when unreadable)."""
        attribute_set = enemy.get_mordecai_attribute_set()
        if attribute_set is None:
            return None
        return attribute_set.get_editor_property(name).get_editor_property("current_value")

    print("=" * 60)
    print("PIE SMOKE TEST: TEST MAPS (MovementTestMap, CombatTestMap)")
    print("=" * 60)

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    check("PIE world exists", world is not None, "" if world else "Start PIE before running this test")
    if not world:
        finish()
        return

    map_name = unreal.GameplayStatics.get_current_level_name(world, True)
    if map_name not in (MOVEMENT_TEST_MAP, COMBAT_TEST_MAP):
        print(f"  SKIP: PIE is on {map_name}, not a test map (run_pie_verify.py --map <test map> to check one)")
        finish()
        return
    print(f"  PIE map: {map_name}")

    # --- Both maps: the Mordecai game mode and the spawn ---
    game_mode = unreal.GameplayStatics.get_game_mode(world)
    check(f"{map_name} game mode is MordecaiGameMode", isinstance(game_mode, unreal.MordecaiGameMode),
          game_mode.get_class().get_name() if game_mode else "no game mode")

    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    check("Player pawn exists", pawn is not None)
    pawn_loc = pawn.get_actor_location() if pawn else None
    if pawn_loc:
        offset = math.dist((pawn_loc.x, pawn_loc.y), SPAWN)
        check("Pawn spawned near (0, 0)", offset <= SPAWN_TOLERANCE_CM,
              f"({pawn_loc.x:.0f}, {pawn_loc.y:.0f}, {pawn_loc.z:.0f}), {offset:.0f} cm from the spawn")

    enemies = list(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.MordecaiEnemyCharacter))
    pickups = list(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.MordecaiItemPickup))
    check(f"{map_name} has no item pickups", not pickups, f"{[p.get_actor_label() for p in pickups]}")

    if map_name == MOVEMENT_TEST_MAP:
        check("MovementTestMap has no enemies", not enemies, f"{[e.get_actor_label() for e in enemies]}")
        finish()
        return

    # --- CombatTestMap: the dummy and the attacker ---
    check("CombatTestMap has exactly the two test enemies", len(enemies) == len(COMBAT_ENEMIES),
          f"{[(e.get_actor_label(), e.get_class().get_name()) for e in enemies]}")
    for label, class_name, ai_class_name, spawn in COMBAT_ENEMIES:
        matches = [e for e in enemies if e.get_class().get_name() == class_name]
        if not matches:
            check(f"{label} ({class_name}) present", False, "not in the PIE world")
            continue
        enemy = matches[0]
        loc = enemy.get_actor_location()
        where = f"{enemy.get_actor_label()} at ({loc.x:.0f}, {loc.y:.0f})"
        check(f"{label} ({class_name}) present", len(matches) == 1 and enemy.get_actor_label() == label, where)
        check(f"{label} stands at its spot", math.dist((loc.x, loc.y), spawn) <= 5.0, where)

        controller = enemy.get_controller()
        controller_class = controller.get_class().get_name() if controller else None
        check(f"{label} is possessed by {ai_class_name}", controller_class == ai_class_name, f"{controller_class}")

        try:
            health, max_health = attribute(enemy, "health"), attribute(enemy, "max_health")
        except Exception as e:
            check(f"{label} health readable", False, str(e))
            health = max_health = None
        check(f"{label} alive at {EXPECTED_MAX_HEALTH:g}/{EXPECTED_MAX_HEALTH:g}",
              not enemy.is_dead() and health == EXPECTED_MAX_HEALTH and max_health == EXPECTED_MAX_HEALTH,
              f"{health}/{max_health}, dead {enemy.is_dead()}")

        movement = enemy.get_editor_property("character_movement")
        speed = movement.get_editor_property("max_walk_speed") if movement else None
        check(f"{label} MaxWalkSpeed is 0 (never moves)", speed == 0.0, f"{speed}")

        if isinstance(controller, unreal.MordecaiEnemyAIController):
            state = controller.get_current_state()
            check(f"{label} AI is Idle", state == unreal.MordecaiEnemyAIState.IDLE, f"{state}")
            aggro = controller.get_editor_property("aggro_range")
            if label == "TestDummy":
                check("TestDummy AI AggroRange is 0 (never aggroes)", aggro == 0.0, f"{aggro}")
            elif pawn_loc:
                distance = enemy.get_distance_to(pawn)
                check(f"TestAttacker is farther than its AggroRange {aggro:.0f} from the pawn", distance > aggro,
                      f"{distance:.0f} cm")

    # --- Navigation: the saved navmesh the attacker's MoveToActor uses ---
    # Presence only. A path query here would be a static NavigationSystemV1 call, and during PIE the editor resolves
    # its world from the class default object: a handled ensure (NavigationSystem.h:297) in the log. The path itself
    # is checked by Scripts/us100_02_create_test_maps.py in the editor world.
    navmeshes = list(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.RecastNavMesh))
    check("CombatTestMap has navigation data (RecastNavMesh)", len(navmeshes) >= 1, f"{[n.get_name() for n in navmeshes]}")

    finish()


_run()
del _run
