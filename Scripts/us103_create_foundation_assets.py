"""Create and save the complete cooked Tin Tally foundation (US-103).

Run in the editor after us103_generate_foundation_sources.py. The script is
idempotent: it updates existing assets and replaces imported source data.
"""

import json
import pathlib
import re
import tempfile

import unreal


ROOT = pathlib.Path(unreal.Paths.project_dir()).resolve()
TOKENS = ROOT / "Design" / "TinTally" / "ds" / "project" / "tokens.json"
FONT_SOURCE = ROOT / "Design" / "TinTally" / "ds" / "project" / "fonts" / "static"
TEMP = pathlib.Path(tempfile.gettempdir())
FONT_IMPORT_STAGE = TEMP / "us103-font-import"
IMAGE_SOURCE = TEMP / "us103-foundation"
BASE = "/MordecaiCore/UI/TinTally"
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def log(message):
    unreal.log(f"[US-103] {message}")


def ensure(path):
    unreal.EditorAssetLibrary.make_directory(path)


def create_data_asset(name, path, asset_class):
    asset_path = f"{path}/{name}"
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if asset:
        return asset
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", asset_class)
    return TOOLS.create_asset(name, path, asset_class, factory)


def srgb(hex_value):
    match = re.fullmatch(
        r"rgba\(\s*(\d+),\s*(\d+),\s*(\d+),\s*([0-9.]+)\s*\)", hex_value
    )
    if match:
        red, green, blue = (int(match.group(i)) for i in range(1, 4))
        alpha = float(match.group(4))
    else:
        value = hex_value.lstrip("#")
        red, green, blue = (int(value[i : i + 2], 16) for i in (0, 2, 4))
        alpha = 1.0

    def channel(value):
        value /= 255.0
        return value / 12.92 if value <= 0.04045 else ((value + 0.055) / 1.055) ** 2.4

    return unreal.LinearColor(channel(red), channel(green), channel(blue), alpha)


def color_property(token):
    name = token.replace("-", "_")
    if name.startswith("dmg_"):
        name = "damage_" + name[4:]
    return name


def build_theme(theme_id, asset_name):
    data = json.loads(TOKENS.read_text(encoding="utf-8"))
    asset = create_data_asset(
        asset_name, f"{BASE}/Themes", unreal.MordecaiUIThemeDataAsset
    )
    asset.set_editor_property(
        "theme",
        unreal.MordecaiUITheme.HIGH_CONTRAST
        if theme_id == "high-contrast"
        else unreal.MordecaiUITheme.DIORAMA,
    )
    colors = unreal.MordecaiUIThemeColors()
    resolved = {}
    for token in data["color"]["tokens"]:
        value = token["value"]
        if isinstance(value, dict):
            resolved[token["name"]] = value[theme_id]
        else:
            alias = re.fullmatch(r"\{([^}]+)\}", value)
            if alias:
                resolved[token["name"]] = resolved[alias.group(1)]
            else:
                resolved[token["name"]] = value
        colors.set_editor_property(
            color_property(token["name"]), srgb(resolved[token["name"]])
        )
    asset.set_editor_property("colors", colors)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return asset


def import_asset(filename, destination, factory=None):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(filename))
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("destination_name", filename.stem)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("replace_existing", True)
    if factory:
        task.set_editor_property("factory", factory)
    TOOLS.import_asset_tasks([task])
    if not task.get_editor_property("imported_object_paths"):
        raise RuntimeError(f"import produced no asset: {filename}")
    return unreal.EditorAssetLibrary.load_asset(
        task.get_editor_property("imported_object_paths")[0]
    )


def import_fonts():
    faces = {
        "FF_Rowdies_Regular": "Rowdies-Regular.ttf",
        "FF_Rowdies_Bold": "Rowdies-Bold.ttf",
        "FF_AtkinsonNext_Medium": "AtkinsonHyperlegibleNext-Medium.ttf",
        "FF_AtkinsonNext_SemiBold": "AtkinsonHyperlegibleNext-SemiBold.ttf",
        "FF_AtkinsonNext_Bold": "AtkinsonHyperlegibleNext-Bold.ttf",
        "FF_AtkinsonNext_ExtraBold": "AtkinsonHyperlegibleNext-ExtraBold.ttf",
        "FF_AtkinsonMono_SemiBold": "AtkinsonHyperlegibleMono-SemiBold.ttf",
        "FF_AtkinsonMono_Bold": "AtkinsonHyperlegibleMono-Bold.ttf",
        "FF_AtkinsonMono_ExtraBold": "AtkinsonHyperlegibleMono-ExtraBold.ttf",
    }
    FONT_IMPORT_STAGE.mkdir(parents=True, exist_ok=True)
    face_assets = {}
    for asset_name, source_name in faces.items():
        source = FONT_SOURCE / source_name
        if not source.is_file():
            raise RuntimeError(f"missing static TTF: {source}")
        copied = FONT_IMPORT_STAGE / f"{asset_name}.ttf"
        copied.write_bytes(source.read_bytes())
        face = import_asset(
            copied, f"{BASE}/Fonts/Faces", unreal.FontFileImportFactory()
        )
        try:
            face.set_editor_property("loading_policy", unreal.FontLoadingPolicy.INLINE)
        except Exception as exc:
            log(f"font loading policy unavailable for {asset_name}: {exc}")
        unreal.EditorAssetLibrary.save_loaded_asset(face)
        face_assets[asset_name] = face

    fonts = {}
    for name in ("F_Tally_Display", "F_Tally_UI", "F_Tally_Mono"):
        path = f"{BASE}/Fonts/{name}"
        font = unreal.EditorAssetLibrary.load_asset(path)
        if not font:
            font = TOOLS.create_asset(
                name, f"{BASE}/Fonts", unreal.Font, unreal.FontFactory()
            )
        font.set_editor_property("font_cache_type", unreal.FontCacheType.RUNTIME)
        unreal.EditorAssetLibrary.save_loaded_asset(font)
        fonts[name] = font
    return face_assets, fonts


def configure_composite_fonts(face_assets, fonts):
    families = {
        "F_Tally_Display": [
            ("Regular", "FF_Rowdies_Regular"),
            ("Bold", "FF_Rowdies_Bold"),
        ],
        "F_Tally_UI": [
            ("Medium", "FF_AtkinsonNext_Medium"),
            ("SemiBold", "FF_AtkinsonNext_SemiBold"),
            ("Bold", "FF_AtkinsonNext_Bold"),
            ("ExtraBold", "FF_AtkinsonNext_ExtraBold"),
        ],
        "F_Tally_Mono": [
            ("SemiBold", "FF_AtkinsonMono_SemiBold"),
            ("Bold", "FF_AtkinsonMono_Bold"),
            ("ExtraBold", "FF_AtkinsonMono_ExtraBold"),
        ],
    }
    for font_name, entries in families.items():
        serialized_entries = ",".join(
            (
                f'(Name="{entry_name}",Font=(FontFaceAsset='
                f'"/Script/Engine.FontFace\'{face_assets[face_name].get_path_name()}\'"))'
            )
            for entry_name, face_name in entries
        )
        serialized = (
            f"(DefaultTypeface=(Fonts=({serialized_entries})),"
            "FallbackTypeface=(Typeface=(Fonts=),ScalingFactor=1.000000),"
            "SubTypefaces=,bEnableAscentDescentOverride=True)"
        )
        composite = fonts[font_name].get_editor_property("composite_font")
        composite.import_text(serialized)
        fonts[font_name].set_editor_property("composite_font", composite)
        if not unreal.EditorAssetLibrary.save_loaded_asset(
            fonts[font_name], only_if_is_dirty=False
        ):
            raise RuntimeError(f"failed to save configured composite font {font_name}")


def create_text_styles(fonts):
    data = json.loads(TOKENS.read_text(encoding="utf-8"))
    family_font = {
        "display": fonts["F_Tally_Display"],
        "ui": fonts["F_Tally_UI"],
        "mono": fonts["F_Tally_Mono"],
    }
    for group in data["type"]["groups"]:
        for style in group["styles"]:
            raw_name = style["name"]
            name = "TS_Tally_" + raw_name.replace("-", "_")
            path = f"{BASE}/Fonts/Styles/{name}"
            blueprint = unreal.EditorAssetLibrary.load_asset(path)
            if not blueprint:
                factory = unreal.BlueprintFactory()
                factory.set_editor_property("parent_class", unreal.CommonTextStyle)
                blueprint = TOOLS.create_asset(
                    name, f"{BASE}/Fonts/Styles", unreal.Blueprint, factory
                )
            unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
            generated = unreal.EditorAssetLibrary.load_blueprint_class(path)
            default = unreal.get_default_object(generated)
            point_size = int(
                int(style["fontSize"].removesuffix("px")) * 0.75 + 0.5
            )
            line_height = int(style["lineHeight"].removesuffix("px")) / int(
                style["fontSize"].removesuffix("px")
            )
            family = style.get("family", group.get("family"))
            weight = int(style["fontWeight"])
            face_name = {
                "display": {400: "Regular", 700: "Bold"},
                "ui": {
                    500: "Medium",
                    600: "SemiBold",
                    700: "Bold",
                    800: "ExtraBold",
                },
                "mono": {
                    600: "SemiBold",
                    700: "Bold",
                    800: "ExtraBold",
                },
            }[family][weight]
            font_info = unreal.SlateFontInfo(
                font_object=family_font[family],
                size=point_size,
                typeface_font_name=face_name,
                letter_spacing=round(
                    float(style.get("letterSpacing", "0em").removesuffix("em")) * 1000
                ),
            )
            default.set_editor_property("font", font_info)
            default.set_editor_property("line_height_percentage", line_height)
            unreal.EditorAssetLibrary.save_loaded_asset(blueprint)


def configure_texture(texture):
    texture.set_editor_property("srgb", False)
    texture.set_editor_property(
        "compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON
    )
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)


def import_textures():
    textures = []
    for source in sorted(IMAGE_SOURCE.glob("T_UI_Glyph_*.png")):
        texture = import_asset(source, f"{BASE}/Textures/Glyphs")
        configure_texture(texture)
        textures.append(texture)
    if len(textures) != 117:
        raise RuntimeError(f"expected 117 glyph textures, imported {len(textures)}")
    for name in ("T_UI_Hatch16", "T_UI_TallyAtlas"):
        texture = import_asset(IMAGE_SOURCE / f"{name}.png", f"{BASE}/Textures")
        configure_texture(texture)
        textures.append(texture)
    for name in (
        "T_UI_Badge_FeatCommon",
        "T_UI_Badge_FeatRare",
        "T_UI_Badge_FeatLegendary",
        "T_UI_Badge_Technique",
        "T_UI_Badge_KeyTag",
    ):
        texture = import_asset(IMAGE_SOURCE / f"{name}.png", f"{BASE}/Textures/Badges")
        configure_texture(texture)
        textures.append(texture)
    return textures


def create_material():
    path = f"{BASE}/Materials/M_UI_PatternFill"
    material = unreal.EditorAssetLibrary.load_asset(path)
    if not material:
        material = TOOLS.create_asset(
            "M_UI_PatternFill",
            f"{BASE}/Materials",
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )
    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    screen = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScreenPosition, -300, 0
    )
    period = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -300, 160
    )
    period.set_editor_property("parameter_name", "Period")
    period.set_editor_property("default_value", 16.0)
    angle = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -300, 260
    )
    angle.set_editor_property("parameter_name", "Angle")
    angle.set_editor_property("default_value", 0.0)
    line_width = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, 80, 300
    )
    line_width.set_editor_property("parameter_name", "LineWidth")
    line_width.set_editor_property("default_value", 1.0)
    base = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, 320, -120
    )
    base.set_editor_property("parameter_name", "Base")
    base.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, 0.0, 0.0))
    line = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, 320, -20
    )
    line.set_editor_property("parameter_name", "Line")
    line.set_editor_property("default_value", unreal.LinearColor.WHITE)
    divide = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionDivide, -100, 40
    )
    rotator = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionRotator, 80, 40
    )
    rotator.set_editor_property("speed", 1.0)
    sample = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSample, 260, 140
    )
    sample.set_editor_property(
        "texture",
        unreal.EditorAssetLibrary.load_asset(f"{BASE}/Textures/T_UI_Hatch16"),
    )
    multiply = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionMultiply, 480, 140
    )
    color_lerp = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionLinearInterpolate, 700, 20
    )
    base_alpha = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionComponentMask, 500, -120
    )
    base_alpha.set_editor_property("a", True)
    line_alpha = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionComponentMask, 500, -40
    )
    line_alpha.set_editor_property("a", True)
    opacity_lerp = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionLinearInterpolate, 700, 180
    )

    connect = unreal.MaterialEditingLibrary.connect_material_expressions
    connect(screen, "PixelPosition", divide, "A")
    connect(period, "", divide, "B")
    connect(divide, "", rotator, "Coordinate")
    connect(angle, "", rotator, "Time")
    connect(rotator, "", sample, "Coordinates")
    connect(sample, "R", multiply, "A")
    connect(line_width, "", multiply, "B")
    connect(base, "", color_lerp, "A")
    connect(line, "", color_lerp, "B")
    connect(multiply, "", color_lerp, "Alpha")
    connect(base, "", base_alpha, "Input")
    connect(line, "", line_alpha, "Input")
    connect(base_alpha, "", opacity_lerp, "A")
    connect(line_alpha, "", opacity_lerp, "B")
    connect(multiply, "", opacity_lerp, "Alpha")
    unreal.MaterialEditingLibrary.connect_material_property(
        color_lerp, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        opacity_lerp, "", unreal.MaterialProperty.MP_OPACITY
    )
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def create_styles():
    styles = []
    for name in ("Plate", "Pill", "Tag", "Well", "Keycap", "Focus"):
        style = create_data_asset(
            f"ST_Tally_{name}",
            f"{BASE}/Styles",
            unreal.MordecaiUIPrimitiveStyleDataAsset,
        )
        if not style.populate_from_builder(name):
            raise RuntimeError(f"unknown primitive builder: {name}")
        unreal.EditorAssetLibrary.save_loaded_asset(style)
        styles.append(style)
    return styles


def create_manifest(fonts, textures, material, styles):
    manifest = create_data_asset(
        "DA_TinTallyFoundation", BASE, unreal.MordecaiUIFoundationDataAsset
    )
    manifest.set_editor_property(
        "font_license_notices",
        [
            "Rowdies — Copyright 2018 The Rowdies Project Authors (https://github.com/magictype/rowdies) — SIL Open Font License 1.1",
            "Atkinson Hyperlegible Next — Copyright 2020-2024 The Atkinson Hyperlegible Next Project Authors (https://github.com/googlefonts/atkinson-hyperlegible-next) — SIL Open Font License 1.1",
            "Atkinson Hyperlegible Mono — Copyright 2020-2024 The Atkinson Hyperlegible Mono Project Authors (https://github.com/googlefonts/atkinson-hyperlegible-next-mono) — SIL Open Font License 1.1",
        ],
    )
    manifest.set_editor_property("composite_fonts", list(fonts.values()))
    manifest.set_editor_property("textures", textures)
    manifest.set_editor_property("pattern_fill_material", material)
    manifest.set_editor_property("primitive_styles", styles)
    unreal.EditorAssetLibrary.save_loaded_asset(manifest)


def main():
    for folder in (
        "Themes",
        "Fonts",
        "Fonts/Faces",
        "Fonts/Styles",
        "Styles",
        "Materials",
        "Textures",
        "Textures/Glyphs",
        "Textures/Badges",
    ):
        ensure(f"{BASE}/{folder}")
    build_theme("diorama", "DA_UITheme_Diorama")
    build_theme("high-contrast", "DA_UITheme_HighContrast")
    faces, fonts = import_fonts()
    configure_composite_fonts(faces, fonts)
    create_text_styles(fonts)
    textures = import_textures()
    material = create_material()
    styles = create_styles()
    create_manifest(fonts, textures, material, styles)
    unreal.EditorAssetLibrary.save_directory(BASE, only_if_is_dirty=False, recursive=True)
    log("foundation assets created and saved")


main()
