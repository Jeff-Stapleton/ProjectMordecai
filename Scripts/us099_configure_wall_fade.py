"""US-099: create the masked wall-fade material and apply it to DevTestMap arena walls."""

import unreal


MAP = "/MordecaiCore/Game/Maps/DevTestMap"
MATERIAL_PATH = "/MordecaiCore/Environment/Materials/M_OcclusionFadeWall"
FADE_PARAMETER = "MordecaiOcclusionFade"
FADE_TAG = "Mordecai.OcclusionFade"
WALL_PREFIX = "Arena_Wall_"
DITHER_FUNCTION = "/Engine/Functions/Engine_MaterialFunctions02/Utility/DitherTemporalAA"

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def fail(message):
    raise RuntimeError(f"US-099 wall fade: {message}")


def expression(material, cls, x, y):
    node = MEL.create_material_expression(material, cls, x, y)
    if node is None:
        fail(f"could not create {cls.__name__}")
    return node


def connect(source, output_name, target, input_name):
    if not MEL.connect_material_expressions(source, output_name, target, input_name):
        fail(f"could not connect {source.get_name()} to {target.get_name()}.{input_name}")


def create_material():
    folder, name = MATERIAL_PATH.rsplit("/", 1)
    EAL.make_directory(folder)
    material = unreal.load_asset(MATERIAL_PATH)
    if material is None:
        material = TOOLS.create_asset(
            name, folder, unreal.Material, unreal.MaterialFactoryNew()
        )
    if material is None:
        fail(f"could not create {MATERIAL_PATH}")

    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    material.set_editor_property(
        "shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT
    )
    MEL.delete_all_material_expressions(material)

    color = expression(material, unreal.MaterialExpressionConstant3Vector, -500, -180)
    color.set_editor_property("constant", unreal.LinearColor(0.16, 0.19, 0.23, 1.0))
    MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)

    roughness = expression(material, unreal.MaterialExpressionConstant, -500, -60)
    roughness.set_editor_property("r", 0.82)
    MEL.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

    fade = expression(material, unreal.MaterialExpressionScalarParameter, -500, 100)
    fade.set_editor_property("parameter_name", FADE_PARAMETER)
    fade.set_editor_property("default_value", 1.0)

    dither_asset = unreal.load_asset(DITHER_FUNCTION)
    if dither_asset is None:
        fail(f"missing engine material function {DITHER_FUNCTION}")
    dither = expression(
        material, unreal.MaterialExpressionMaterialFunctionCall, -180, 100
    )
    dither.set_material_function(dither_asset)
    connect(fade, "", dither, "Alpha Threshold")
    if not MEL.connect_material_property(
        dither, "Result", unreal.MaterialProperty.MP_OPACITY_MASK
    ):
        fail("could not connect DitherTemporalAA result to Opacity Mask")

    MEL.recompile_material(material)
    if not EAL.save_loaded_asset(material, only_if_is_dirty=False):
        fail(f"could not save {MATERIAL_PATH}")
    if material.get_editor_property("blend_mode") != unreal.BlendMode.BLEND_MASKED:
        fail("saved material is not masked")
    return material


def apply_to_walls(material):
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not level.load_level(MAP):
        fail(f"could not load {MAP}")

    walls = [a for a in actors.get_all_level_actors()
             if a.get_actor_label().startswith(WALL_PREFIX)]
    if len(walls) != 4:
        fail(f"expected 4 arena walls, found {[a.get_actor_label() for a in walls]}")

    for wall in walls:
        meshes = wall.get_components_by_class(unreal.StaticMeshComponent)
        if len(meshes) != 1:
            fail(f"{wall.get_actor_label()} has {len(meshes)} static mesh components")
        mesh = meshes[0]
        mesh.set_material(0, material)
        tags = list(mesh.get_editor_property("component_tags"))
        if FADE_TAG not in [str(tag) for tag in tags]:
            tags.append(FADE_TAG)
            mesh.set_editor_property("component_tags", tags)
        if mesh.get_material(0) != material:
            fail(f"{wall.get_actor_label()} material readback failed")
        if FADE_TAG not in [str(tag) for tag in mesh.get_editor_property("component_tags")]:
            fail(f"{wall.get_actor_label()} fade tag readback failed")

    floors = [a for a in actors.get_all_level_actors()
              if a.get_actor_label() == "Arena_Floor"]
    for floor in floors:
        for mesh in floor.get_components_by_class(unreal.StaticMeshComponent):
            if FADE_TAG in [str(tag) for tag in mesh.get_editor_property("component_tags")]:
                fail("Arena_Floor must never carry the wall-fade tag")

    if not level.save_current_level():
        fail("could not save DevTestMap")
    return sorted(wall.get_actor_label() for wall in walls)


def main():
    material = create_material()
    walls = apply_to_walls(material)
    dirty = (unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
             + unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages())
    if dirty:
        fail(f"dirty packages remain: {[package.get_name() for package in dirty]}")
    print(f"PASS US-099: {MATERIAL_PATH} applied to {walls}")
    print("RESULT: wall fade content saved")


main()
