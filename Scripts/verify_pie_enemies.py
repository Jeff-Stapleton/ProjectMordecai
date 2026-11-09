# PIE Smoke Test: Frontliner enemies (US-085, issue #18 / N13, issue #19 / N14)
# Run during PIE via ue_remote_exec.py to verify the arena's three Frontliners start with the
# Blueprint's AC-054.2 health (200), that no placed instance overrides its class's stats, and that
# each has a visible body that does not collide (AC-085.1).
# Issue #17 also verifies the live player and every enemy have the same rendered green facing arrow.
# Reads state only: no gameplay function is called, so no check here is [LOGIC].
# US-085 extends this file with the attack-profile checks.
#
# Usage:
#   py ue_remote_exec.py --file Scripts/verify_pie_enemies.py
#
# NOTE: All PIE references are held inside _run() so they go out of scope
# when the function returns, avoiding GC reference leaks after PIE ends.

import unreal

FRONTLINER_CLASS = "/MordecaiCore/Enemies/BP_MordecaiEnemy_Frontliner.BP_MordecaiEnemy_Frontliner_C"
EXPECTED_ENEMIES = 3
EXPECTED_MAX_HEALTH = 200.0  # AC-054.2
STATS = ("default_max_health", "default_max_posture", "default_move_speed")


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
            raise RuntimeError(f"PIE enemies smoke test failed: {fail_count} failure(s)")
        print("ALL CHECKS PASSED")

    def body_problems(body):
        """Why a BodyMesh would not show a player a solid-looking, non-blocking body ([] = none)."""
        if body is None:
            return ["no BodyMesh component"]
        problems = []
        if body.get_editor_property("static_mesh") is None:
            problems.append("no mesh")
        if not body.is_visible() or body.get_editor_property("hidden_in_game"):
            problems.append("hidden")
        if body.get_collision_enabled() != unreal.CollisionEnabled.NO_COLLISION:
            problems.append(f"collision {body.get_collision_enabled()}")
        return problems

    def body_detail(body, problems):
        mesh = body.get_editor_property("static_mesh") if body else None
        return "; ".join(problems) if problems else f"{mesh.get_name()}, no collision"

    def arrow_problems(owner):
        """Why the issue #17 FacingArrow would not be the rendered, owner-agnostic indicator."""
        arrow = owner.get_facing_arrow() if owner else None
        if arrow is None:
            return None, ["no FacingArrow component"]
        problems = []
        loc = arrow.get_editor_property("relative_location")
        color = arrow.get_editor_property("arrow_color")
        if arrow.get_attach_parent() != owner.get_editor_property("root_component"):
            problems.append("not attached to root")
        if abs(loc.x) > 0.01 or abs(loc.y) > 0.01 or abs(loc.z - 50.0) > 0.01:
            problems.append(f"relative location ({loc.x}, {loc.y}, {loc.z}), expected Z +50")
        if (color.r, color.g, color.b) != (0, 255, 0):
            problems.append(f"color ({color.r}, {color.g}, {color.b}), expected green")
        if abs(arrow.get_editor_property("arrow_size") - 1.5) > 0.001:
            problems.append(f"size {arrow.get_editor_property('arrow_size')}, expected 1.5")
        if abs(arrow.get_editor_property("arrow_length") - 80.0) > 0.001:
            problems.append(f"length {arrow.get_editor_property('arrow_length')}, expected 80")
        if arrow.get_editor_property("hidden_in_game") or not arrow.is_visible():
            problems.append("hidden")
        if (arrow.get_editor_property("only_owner_see")
                or arrow.get_editor_property("owner_no_see")):
            problems.append("owner visibility restricted")
        if not arrow.was_recently_rendered(1.0):
            problems.append("not rendered in the last second")
        if arrow.get_collision_enabled() != unreal.CollisionEnabled.NO_COLLISION:
            problems.append(f"collision {arrow.get_collision_enabled()}")
        return arrow, problems

    def arrow_detail(arrow, problems):
        if problems:
            return "; ".join(problems)
        return (f"green, Z +50, size {arrow.get_editor_property('arrow_size')}, "
                f"length {arrow.get_editor_property('arrow_length')}, rendered")

    def attribute(enemy, name):
        """Current value of a UMordecaiAttributeSet attribute (None when unreadable)."""
        attribute_set = enemy.get_mordecai_attribute_set()
        if attribute_set is None:
            return None
        return attribute_set.get_editor_property(name).get_editor_property("current_value")

    print("=" * 60)
    print("PIE SMOKE TEST: FRONTLINER ENEMIES")
    print("=" * 60)

    # --- The Blueprint: AC-054.2's 200 lives on its class default object ---
    frontliner_class = unreal.load_class(None, FRONTLINER_CLASS)
    check("Frontliner Blueprint class loads", frontliner_class is not None, FRONTLINER_CLASS)
    if frontliner_class:
        bp_health = unreal.get_default_object(frontliner_class).get_editor_property("default_max_health")
        check("Frontliner Blueprint DefaultMaxHealth is 200", bp_health == EXPECTED_MAX_HEALTH,
              f"Got {bp_health}")
        bp_body = unreal.get_default_object(frontliner_class).get_editor_property("body_mesh")
        problems = body_problems(bp_body)
        check("Frontliner Blueprint BodyMesh has a visible, non-colliding mesh", not problems,
              body_detail(bp_body, problems))

    # --- PIE World ---
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    check("PIE world exists", world is not None, "" if world else "Start PIE before running this test")
    if not world:
        finish()
        return

    players = list(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.MordecaiCharacter))
    check("one Mordecai player in the arena", len(players) == 1,
          f"Found {len(players)}: {[p.get_name() for p in players]}")
    if players:
        arrow, problems = arrow_problems(players[0])
        check("Player FacingArrow is rendered and visible to every player", not problems,
              arrow_detail(arrow, problems))

    enemies = list(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.MordecaiEnemyCharacter))
    check(f"{EXPECTED_ENEMIES} enemies in the arena", len(enemies) == EXPECTED_ENEMIES,
          f"Found {len(enemies)}: {[e.get_name() for e in enemies]}")

    for enemy in enemies:
        name = enemy.get_name()
        loc = enemy.get_actor_location()
        where = f"{name} at ({loc.x:.0f}, {loc.y:.0f})"

        # A placed actor whose level saved an old value overrides its Blueprint (issue #18)
        class_defaults = unreal.get_default_object(enemy.get_class())
        overrides = []
        for stat in STATS:
            value, class_value = enemy.get_editor_property(stat), class_defaults.get_editor_property(stat)
            if value != class_value:
                overrides.append(f"{stat} {value} (class {class_value})")
        check(f"{where} takes its stats from {enemy.get_class().get_name()}", not overrides,
              "; ".join(overrides) if overrides else "no instance override")

        # Issue #19: a body players can see; the capsule alone blocks
        body = enemy.get_editor_property("body_mesh")
        problems = body_problems(body)
        check(f"{where} has a visible body that does not collide", not problems,
              body_detail(body, problems))

        arrow, problems = arrow_problems(enemy)
        check(f"{where} FacingArrow is rendered and visible to every player", not problems,
              arrow_detail(arrow, problems))

        try:
            health, max_health = attribute(enemy, "health"), attribute(enemy, "max_health")
        except Exception as e:
            check(f"{where} health readable", False, str(e))
            continue
        check(f"{where} starts at {EXPECTED_MAX_HEALTH:g}/{EXPECTED_MAX_HEALTH:g} health",
              health == EXPECTED_MAX_HEALTH and max_health == EXPECTED_MAX_HEALTH,
              f"Got {health}/{max_health}")

    finish()


_run()
del _run
