"""Read-only PIE smoke checks for the US-120 Verdant Wilds tile fixture.

Usage:
  py Scripts/run_pie_verify.py --map /MordecaiCore/Game/Maps/L_VW_TileTest --tests spawn,hud,vw_tiles

This verifies saved integration state only. Player traversal and rejection are
accepted exclusively by Scripts/playtests/us120_vw_tiles.py in Tier 4.
"""

import json
import math
import pathlib

import unreal


MAP_NAME = "L_VW_TileTest"
MESH_ROOT = "/MordecaiCore/World/Terrain/VerdantWilds/Meshes"
MANIFEST = (
    pathlib.Path(unreal.Paths.project_dir())
    / "Scripts"
    / "us120_vw_source_manifest.json"
)
def _run():
    passed = 0
    failed = 0

    def check(name, condition, detail=""):
        nonlocal passed, failed
        if condition:
            passed += 1
            print(f"  PASS: {name}" + (f" -- {detail}" if detail else ""))
        else:
            failed += 1
            print(f"  FAIL: {name}" + (f" -- {detail}" if detail else ""))

    def finish():
        print("")
        print(f"RESULT: {passed} passed, {failed} failed")
        if failed:
            raise RuntimeError(
                f"PIE Verdant Wilds tile smoke failed: {failed} failure(s)"
            )
        print("ALL CHECKS PASSED")

    print("=" * 60)
    print("PIE SMOKE TEST: VERDANT WILDS TERRAIN TILES (US-120)")
    print("=" * 60)

    world = unreal.get_editor_subsystem(
        unreal.UnrealEditorSubsystem
    ).get_game_world()
    check("PIE world exists", world is not None)
    if world is None:
        finish()
        return

    map_name = unreal.GameplayStatics.get_current_level_name(world, True)
    if map_name != MAP_NAME:
        print(f"  SKIP: PIE is on {map_name}, not {MAP_NAME}")
        finish()
        return

    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    entries = {entry["id"]: entry for entry in manifest["meshes"]}
    fixture = manifest["test_fixture"]
    check("committed manifest has exactly 17 meshes", len(entries) == 17)

    game_mode = unreal.GameplayStatics.get_game_mode(world)
    check(
        "map uses MordecaiGameMode",
        isinstance(game_mode, unreal.MordecaiGameMode),
        game_mode.get_class().get_name() if game_mode else "none",
    )

    static_actors = list(
        unreal.GameplayStatics.get_all_actors_of_class(
            world, unreal.StaticMeshActor
        )
    )
    by_label = {actor.get_actor_label(): actor for actor in static_actors}
    for mesh_id, entry in entries.items():
        actor = by_label.get(mesh_id)
        check(f"gallery actor {mesh_id} exists", actor is not None)
        if actor is None:
            continue
        component = actor.get_editor_property("static_mesh_component")
        mesh = component.get_editor_property("static_mesh") if component else None
        check(
            f"{mesh_id} uses its exact mesh",
            mesh is not None
            and mesh.get_path_name().split(".")[0] == f"{MESH_ROOT}/{mesh_id}",
            mesh.get_path_name() if mesh else "none",
        )
        scale = actor.get_actor_scale3d()
        check(
            f"{mesh_id} actor scale is unit",
            math.isclose(scale.x, 1.0)
            and math.isclose(scale.y, 1.0)
            and math.isclose(scale.z, 1.0),
            f"({scale.x:g}, {scale.y:g}, {scale.z:g})",
        )
        location = actor.get_actor_location()
        check(
            f"{mesh_id} is on the 200 cm XY grid",
            math.isclose(math.fmod(abs(location.x), 200.0), 0.0, abs_tol=0.1)
            and math.isclose(
                math.fmod(abs(location.y), 200.0), 0.0, abs_tol=0.1
            ),
            f"({location.x:.1f}, {location.y:.1f}, {location.z:.1f})",
        )
        nanite = mesh.get_editor_property("nanite_settings")
        check(
            f"{mesh_id} has Nanite enabled",
            nanite.get_editor_property("enabled"),
        )
        collision = (
            unreal.GeometryScript_Collision.get_simple_collision_from_static_mesh(
                mesh
            )
        )
        hull_count = (
            unreal.GeometryScript_Collision.get_simple_collision_shape_count(
                collision
            )
        )
        check(
            f"{mesh_id} preserves simple collision",
            hull_count == entry["collision"]["count"],
            f"{hull_count} / expected {entry['collision']['count']}",
        )
        slots = [
            str(slot.get_editor_property("material_slot_name"))
            for slot in mesh.get_editor_property("static_materials")
        ]
        imported_slots = [
            str(slot.get_editor_property("imported_material_slot_name"))
            for slot in mesh.get_editor_property("static_materials")
        ]
        check(
            f"{mesh_id} material slots match FBX ground truth",
            slots == entry["material_slots"],
            f"{slots}",
        )
        check(
            f"{mesh_id} imported and assigned slot names remain identical",
            imported_slots == slots,
            f"imported={imported_slots}, assigned={slots}",
        )

    for table_name in ("gallery", "lanes", "shallow_bank_ring"):
        for expected in fixture[table_name]:
            label = expected["label"]
            actor = by_label.get(label)
            check(f"{table_name} exact actor {label} exists", actor is not None)
            if actor is None:
                continue
            component = actor.get_editor_property("static_mesh_component")
            mesh = component.get_editor_property("static_mesh") if component else None
            scale = actor.get_actor_scale3d()
            location = actor.get_actor_location()
            yaw = actor.get_actor_rotation().yaw
            expected_location = expected["location_cm"]
            expected_scale = expected["scale"]
            check(
                f"{label} exact mesh",
                mesh is not None and mesh.get_name() == expected["mesh"],
                mesh.get_name() if mesh else "none",
            )
            check(
                f"{label} exact unit scale",
                all(
                    math.isclose(actual, wanted, abs_tol=0.001)
                    for actual, wanted in zip(
                        (scale.x, scale.y, scale.z), expected_scale
                    )
                ),
                f"({scale.x:g}, {scale.y:g}, {scale.z:g})",
            )
            check(
                f"{label} exact location",
                all(
                    math.isclose(actual, wanted, abs_tol=0.1)
                    for actual, wanted in zip(
                        (location.x, location.y, location.z), expected_location
                    )
                ),
                f"({location.x:.1f}, {location.y:.1f}, {location.z:.1f})",
            )
            check(
                f"{label} exact yaw",
                math.isclose(
                    unreal.MathLibrary.normalized_delta_rotator(
                        unreal.Rotator(yaw=yaw),
                        unreal.Rotator(yaw=expected["yaw_deg"]),
                    ).yaw,
                    0.0,
                    abs_tol=0.1,
                ),
                f"{yaw:.1f} / expected {expected['yaw_deg']:.1f}",
            )

    check(
        "old basin-covering monolithic floor is absent",
        "VW_Floor" not in by_label,
    )
    expected_surfaces = {
        "VW_Shallow_Bed": (-38.0, False),
        "VW_Shallow_Water": (-8.0, True),
        "VW_DeepEdge_Bed": (-141.0, False),
        "VW_DeepEdge_Water": (-8.0, True),
        "VW_DeepOuterCorner_Bed": (-141.0, False),
        "VW_DeepOuterCorner_Water": (-8.0, True),
        "VW_DeepInnerCorner_Bed": (-141.0, False),
        "VW_DeepInnerCorner_Water": (-8.0, True),
    }
    for label, (top_z, no_collision) in expected_surfaces.items():
        actor = by_label.get(label)
        check(f"pond surface {label} exists", actor is not None)
        if actor is None:
            continue
        component = actor.get_editor_property("static_mesh_component")
        bounds_origin, bounds_extent = actor.get_actor_bounds(False)
        actual_top = bounds_origin.z + bounds_extent.z
        check(
            f"{label} top Z matches catalog fixture",
            math.isclose(actual_top, top_z, abs_tol=0.1),
            f"{actual_top:.1f} / expected {top_z:.1f}",
        )
        collision = component.get_collision_enabled()
        expected_collision = (
            unreal.CollisionEnabled.NO_COLLISION
            if no_collision
            else unreal.CollisionEnabled.QUERY_AND_PHYSICS
        )
        check(
            f"{label} collision matches fixture contract",
            collision == expected_collision,
            str(collision),
        )

    for lane in fixture["deep_lanes"]:
        for support in lane["land_supports"]:
            label = support["label"]
            actor = by_label.get(label)
            check(f"{lane['label']} intended land support {label} exists", actor is not None)
            if actor is None:
                continue
            origin, extent = actor.get_actor_bounds(False)
            actual = (
                origin.x - extent.x,
                origin.x + extent.x,
                origin.y - extent.y,
                origin.y + extent.y,
                origin.z - extent.z,
                origin.z + extent.z,
            )
            expected = tuple(support["bounds_cm"])
            check(
                f"{label} stops exactly at its authored land-side face",
                all(
                    math.isclose(value, wanted, abs_tol=0.1)
                    for value, wanted in zip(actual, expected)
                ),
                f"{actual} / expected {expected}",
            )
        for region_index, region in enumerate(lane["water_regions_xy_cm"]):
            water_bounds = (
                region[0],
                region[1],
                region[2],
                region[3],
                lane["bed_top_z_cm"] + 1.0,
                300.0,
            )
            for label, actor in by_label.items():
                component = actor.get_editor_property("static_mesh_component")
                if (
                    component is None
                    or component.get_collision_enabled()
                    == unreal.CollisionEnabled.NO_COLLISION
                    or label in (lane["actor_label"], lane["bed_label"])
                ):
                    continue
                origin, extent = actor.get_actor_bounds(False)
                candidate = (
                    origin.x - extent.x,
                    origin.x + extent.x,
                    origin.y - extent.y,
                    origin.y + extent.y,
                    origin.z - extent.z,
                    origin.z + extent.z,
                )
                overlap_x = min(candidate[1], water_bounds[1]) - max(
                    candidate[0], water_bounds[0]
                )
                overlap_y = min(candidate[3], water_bounds[3]) - max(
                    candidate[2], water_bounds[2]
                )
                overlap_z = min(candidate[5], water_bounds[5]) - max(
                    candidate[4], water_bounds[4]
                )
                check(
                    f"{lane['label']} water region {region_index} has no collision from {label}",
                    not (overlap_x > 0.1 and overlap_y > 0.1 and overlap_z > 0.1),
                    f"candidate={candidate}, water={water_bounds}",
                )

    pond_holes = (
        (2200.0, 2800.0, -300.0, 300.0),
        *(tuple(lane["basin_bounds_cm"][:4]) for lane in fixture["deep_lanes"]),
    )
    floor_segments = [
        actor for label, actor in by_label.items() if label.startswith("VW_Floor_")
    ]
    check("floor is segmented around pond holes", len(floor_segments) > 1)
    for actor in floor_segments:
        bounds_origin, bounds_extent = actor.get_actor_bounds(False)
        min_x = bounds_origin.x - bounds_extent.x
        max_x = bounds_origin.x + bounds_extent.x
        min_y = bounds_origin.y - bounds_extent.y
        max_y = bounds_origin.y + bounds_extent.y
        for index, hole in enumerate(pond_holes):
            overlap_x = min(max_x, hole[1]) - max(min_x, hole[0])
            overlap_y = min(max_y, hole[3]) - max(min_y, hole[2])
            check(
                f"{actor.get_actor_label()} does not cover pond hole {index}",
                not (overlap_x > 0.1 and overlap_y > 0.1),
            )

    blockers = list(
        unreal.GameplayStatics.get_all_actors_of_class(
            world, unreal.BlockingVolume
        )
    )
    check(
        "no supplemental BlockingVolume masks imported collision",
        not blockers,
        f"{[actor.get_actor_label() for actor in blockers]}",
    )

    cliff_band = (1300.0, 1500.0, -100.0, 100.0, 1.0, 250.0)
    for label, actor in by_label.items():
        component = actor.get_editor_property("static_mesh_component")
        if (
            component is None
            or component.get_collision_enabled()
            == unreal.CollisionEnabled.NO_COLLISION
        ):
            continue
        mesh = component.get_editor_property("static_mesh")
        mesh_path = mesh.get_path_name().split(".")[0] if mesh else ""
        if mesh_path.startswith(MESH_ROOT + "/"):
            continue
        origin, extent = actor.get_actor_bounds(False)
        actor_bounds = (
            origin.x - extent.x,
            origin.x + extent.x,
            origin.y - extent.y,
            origin.y + extent.y,
            origin.z - extent.z,
            origin.z + extent.z,
        )
        overlap = (
            min(actor_bounds[1], cliff_band[1])
            - max(actor_bounds[0], cliff_band[0])
            > 0.1
            and min(actor_bounds[3], cliff_band[3])
            - max(actor_bounds[2], cliff_band[2])
            > 0.1
            and min(actor_bounds[5], cliff_band[5])
            - max(actor_bounds[4], cliff_band[4])
            > 0.1
        )
        check(
            f"non-tile collision actor {label} stays out of cliff collision band",
            not overlap,
            f"bounds={actor_bounds}",
        )

    finish()


_run()
del _run
