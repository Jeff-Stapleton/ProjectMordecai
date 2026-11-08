"""Create and verify the isolated issue-16 pickup/projectile playtest map.

Run inside the editor through Scripts/ue_remote_exec.py. The script owns only
PickupProjectileTestMap, saves it as one non-World-Partition .umap, reads it
back from disk, and returns to DevTestMap without saving any existing map.
"""
import math
import os
import time

import unreal

MAP = "/MordecaiCore/Game/Maps/PickupProjectileTestMap"
DEVTEST_MAP = "/MordecaiCore/Game/Maps/DevTestMap"
ITEM = "/MordecaiCore/Items/TestPickups/DA_TestItem_MysteriousAmulet"
MATERIAL = "/MordecaiCore/Items/TestPickups/Materials/MI_TestPickup_MysteriousAmulet"
CUBE = "/Engine/BasicShapes/Cube"
SPHERE = "/Engine/BasicShapes/Sphere"
GRID = "/Engine/EngineMaterials/WorldGridMaterial"
CONTENT = "Plugins/GameFeatures/MordecaiCore/Content"

START = (0.0, 0.0, 100.0)
PICKUP_LOCATION = (400.0, 0.0, 30.0)
FLOOR = (-1000.0, 1000.0, -600.0, 600.0, -100.0, 0.0)
WALLS = (
    ("I16_Wall_N", (-1050.0, 1050.0, 600.0, 650.0, 0.0, 100.0)),
    ("I16_Wall_S", (-1050.0, 1050.0, -650.0, -600.0, 0.0, 100.0)),
    ("I16_Wall_E", (1000.0, 1050.0, -650.0, 650.0, 0.0, 100.0)),
    ("I16_Wall_W", (-1050.0, -1000.0, -650.0, 650.0, 0.0, 100.0)),
)
# Player stops 500 cm left of the pickup. The Fireball centre must clear pickup.x + 175.
LANE_BOX = (-110.0, 585.0, -160.0, 160.0, 30.0, 155.0)

EAL = unreal.EditorAssetLibrary
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
world_sub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)


class Checks:
    def __init__(self):
        self.passed = 0
        self.failed = 0

    def check(self, name, condition, detail=""):
        if condition:
            self.passed += 1
            print(f"  PASS: {name}" + (f" -- {detail}" if detail else ""))
        else:
            self.failed += 1
            print(f"  FAIL: {name}" + (f" -- {detail}" if detail else ""))


def map_file(path):
    relative = path[len("/MordecaiCore/"):]
    return os.path.normpath(os.path.join(
        unreal.SystemLibrary.get_project_directory(), CONTENT, relative + ".umap"))


def world():
    return world_sub.get_editor_world()


def is_open(path):
    return world() is not None and world().get_path_name().startswith(path + ".")


def dirty_packages():
    return [p.get_name() for p in (
        unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
        + unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())]


def spawn_box(label, bounds, mesh, material):
    x0, x1, y0, y1, z0, z1 = bounds
    origin, extent = mesh.get_bounds().origin, mesh.get_bounds().box_extent
    scale = unreal.Vector(
        (x1 - x0) / (2 * extent.x),
        (y1 - y0) / (2 * extent.y),
        (z1 - z0) / (2 * extent.z),
    )
    location = unreal.Vector(
        (x0 + x1) / 2 - origin.x * scale.x,
        (y0 + y1) / 2 - origin.y * scale.y,
        (z0 + z1) / 2 - origin.z * scale.z,
    )
    actor = actor_sub.spawn_actor_from_object(mesh, location)
    if actor is None:
        raise RuntimeError(f"could not spawn {label}")
    actor.set_actor_label(label)
    actor.set_actor_scale3d(scale)
    actor.static_mesh_component.set_material(0, material)


def build():
    dirty = dirty_packages()
    if dirty:
        raise RuntimeError(f"unsaved packages before map authoring: {dirty}")
    if EAL.does_asset_exist(MAP):
        if not level_sub.load_level(MAP):
            raise RuntimeError(f"could not load {MAP}")
        for actor in actor_sub.get_all_level_actors():
            actor_sub.destroy_actor(actor)
    elif not level_sub.new_level(MAP, False):
        raise RuntimeError(f"could not create {MAP}")

    settings = world().get_world_settings()
    settings.set_editor_property("default_game_mode", unreal.MordecaiGameMode.static_class())
    cube, grid = unreal.load_asset(CUBE), unreal.load_asset(GRID)
    item, sphere, material = (
        unreal.load_asset(ITEM), unreal.load_asset(SPHERE), unreal.load_asset(MATERIAL))
    if None in (cube, grid, item, sphere, material):
        raise RuntimeError("one or more required pickup/map assets did not load")

    spawn_box("I16_Floor", FLOOR, cube, grid)
    for label, bounds in WALLS:
        spawn_box(label, bounds, cube, grid)

    start = actor_sub.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(*START))
    start.set_actor_label("PlayerStart")

    pickup = actor_sub.spawn_actor_from_class(
        unreal.MordecaiItemPickup, unreal.Vector(*PICKUP_LOCATION))
    pickup.set_actor_label("TestPickup_MysteriousAmulet")
    pickup.set_editor_property("item_definition", item)
    mesh = pickup.get_editor_property("display_mesh")
    mesh.set_static_mesh(sphere)
    mesh.set_relative_scale3d(unreal.Vector(0.35, 0.35, 0.35))
    mesh.set_material(0, material)

    sun = actor_sub.spawn_actor_from_class(
        unreal.DirectionalLight, unreal.Vector(0, 0, 1000),
        unreal.Rotator(pitch=-50.0, yaw=30.0, roll=0.0))
    sun.set_actor_label("I16_Sun")
    sun.get_component_by_class(unreal.DirectionalLightComponent).set_mobility(
        unreal.ComponentMobility.MOVABLE)
    sky = actor_sub.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
    sky.set_actor_label("I16_SkyLight")
    sky_component = sky.get_component_by_class(unreal.SkyLightComponent)
    sky_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky_component.set_editor_property("real_time_capture", True)
    actor_sub.spawn_actor_from_class(
        unreal.SkyAtmosphere, unreal.Vector()).set_actor_label("I16_SkyAtmosphere")
    actor_sub.spawn_actor_from_class(
        unreal.ExponentialHeightFog, unreal.Vector()).set_actor_label("I16_HeightFog")

    if not level_sub.save_current_level():
        raise RuntimeError(f"save_current_level failed for {MAP}")


def component_box(component):
    origin, extent, _ = unreal.SystemLibrary.get_component_bounds(component)
    return (origin.x - extent.x, origin.x + extent.x,
            origin.y - extent.y, origin.y + extent.y,
            origin.z - extent.z, origin.z + extent.z)


def overlaps(a, b):
    return all((a[i] < b[i + 1] and b[i] < a[i + 1]) for i in (0, 2, 4))


def verify(checks, built_at):
    checks.check("saved map reloads from disk", level_sub.load_level(MAP) and is_open(MAP))
    path = map_file(MAP)
    checks.check("map file was written this run",
                 os.path.isfile(path) and os.path.getmtime(path) >= built_at - 1,
                 f"{path} ({os.path.getsize(path) if os.path.isfile(path) else 0} bytes)")
    settings = world().get_world_settings()
    checks.check("map is one non-World-Partition umap",
                 settings.get_editor_property("world_partition") is None)
    mode = settings.get_editor_property("default_game_mode")
    checks.check("GameMode Override is native MordecaiGameMode",
                 mode is not None and mode.get_name() == "MordecaiGameMode",
                 mode.get_name() if mode else "None")
    checks.check("World Settings name no explicit experience",
                 settings.get_editor_property("default_gameplay_experience") is None)

    actors = actor_sub.get_all_level_actors()
    starts = [a for a in actors if isinstance(a, unreal.PlayerStart)]
    enemies = [a for a in actors if isinstance(a, unreal.MordecaiEnemyCharacter)]
    pickups = [a for a in actors if isinstance(a, unreal.MordecaiItemPickup)]
    start_location = starts[0].get_actor_location() if len(starts) == 1 else None
    checks.check("exactly one PlayerStart at (0, 0, 100), yaw 0",
                 len(starts) == 1
                 and math.dist(
                     (start_location.x, start_location.y, start_location.z), START) <= 1.0
                 and abs(starts[0].get_actor_rotation().yaw) <= 0.1)
    checks.check("map has no enemies", not enemies, repr([a.get_actor_label() for a in enemies]))
    checks.check("map has exactly one native AMordecaiItemPickup", len(pickups) == 1
                 and pickups[0].get_class() == unreal.MordecaiItemPickup.static_class(),
                 repr([(a.get_actor_label(), a.get_class().get_name()) for a in pickups]))

    if len(pickups) == 1:
        pickup = pickups[0]
        location = pickup.get_actor_location()
        item = pickup.get_editor_property("item_definition")
        trigger = pickup.get_editor_property("pickup_trigger")
        mesh = pickup.get_editor_property("display_mesh")
        checks.check("pickup label, location and item provenance are exact",
                     pickup.get_actor_label() == "TestPickup_MysteriousAmulet"
                     and math.dist((location.x, location.y, location.z), PICKUP_LOCATION) <= 1.0
                     and item.get_path_name().split(".")[0] == ITEM,
                     f"{pickup.get_actor_label()} at {location}; {item.get_path_name() if item else None}")
        # Python does not expose unnamed game trace channel 7; the Tier 1 map test pins that
        # response directly. WorldDynamic is also ignored by the native ignore-all/overlap-Pawn setup.
        checks.check("pickup trigger is 120 cm QueryOnly; Pawn overlap; WorldDynamic ignore",
                     abs(trigger.get_unscaled_sphere_radius() - 120.0) <= 0.01
                     and trigger.get_collision_enabled() == unreal.CollisionEnabled.QUERY_ONLY
                     and trigger.get_collision_response_to_channel(
                         unreal.CollisionChannel.ECC_PAWN) == unreal.CollisionResponseType.ECR_OVERLAP
                     and trigger.get_collision_response_to_channel(
                         unreal.CollisionChannel.ECC_WORLD_DYNAMIC) == unreal.CollisionResponseType.ECR_IGNORE)
        checks.check("pickup mesh has no collision and uses the committed sphere/material",
                     mesh.get_collision_enabled() == unreal.CollisionEnabled.NO_COLLISION
                     and mesh.get_editor_property("static_mesh").get_path_name().split(".")[0] == SPHERE
                     and mesh.get_material(0).get_path_name().split(".")[0] == MATERIAL)

    blockers = []
    for actor in actors:
        if actor in starts or actor in pickups:
            continue
        for component in actor.get_components_by_class(unreal.PrimitiveComponent):
            if component.get_collision_enabled() == unreal.CollisionEnabled.NO_COLLISION:
                continue
            box = component_box(component)
            if overlaps(box, LANE_BOX):
                blockers.append(f"{actor.get_actor_label()}.{component.get_name()} {box}")
    checks.check("Fireball lane has no colliding utility component", not blockers, "; ".join(blockers))
    checks.check("saved map is clean after reload", MAP not in dirty_packages(), repr(dirty_packages()))
    print("actors: " + ", ".join(sorted(
        f"{a.get_actor_label()} ({a.get_class().get_name()})" for a in actors)))


def main():
    checks = Checks()
    if world_sub.get_game_world():
        raise RuntimeError("PIE is running")
    built_at = time.time()
    devtest_time = os.path.getmtime(map_file(DEVTEST_MAP))
    try:
        build()
        verify(checks, built_at)
    finally:
        if not level_sub.load_level(DEVTEST_MAP):
            checks.failed += 1
            print(f"  FAIL: could not reload {DEVTEST_MAP}")
    checks.check("DevTestMap remained byte-for-byte timestamp untouched",
                 os.path.getmtime(map_file(DEVTEST_MAP)) == devtest_time)
    checks.check("no unsaved package remains", not dirty_packages(), repr(dirty_packages()))
    print(f"RESULT: {'PASS' if not checks.failed else 'FAIL'} "
          f"({checks.passed} passed, {checks.failed} failed)")
    if checks.failed:
        raise RuntimeError(f"{checks.failed} isolated-map authoring check(s) failed")


main()
