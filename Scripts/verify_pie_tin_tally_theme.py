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

    local_huds = []
    if world:
        for player_index in range(8):
            controller = unreal.GameplayStatics.get_player_controller(
                world, player_index
            )
            if controller is None:
                break
            if not controller.is_local_controller():
                continue
            try:
                hud = controller.get_editor_property("combat_hud_widget")
            except Exception:
                hud = None
            if hud is not None:
                local_huds.append(hud)

    services = []
    accessor_error = None
    for hud in local_huds:
        try:
            services.append(
                unreal.MordecaiUIThemeSubsystem.resolve_for_widget(hud)
            )
        except Exception as error:
            accessor_error = str(error)
            break

    # One aggregate assertion means an accessor exception is reported once,
    # rather than again as a second "service missing" failure.
    service_access_ok = (
        accessor_error is None
        and len(local_huds) > 0
        and len(services) == len(local_huds)
        and all(service is not None for service in services)
        and len({service.get_path_name() for service in services}) == len(services)
    )
    check(
        "Exactly one local UI theme service resolves per local-player HUD",
        service_access_ok,
        accessor_error
        or f"{len(local_huds)} local HUD(s), {len(services)} service(s)",
    )

    if service_access_ok:
        default_ids = [str(service.get_active_theme_id()) for service in services]
        check(
            "Every local theme service resolves Diorama by default",
            all(theme_id == "Diorama" for theme_id in default_ids),
            f"Got {default_ids!r}",
        )
        plate_alphas = [
            service.get_active_colors().plate.a for service in services
        ]
        check(
            "Every resolved theme has an opaque plate",
            all(abs(alpha - 1.0) < 0.0001 for alpha in plate_alphas),
            f"Alphas were {plate_alphas!r}",
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
