# PIE Smoke Test: cooked Tin Tally foundation and Development gallery (US-103)
# Theme toggles are direct subsystem calls and are labelled [LOGIC].

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

    print("=" * 60)
    print("PIE SMOKE TEST: TIN TALLY FOUNDATION GALLERY")
    print("=" * 60)

    required = (
        "/MordecaiCore/UI/TinTally/Themes/DA_UITheme_Diorama",
        "/MordecaiCore/UI/TinTally/Themes/DA_UITheme_HighContrast",
        "/MordecaiCore/UI/TinTally/Fonts/F_Tally_Display",
        "/MordecaiCore/UI/TinTally/Fonts/F_Tally_UI",
        "/MordecaiCore/UI/TinTally/Fonts/F_Tally_Mono",
        "/MordecaiCore/UI/TinTally/Materials/M_UI_PatternFill",
        "/MordecaiCore/UI/TinTally/Textures/T_UI_Hatch16",
        "/MordecaiCore/UI/TinTally/Textures/T_UI_TallyAtlas",
        "/MordecaiCore/UI/TinTally/DA_TinTallyFoundation",
    )
    loaded = {path: unreal.load_asset(path) for path in required}
    check(
        "Every core foundation asset loads",
        all(asset is not None for asset in loaded.values()),
        ", ".join(path for path, asset in loaded.items() if asset is None),
    )

    gallery_class = unreal.load_class(
        None, "/Script/LyraGame.MordecaiTinTallyGallerySubsystem"
    )
    check(
        "Development gallery subsystem class loads",
        gallery_class is not None,
    )

    world = unreal.get_editor_subsystem(
        unreal.UnrealEditorSubsystem
    ).get_game_world()
    controller = (
        unreal.GameplayStatics.get_player_controller(world, 0) if world else None
    )
    hud = None
    if controller:
        try:
            hud = controller.get_editor_property("combat_hud_widget")
        except Exception:
            hud = None
    service = (
        unreal.MordecaiUIThemeSubsystem.resolve_for_widget(hud) if hud else None
    )
    check("Local theme subsystem resolves", service is not None)
    if service and loaded[required[0]] and loaded[required[1]]:
        service.set_theme_assets(loaded[required[0]], loaded[required[1]])
        service.set_active_theme(unreal.MordecaiUITheme.HIGH_CONTRAST)
        check(
            "[LOGIC] High Contrast gallery theme selects the cooked asset",
            str(service.get_active_theme_id()) == "HighContrast",
        )
        service.set_active_theme(unreal.MordecaiUITheme.DIORAMA)
        check(
            "[LOGIC] Diorama gallery theme selects the cooked asset",
            str(service.get_active_theme_id()) == "Diorama",
        )

    print("")
    print(f"RESULT: {passed} passed, {failed} failed")
    if failed:
        raise RuntimeError(
            f"PIE Tin Tally foundation gallery failed: {failed} failure(s)"
        )
    print("ALL CHECKS PASSED")


_run()
del _run
