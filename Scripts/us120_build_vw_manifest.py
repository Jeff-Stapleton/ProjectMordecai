#!/usr/bin/env python3
"""Build US-120's credential-free canonical manifest from staged catalog.json."""

import argparse
import copy
import hashlib
import json
import pathlib


READY_IDS = (
    "SM_VW_FeltCliff_Straight_01",
    "SM_VW_FeltCliff_Straight_02",
    "SM_VW_FeltCliff_Straight_03",
    "SM_VW_FeltCliff_OuterCorner_01",
    "SM_VW_FeltCliff_InnerCorner_01",
    "SM_VW_FeltCliff_EndCap_01",
    "SM_VW_Pit_Edge_01",
    "SM_VW_Pit_OuterCorner_01",
    "SM_VW_Pit_InnerCorner_01",
    "SM_VW_Pond_Edge_01",
    "SM_VW_Pond_OuterCorner_01",
    "SM_VW_Pond_InnerCorner_01",
    "SM_VW_Pond_Edge_Deep_01",
    "SM_VW_Pond_OuterCorner_Deep_01",
    "SM_VW_Pond_InnerCorner_Deep_01",
    "SM_VW_FeltCliff_Ramp_2T_01",
    "SM_VW_FeltCliff_Ramp_3T_01",
)

POND_IDS = frozenset(
    mesh_id for mesh_id in READY_IDS if "_Pond_" in mesh_id
)

MATERIAL_TEXTURES = {
    "MI_VW_Grass": {
        "BaseColorTexture": "T_VW_Grass_D",
        "NormalTexture": "T_VW_Grass_N",
        "ORMTexture": "T_VW_Grass_ORM",
    },
    "MI_VW_CliffFace": {
        "BaseColorTexture": "T_VW_CliffFace_D",
        "NormalTexture": "T_VW_CliffFace_N",
        "ORMTexture": "T_VW_CliffFace_ORM",
    },
    "MI_VW_YarnRoot": {
        "BaseColorTexture": "T_VW_YarnRoot_D",
        "NormalTexture": "T_VW_YarnRoot_N",
        "ORMTexture": "T_VW_YarnRoot_ORM",
    },
    "MI_VW_GrassToPath": {
        "BaseColorTexture": "T_VW_Path_D",
        "NormalTexture": "T_VW_Path_N",
        "ORMTexture": "T_VW_Path_ORM",
    },
}

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
    (800.0, 1600.0, 160.0),
    (800.0, 2400.0, 160.0),
    (800.0, 2600.0, 160.0),
    (1600.0, 1200.0, 160.0),
    (1600.0, 1400.0, 160.0),
    (1600.0, 1600.0, 160.0),
    (1600.0, 2200.0, 0.0),
    (1600.0, 2800.0, 0.0),
)

LANES = (
    ("VW_Lane_Ramp2T", "SM_VW_FeltCliff_Ramp_2T_01", (200.0, -600.0, 0.0), 90.0),
    ("VW_Lane_Ramp3T", "SM_VW_FeltCliff_Ramp_3T_01", (200.0, 600.0, 0.0), 90.0),
    ("VW_Lane_Cliff", "SM_VW_FeltCliff_Straight_01", (1400.0, 0.0, 0.0), 90.0),
    ("VW_Lane_ShallowPond", "SM_VW_Pond_Edge_01", (2200.0, 0.0, 0.0), -90.0),
    ("VW_Lane_ShallowPond_Far", "SM_VW_Pond_Edge_01", (2800.0, 0.0, 0.0), 90.0),
    ("VW_Lane_DeepEdge", "SM_VW_Pond_Edge_Deep_01", (3800.0, -1000.0, 0.0), 90.0),
    ("VW_Lane_DeepOuterCorner", "SM_VW_Pond_OuterCorner_Deep_01", (3800.0, 0.0, 0.0), 0.0),
    ("VW_Lane_DeepInnerCorner", "SM_VW_Pond_InnerCorner_Deep_01", (3800.0, 1000.0, 0.0), 0.0),
)

DEEP_LANES = (
    {
        "label": "edge",
        "actor_label": "VW_Lane_DeepEdge",
        "mesh": "SM_VW_Pond_Edge_Deep_01",
        "basin_bounds_cm": [3500.0, 4100.0, -1300.0, -700.0, -141.0, 0.0],
        "bed_label": "VW_DeepEdge_Bed",
        "bed_bounds_cm": [3500.0, 4100.0, -1300.0, -700.0, -200.0, -141.0],
        "bed_top_z_cm": -141.0,
        "water_label": "VW_DeepEdge_Water",
        "water_bounds_cm": [3530.0, 4070.0, -1270.0, -730.0, -10.0, -8.0],
        "water_regions_xy_cm": [[3500.0, 3800.0, -1300.0, -700.0]],
        "land_supports": [
            {
                "label": "VW_DeepEdge_Land",
                "bounds_cm": [3800.0, 4100.0, -1300.0, -700.0, -100.0, 0.0],
            },
        ],
        "clear_route_xy_cm": [[4200.0, -1400.0], [4200.0, -1000.0]],
    },
    {
        "label": "outer_corner",
        "actor_label": "VW_Lane_DeepOuterCorner",
        "mesh": "SM_VW_Pond_OuterCorner_Deep_01",
        "basin_bounds_cm": [3500.0, 4100.0, -300.0, 300.0, -141.0, 0.0],
        "bed_label": "VW_DeepOuterCorner_Bed",
        "bed_bounds_cm": [3500.0, 4100.0, -300.0, 300.0, -200.0, -141.0],
        "bed_top_z_cm": -141.0,
        "water_label": "VW_DeepOuterCorner_Water",
        "water_bounds_cm": [3530.0, 4070.0, -270.0, 270.0, -10.0, -8.0],
        "water_regions_xy_cm": [
            [3500.0, 3800.0, -300.0, 300.0],
            [3800.0, 4100.0, 0.0, 300.0],
        ],
        "land_supports": [
            {
                "label": "VW_DeepOuterCorner_Land",
                "bounds_cm": [3800.0, 4100.0, -300.0, 0.0, -100.0, 0.0],
            },
        ],
        "clear_route_xy_cm": [[4200.0, -1000.0], [4200.0, -300.0]],
    },
    {
        "label": "inner_corner",
        "actor_label": "VW_Lane_DeepInnerCorner",
        "mesh": "SM_VW_Pond_InnerCorner_Deep_01",
        "basin_bounds_cm": [3500.0, 4100.0, 700.0, 1300.0, -141.0, 0.0],
        "bed_label": "VW_DeepInnerCorner_Bed",
        "bed_bounds_cm": [3500.0, 4100.0, 700.0, 1300.0, -200.0, -141.0],
        "bed_top_z_cm": -141.0,
        "water_label": "VW_DeepInnerCorner_Water",
        "water_bounds_cm": [3530.0, 4070.0, 730.0, 1270.0, -10.0, -8.0],
        "water_regions_xy_cm": [[3500.0, 3800.0, 1000.0, 1300.0]],
        "land_supports": [
            {
                "label": "VW_DeepInnerCorner_LandEast",
                "bounds_cm": [3800.0, 4100.0, 700.0, 1300.0, -100.0, 0.0],
            },
            {
                "label": "VW_DeepInnerCorner_LandSouth",
                "bounds_cm": [3500.0, 3800.0, 700.0, 1000.0, -100.0, 0.0],
            },
        ],
        "clear_route_xy_cm": [[4200.0, 500.0], [4200.0, 880.0]],
    },
)

SHALLOW_BANK_RING = (
    ("VW_Shallow_Bank_S_0", "SM_VW_Pond_Edge_01", (2200.0, -200.0, 0.0), -90.0),
    ("VW_Shallow_Bank_S_1", "SM_VW_Pond_Edge_01", (2200.0, 200.0, 0.0), -90.0),
    ("VW_Shallow_Bank_N_0", "SM_VW_Pond_Edge_01", (2800.0, -200.0, 0.0), 90.0),
    ("VW_Shallow_Bank_N_1", "SM_VW_Pond_Edge_01", (2800.0, 200.0, 0.0), 90.0),
    ("VW_Shallow_Bank_W_0", "SM_VW_Pond_Edge_01", (2300.0, -300.0, 0.0), 0.0),
    ("VW_Shallow_Bank_W_1", "SM_VW_Pond_Edge_01", (2500.0, -300.0, 0.0), 0.0),
    ("VW_Shallow_Bank_W_2", "SM_VW_Pond_Edge_01", (2700.0, -300.0, 0.0), 0.0),
    ("VW_Shallow_Bank_E_0", "SM_VW_Pond_Edge_01", (2300.0, 300.0, 0.0), 180.0),
    ("VW_Shallow_Bank_E_1", "SM_VW_Pond_Edge_01", (2500.0, 300.0, 0.0), 180.0),
    ("VW_Shallow_Bank_E_2", "SM_VW_Pond_Edge_01", (2700.0, 300.0, 0.0), 180.0),
    ("VW_Shallow_Bank_Corner_SW", "SM_VW_Pond_InnerCorner_01", (2200.0, -300.0, 0.0), -90.0),
    ("VW_Shallow_Bank_Corner_NW", "SM_VW_Pond_InnerCorner_01", (2800.0, -300.0, 0.0), 0.0),
    ("VW_Shallow_Bank_Corner_NE", "SM_VW_Pond_InnerCorner_01", (2800.0, 300.0, 0.0), 90.0),
    ("VW_Shallow_Bank_Corner_SE", "SM_VW_Pond_InnerCorner_01", (2200.0, 300.0, 0.0), 180.0),
)

PROPERTY_FIELDS = (
    "height_cm",
    "depth_cm",
    "bed_z",
    "water_z",
    "water_depth_cm",
    "walkable",
    "rise_cm",
    "run_cm",
    "run_tiles",
    "width_cm",
    "slope_deg",
    "max_surface_slope_deg",
)


def ue_bounds(source_bounds):
    return {
        "min": [
            source_bounds["min"][0],
            -source_bounds["max"][1],
            source_bounds["min"][2],
        ],
        "max": [
            source_bounds["max"][0],
            -source_bounds["min"][1],
            source_bounds["max"][2],
        ],
    }


def ue_axis(source_axis):
    if source_axis.endswith("Y"):
        return ("-" if source_axis.startswith("+") else "+") + "Y"
    return source_axis


def ue_rect(source_rect):
    min_x, max_x, min_y, max_y = source_rect
    return [min_x, max_x, -max_y, -min_y]


def source_footprint(asset):
    mesh_id = asset["id"]
    if asset["kind"] == "ramp":
        run_tiles = int(asset["run_tiles"])
        width_cm = float(asset["width_cm"])
        return (
            [[-width_cm / 2.0, width_cm / 2.0, -run_tiles * 200.0, 100.0]],
            [{
                "extent_tiles": [int(width_cm / 200.0), run_tiles],
                "pivot_offset_tiles": [width_cm / 400.0, float(run_tiles)],
                "top_half_row_cm": 100.0,
            }],
            ["+Y"],
            ["-Y"],
        )
    if "_InnerCorner_" in mesh_id:
        return (
            [[-100.0, 100.0, 0.0, 200.0], [0.0, 200.0, -100.0, 100.0]],
            [
                {"extent_tiles": [1, 1], "pivot_offset_tiles": [0.5, 0.0]},
                {"extent_tiles": [1, 1], "pivot_offset_tiles": [0.0, 0.5]},
            ],
            ["+X", "+Y"],
            ["-X", "-Y"],
        )
    if "_OuterCorner_" in mesh_id:
        return (
            [[0.0, 200.0, 0.0, 200.0]],
            [{"extent_tiles": [1, 1], "pivot_offset_tiles": [0.0, 0.0]}],
            ["+X", "+Y"],
            ["-X", "-Y"],
        )
    if "_EndCap_" in mesh_id:
        return (
            [[-100.0, 100.0, 0.0, 400.0]],
            [{"extent_tiles": [1, 2], "pivot_offset_tiles": [0.5, 0.0]}],
            ["+Y"],
            ["-Y"],
        )
    return (
        [[-100.0, 100.0, 0.0, 200.0]],
        [{"extent_tiles": [1, 1], "pivot_offset_tiles": [0.5, 0.0]}],
        ["+Y"],
        ["-Y"],
    )


def footprint(asset, converted_bounds):
    rects, tile_rects, high_axes, face_axes = source_footprint(asset)
    return {
        "catalog_text": asset["footprint_tiles"],
        "rects_cm": [ue_rect(rect) for rect in rects],
        "z_cm": [converted_bounds["min"][2], converted_bounds["max"][2]],
        "tile_rects": tile_rects,
        "pivot_local_cm": [0.0, 0.0, 0.0],
        "high_or_land_axes": [ue_axis(axis) for axis in high_axes],
        "face_or_downhill_axes": [ue_axis(axis) for axis in face_axes],
        "visual_overhang_limit_cm": 15.0,
    }


def land_region(mesh_id):
    if "_Edge_" in mesh_id:
        return {"operator": "half_plane", "y_lt_cm": -15.0}
    if "_OuterCorner_" in mesh_id:
        return {"operator": "and", "x_gt_cm": 15.0, "y_lt_cm": -15.0}
    return {"operator": "or", "x_gt_cm": 15.0, "y_lt_cm": -15.0}


def actor_entry(label, mesh, location, yaw):
    return {
        "label": label,
        "mesh": mesh,
        "location_cm": list(location),
        "yaw_deg": yaw,
        "scale": [1.0, 1.0, 1.0],
    }


def test_fixture(mesh_ids):
    mesh_ids = tuple(mesh_ids)
    mesh_set = set(mesh_ids)
    gallery = [
        actor_entry(mesh_id, mesh_id, location, 0.0)
        for mesh_id, location in zip(READY_IDS, GALLERY_LOCATIONS)
        if mesh_id in mesh_set
    ]
    lanes = [
        actor_entry(*entry)
        for entry in LANES
        if entry[1] in mesh_set
    ]
    bank_ring = [
        actor_entry(*entry)
        for entry in SHALLOW_BANK_RING
        if entry[1] in mesh_set
    ]
    deep_lanes = [
        json.loads(json.dumps(entry))
        for entry in DEEP_LANES
        if entry["mesh"] in mesh_set
    ]
    return {
        "camera_space": {
            "yaw_deg": 0,
            "screen_north_axis": "+X",
            "screen_east_axis": "+Y",
        },
        "gallery": gallery,
        "lanes": lanes,
        "shallow_bank_ring": bank_ring,
        "deep_lanes": deep_lanes,
    }


def source_record(file_entry):
    return {
        "key": file_entry["path"],
        "size": file_entry["size"],
        "sha1": file_entry["sha1"],
        "sha256": file_entry["sha256"],
    }


def build_manifest(catalog, catalog_bytes, expected_ready_ids=READY_IDS):
    expected_ready_ids = tuple(expected_ready_ids)
    kit = next(kit for kit in catalog["kits"] if kit["id"] == "VW_tiles_v3")
    assets_by_id = {asset["id"]: asset for asset in catalog["assets"]}
    ready_ids = tuple(
        asset_id
        for asset_id in kit["asset_ids"]
        if assets_by_id[asset_id]["status"] == "ready"
    )
    if ready_ids != expected_ready_ids:
        raise RuntimeError(
            f"ready asset ids differ from contract: {ready_ids!r} != {expected_ready_ids!r}"
        )

    used_slots = []
    meshes = []
    mismatches = []
    for mesh_id in ready_ids:
        asset = assets_by_id[mesh_id]
        mesh_files = [entry for entry in asset["files"] if entry["role"] == "mesh"]
        if len(mesh_files) != 1:
            raise RuntimeError(f"{mesh_id} must have exactly one mesh file")
        catalog_slots = list(asset["materials"])
        actual_slots = (
            ["MI_VW_Grass", "MI_VW_YarnRoot"]
            if mesh_id in POND_IDS
            else catalog_slots
        )
        if actual_slots != catalog_slots:
            mismatches.append({
                "id": mesh_id,
                "catalog_material_slots": catalog_slots,
                "fbx_material_slots": actual_slots,
                "rationale": "FBX ImportedMaterialSlotName is ground truth",
            })
        for slot in actual_slots:
            if slot not in MATERIAL_TEXTURES:
                raise RuntimeError(f"{mesh_id} has unsupported material slot {slot}")
            if slot not in used_slots:
                used_slots.append(slot)
        source_bounds = copy.deepcopy(asset["bounds_cm"])
        converted_bounds = ue_bounds(source_bounds)
        collision = copy.deepcopy(asset["collision"])
        if mesh_id in POND_IDS:
            collision["corner_land_region"] = land_region(mesh_id)
        properties = {
            field: asset[field] for field in PROPERTY_FIELDS if field in asset
        }
        texture_references = []
        for slot in actual_slots:
            for texture_id in MATERIAL_TEXTURES[slot].values():
                if texture_id not in texture_references:
                    texture_references.append(texture_id)
        meshes.append({
            "id": mesh_id,
            "name": asset["name"],
            "status": asset["status"],
            "kind": asset["kind"],
            "category": asset["category"],
            "version": asset["version"],
            "source": source_record(mesh_files[0]),
            "units": copy.deepcopy(asset["units"]),
            "pivot": asset["pivot"],
            "source_bounds_cm": source_bounds,
            "bounds_cm": converted_bounds,
            "footprint": footprint(asset, converted_bounds),
            "collision": collision,
            "material_slots": actual_slots,
            "texture_references": texture_references,
            "catalog_properties": properties,
        })

    material_textures = {
        slot: copy.deepcopy(MATERIAL_TEXTURES[slot])
        for slot in MATERIAL_TEXTURES
        if slot in used_slots
    }
    texture_ids = []
    for slot in material_textures.values():
        for texture_id in slot.values():
            if texture_id not in texture_ids:
                texture_ids.append(texture_id)
    kit_files = {pathlib.PurePosixPath(entry["path"]).stem: entry
                 for entry in kit["files"]}
    textures = []
    for texture_id in texture_ids:
        if texture_id not in kit_files:
            raise RuntimeError(f"catalog kit files omit sampled texture {texture_id}")
        suffix = texture_id.rsplit("_", 1)[1]
        textures.append({
            "id": texture_id,
            "source": source_record(kit_files[texture_id]),
            "srgb": suffix == "D",
            "compression": {
                "D": "Default",
                "N": "Normalmap",
                "ORM": "Masks",
            }[suffix],
            "lod_group": "WorldNormalMap" if suffix == "N" else "World",
        })

    return {
        "schema_version": 2,
        "source": {
            "bucket": catalog["bucket"],
            "catalog_key": "catalog/catalog.json",
            "catalog_updated_at": catalog["updated_at"],
            "catalog_sha256": hashlib.sha256(catalog_bytes).hexdigest(),
            "kit_id": kit["id"],
            "kit_prefix": kit["prefix"],
            "only_import_status": "ready",
        },
        "import_contract": {
            "units": "cm",
            "grid_cm": 200,
            "asset_root": "/MordecaiCore/World/Terrain/VerdantWilds",
            "mesh_root": "/MordecaiCore/World/Terrain/VerdantWilds/Meshes",
            "texture_root": "/MordecaiCore/World/Terrain/VerdantWilds/Textures",
            "material_root": "/MordecaiCore/World/Terrain/VerdantWilds/Materials",
            "master_material": "/MordecaiCore/World/Terrain/Shared/Materials/M_Terrain_Master",
            "master_material_default_textures": {
                "BaseColorTexture": "/Engine/EngineResources/DefaultTexture",
                "NormalTexture": "/Engine/EngineMaterials/DefaultNormal",
                "ORMTexture": "/Engine/EngineResources/DefaultTexture",
            },
            "map": "/MordecaiCore/Game/Maps/L_VW_TileTest",
            "camera_space": {
                "yaw_deg": 0,
                "screen_north_axis": "+X",
                "screen_east_axis": "+Y",
            },
        },
        "orientation_contract": {
            "coordinate_conversion": "source Y negated to UE Y",
            "bounds_tolerance_cm": 1.0,
            "yaw_offset_deg": -90.0,
            "source_straight_cliff_face_axis": "-Y",
            "source_ramp_2t_downhill_axis": "-Y",
            "straight_cliff_face_axis": "+Y",
            "ramp_2t_downhill_axis": "+Y",
            "by_kind": {
                "cliff": {"face_or_downhill_axes": ["+Y"], "yaw_offset_deg": -90.0},
                "pit": {"face_or_downhill_axes": ["+Y"], "yaw_offset_deg": -90.0},
                "pond_bank": {"face_or_downhill_axes": ["+Y"], "yaw_offset_deg": -90.0},
                "pond_bank_deep": {"face_or_downhill_axes": ["+Y"], "yaw_offset_deg": -90.0},
                "ramp": {"face_or_downhill_axes": ["+Y"], "yaw_offset_deg": -90.0},
            },
        },
        "pond_collision_contracts": {
            "coordinator_decision": "D3 pond amendment, 2026-11-13 18:46 MT; converted to UE space by R1",
            "rationale": "The bank and bed occupy the adjacent water tile; deep pieces carry deliberate blocking walls.",
            "edge_xy_cm": [-100.0, 100.0, -200.0, 200.0],
            "edge_x_tolerance_cm": 15.0,
            "corner_xy_cm": [-200.0, 200.0, -200.0, 200.0],
            "min_z_cm": -200.0,
            "shallow_max_z_cm": 15.0,
            "deep_wall_max_z_cm": 250.0,
            "land_intrusion_limit_cm": 15.0,
        },
        "material_textures": material_textures,
        "catalog_slot_mismatch": mismatches,
        "test_fixture": test_fixture(ready_ids),
        "meshes": meshes,
        "textures": textures,
    }


def canonical_json(manifest):
    return json.dumps(manifest, indent=2, ensure_ascii=False) + "\n"


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("--catalog", required=True, type=pathlib.Path)
    parser.add_argument("--output", required=True, type=pathlib.Path)
    args = parser.parse_args(argv)
    raw = args.catalog.read_bytes()
    manifest = build_manifest(json.loads(raw), raw)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(canonical_json(manifest), encoding="utf-8")
    print(
        f"US-120 manifest: {len(manifest['meshes'])} ready meshes, "
        f"{len(manifest['textures'])} sampled textures"
    )


if __name__ == "__main__":
    main()
