# PIE Smoke Test: Tin Tally local theme service (US-102)
# Read-only integration check: no gameplay or UI functions are called.

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
            message = f"  FAIL: {name}"
            if detail:
                message += f" -- {detail}"
            print(message)

    print("=" * 60)
    print("PIE SMOKE TEST: TIN TALLY THEME")
    print("=" * 60)

    world = unreal.get_editor_subsystem(
        unreal.UnrealEditorSubsystem
    ).get_game_world()
    check("PIE world exists", world is not None, "Start PIE before running this test")

    service = None
    if world:
        try:
            service = unreal.MordecaiUIThemeSubsystem.resolve_for_world(world)
        except Exception as error:
            check("Theme service accessor is callable", False, str(error))

    check("Local UI theme service exists", service is not None)
    if service:
        theme_id = str(service.get_active_theme_id())
        check(
            "Theme service resolves Diorama by default",
            theme_id == "Diorama",
            f"Got {theme_id!r}",
        )
        colors = service.get_active_colors()
        check(
            "Resolved theme has an opaque plate",
            abs(colors.plate.a - 1.0) < 0.0001,
            f"Alpha was {colors.plate.a}",
        )

    print("")
    print(f"RESULT: {pass_count} passed, {fail_count} failed")
    if fail_count:
        raise RuntimeError(
            f"PIE Tin Tally theme smoke test failed: {fail_count} failure(s)"
        )
    print("ALL CHECKS PASSED")


_run()
del _run
