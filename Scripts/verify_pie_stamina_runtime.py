# PIE Smoke Test: authoritative stamina runtime and HUD tier binding (US-122)
#
# This is integration evidence only. Real sprint input, drain, delay and refill
# are proven by Scripts/playtests/us122_stamina_runtime.py in the package.

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

    def make_tag(name):
        tag = unreal.GameplayTag()
        if not tag.import_text(f'(TagName="{name}")'):
            return None
        return tag

    print("=" * 60)
    print("PIE SMOKE TEST: STAMINA RUNTIME (US-122)")
    print("=" * 60)

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    check("PIE world exists", world is not None, "Start PIE before running this test")
    if not world:
        raise RuntimeError("PIE stamina runtime smoke test failed: no PIE world")

    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    check("Player controller exists", pc is not None)
    ps = pc.player_state if pc else None
    check("PlayerState exists", ps is not None)
    if not ps:
        raise RuntimeError("PIE stamina runtime smoke test failed: no PlayerState")

    components = ps.get_components_by_class(unreal.ActorComponent)
    runtime_components = [
        component for component in components
        if component.get_class().get_name() == "MordecaiStaminaRuntimeComponent"
    ]
    check(
        "PlayerState owns exactly one stamina runtime component",
        len(runtime_components) == 1,
        f"Found {[component.get_class().get_name() for component in components]}",
    )

    try:
        asc = ps.get_lyra_ability_system_component()
    except Exception:
        asc = None
    check("PlayerState ASC exists", asc is not None)

    tier_names = ("Green", "Yellow", "Red", "Exhausted")
    tier_counts = {}
    if asc:
        for tier_name in tier_names:
            full_name = f"Mordecai.Stamina.Tier.{tier_name}"
            tag = make_tag(full_name)
            try:
                tier_counts[tier_name] = int(asc.get_gameplay_tag_count(tag)) if tag else -1
            except Exception:
                tier_counts[tier_name] = (
                    1 if tag and asc.has_matching_gameplay_tag(tag) else 0)
    active_tiers = [name for name, count in tier_counts.items() if count > 0]
    check(
        "ASC owns exactly one stamina tier tag",
        len(active_tiers) == 1,
        f"Counts {tier_counts}",
    )

    widgets = unreal.WidgetLibrary.get_all_widgets_of_class(
        world, unreal.MordecaiStaminaBarWidget, False)
    check("Exactly one stamina HUD widget exists", len(widgets) == 1, f"Found {len(widgets)}")
    if widgets and active_tiers:
        try:
            widget_tier = str(widgets[0].get_stamina_tier()).split(".")[-1].lower()
        except Exception as error:
            widget_tier = f"unreadable: {error}"
        asc_tier = active_tiers[0].lower()
        check(
            "HUD tier matches authoritative ASC tag",
            asc_tier in widget_tier,
            f"HUD {widget_tier}, ASC {asc_tier}",
        )

    print("")
    print(f"RESULT: {passed} passed, {failed} failed")
    if failed:
        raise RuntimeError(f"PIE stamina runtime smoke test failed: {failed} failure(s)")
    print("ALL CHECKS PASSED")


_run()
del _run
