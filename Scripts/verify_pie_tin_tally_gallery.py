# PIE Smoke Test: cooked Tin Tally foundation and non-Shipping gallery (US-103)
# Theme toggles are direct subsystem calls and are labelled [LOGIC].

import json
import pathlib

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

    manifest = loaded[required[-1]]
    manifest_assets = []
    if manifest:
        for field in ("composite_fonts", "textures", "primitive_styles"):
            manifest_assets.extend(manifest.get_editor_property(field))
        manifest_assets.append(manifest.get_editor_property("pattern_fill_material"))
    check(
        "The whole foundation manifest resolves",
        bool(manifest_assets) and all(asset is not None for asset in manifest_assets),
        f"{sum(asset is None for asset in manifest_assets)} unresolved reference(s)",
    )

    gallery_class = unreal.load_class(
        None, "/Script/LyraGame.MordecaiTinTallyGallerySubsystem"
    )
    check(
        "Non-Shipping gallery subsystem class loads",
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
        tokens_path = (
            pathlib.Path(unreal.Paths.project_dir())
            / "Design"
            / "TinTally"
            / "ds"
            / "project"
            / "tokens.json"
        )
        token_data = json.loads(tokens_path.read_text(encoding="utf-8"))
        color_properties = []
        for token in token_data["color"]["tokens"]:
            property_name = token["name"].replace("-", "_")
            if property_name.startswith("dmg_"):
                property_name = "damage_" + property_name[4:]
            color_properties.append(property_name)

        def colors_match(actual, expected):
            for property_name in color_properties:
                actual_color = actual.get_editor_property(property_name)
                expected_color = expected.get_editor_property(property_name)
                if any(
                    abs(getattr(actual_color, channel) - getattr(expected_color, channel))
                    > 0.0001
                    for channel in ("r", "g", "b", "a")
                ):
                    return False
            return True

        service.set_theme_assets(loaded[required[0]], loaded[required[1]])
        service.set_active_theme(unreal.MordecaiUITheme.HIGH_CONTRAST)
        check(
            "[LOGIC] High Contrast gallery theme selects the cooked asset",
            str(service.get_active_theme_id()) == "HighContrast",
        )
        check(
            "[LOGIC] High Contrast active colours equal every saved manifest colour",
            colors_match(
                service.get_active_colors(),
                loaded[required[1]].get_editor_property("colors"),
            ),
        )
        service.set_active_theme(unreal.MordecaiUITheme.DIORAMA)
        check(
            "[LOGIC] Diorama gallery theme selects the cooked asset",
            str(service.get_active_theme_id()) == "Diorama",
        )
        check(
            "[LOGIC] Diorama active colours equal every saved manifest colour",
            colors_match(
                service.get_active_colors(),
                loaded[required[0]].get_editor_property("colors"),
            ),
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
