"""Create and save the isolated US-120 Verdant Wilds terrain test map.

Run inside the GUI editor through Scripts/ue_remote_exec.py after
us120_import_vw_tiles.py has completed.
"""

import gc
import json
import math
import os
import pathlib

import unreal


MAP = "/MordecaiCore/Game/Maps/L_VW_TileTest"
DEVTEST_MAP = "/MordecaiCore/Game/Maps/DevTestMap"
MESH_ROOT = "/MordecaiCore/World/Terrain/VerdantWilds/Meshes"
MASTER_MATERIAL = "/MordecaiCore/World/Terrain/Shared/Materials/M_Terrain_Master"
MANIFEST = (
    pathlib.Path(unreal.Paths.project_dir())
    / "Scripts"
    / "us120_vw_source_manifest.json"
)
IMPORT_REPORT = (
    pathlib.Path(unreal.Paths.project_saved_dir())
    / "Playtests"
    / "us120-vw-import-report.json"
)
CUBE = "/Engine/BasicShapes/Cube"
GRID = "/Engine/EngineMaterials/WorldGridMaterial"
BASIC_SHAPE_MATERIAL = "/Engine/BasicShapes/BasicShapeMaterial"
WATER_MATERIAL = "/MordecaiCore/Game/Maps/Fixtures/MI_VW_TestWater"

PLAYER_START = (-1200.0, 0.0, 100.0)
FLOOR_XY = (-1700.0, 4400.0, -2800.0, 3200.0)
FLOOR_Z = (-100.0, 0.0)
SHALLOW_HOLE = (2200.0, 2800.0, -300.0, 300.0)
DEEP_FIXTURES = (
    ("DeepEdge", -1000.0),
    ("DeepOuterCorner", 0.0),
    ("DeepInnerCorner", 1000.0),
)
POND_HOLES = (
    SHALLOW_HOLE,
    *((3500.0, 4100.0, centre_y - 300.0, centre_y + 300.0)
      for _, centre_y in DEEP_FIXTURES),
)

# Screen north is world +X and screen east is world +Y at camera yaw 0.
GALLERY_LOCATIONS = (
    (-800.0, 1400.0, 0.0),
    (-800.0, 1600.0, 0.0),
    (-800.0, 2400.0, 0.0),
    (-800.0, 2600.0, 0.0),
    (0.0, 1400.0, 0.0),
    (0.0, 1600.0, 0.0),
    (0.0, 2400.0, 0.0),
    (0.0, 2600.0, 0.0),
    (800.0, 1400.0, 0.0),
    # Pond gallery pieces are raised so their catalog beds are not buried by
    # the display floor. The traversable pond fixtures remain at Z=0 below.
    (800.0, 1600.0, 160.0),
    (800.0, 2400.0, 160.0),
    (800.0, 2600.0, 160.0),
    (1600.0, 1200.0, 160.0),
    (1600.0, 1400.0, 160.0),
    (1600.0, 1600.0, 160.0),
    (1600.0, 2400.0, 0.0),
    (1600.0, 2600.0, 0.0),
)

LANES = (
    # label, mesh id, location, yaw. Imported local -Y is face/downhill.
    (
        "VW_Lane_Ramp2T",
        "SM_VW_FeltCliff_Ramp_2T_01",
        (200.0, -600.0, 0.0),
        -90.0,
    ),
    (
        "VW_Lane_Ramp3T",
        "SM_VW_FeltCliff_Ramp_3T_01",
        (100.0, 600.0, 0.0),
        -90.0,
    ),
    (
        "VW_Lane_Cliff",
        "SM_VW_FeltCliff_Straight_01",
        (1400.0, 0.0, 0.0),
        -90.0,
    ),
    (
        "VW_Lane_ShallowPond",
        "SM_VW_Pond_Edge_01",
        (2200.0, 0.0, 0.0),
        90.0,
    ),
    (
        "VW_Lane_ShallowPond_Far",
        "SM_VW_Pond_Edge_01",
        (2800.0, 0.0, 0.0),
        -90.0,
    ),
    (
        "VW_Lane_DeepEdge",
        "SM_VW_Pond_Edge_Deep_01",
        (3800.0, -1000.0, 0.0),
        -90.0,
    ),
    (
        "VW_Lane_DeepOuterCorner",
        "SM_VW_Pond_OuterCorner_Deep_01",
        (3800.0, 0.0, 0.0),
        -90.0,
    ),
    (
        "VW_Lane_DeepInnerCorner",
        "SM_VW_Pond_InnerCorner_Deep_01",
        (3800.0, 1000.0, 0.0),
        -90.0,
    ),
)

EAL = unreal.EditorAssetLibrary


def fail(message):
    unreal.log_error(f"[US-120 map] {message}")
    raise RuntimeError(message)


def subsystems():
    return (
        unreal.get_editor_subsystem(unreal.EditorActorSubsystem),
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem),
        unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem),
    )


def editor_world():
    return subsystems()[2].get_editor_world()


def dirty_packages():
    packages = (
        unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
        + unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    )
    return [package.get_name() for package in packages]


def finalize_orientation(manifest, meshes):
    """Compensate for FBX handedness while keeping every placed actor unit-scaled.

    Legacy FBX import preserves the named UCX hulls but reflects local Y while
    converting handedness. Geometry Script applies the exact compensating
    reflection to source geometry and authored collision after the unattended
    import. Bounds detection makes this finalizer idempotent.
    """
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    if subsystem is None:
        fail("StaticMeshEditorSubsystem is unavailable")

    transform = unreal.Transform()
    transform.set_editor_property("scale3d", unreal.Vector(1.0, -1.0, 1.0))
    finalized = []
    for entry in manifest["meshes"]:
        mesh = meshes[entry["id"]]
        bounds = mesh.get_bounds()
        actual_min = (
            bounds.origin.x - bounds.box_extent.x,
            bounds.origin.y - bounds.box_extent.y,
            bounds.origin.z - bounds.box_extent.z,
        )
        actual_max = (
            bounds.origin.x + bounds.box_extent.x,
            bounds.origin.y + bounds.box_extent.y,
            bounds.origin.z + bounds.box_extent.z,
        )
        expected_min = entry["bounds_cm"]["min"]
        expected_max = entry["bounds_cm"]["max"]
        already_final = all(
            abs(actual - expected) <= 1.0
            for actual, expected in zip(
                actual_min + actual_max, expected_min + expected_max
            )
        )
        mirrored_min = (
            expected_min[0],
            -expected_max[1],
            expected_min[2],
        )
        mirrored_max = (
            expected_max[0],
            -expected_min[1],
            expected_max[2],
        )
        needs_reflection = not already_final and all(
            abs(actual - expected) <= 1.0
            for actual, expected in zip(
                actual_min + actual_max, mirrored_min + mirrored_max
            )
        )
        if not already_final and not needs_reflection:
            fail(
                f"{entry['id']} is neither catalog-facing nor the expected "
                f"FBX Y reflection: min={actual_min}, max={actual_max}"
            )

        if needs_reflection:
            dynamic_mesh = unreal.DynamicMesh()
            read_lod = unreal.GeometryScriptMeshReadLOD()
            read_lod.set_editor_property(
                "lod_type", unreal.GeometryScriptLODType.SOURCE_MODEL
            )
            read_lod.set_editor_property("lod_index", 0)
            read_options = unreal.GeometryScriptCopyMeshFromAssetOptions()
            read_options.set_editor_property("apply_build_settings", False)
            read_options.set_editor_property("use_build_scale", False)
            dynamic_mesh, read_outcome = (
                unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(
                    mesh, dynamic_mesh, read_options, read_lod
                )
            )
            if read_outcome != unreal.GeometryScriptOutcomePins.SUCCESS:
                fail(f"{entry['id']} source-geometry copy failed: {read_outcome}")
            unreal.GeometryScript_MeshTransforms.transform_mesh(
                dynamic_mesh, transform, True
            )

            write_options = unreal.GeometryScriptCopyMeshToAssetOptions()
            write_options.set_editor_property("use_build_scale", False)
            write_options.set_editor_property("replace_materials", False)
            write_lod = unreal.GeometryScriptMeshWriteLOD()
            write_lod.set_editor_property("lod_index", 0)
            _, write_outcome = (
                unreal.GeometryScript_AssetUtils.copy_mesh_to_static_mesh(
                    dynamic_mesh, mesh, write_options, write_lod, True
                )
            )
            if write_outcome != unreal.GeometryScriptOutcomePins.SUCCESS:
                fail(f"{entry['id']} geometry reflection failed: {write_outcome}")

            collision = (
                unreal.GeometryScript_Collision.get_simple_collision_from_static_mesh(
                    mesh
                )
            )
            collision_options = unreal.GeometryScriptTransformCollisionOptions()
            collision_options.set_editor_property(
                "center_transform_pivot_per_shape", False
            )
            collision, collision_ok = (
                unreal.GeometryScript_Collision.transform_simple_collision_shapes(
                    collision, transform, collision_options
                )
            )
            if not collision_ok:
                fail(f"{entry['id']} collision reflection was not exact")
            unreal.GeometryScript_Collision.set_simple_collision_of_static_mesh(
                collision,
                mesh,
                unreal.GeometryScriptSetSimpleCollisionOptions(),
                unreal.GeometryScriptSetStaticMeshCollisionOptions(),
            )
            if not EAL.save_loaded_asset(mesh, only_if_is_dirty=False):
                fail(f"could not save finalized mesh {entry['id']}")

        bounds = mesh.get_bounds()
        actual_min = (
            bounds.origin.x - bounds.box_extent.x,
            bounds.origin.y - bounds.box_extent.y,
            bounds.origin.z - bounds.box_extent.z,
        )
        actual_max = (
            bounds.origin.x + bounds.box_extent.x,
            bounds.origin.y + bounds.box_extent.y,
            bounds.origin.z + bounds.box_extent.z,
        )
        for bound, actual, expected in zip(
            ("min X", "min Y", "min Z", "max X", "max Y", "max Z"),
            actual_min + actual_max,
            expected_min + expected_max,
        ):
            if abs(actual - expected) > 1.0:
                fail(
                    f"{entry['id']} finalized {bound} {actual:.2f} "
                    f"does not match catalog {expected:.2f}"
                )
        scale = subsystem.get_lod_build_settings(mesh, 0).get_editor_property(
            "build_scale3d"
        )
        if (
            abs(scale.x - 1.0) > 0.001
            or abs(scale.y - 1.0) > 0.001
            or abs(scale.z - 1.0) > 0.001
        ):
            fail(f"{entry['id']} retained non-unit build scale {scale}")
        hull_count = subsystem.get_convex_collision_count(mesh)
        if hull_count != entry["collision"]["count"]:
            fail(
                f"{entry['id']} finalized hull count {hull_count} != "
                f"{entry['collision']['count']}"
            )
        finalized.append(entry["id"])

    if not IMPORT_REPORT.is_file():
        fail(f"import report is missing: {IMPORT_REPORT}")
    report = json.loads(IMPORT_REPORT.read_text(encoding="utf-8"))
    report["editor_finalization"] = {
        "result": "PASS",
        "mesh_count": len(finalized),
        "method": "Geometry Script source geometry and simple collision reflection",
        "build_scale3d": [1.0, 1.0, 1.0],
        "face_or_downhill_local_axis": "-Y",
        "bounds_tolerance_cm": 1.0,
    }
    IMPORT_REPORT.write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    unreal.log(
        f"[US-120 map] PASS orientation: {len(finalized)} meshes finalized to local -Y"
    )


def owned(actor, mesh_ids):
    label = actor.get_actor_label()
    return (
        label.startswith("VW_")
        or label in mesh_ids
        or isinstance(actor, unreal.PlayerStart)
    )


def open_for_rebuild(mesh_ids):
    actor_sub, level_sub, _ = subsystems()
    dirty = dirty_packages()
    if dirty:
        fail(f"unsaved packages before map build: {dirty}")
    if EAL.does_asset_exist(MAP):
        if not level_sub.load_level(MAP):
            fail(f"could not load existing {MAP}")
        actors = [
            actor for actor in actor_sub.get_all_level_actors()
            if owned(actor, mesh_ids)
        ]
        actor_sub.destroy_actors(actors)
        unreal.log(f"[US-120 map] loaded map and removed {len(actors)} owned actors")
    else:
        if not level_sub.new_level(MAP, False):
            fail(f"new_level failed for {MAP}")
        unreal.log(f"[US-120 map] created {MAP}")


def spawn_mesh(label, mesh, location, yaw=0.0):
    actor_sub, _, _ = subsystems()
    actor = actor_sub.spawn_actor_from_object(
        mesh,
        unreal.Vector(*location),
        unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0),
    )
    if actor is None:
        fail(f"could not spawn {label}")
    actor.set_actor_label(label)
    actor.set_actor_scale3d(unreal.Vector(1.0, 1.0, 1.0))
    return actor


def spawn_box(label, box, cube, material, collision=True):
    x0, x1, y0, y1, z0, z1 = box
    bounds = cube.get_bounds()
    extent = bounds.box_extent
    origin = bounds.origin
    scale = unreal.Vector(
        (x1 - x0) / (2.0 * extent.x),
        (y1 - y0) / (2.0 * extent.y),
        (z1 - z0) / (2.0 * extent.z),
    )
    centre = unreal.Vector(
        (x0 + x1) / 2.0 - origin.x * scale.x,
        (y0 + y1) / 2.0 - origin.y * scale.y,
        (z0 + z1) / 2.0 - origin.z * scale.z,
    )
    actor = spawn_mesh(label, cube, (centre.x, centre.y, centre.z))
    actor.set_actor_scale3d(scale)
    actor.static_mesh_component.set_material(0, material)
    if collision:
        actor.static_mesh_component.set_collision_profile_name("BlockAll")
        actor.static_mesh_component.set_collision_enabled(
            unreal.CollisionEnabled.QUERY_AND_PHYSICS
        )
    else:
        actor.static_mesh_component.set_collision_profile_name("NoCollision")
        actor.static_mesh_component.set_collision_enabled(
            unreal.CollisionEnabled.NO_COLLISION
        )
    return actor


def ensure_water_material():
    material = unreal.load_asset(WATER_MATERIAL)
    if material is None:
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        material = asset_tools.create_asset(
            "MI_VW_TestWater",
            "/MordecaiCore/Game/Maps/Fixtures",
            unreal.MaterialInstanceConstant,
            unreal.MaterialInstanceConstantFactoryNew(),
        )
        if material is None:
            fail(f"could not create {WATER_MATERIAL}")
    parent = unreal.load_asset(BASIC_SHAPE_MATERIAL)
    if parent is None:
        fail(f"could not load {BASIC_SHAPE_MATERIAL}")
    material.set_editor_property("parent", parent)
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        material,
        "Color",
        unreal.LinearColor(0.015, 0.18, 0.42, 1.0),
    )
    if not EAL.save_loaded_asset(material, only_if_is_dirty=False):
        fail(f"could not save {WATER_MATERIAL}")
    return material


def spawn_segmented_floor(cube, material):
    x0, x1, y0, y1 = FLOOR_XY
    x_cuts = sorted({x0, x1, *(value for hole in POND_HOLES for value in hole[:2])})
    y_cuts = sorted({y0, y1, *(value for hole in POND_HOLES for value in hole[2:])})
    count = 0
    for min_x, max_x in zip(x_cuts, x_cuts[1:]):
        for min_y, max_y in zip(y_cuts, y_cuts[1:]):
            centre_x = (min_x + max_x) * 0.5
            centre_y = (min_y + max_y) * 0.5
            inside_hole = any(
                hole[0] < centre_x < hole[1] and hole[2] < centre_y < hole[3]
                for hole in POND_HOLES
            )
            if inside_hole:
                continue
            spawn_box(
                f"VW_Floor_{count:02d}",
                (min_x, max_x, min_y, max_y, *FLOOR_Z),
                cube,
                material,
            )
            count += 1
    unreal.log(
        f"[US-120 map] segmented floor: {count} pieces around {len(POND_HOLES)} pond holes"
    )


def spawn_pond_fixtures(meshes, cube, ground_material, water_material):
    # Real 3x3 shallow pond: collision bed at catalog Z=-38, an inset
    # non-colliding water plane at Z=-8, and a closed bank ring. The inset
    # leaves a visible bed border in evidence captures.
    spawn_box(
        "VW_Shallow_Bed",
        (*SHALLOW_HOLE, -100.0, -38.0),
        cube,
        ground_material,
    )
    spawn_box(
        "VW_Shallow_Water",
        (2230.0, 2770.0, -270.0, 270.0, -10.0, -8.0),
        cube,
        water_material,
        collision=False,
    )
    edge = meshes["SM_VW_Pond_Edge_01"]
    # The centre west/east pieces are the labelled LANES actors below.
    for index, y in enumerate((-200.0, 200.0)):
        spawn_mesh(f"VW_Shallow_Bank_W_{index}", edge, (2200.0, y, 0.0), 90.0)
        spawn_mesh(f"VW_Shallow_Bank_E_{index}", edge, (2800.0, y, 0.0), -90.0)
    for index, x in enumerate((2300.0, 2500.0, 2700.0)):
        spawn_mesh(f"VW_Shallow_Bank_S_{index}", edge, (x, -300.0, 0.0), 180.0)
        spawn_mesh(f"VW_Shallow_Bank_N_{index}", edge, (x, 300.0, 0.0), 0.0)
    inner = meshes["SM_VW_Pond_InnerCorner_01"]
    for suffix, location, yaw in (
        ("SW", (2200.0, -300.0, 0.0), 180.0),
        ("NW", (2200.0, 300.0, 0.0), 90.0),
        ("NE", (2800.0, 300.0, 0.0), 0.0),
        ("SE", (2800.0, -300.0, 0.0), -90.0),
    ):
        spawn_mesh(f"VW_Shallow_Bank_Corner_{suffix}", inner, location, yaw)

    # Three independent deep 3x3 ponds isolate the representative bank
    # pieces. Bed and water occupy the open water side; collision support
    # exists only in each piece's authored land-side region.
    for prefix, centre_y in DEEP_FIXTURES:
        hole = (3500.0, 4100.0, centre_y - 300.0, centre_y + 300.0)
        spawn_box(
            f"VW_{prefix}_Bed",
            (*hole, -200.0, -141.0),
            cube,
            ground_material,
        )
        spawn_box(
            f"VW_{prefix}_Water",
            (
                hole[0] + 30.0,
                hole[1] - 30.0,
                hole[2] + 30.0,
                hole[3] - 30.0,
                -10.0,
                -8.0,
            ),
            cube,
            water_material,
            collision=False,
        )

    spawn_box(
        "VW_DeepEdge_Land",
        (3800.0, 4100.0, -1300.0, -700.0, -100.0, 0.0),
        cube,
        ground_material,
    )
    spawn_box(
        "VW_DeepOuterCorner_Land",
        (3800.0, 4100.0, -300.0, 0.0, -100.0, 0.0),
        cube,
        ground_material,
    )
    spawn_box(
        "VW_DeepInnerCorner_LandEast",
        (3800.0, 4100.0, 700.0, 1300.0, -100.0, 0.0),
        cube,
        ground_material,
    )
    spawn_box(
        "VW_DeepInnerCorner_LandSouth",
        (3500.0, 3800.0, 700.0, 1000.0, -100.0, 0.0),
        cube,
        ground_material,
    )


def spawn_text(label, text, location):
    actor_sub, _, _ = subsystems()
    actor = actor_sub.spawn_actor_from_class(
        unreal.TextRenderActor,
        unreal.Vector(*location),
        unreal.Rotator(pitch=50.0, yaw=180.0, roll=0.0),
    )
    if actor is None:
        fail(f"could not spawn text {label}")
    actor.set_actor_label(label)
    component = actor.get_component_by_class(unreal.TextRenderComponent)
    component.set_editor_property("text", text)
    component.set_editor_property("world_size", 24.0)
    component.set_editor_property(
        "horizontal_alignment", unreal.HorizTextAligment.EHTA_CENTER
    )
    component.set_editor_property("text_render_color", unreal.Color(232, 224, 190, 255))
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return actor


def spawn_lights():
    actor_sub, _, _ = subsystems()
    movable = unreal.ComponentMobility.MOVABLE
    sun = actor_sub.spawn_actor_from_class(
        unreal.DirectionalLight,
        unreal.Vector(0.0, 0.0, 1200.0),
        unreal.Rotator(pitch=-50.0, yaw=30.0, roll=0.0),
    )
    sun.set_actor_label("VW_Sun")
    sun_component = sun.get_component_by_class(unreal.DirectionalLightComponent)
    sun_component.set_mobility(movable)
    sun_component.set_editor_property("atmosphere_sun_light", True)
    sky = actor_sub.spawn_actor_from_class(
        unreal.SkyLight, unreal.Vector(0.0, 0.0, 500.0)
    )
    sky.set_actor_label("VW_SkyLight")
    sky_component = sky.get_component_by_class(unreal.SkyLightComponent)
    sky_component.set_mobility(movable)
    sky_component.set_editor_property("real_time_capture", True)
    atmosphere = actor_sub.spawn_actor_from_class(
        unreal.SkyAtmosphere, unreal.Vector()
    )
    atmosphere.set_actor_label("VW_SkyAtmosphere")
    fog = actor_sub.spawn_actor_from_class(
        unreal.ExponentialHeightFog, unreal.Vector()
    )
    fog.set_actor_label("VW_HeightFog")


def build_map():
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    mesh_ids = [entry["id"] for entry in manifest["meshes"]]
    if len(mesh_ids) != 17:
        fail(f"manifest has {len(mesh_ids)} meshes, expected 17")
    meshes = {
        mesh_id: unreal.load_asset(f"{MESH_ROOT}/{mesh_id}")
        for mesh_id in mesh_ids
    }
    missing = [name for name, mesh in meshes.items() if mesh is None]
    if missing:
        fail(f"imported meshes missing: {missing}")

    finalize_orientation(manifest, meshes)
    if not EAL.save_asset(MASTER_MATERIAL, only_if_is_dirty=False):
        fail(f"could not save imported master material {MASTER_MATERIAL}")
    open_for_rebuild(set(mesh_ids))
    actor_sub, level_sub, _ = subsystems()
    world = editor_world()
    settings = world.get_world_settings()
    settings.set_editor_property(
        "default_game_mode", unreal.MordecaiGameMode.static_class()
    )

    cube = unreal.load_asset(CUBE)
    grid = unreal.load_asset(GRID)
    if cube is None or grid is None:
        fail("engine cube or grid material did not load")
    water = ensure_water_material()
    spawn_segmented_floor(cube, grid)
    # Boundary walls keep scripted navigation inside the fixture.
    spawn_box("VW_Wall_W", (-1750.0, -1700.0, -2850.0, 3250.0, 0.0, 300.0), cube, grid)
    spawn_box("VW_Wall_E", (4400.0, 4450.0, -2850.0, 3250.0, 0.0, 300.0), cube, grid)
    spawn_box("VW_Wall_S", (-1750.0, 4450.0, -2850.0, -2800.0, 0.0, 300.0), cube, grid)
    spawn_box("VW_Wall_N", (-1750.0, 4450.0, 3200.0, 3250.0, 0.0, 300.0), cube, grid)

    # Tier-1 support is behind the imported ramp/cliff faces, never in front of them.
    spawn_box("VW_Ramp2T_Tier1", (200.0, 700.0, -900.0, -300.0, 0.0, 200.0), cube, grid)
    spawn_box("VW_Ramp3T_Tier1", (200.0, 700.0, 300.0, 900.0, 0.0, 200.0), cube, grid)
    spawn_box("VW_Cliff_Tier1", (1400.0, 1800.0, -300.0, 300.0, 0.0, 200.0), cube, grid)

    spawn_pond_fixtures(meshes, cube, grid, water)

    for entry, location in zip(manifest["meshes"], GALLERY_LOCATIONS):
        mesh_id = entry["id"]
        spawn_mesh(mesh_id, meshes[mesh_id], location)
        spawn_text(
            f"VW_Label_{mesh_id}",
            mesh_id.replace("SM_VW_", "").replace("_", " "),
            (location[0] - 100.0, location[1], 280.0),
        )

    # Pit samples are geometry-only and fenced off from the gallery walking line.
    spawn_box("VW_PitFence_W", (-200.0, 1000.0, 2250.0, 2300.0, 0.0, 120.0), cube, grid)
    spawn_box("VW_PitFence_E", (-200.0, 1000.0, 2750.0, 2800.0, 0.0, 120.0), cube, grid)
    spawn_box("VW_PitFence_N", (950.0, 1000.0, 2250.0, 2800.0, 0.0, 120.0), cube, grid)
    spawn_box("VW_PitFence_S", (-200.0, -150.0, 2250.0, 2800.0, 0.0, 120.0), cube, grid)

    for label, mesh_id, location, yaw in LANES:
        spawn_mesh(label, meshes[mesh_id], location, yaw)
        spawn_text(
            f"VW_Label_{label}",
            label.replace("VW_Lane_", "").replace("_", " "),
            (location[0] - 120.0, location[1], 300.0),
        )

    start = actor_sub.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(*PLAYER_START), unreal.Rotator()
    )
    if start is None:
        fail("could not spawn PlayerStart")
    start.set_actor_label("PlayerStart")
    spawn_lights()

    if not level_sub.save_current_level():
        fail(f"save_current_level failed for {MAP}")
    if MAP in dirty_packages():
        fail(f"{MAP} remains dirty after save")
    unreal.log(
        "[US-120 map] RESULT PASS: saved L_VW_TileTest; "
        "screen north=+X, east=+Y; straight/ramp face-downhill local axis=-Y"
    )


build_map()
gc.collect()
