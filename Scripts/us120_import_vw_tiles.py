"""Import and save the ready-only Verdant Wilds v3 terrain kit (US-120).

Run unattended:
  UnrealEditor-Cmd.exe ProjectMordecai.uproject -run=pythonscript
      -script=<absolute path to this file> -unattended -nop4 -nosplash

The committed manifest is the allowlist. Raw FBX/PNG files stay in the caller's
temporary staging directory and are never copied into the repository.
"""

import hashlib
import json
import math
import os
import pathlib
import re

import unreal


ROOT = pathlib.Path(unreal.Paths.project_dir()).resolve()
MANIFEST_PATH = ROOT / "Scripts" / "us120_vw_source_manifest.json"
SOURCE_ROOT = pathlib.Path(
    os.environ.get(
        "US120_SOURCE_ROOT",
        pathlib.Path(os.environ["TEMP"]) / "ProjectMordecai-US120" / "sources",
    )
).resolve()
REPORT_PATH = (
    ROOT / "Saved" / "Playtests" / "us120-vw-import-report.json"
)

EAL = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
ASSET_REGISTRY = unreal.AssetRegistryHelpers.get_asset_registry()

COMPRESSION = {
    "Default": unreal.TextureCompressionSettings.TC_DEFAULT,
    "Normalmap": unreal.TextureCompressionSettings.TC_NORMALMAP,
    "Masks": unreal.TextureCompressionSettings.TC_MASKS,
}

LOD_GROUPS = {
    "World": unreal.TextureGroup.TEXTUREGROUP_WORLD,
    "WorldNormalMap": unreal.TextureGroup.TEXTUREGROUP_WORLD_NORMAL_MAP,
}


def log(message):
    unreal.log(f"[US-120] {message}")


def fail(message):
    unreal.log_error(f"[US-120] {message}")
    raise RuntimeError(message)


def digest(path):
    sha1 = hashlib.sha1()
    sha256 = hashlib.sha256()
    size = 0
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            size += len(chunk)
            sha1.update(chunk)
            sha256.update(chunk)
    return size, sha1.hexdigest(), sha256.hexdigest()


def staged_source_md5(path):
    value = hashlib.md5()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(chunk)
    return value.hexdigest()


def source_file_hash(asset):
    # CreateAssetData asks the live object to append its registry tags, so this
    # cannot accidentally read a stale cached tag for the prior on-disk asset.
    asset_data = unreal.AssetRegistryHelpers.create_asset_data(asset)
    value = asset_data.get_tag_value("AssetImportData")
    serialized = str(value or "")
    hashes = re.findall(
        r"FileMD5[^0-9a-fA-F]*([0-9a-fA-F]{32})", serialized,
        flags=re.IGNORECASE,
    )
    if len(hashes) != 1:
        fail(
            f"{asset.get_name()} live AssetImportData tag exposed {len(hashes)} "
            f"MD5 values (serialized length {len(serialized)})"
        )
    return hashes[0]


def verify_import_source(asset, source):
    expected_path = str(source.resolve()).replace("\\", "/").lower()
    import_data = asset.get_editor_property("asset_import_data")
    actual_path = str(import_data.get_first_filename()).replace("\\", "/").lower()
    if actual_path != expected_path:
        fail(f"{asset.get_name()} stored source path {actual_path} != {expected_path}")
    expected_md5 = staged_source_md5(source)
    stored = source_file_hash(asset).lower()
    if expected_md5 not in stored:
        fail(
            f"{asset.get_name()} source MD5 readback mismatch: "
            f"stored source_file_hash does not contain staged_source_md5"
        )


def load_manifest():
    data = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    meshes = data.get("meshes", [])
    textures = data.get("textures", [])
    if len(meshes) != 17 or any(mesh.get("status") != "ready" for mesh in meshes):
        fail("manifest must contain exactly 17 ready meshes")
    if len(textures) != 12:
        fail("manifest must contain exactly 12 sampled textures")
    return data


def verify_sources(manifest):
    entries = [mesh["source"] for mesh in manifest["meshes"]]
    entries.extend(texture["source"] for texture in manifest["textures"])
    for entry in entries:
        source = SOURCE_ROOT.joinpath(*entry["key"].split("/"))
        if not source.is_file():
            fail(f"staged source missing for catalog key {entry['key']}")
        expected = (entry["size"], entry["sha1"], entry["sha256"])
        if digest(source) != expected:
            fail(f"staged source hash mismatch for catalog key {entry['key']}")
    log(f"PASS source hashes: {len(entries)} allowlisted files")


def import_task(source, destination, name, options=None, factory=None):
    asset_path = f"{destination}/{name}"
    # FbxFactory routes an existing asset through ReimportManager, whose
    # handler can retain stale per-asset import options. A deterministic
    # commandlet import is always fresh from the hash-verified staged source.
    if EAL.does_asset_exist(asset_path):
        if not EAL.delete_asset(asset_path):
            fail(f"could not remove prior asset before deterministic import: {asset_path}")
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    if options is not None:
        task.set_editor_property("options", options)
    if factory is not None:
        task.set_editor_property("factory", factory)
    TOOLS.import_asset_tasks([task])
    paths = list(task.get_editor_property("imported_object_paths"))
    expected = f"{destination}/{name}.{name}"
    if expected not in paths:
        fail(
            f"import task did not report expected asset {expected}; "
            f"reported {paths}"
        )
    asset = EAL.load_asset(asset_path)
    if asset is None:
        fail(f"imported asset did not load: {destination}/{name}")
    verify_import_source(asset, source)
    return asset


def import_textures(manifest):
    destination = manifest["import_contract"]["texture_root"]
    EAL.make_directory(destination)
    result = {}
    for entry in manifest["textures"]:
        source = SOURCE_ROOT.joinpath(*entry["source"]["key"].split("/"))
        texture = import_task(
            source, destination, entry["id"], factory=unreal.TextureFactory()
        )
        if not isinstance(texture, unreal.Texture2D):
            fail(f"{entry['id']} did not import as Texture2D")
        texture.set_editor_property("srgb", entry["srgb"])
        texture.set_editor_property(
            "compression_settings", COMPRESSION[entry["compression"]]
        )
        texture.set_editor_property("lod_group", LOD_GROUPS[entry["lod_group"]])
        if not EAL.save_loaded_asset(texture, only_if_is_dirty=False):
            fail(f"save failed for texture {entry['id']}")
        if texture.get_editor_property("srgb") != entry["srgb"]:
            fail(f"{entry['id']} sRGB readback mismatch")
        if (
            texture.get_editor_property("compression_settings")
            != COMPRESSION[entry["compression"]]
        ):
            fail(f"{entry['id']} compression readback mismatch")
        if texture.get_editor_property("lod_group") != LOD_GROUPS[entry["lod_group"]]:
            fail(f"{entry['id']} LOD group readback mismatch")
        result[entry["id"]] = texture
    log(f"PASS textures: {len(result)} imported, configured, and saved")
    return result


def material_expression(material, expression_class, x, y):
    expression = MEL.create_material_expression(material, expression_class, x, y)
    if expression is None:
        fail(f"could not create {expression_class.__name__} in terrain master")
    return expression


def connect(source, output_name, target, input_name):
    if not MEL.connect_material_expressions(source, output_name, target, input_name):
        fail(
            f"material connection failed: {source.get_name()}.{output_name} "
            f"-> {target.get_name()}.{input_name}"
        )


def set_default_texture(expression, parameter, textures, defaults):
    texture_path = defaults.get(parameter)
    texture = textures.get(texture_path)
    if texture is None and texture_path:
        texture = unreal.load_asset(texture_path)
    if texture is None:
        fail(f"missing master material default texture {parameter}: {texture_path}")
    expression.set_editor_property("texture", texture)
    if expression.get_editor_property("texture") != texture:
        fail(f"master material default texture readback failed: {parameter}")


def build_master_material(path, textures, defaults):
    folder, name = path.rsplit("/", 1)
    EAL.make_directory(folder)
    material = unreal.load_asset(path)
    if material is None:
        material = TOOLS.create_asset(
            name, folder, unreal.Material, unreal.MaterialFactoryNew()
        )
    if material is None:
        fail(f"could not create {path}")

    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property(
        "shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT
    )
    material.set_editor_property("used_with_nanite", True)
    MEL.delete_all_material_expressions(material)

    base = material_expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -720, -240
    )
    base.set_editor_property("parameter_name", "BaseColorTexture")
    set_default_texture(base, "BaseColorTexture", textures, defaults)
    tint = material_expression(
        material, unreal.MaterialExpressionVectorParameter, -720, -80
    )
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property(
        "default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
    )
    tinted = material_expression(
        material, unreal.MaterialExpressionMultiply, -440, -180
    )
    connect(base, "RGB", tinted, "A")
    connect(tint, "", tinted, "B")
    MEL.connect_material_property(
        tinted, "", unreal.MaterialProperty.MP_BASE_COLOR
    )

    normal = material_expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -720, 100
    )
    normal.set_editor_property("parameter_name", "NormalTexture")
    set_default_texture(normal, "NormalTexture", textures, defaults)
    normal.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    normal_strength = material_expression(
        material, unreal.MaterialExpressionScalarParameter, -720, 250
    )
    normal_strength.set_editor_property("parameter_name", "NormalStrength")
    normal_strength.set_editor_property("default_value", 1.0)
    flat_normal = material_expression(
        material, unreal.MaterialExpressionConstant3Vector, -470, 100
    )
    flat_normal.set_editor_property(
        "constant", unreal.LinearColor(0.0, 0.0, 1.0, 1.0)
    )
    flatten = material_expression(
        material, unreal.MaterialExpressionLinearInterpolate, -220, 130
    )
    connect(flat_normal, "", flatten, "A")
    connect(normal, "RGB", flatten, "B")
    connect(normal_strength, "", flatten, "Alpha")
    MEL.connect_material_property(
        flatten, "", unreal.MaterialProperty.MP_NORMAL
    )

    orm = material_expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -720, 470
    )
    orm.set_editor_property("parameter_name", "ORMTexture")
    set_default_texture(orm, "ORMTexture", textures, defaults)
    orm.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    roughness_scale = material_expression(
        material, unreal.MaterialExpressionScalarParameter, -460, 610
    )
    roughness_scale.set_editor_property("parameter_name", "RoughnessMultiplier")
    roughness_scale.set_editor_property("default_value", 1.0)
    roughness = material_expression(
        material, unreal.MaterialExpressionMultiply, -220, 530
    )
    connect(orm, "G", roughness, "A")
    connect(roughness_scale, "", roughness, "B")
    MEL.connect_material_property(
        orm, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION
    )
    MEL.connect_material_property(
        roughness, "", unreal.MaterialProperty.MP_ROUGHNESS
    )
    MEL.connect_material_property(
        orm, "B", unreal.MaterialProperty.MP_METALLIC
    )

    MEL.recompile_material(material)
    if not EAL.save_loaded_asset(material, only_if_is_dirty=False):
        fail(f"save failed for {path}")
    if material.get_editor_property("blend_mode") != unreal.BlendMode.BLEND_OPAQUE:
        fail("master material blend mode readback mismatch")
    if (
        material.get_editor_property("shading_model")
        != unreal.MaterialShadingModel.MSM_DEFAULT_LIT
    ):
        fail("master material shading model readback mismatch")
    if not material.get_editor_property("used_with_nanite"):
        fail("master material Nanite usage readback mismatch")
    log(f"PASS opaque Default Lit Nanite master material: {path}")
    return material


def texture_or_none(textures, name):
    return textures.get(name)


def set_texture_parameter(instance, parameter, texture):
    if texture is None:
        fail(
            f"missing texture for material parameter "
            f"{instance.get_name()}.{parameter}"
        )
    MEL.set_material_instance_texture_parameter_value(instance, parameter, texture)
    actual = MEL.get_material_instance_texture_parameter_value(instance, parameter)
    if actual != texture:
        fail(
            f"{instance.get_name()} parameter {parameter} failed readback "
            f"({actual} != {texture})"
        )


def build_material_instances(manifest, master, textures):
    folder = manifest["import_contract"]["material_root"]
    EAL.make_directory(folder)
    instances = {}
    for name, parameter_values in manifest["material_textures"].items():
        path = f"{folder}/{name}"
        instance = unreal.load_asset(path)
        if instance is None:
            instance = TOOLS.create_asset(
                name,
                folder,
                unreal.MaterialInstanceConstant,
                unreal.MaterialInstanceConstantFactoryNew(),
            )
        if instance is None:
            fail(f"could not create {path}")
        MEL.set_material_instance_parent(instance, master)
        MEL.clear_all_material_instance_parameters(instance)
        for parameter, texture_id in parameter_values.items():
            set_texture_parameter(
                instance, parameter, texture_or_none(textures, texture_id)
            )
        if not EAL.save_loaded_asset(instance, only_if_is_dirty=False):
            fail(f"save failed for {path}")
        if instance.get_editor_property("parent") != master:
            fail(f"{name} parent readback mismatch")
        instances[name] = instance
    required = {
        slot
        for mesh in manifest["meshes"]
        for slot in mesh["material_slots"]
    }
    if required != set(instances):
        fail(
            f"material instance allowlist mismatch: required {sorted(required)}, "
            f"built {sorted(instances)}"
        )
    log(f"PASS material instances: {len(instances)} created and saved")
    return instances


def fbx_options():
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property(
        "mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH
    )
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    data = options.get_editor_property("static_mesh_import_data")
    data.set_editor_property("import_uniform_scale", 1.0)
    data.set_editor_property("convert_scene", True)
    data.set_editor_property("convert_scene_unit", True)
    data.set_editor_property("force_front_x_axis", False)
    data.set_editor_property("combine_meshes", False)
    data.set_editor_property("auto_generate_collision", False)
    data.set_editor_property("one_convex_hull_per_ucx", True)
    data.set_editor_property("generate_lightmap_u_vs", False)
    data.set_editor_property("build_nanite", True)
    return options


def mesh_bounds(mesh):
    bounds = mesh.get_bounds()
    return {
        "min": [
            bounds.origin.x - bounds.box_extent.x,
            bounds.origin.y - bounds.box_extent.y,
            bounds.origin.z - bounds.box_extent.z,
        ],
        "max": [
            bounds.origin.x + bounds.box_extent.x,
            bounds.origin.y + bounds.box_extent.y,
            bounds.origin.z + bounds.box_extent.z,
        ],
    }


def bounds_match(actual, expected, tolerance=1.0):
    return all(
        abs(actual_value - expected_value) <= tolerance
        for actual_values, expected_values in (
            (actual["min"], expected["min"]),
            (actual["max"], expected["max"]),
        )
        for actual_value, expected_value in zip(actual_values, expected_values)
    )


def import_meshes(manifest, instances):
    destination = manifest["import_contract"]["mesh_root"]
    EAL.make_directory(destination)
    imported = {}
    for entry in manifest["meshes"]:
        source = SOURCE_ROOT.joinpath(*entry["source"]["key"].split("/"))
        mesh = import_task(
            source,
            destination,
            entry["id"],
            fbx_options(),
            factory=unreal.FbxFactory(),
        )
        if not isinstance(mesh, unreal.StaticMesh):
            fail(f"{entry['id']} did not import as StaticMesh")

        materials = list(mesh.get_editor_property("static_materials"))
        expected_slots = entry["material_slots"]
        imported_slots = [
            str(slot.get_editor_property("imported_material_slot_name"))
            for slot in materials
        ]
        assigned_slots = [
            str(slot.get_editor_property("material_slot_name"))
            for slot in materials
        ]
        if imported_slots != expected_slots:
            fail(
                f"{entry['id']} ImportedMaterialSlotName set {imported_slots} "
                f"differs from manifest FBX ground truth {expected_slots}"
            )
        if assigned_slots != imported_slots:
            fail(
                f"{entry['id']} MaterialSlotName {assigned_slots} differs from "
                f"ImportedMaterialSlotName {imported_slots}"
            )
        for slot, slot_name in zip(materials, imported_slots):
            slot.set_editor_property("material_interface", instances[slot_name])
        mesh.set_editor_property("static_materials", materials)

        nanite = mesh.get_editor_property("nanite_settings")
        nanite.set_editor_property("enabled", True)
        mesh.set_editor_property("nanite_settings", nanite)
        body_setup = mesh.get_editor_property("body_setup")
        if body_setup is None:
            fail(f"{entry['id']} has no BodySetup after import")
        body_setup.set_editor_property(
            "collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_DEFAULT
        )
        actual_bounds = mesh_bounds(mesh)
        expected_bounds = entry["bounds_cm"]
        source_bounds = entry["source_bounds_cm"]
        matches_ue = bounds_match(actual_bounds, expected_bounds)
        matches_source = bounds_match(actual_bounds, source_bounds)
        if not matches_ue and not matches_source:
            fail(
                f"{entry['id']} imported bounds match neither catalog source "
                f"nor converted UE bounds: {actual_bounds}"
            )
        if not matches_ue:
            fail(
                f"{entry['id']} import kept source-space bounds instead of "
                f"the required UE Y-negation"
            )

        import_data = mesh.get_editor_property("asset_import_data")
        import_settings = (
            ("auto_generate_collision", False),
            ("one_convex_hull_per_ucx", True),
            ("generate_lightmap_u_vs", False),
            ("build_nanite", True),
        )
        # Persist the exact task contract as the reimport contract too. The
        # legacy importer can otherwise serialize its class defaults even
        # though the task-level options were used for the fresh import.
        for property_name, expected in import_settings:
            import_data.set_editor_property(property_name, expected)
        for property_name, expected in import_settings:
            actual = import_data.get_editor_property(property_name)
            if actual != expected:
                fail(
                    f"{entry['id']} import setting {property_name} "
                    f"readback {actual} != {expected}"
                )
        # The commandlet has no StaticMeshEditorSubsystem. Exact converted
        # bounds above prove the effective scale here; the GUI map builder and
        # Tier-1 contract independently read SourceModel BuildScale3D.
        if not EAL.save_loaded_asset(mesh, only_if_is_dirty=False):
            fail(f"save failed for mesh {entry['id']}")

        actual_slots = [
            str(item.get_editor_property("material_slot_name"))
            for item in mesh.get_editor_property("static_materials")
        ]
        if actual_slots != expected_slots:
            fail(
                f"{entry['id']} material slot readback {actual_slots} != {expected_slots}"
            )
        if not mesh.get_editor_property("nanite_settings").get_editor_property("enabled"):
            fail(f"{entry['id']} Nanite readback is disabled")
        imported[entry["id"]] = mesh
    log(f"PASS static meshes: {len(imported)} imported, configured, and saved")
    return imported


def dirty_packages():
    packages = (
        unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
        + unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    )
    return [package.get_name() for package in packages]


def collision_hull_extents(mesh):
    collision = (
        unreal.GeometryScript_Collision.get_simple_collision_from_static_mesh(mesh)
    )
    triangulated = unreal.DynamicMesh()
    unreal.GeometryScript_Primitives.append_simple_collision_shapes(
        triangulated,
        unreal.GeometryScriptPrimitiveOptions(),
        unreal.Transform(),
        collision,
        unreal.GeometryScriptSimpleCollisionTriangulationOptions(),
    )
    _, convexes = (
        unreal.GeometryScript_MeshDecomposition.split_mesh_by_components(
            triangulated, None
        )
    )
    result = []
    for index, convex in enumerate(convexes):
        bounds = convex.get_mesh_bounding_box()
        minimum = bounds.min
        maximum = bounds.max
        result.append({
            "index": index,
            "triangle_count": convex.get_triangle_count(),
            "min": [
                minimum.x,
                minimum.y,
                minimum.z,
            ],
            "max": [
                maximum.x,
                maximum.y,
                maximum.z,
            ],
        })
    expected = unreal.GeometryScript_Collision.get_simple_collision_shape_count(
        collision
    )
    if len(result) != expected:
        fail(
            f"{mesh.get_name()} collision triangulated into {len(result)} "
            f"components, expected {expected} simple shapes"
        )
    return result


def visual_overhang(entry, actual_bounds):
    mesh_id = entry["id"]
    if "_InnerCorner_" in mesh_id:
        return max(-actual_bounds["min"][0], actual_bounds["max"][1], 0.0)
    if "_OuterCorner_" in mesh_id:
        return max(-actual_bounds["min"][0], actual_bounds["max"][1], 0.0)
    footprint_max_y = max(rect[3] for rect in entry["footprint"]["rects_cm"])
    return max(actual_bounds["max"][1] - footprint_max_y, 0.0)


def purge_unlisted_assets(folder, expected_names):
    expected_names = set(expected_names)
    removed = []
    ASSET_REGISTRY.scan_paths_synchronous([folder], True, True)
    for object_path in EAL.list_assets(folder, recursive=True, include_folder=False):
        package_path = object_path.split(".", 1)[0]
        name = package_path.rsplit("/", 1)[-1]
        if name in expected_names:
            continue
        if not EAL.delete_asset(package_path):
            fail(f"could not remove unlisted imported asset {package_path}")
        removed.append(name)
    if removed:
        log(f"PASS removed {len(removed)} unlisted assets: {sorted(removed)}")


def write_report(manifest, meshes, textures, instances):
    mesh_entries = {entry["id"]: entry for entry in manifest["meshes"]}
    mesh_measurements = {}
    defects = []
    for mesh_id, mesh in meshes.items():
        entry = mesh_entries[mesh_id]
        bounds = mesh_bounds(mesh)
        overhang = visual_overhang(entry, bounds)
        limit = entry["footprint"]["visual_overhang_limit_cm"]
        hulls = collision_hull_extents(mesh)
        mesh_measurements[mesh_id] = {
            "bounds_cm": bounds,
            "visual_overhang_downhill_cm": overhang,
            "visual_overhang_limit_cm": limit,
            "collision_hulls": hulls,
            "material_slots": [
                {
                    "MaterialSlotName": str(
                        slot.get_editor_property("material_slot_name")
                    ),
                    "ImportedMaterialSlotName": str(
                        slot.get_editor_property("imported_material_slot_name")
                    ),
                }
                for slot in mesh.get_editor_property("static_materials")
            ],
            "source_md5": staged_source_md5(
                SOURCE_ROOT.joinpath(*entry["source"]["key"].split("/"))
            ),
        }
        if overhang > limit:
            defects.append({
                "id": mesh_id,
                "visual_overhang_downhill_cm": overhang,
                "limit_cm": limit,
                "status": "KNOWN ART DEFECT",
            })
    representatives = {
        "cliff": "SM_VW_FeltCliff_Straight_01",
        "pit": "SM_VW_Pit_Edge_01",
        "pond_bank": "SM_VW_Pond_Edge_01",
        "pond_bank_deep": "SM_VW_Pond_Edge_Deep_01",
        "ramp": "SM_VW_FeltCliff_Ramp_2T_01",
    }
    source_axis = manifest["orientation_contract"][
        "source_straight_cliff_face_axis"
    ]
    axis_vectors = {
        "+X": (1.0, 0.0),
        "-X": (-1.0, 0.0),
        "+Y": (0.0, 1.0),
        "-Y": (0.0, -1.0),
    }
    source_vector = axis_vectors[source_axis]
    converted_vector = (source_vector[0], -source_vector[1])
    measured_axis = next(
        name for name, vector in axis_vectors.items() if vector == converted_vector
    )
    measured_yaw_offset = -math.degrees(
        math.atan2(converted_vector[1], converted_vector[0])
    )
    orientation = {}
    for kind, representative in representatives.items():
        entry = mesh_entries[representative]
        actual = mesh_measurements[representative]["bounds_cm"]
        orientation[kind] = {
            "representative": representative,
            "measurement_method": (
                "imported bounds matched source bounds after FBX Y-negation; "
                "source face/downhill axis transformed by the same conversion"
            ),
            "source_face_or_downhill_axis": source_axis,
            "face_or_downhill_axes": [measured_axis],
            "yaw_offset_deg": measured_yaw_offset,
            "source_bounds_cm": entry["source_bounds_cm"],
            "imported_bounds_cm": actual,
            "converted_bounds_match_within_1cm": bounds_match(
                actual, entry["bounds_cm"]
            ),
        }
    report = {
        "result": "PASS",
        "manifest_sha256": hashlib.sha256(MANIFEST_PATH.read_bytes()).hexdigest(),
        "catalog_sha256": manifest["source"]["catalog_sha256"],
        "counts": {
            "meshes": len(meshes),
            "textures": len(textures),
            "material_instances": len(instances),
            "master_materials": 1,
        },
        "mesh_ids": sorted(meshes),
        "texture_ids": sorted(textures),
        "material_instances": sorted(instances),
        "master_material": manifest["import_contract"]["master_material"],
        "orientation_measurements": orientation,
        "yaw_offset_deg": measured_yaw_offset,
        "face_or_downhill_axes": {
            entry["id"]: entry["footprint"]["face_or_downhill_axes"]
            for entry in manifest["meshes"]
        },
        "catalog_slot_mismatch": manifest["catalog_slot_mismatch"],
        "pond_collision_contracts": manifest["pond_collision_contracts"],
        "mesh_measurements": mesh_measurements,
        "known_art_defects": defects,
    }
    REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
    REPORT_PATH.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    log(
        f"PASS report: counts={report['counts']}, "
        f"known overhang art defects={len(defects)}"
    )


def main():
    # Interchange's default tries to sync the Content Browser after each import.
    # Python commandlets have no Content Browser and crash in SyncBrowserToAssets.
    unreal.SystemLibrary.execute_console_command(
        None, "Interchange.FeatureFlags.Import.SyncToBrowser 0"
    )
    # Force the explicit FbxFactory path: Interchange otherwise ignores parts
    # of FbxStaticMeshImportData during a Python commandlet import.
    unreal.SystemLibrary.execute_console_command(
        None, "Interchange.FeatureFlags.Import.FBX 0"
    )
    manifest = load_manifest()
    verify_sources(manifest)
    textures = import_textures(manifest)
    master = build_master_material(
        manifest["import_contract"]["master_material"],
        textures,
        manifest["import_contract"]["master_material_default_textures"],
    )
    instances = build_material_instances(manifest, master, textures)
    meshes = import_meshes(manifest, instances)
    purge_unlisted_assets(
        manifest["import_contract"]["texture_root"], textures.keys()
    )
    EAL.save_directory(
        "/MordecaiCore/World/Terrain", only_if_is_dirty=False, recursive=True
    )
    dirty = [
        package
        for package in dirty_packages()
        if package.startswith("/MordecaiCore/World/Terrain/")
    ]
    if dirty:
        fail(f"terrain packages remain dirty after save: {dirty}")
    write_report(manifest, meshes, textures, instances)
    log("RESULT PASS: Verdant Wilds v3 ready-only terrain kit imported")


try:
    main()
except Exception as exc:
    unreal.log_error(f"[US-120] RESULT FAIL: {exc}")
    raise
