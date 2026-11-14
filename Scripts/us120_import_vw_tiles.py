"""Import and save the ready-only Verdant Wilds v3 terrain kit (US-120).

Run unattended:
  UnrealEditor-Cmd.exe ProjectMordecai.uproject -run=pythonscript
      -script=<absolute path to this file> -unattended -nop4 -nosplash

The committed manifest is the allowlist. Raw FBX/PNG files stay in the caller's
temporary staging directory and are never copied into the repository.
"""

import hashlib
import json
import os
import pathlib
import sys

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

MATERIAL_TEXTURES = {
    "MI_VW_Grass": "T_VW_Grass",
    "MI_VW_CliffFace": "T_VW_CliffFace",
    "MI_VW_GrassToPath": "T_VW_Path",
    "MI_VW_YarnRoot": "T_VW_YarnRoot",
}

COMPRESSION = {
    "Default": unreal.TextureCompressionSettings.TC_DEFAULT,
    "Normalmap": unreal.TextureCompressionSettings.TC_NORMALMAP,
    "Masks": unreal.TextureCompressionSettings.TC_MASKS,
    "Grayscale": unreal.TextureCompressionSettings.TC_GRAYSCALE,
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


def load_manifest():
    data = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    meshes = data.get("meshes", [])
    textures = data.get("textures", [])
    if len(meshes) != 17 or any(mesh.get("status") != "ready" for mesh in meshes):
        fail("manifest must contain exactly 17 ready meshes")
    if len(textures) != 34:
        fail("manifest must contain exactly 34 referenced textures")
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
    if expected not in paths and not EAL.does_asset_exist(f"{destination}/{name}"):
        fail(f"import produced no expected asset {destination}/{name}")
    asset = EAL.load_asset(f"{destination}/{name}")
    if asset is None:
        fail(f"imported asset did not load: {destination}/{name}")
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
        if not EAL.save_loaded_asset(texture, only_if_is_dirty=False):
            fail(f"save failed for texture {entry['id']}")
        if texture.get_editor_property("srgb") != entry["srgb"]:
            fail(f"{entry['id']} sRGB readback mismatch")
        if (
            texture.get_editor_property("compression_settings")
            != COMPRESSION[entry["compression"]]
        ):
            fail(f"{entry['id']} compression readback mismatch")
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


def build_master_material(path):
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
    MEL.delete_all_material_expressions(material)

    base = material_expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -720, -240
    )
    base.set_editor_property("parameter_name", "BaseColorTexture")
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

    height = material_expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -720, 760
    )
    height.set_editor_property("parameter_name", "HeightTexture")
    height.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)
    height_strength = material_expression(
        material, unreal.MaterialExpressionScalarParameter, -450, 820
    )
    height_strength.set_editor_property("parameter_name", "HeightStrength")
    height_strength.set_editor_property("default_value", 0.0)

    MEL.recompile_material(material)
    if not EAL.save_loaded_asset(material, only_if_is_dirty=False):
        fail(f"save failed for {path}")
    log(f"PASS master material: {path}")
    return material


def texture_or_none(textures, name):
    return textures.get(name)


def set_texture_parameter(instance, parameter, texture):
    if texture is None:
        return
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
    for name, texture_prefix in MATERIAL_TEXTURES.items():
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
        set_texture_parameter(
            instance, "BaseColorTexture", texture_or_none(textures, texture_prefix + "_D")
        )
        set_texture_parameter(
            instance, "NormalTexture", texture_or_none(textures, texture_prefix + "_N")
        )
        set_texture_parameter(
            instance, "ORMTexture", texture_or_none(textures, texture_prefix + "_ORM")
        )
        set_texture_parameter(
            instance, "HeightTexture", texture_or_none(textures, texture_prefix + "_H")
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
    data.set_editor_property("build_nanite", True)
    return options


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
        if len(materials) > len(expected_slots):
            fail(
                f"{entry['id']} imported {len(materials)} material slots; "
                f"catalog allows only {len(expected_slots)}"
            )
        while len(materials) < len(expected_slots):
            materials.append(unreal.StaticMaterial())
        for index, slot_name in enumerate(expected_slots):
            slot = materials[index]
            slot.set_editor_property("material_slot_name", slot_name)
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


def write_report(manifest, meshes, textures, instances):
    defects = [
        {
            "id": entry["id"],
            "visual_overhang_downhill_cm": entry["footprint"][
                "visual_overhang_downhill_cm"
            ],
            "limit_cm": entry["footprint"]["visual_overhang_limit_cm"],
            "status": "KNOWN ART DEFECT",
        }
        for entry in manifest["meshes"]
        if entry["footprint"]["visual_overhang_art_defect"]
    ]
    report = {
        "result": "PASS",
        "manifest_sha256": hashlib.sha256(MANIFEST_PATH.read_bytes()).hexdigest(),
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
        "imported_facing": {
            "SM_VW_FeltCliff_Straight_01_face_or_downhill_local_axis": "-Y",
            "SM_VW_FeltCliff_Ramp_2T_01_face_or_downhill_local_axis": "-Y",
        },
        "pond_collision_contracts": manifest["pond_collision_contracts"],
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
    manifest = load_manifest()
    verify_sources(manifest)
    textures = import_textures(manifest)
    master = build_master_material(manifest["import_contract"]["master_material"])
    instances = build_material_instances(manifest, master, textures)
    meshes = import_meshes(manifest, instances)
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
