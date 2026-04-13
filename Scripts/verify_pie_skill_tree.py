# PIE Smoke Test: Skill Tree Widget (US-067)
# Verify the skill tree widget class is registered, its UFUNCTIONs are
# Python-reachable, and the bound SkillComponent is reachable on the local
# player's PlayerState (where US-011 attaches it).
#
# Usage:
#   py ue_remote_exec.py "exec(open(r'Scripts/verify_pie_skill_tree.py').read())"
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
    print("PIE SMOKE TEST: SKILL TREE")
    print("=" * 60)

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    check("PIE world exists", world is not None,
          "Start PIE before running this test")

    if not world:
        print(f"\nRESULT: {pass_count} passed, {fail_count} failed")
        if fail_count > 0:
            raise RuntimeError(
                f"PIE skill tree smoke test failed: {fail_count} failure(s)"
            )
        return

    # Skill tree widget class must be registered.
    has_widget = hasattr(unreal, 'MordecaiSkillTreeWidget')
    check("MordecaiSkillTreeWidget class registered", has_widget,
          "Class not found in unreal module")

    # Pause menu must exist so the "skills" tab has somewhere to land.
    has_pause_menu = hasattr(unreal, 'MordecaiPauseMenuWidget')
    check("MordecaiPauseMenuWidget class registered", has_pause_menu)

    # Skill component (US-011) is the data source for this widget.
    has_skill_comp = hasattr(unreal, 'MordecaiSkillComponent')
    check("MordecaiSkillComponent class registered", has_skill_comp)

    if has_widget:
        widget_class = unreal.MordecaiSkillTreeWidget
        check(
            "BindToSkillComponent exposed to Python",
            hasattr(widget_class, 'bind_to_skill_component'),
            "UFUNCTION missing from Python-exposed surface"
        )
        check(
            "UnbindFromSkillComponent exposed to Python",
            hasattr(widget_class, 'unbind_from_skill_component'),
            "UFUNCTION missing from Python-exposed surface"
        )
        check(
            "RegisterWithPauseMenu exposed to Python",
            hasattr(widget_class, 'register_with_pause_menu'),
            "Static UFUNCTION missing from Python-exposed surface"
        )
        check(
            "SetActiveCategory exposed to Python",
            hasattr(widget_class, 'set_active_category'),
            "UFUNCTION missing from Python-exposed surface"
        )
        check(
            "GetVisibleSkills exposed to Python",
            hasattr(widget_class, 'get_visible_skills'),
            "UFUNCTION missing from Python-exposed surface"
        )
        check(
            "RequestAllocateSkillPoint exposed to Python",
            hasattr(widget_class, 'request_allocate_skill_point'),
            "UFUNCTION missing from Python-exposed surface"
        )
        check(
            "SetSelectedSkill exposed to Python",
            hasattr(widget_class, 'set_selected_skill'),
            "UFUNCTION missing from Python-exposed surface"
        )
        check(
            "GetMilestonesForSelectedSkill exposed to Python",
            hasattr(widget_class, 'get_milestones_for_selected_skill'),
            "UFUNCTION missing from Python-exposed surface"
        )

        pc = unreal.GameplayStatics.get_player_controller(world, 0)
        check("PlayerController exists", pc is not None)

        # Confirm the local player's PlayerState has the SkillComponent the
        # widget binds to (US-011 attaches it to PlayerState at construction).
        if pc is not None and has_skill_comp:
            ps = pc.player_state
            check("PlayerState exists", ps is not None)
            if ps is not None:
                skill_comp = ps.get_component_by_class(unreal.MordecaiSkillComponent)
                check("PlayerState has MordecaiSkillComponent", skill_comp is not None)

    print("")
    print(f"RESULT: {pass_count} passed, {fail_count} failed")

    if fail_count > 0:
        raise RuntimeError(
            f"PIE skill tree smoke test failed: {fail_count} failure(s)"
        )
    else:
        print("ALL CHECKS PASSED")


_run()
del _run
