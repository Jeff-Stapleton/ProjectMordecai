# US-100 (issue #46, T1): create and SAVE the two playtest test maps (run in the editor via remote exec).
#
# stories/in-progress/US-100-recovery-playtest-test-maps.md has the layout; Source/LyraGame/Tests/Level/
# MordecaiTestMapTests.cpp (Mordecai.Level.TestMaps.*) pins the same numbers in the saved .umaps.
# - /MordecaiCore/Game/Maps/MovementTestMap: floor x, y in [-2000, 2000] (top z 0), 100 cm walls on its edges, one
#   PlayerStart at (0, 0, 100), lane L1 (y -800, x -1000..1600) clear, and two steps at +Y: T1_Step_Walk30
#   (x 600..1000, y 800..1200, top z 30: walkable, MaxStepHeight 45) and T1_Step_Jump60 (x 1200..1600, top z 60:
#   only a jump gets on). No enemies, no pickups, no navmesh.
# - /MordecaiCore/Game/Maps/CombatTestMap: floor x in [-1500, 2700], y in [-1600, 1600], walls, the same PlayerStart,
#   TestDummy (BP_MordecaiEnemy_TestDummy) at (600, 600, 100) and TestAttacker (BP_MordecaiEnemy_TestAttacker) at
#   (2100, 0, 100), both yaw 180, and a navmesh built (blocking) and saved in the map. The attacker stands more than
#   its AggroRange (900) + 100 cm from the PlayerStart, the dummy, the dummy's melee spot (440, 600) and shot spot
#   (300, 600), the route corner (440, 0) and the whole Blink lane ((0, -600) to (1100, -600)); only the fight spot
#   (2000, 0), 100 cm -X of it, is in its reach (150 cm, so walk_to's 25 cm tolerance cannot leave the player out of
#   the swing). Run Scripts/us100_01_create_test_enemies.py first: it makes the two Blueprints.
# Both maps: one .umap each (not World Partition: no __ExternalActors__), World Settings GameMode Override =
# MordecaiGameMode and no World Settings experience (AMordecaiGameMode::InitGame then names B_MordecaiArenaExperience
# in every build, as on DevTestMap), Movable lights only and no lighting build (Lumen; *_BuiltData is gitignored but
# cooked from disk), WorldGridMaterial on every box (a measuring grid that is always cooked).
#
# Rerunnable: new_level() fails on an existing map and discards the open world without asking, so an existing map
# is loaded instead and only this script's actors are replaced (labels T1_*, PlayerStart, TestDummy, TestAttacker).
# It saves each map, loads it back from disk to check it, and ends on DevTestMap (run_pie_verify.py plays the open
# map). DevTestMap is never saved. Prints PASS/FAIL lines and a RESULT line, and raises RuntimeError on any FAIL.
#
# Usage (an absolute path: the editor's working directory is not the project root):
#   py Scripts/ue_remote_exec.py "exec(open(r'C:/Users/jeffd/Documents/Gamedev/ProjectMordecai/Scripts/us100_02_create_test_maps.py').read())" --timeout 600
import gc
import math
import os
import time

import unreal

MOVEMENT_MAP = '/MordecaiCore/Game/Maps/MovementTestMap'
COMBAT_MAP = '/MordecaiCore/Game/Maps/CombatTestMap'
DEVTEST_MAP = '/MordecaiCore/Game/Maps/DevTestMap'
DUMMY_BP = '/MordecaiCore/Enemies/BP_MordecaiEnemy_TestDummy'
ATTACKER_BP = '/MordecaiCore/Enemies/BP_MordecaiEnemy_TestAttacker'
CUBE = '/Engine/BasicShapes/Cube'
GRID_MATERIAL = '/Engine/EngineMaterials/WorldGridMaterial'
MORDECAI_CORE_CONTENT = 'Plugins/GameFeatures/MordecaiCore/Content'

OWNED_PREFIX = 'T1_'
PLAYER_START_LABEL = 'PlayerStart'
PLAYER_START = (0.0, 0.0, 100.0)
FLOOR_Z = (-100.0, 0.0)  # a 100 cm slab whose top is the floor, z 0
WALL_Z = (0.0, 100.0)
PAWN_CENTRE_Z = 92.0  # a standing pawn's capsule centre (half height 90) on the floor
TOLERANCE_CM = 1.0
SPOT_TOLERANCE_CM = 5.0
AGGRO_MARGIN_CM = 100.0  # walk_to stops up to 50 cm off a spot, and the player drifts

# (label, (x0, x1, y0, y1)): inner faces on the floor edges, 50 cm past the corners
MOVEMENT_LAYOUT = {
    'floor': (-2000.0, 2000.0, -2000.0, 2000.0),
    'walls': (
        ('T1_Wall_N', (-2050.0, 2050.0, 2000.0, 2050.0)),
        ('T1_Wall_S', (-2050.0, 2050.0, -2050.0, -2000.0)),
        ('T1_Wall_E', (2000.0, 2050.0, -2050.0, 2050.0)),
        ('T1_Wall_W', (-2050.0, -2000.0, -2050.0, 2050.0)),
    ),
    # (label, (x0, x1, y0, y1), top z)
    'steps': (
        ('T1_Step_Walk30', (600.0, 1000.0, 800.0, 1200.0), 30.0),
        ('T1_Step_Jump60', (1200.0, 1600.0, 800.0, 1200.0), 60.0),
    ),
    # Lane L1, where us001 measures walking and sprinting: plain floor from 1 cm up to above a standing head (z 200;
    # the capsule top is ~180)
    'clear': (('lane L1', (-1000.0, 1600.0, -850.0, -750.0)),),
    'enemies': (),
    'nav_bounds': None,
}
COMBAT_LAYOUT = {
    'floor': (-1500.0, 2700.0, -1600.0, 1600.0),
    'walls': (
        ('T1_Wall_N', (-1550.0, 2750.0, 1600.0, 1650.0)),
        ('T1_Wall_S', (-1550.0, 2750.0, -1650.0, -1600.0)),
        ('T1_Wall_E', (2700.0, 2750.0, -1650.0, 1650.0)),
        ('T1_Wall_W', (-1550.0, -1500.0, -1650.0, 1650.0)),
    ),
    'steps': (),
    # The scripted spots and routes stand on open floor (50 cm either side; the fight spot's box stops short of the
    # attacker's capsule, x 2060..2140)
    'clear': (
        ('route to the melee spot', (-50.0, 490.0, -50.0, 650.0)),
        ('route corner to the fight spot', (390.0, 2050.0, -50.0, 50.0)),
        ('Blink lane', (-50.0, 1150.0, -700.0, -500.0)),
    ),
    # (label, Blueprint, location, yaw)
    'enemies': (
        ('TestDummy', DUMMY_BP, (600.0, 600.0, 100.0), 180.0),
        ('TestAttacker', ATTACKER_BP, (2100.0, 0.0, 100.0), 180.0),
    ),
    # (x0, x1, y0, y1), (z0, z1): the whole floor
    'nav_bounds': ((-1500.0, 2700.0, -1600.0, 1600.0), (-100.0, 400.0)),
}
# Paths a script walks; (0, 0) -> (2000, 0) is the one the story requires (AC-100.2). build_navmesh asserts them
# before the map is saved, verify_map again after loading it back
NAV_PATHS = (
    ('PlayerStart -> fight spot', (0.0, 0.0), (2000.0, 0.0)),
    ('PlayerStart -> dummy melee spot', (0.0, 0.0), (440.0, 600.0)),
    ('Blink lane start -> end', (0.0, -600.0), (1100.0, -600.0)),
)
# The attacker must stay Idle at all of these (Mordecai.Level.TestMaps.CombatTestMapAttackerOutsideAggroRangeOfScriptedSpots)
OUT_OF_AGGRO_SPOTS = (
    ('PlayerStart', (0.0, 0.0)),
    ('the TestDummy', (600.0, 600.0)),
    ('the dummy melee spot', (440.0, 600.0)),
    ('the Fireball shot spot', (300.0, 600.0)),
    ('the route corner', (440.0, 0.0)),
) + tuple((f'the Blink lane at x {x:.0f}', (float(x), -600.0)) for x in range(0, 1101, 100))
# 100 cm -X of the attacker: inside its 150 cm swing even when walk_to stops 25 cm off (it was (1980, 0), 120 cm)
FIGHT_SPOT = (2000.0, 0.0)
FIGHT_REACH_CM = 150.0  # BasicSlash's arc radius: the AI attacks from min(AttackRange 200, reach)

EAL = unreal.EditorAssetLibrary


class Checks:
    def __init__(self):
        self.passed = 0
        self.failed = 0

    def check(self, name, condition, detail=''):
        if condition:
            self.passed += 1
            print(f'  PASS: {name}' + (f' -- {detail}' if detail else ''))
        else:
            self.failed += 1
            print(f'  FAIL: {name}' + (f' -- {detail}' if detail else ''))
        return condition


def subsystems():
    return (unreal.get_editor_subsystem(unreal.EditorActorSubsystem),
            unreal.get_editor_subsystem(unreal.LevelEditorSubsystem),
            unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem))


def dirty_packages():
    return [p.get_name() for p in (unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
                                   + unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())]


def map_file(package_path, suffix='.umap'):
    relative = package_path[len('/MordecaiCore/'):]
    return os.path.normpath(os.path.join(unreal.SystemLibrary.get_project_directory(), MORDECAI_CORE_CONTENT,
                                         relative + suffix))


def external_folders(package_path):
    content = os.path.join(unreal.SystemLibrary.get_project_directory(), MORDECAI_CORE_CONTENT)
    relative = package_path[len('/MordecaiCore/'):]
    return [os.path.normpath(os.path.join(content, folder, relative))
            for folder in ('__ExternalActors__', '__ExternalObjects__')]


def editor_world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def is_open(package_path):
    world = editor_world()
    return world is not None and world.get_path_name().startswith(package_path + '.')


def owned(actor):
    label = actor.get_actor_label()
    return (label.startswith(OWNED_PREFIX) or isinstance(actor, unreal.PlayerStart)
            or label in ('TestDummy', 'TestAttacker')
            or actor.get_class().get_name() in ('BP_MordecaiEnemy_TestDummy_C', 'BP_MordecaiEnemy_TestAttacker_C'))


def box_of(actor, colliding_only=True):
    # (x0, x1, y0, y1, z0, z1) of the actor's bounds
    origin, extent = actor.get_actor_bounds(colliding_only)
    return (origin.x - extent.x, origin.x + extent.x, origin.y - extent.y, origin.y + extent.y,
            origin.z - extent.z, origin.z + extent.z)


def boxes_equal(a, b, tolerance=TOLERANCE_CM):
    return all(abs(x - y) <= tolerance for x, y in zip(a, b))


def fmt_box(box):
    return '(' + ', '.join(f'{v:.1f}' for v in box) + ')'


def pawn_distance(a, b):
    return math.dist((a[0], a[1], PAWN_CENTRE_Z), (b[0], b[1], PAWN_CENTRE_Z))


# --- building -----------------------------------------------------------------------------------

def open_for_rebuild(path):
    # An existing map is loaded and loses this script's actors; a missing one is created (new_level saves it)
    _, level_editor, _ = subsystems()
    dirty = dirty_packages()
    if dirty:
        raise RuntimeError(f'unsaved packages {dirty}: save or revert them first (opening another map discards them)')
    if EAL.does_asset_exist(path):
        if not is_open(path) and not level_editor.load_level(path):
            raise RuntimeError(f'could not load {path}')
        actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        mine = [a for a in actor_sub.get_all_level_actors() if owned(a)]
        for actor in mine:
            actor_sub.destroy_actor(actor)
        print(f'{path}: loaded, removed {len(mine)} actor(s) of this script')
    else:
        if not level_editor.new_level(path, False):
            raise RuntimeError(f'new_level failed for {path}')
        print(f'{path}: created')
    if not is_open(path):
        raise RuntimeError(f'{path} is not the open map after loading it')


def spawn_box(label, xy, z, cube, material):
    # A cube scaled to exactly fill the box (x0, x1, y0, y1) x (z0, z1), with the grid material
    actor_sub, _, _ = subsystems()
    (x0, x1, y0, y1), (z0, z1) = xy, z
    origin, extent = cube.get_bounds().origin, cube.get_bounds().box_extent
    scale = unreal.Vector((x1 - x0) / (2 * extent.x), (y1 - y0) / (2 * extent.y), (z1 - z0) / (2 * extent.z))
    centre = unreal.Vector((x0 + x1) / 2 - origin.x * scale.x, (y0 + y1) / 2 - origin.y * scale.y,
                           (z0 + z1) / 2 - origin.z * scale.z)
    actor = actor_sub.spawn_actor_from_object(cube, centre, unreal.Rotator())
    if actor is None:
        raise RuntimeError(f'could not spawn {label}')
    actor.set_actor_scale3d(scale)
    actor.set_actor_label(label)
    actor.static_mesh_component.set_material(0, material)
    return actor


def spawn_lights():
    # Movable only (Lumen; no lighting build). The sun as in DevTestMap's arena: pitch -50, yaw 30
    actor_sub, _, _ = subsystems()
    movable = unreal.ComponentMobility.MOVABLE
    sun = actor_sub.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1000),
                                           unreal.Rotator(pitch=-50.0, yaw=30.0, roll=0.0))
    sun.set_actor_label('T1_Sun')
    sun_light = sun.get_component_by_class(unreal.DirectionalLightComponent)
    sun_light.set_mobility(movable)
    sun_light.set_editor_property('atmosphere_sun_light', True)
    sky = actor_sub.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
    sky.set_actor_label('T1_SkyLight')
    sky_light = sky.get_component_by_class(unreal.SkyLightComponent)
    sky_light.set_mobility(movable)
    sky_light.set_editor_property('real_time_capture', True)
    atmosphere = actor_sub.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    atmosphere.set_actor_label('T1_SkyAtmosphere')
    fog = actor_sub.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0))
    fog.set_actor_label('T1_HeightFog')


def set_game_mode(path):
    world_settings = editor_world().get_world_settings()
    world_settings.set_editor_property('default_game_mode', unreal.MordecaiGameMode.static_class())
    game_mode = world_settings.get_editor_property('default_game_mode')
    if game_mode is None or game_mode.get_name() != 'MordecaiGameMode':
        raise RuntimeError(f'{path} GameMode Override reads {game_mode} after setting MordecaiGameMode')


def spawn_nav_bounds(path, bounds):
    # T1_NavBounds over (x0, x1, y0, y1) x (z0, z1). Keeps no reference to it: a live Python reference to an actor
    # would keep the old world alive through build_navmesh's reload
    actor_sub, _, _ = subsystems()
    (x0, x1, y0, y1), (z0, z1) = bounds
    volume = actor_sub.spawn_actor_from_class(unreal.NavMeshBoundsVolume,
                                              unreal.Vector((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2))
    if volume is None:
        raise RuntimeError(f'{path}: could not spawn a NavMeshBoundsVolume')
    volume.set_actor_label('T1_NavBounds')
    _, extent = volume.get_actor_bounds(False)
    if extent.x <= 0 or extent.y <= 0 or extent.z <= 0:
        raise RuntimeError(f'{path}: the NavMeshBoundsVolume has no brush (extent {extent})')
    volume.set_actor_scale3d(unreal.Vector((x1 - x0) / (2 * extent.x), (y1 - y0) / (2 * extent.y), (z1 - z0) / (2 * extent.z)))


def build_navmesh(path, bounds):
    # Navigation is engine-default RuntimeGeneration=Static: the package never builds a navmesh, so build it here,
    # blocking, and save it in the map (the RecastNavMesh actor).
    # RebuildNavigation does nothing until the world has navigation data (FNavigationSystemExec runs Build() only when
    # NavDataSet is not empty), and a volume spawned in this call only queues a bounds update for the next nav tick.
    # One remote exec is one editor frame, so on a map new_level just made, the command built nothing and the map was
    # saved without a navmesh. So always: save, load the map back (OnWorldInitDone gathers the saved volume's bounds
    # and spawns RecastNavMesh-Default; on a rerun it also drops the destroyed volume's stale bounds), RebuildNavigation
    # (now a blocking Build()), and check it all here, before build_map's final save.
    actor_sub, level_editor, _ = subsystems()
    spawn_nav_bounds(path, bounds)
    if not level_editor.save_current_level():
        raise RuntimeError(f'save_current_level failed for {path} (before building its navmesh)')
    if not level_editor.load_level(path) or not is_open(path):
        raise RuntimeError(f'could not load {path} back to build its navmesh')
    world = editor_world()  # the reload replaced the world: never reuse a reference from before it
    unreal.SystemLibrary.execute_console_command(world, 'RebuildNavigation')

    problems = []
    if unreal.NavigationSystemV1.is_navigation_being_built(world):
        problems.append('navigation is still being built after RebuildNavigation')
    navmeshes = [a.get_name() for a in actor_sub.get_all_level_actors() if isinstance(a, unreal.RecastNavMesh)]
    if not navmeshes:
        problems.append('no RecastNavMesh actor after RebuildNavigation')
    for what, (sx, sy), (ex, ey) in NAV_PATHS:
        nav_path = unreal.NavigationSystemV1.find_path_to_location_synchronously(
            world, unreal.Vector(sx, sy, PAWN_CENTRE_Z), unreal.Vector(ex, ey, PAWN_CENTRE_Z))
        if nav_path is None or not nav_path.is_valid() or nav_path.is_partial():
            problems.append(f'path {what} ({sx:.0f}, {sy:.0f}) -> ({ex:.0f}, {ey:.0f}) is '
                            + ('missing' if nav_path is None else f'valid {nav_path.is_valid()}, partial {nav_path.is_partial()}'))
        else:
            print(f'{path}: navmesh path {what} valid and complete, length {nav_path.get_path_length():.0f} cm')
    if problems:
        raise RuntimeError(f'{path}: navmesh not built before the save: {problems}')
    print(f'{path}: navmesh built ({", ".join(navmeshes)}), not being built, every scripted path valid')


def spawn_player_start(path):
    # Keeps no reference to the actor (build_navmesh reloads the map)
    actor_sub, _, _ = subsystems()
    start = actor_sub.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(*PLAYER_START), unreal.Rotator())
    if start is None:
        raise RuntimeError(f'{path}: could not spawn the PlayerStart')
    start.set_actor_label(PLAYER_START_LABEL)


def place_enemies(enemies):
    actor_sub, _, _ = subsystems()
    for label, blueprint, (x, y, z), yaw in enemies:
        if not EAL.does_asset_exist(blueprint):
            raise RuntimeError(f'{blueprint} not found: run Scripts/us100_01_create_test_enemies.py first')
        enemy_class = EAL.load_blueprint_class(blueprint)
        # Placed with the Blueprint's values: base stats are never written on the instance (N13)
        actor = actor_sub.spawn_actor_from_class(enemy_class, unreal.Vector(x, y, z), unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0))
        if actor is None:
            raise RuntimeError(f'could not place {label}')
        actor.set_actor_label(label)


def build_map(path, layout):
    print('')
    print(f'--- building {path} ---')
    _, level_editor, _ = subsystems()
    open_for_rebuild(path)
    set_game_mode(path)

    # No actor reference outlives its helper: build_navmesh reloads the map
    cube = unreal.load_asset(CUBE)
    grid = unreal.load_asset(GRID_MATERIAL)
    if cube is None or grid is None:
        raise RuntimeError(f'{CUBE} or {GRID_MATERIAL} not found')
    spawn_box('T1_Floor', layout['floor'], FLOOR_Z, cube, grid)
    for label, xy in layout['walls']:
        spawn_box(label, xy, WALL_Z, cube, grid)
    for label, xy, top in layout['steps']:
        spawn_box(label, xy, (0.0, top), cube, grid)
    spawn_lights()
    spawn_player_start(path)
    place_enemies(layout['enemies'])
    if layout['nav_bounds']:
        build_navmesh(path, layout['nav_bounds'])

    if not level_editor.save_current_level():
        raise RuntimeError(f'save_current_level failed for {path}')
    print(f'saved {path}')
    left = [p for p in dirty_packages() if p == path]
    if left:
        raise RuntimeError(f'{path} is still dirty after the save')


# --- verifying (from disk) ----------------------------------------------------------------------

def actors_by_label():
    actor_sub, _, _ = subsystems()
    found = {}
    for actor in actor_sub.get_all_level_actors():
        found.setdefault(actor.get_actor_label(), []).append(actor)
    return found


def pawn_blocking_boxes(actor):
    boxes = []
    for component in actor.get_components_by_class(unreal.PrimitiveComponent):
        if component.get_editor_property('is_editor_only'):
            continue
        if component.get_collision_enabled() == unreal.CollisionEnabled.NO_COLLISION:
            continue
        if component.get_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN) != unreal.CollisionResponseType.ECR_BLOCK:
            continue
        origin, extent, _ = unreal.SystemLibrary.get_component_bounds(component)
        boxes.append((component.get_name(), (origin.x - extent.x, origin.x + extent.x, origin.y - extent.y,
                                             origin.y + extent.y, origin.z - extent.z, origin.z + extent.z)))
    return boxes


def overlaps(a, b):
    return a[0] < b[1] and b[0] < a[1] and a[2] < b[3] and b[2] < a[3] and a[4] < b[5] and b[4] < a[5]


def verify_map(checks, path, layout, built_at):
    print('')
    print(f'--- reading back {path} (loaded from disk) ---')
    actor_sub, level_editor, _ = subsystems()
    checks.check(f'{path} loads', level_editor.load_level(path) and is_open(path))
    world = editor_world()
    name = path.rsplit('/', 1)[1]

    on_disk = map_file(path)
    exists = os.path.isfile(on_disk)
    checks.check(f'{name}.umap saved this run', exists and os.path.getmtime(on_disk) >= built_at - 1,
                 on_disk + (f' ({os.path.getsize(on_disk)} bytes)' if exists else ' missing'))
    built_data = map_file(path + '_BuiltData', '.uasset')
    checks.check(f'{name} has no _BuiltData', not os.path.exists(built_data), built_data)
    externals = [f for f in external_folders(path) if os.path.exists(f)]
    checks.check(f'{name} is one .umap (no external actors)', not externals, f'{externals}')
    world_settings = world.get_world_settings()
    checks.check(f'{name} is not World Partition', world_settings.get_editor_property('world_partition') is None)
    game_mode = world_settings.get_editor_property('default_game_mode')
    checks.check(f'{name} World Settings are LyraWorldSettings', isinstance(world_settings, unreal.LyraWorldSettings),
                 world_settings.get_class().get_name())
    checks.check(f'{name} GameMode Override is MordecaiGameMode', game_mode is not None and game_mode.get_name() == 'MordecaiGameMode',
                 f'{game_mode.get_name() if game_mode else None}')
    try:
        experience = world_settings.get_editor_property('default_gameplay_experience')
        checks.check(f'{name} World Settings name no experience', experience is None, f'{experience}')
    except Exception as e:
        checks.check(f'{name} World Settings experience readable', False, str(e))

    labels = actors_by_label()
    all_actors = actor_sub.get_all_level_actors()

    starts = [a for a in all_actors if isinstance(a, unreal.PlayerStart)]
    if checks.check(f'{name} has exactly one PlayerStart', len(starts) == 1, f'{[a.get_actor_label() for a in starts]}'):
        loc, rot = starts[0].get_actor_location(), starts[0].get_actor_rotation()
        checks.check(f'{name} PlayerStart at (0, 0, 100), yaw 0',
                     math.dist((loc.x, loc.y, loc.z), PLAYER_START) <= TOLERANCE_CM and abs(rot.yaw) <= 0.1,
                     f'({loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f}), yaw {rot.yaw:.1f}')

    def check_box(label, xy, z):
        actors = labels.get(label, [])
        if not checks.check(f'{name} has one {label}', len(actors) == 1, f'{len(actors)} found'):
            return None
        want = (xy[0], xy[1], xy[2], xy[3], z[0], z[1])
        blocking = pawn_blocking_boxes(actors[0])
        got = box_of(actors[0])
        checks.check(f'{name} {label} blocks pawns and spans {fmt_box(want)}', bool(blocking) and boxes_equal(got, want),
                     f'is {fmt_box(got)}')
        component = actors[0].get_component_by_class(unreal.StaticMeshComponent)
        material = component.get_material(0) if component else None
        checks.check(f'{name} {label} has {GRID_MATERIAL}',
                     material is not None and material.get_path_name().split('.')[0] == GRID_MATERIAL,
                     f'{material.get_path_name() if material else None}')
        return actors[0]

    floor = check_box('T1_Floor', layout['floor'], FLOOR_Z)
    for label, xy in layout['walls']:
        check_box(label, xy, WALL_Z)
    for label, xy, top in layout['steps']:
        check_box(label, xy, (0.0, top))

    for what, (x0, x1, y0, y1) in layout['clear']:
        area = (x0, x1, y0, y1, 1.0, 200.0)
        blockers = [f'{a.get_actor_label()}.{component} {fmt_box(box)}' for a in all_actors if a != floor
                    for component, box in pawn_blocking_boxes(a) if overlaps(box, area)]
        checks.check(f'{name} {what} is plain floor (nothing else blocks pawns over x {x0:.0f}..{x1:.0f}, y {y0:.0f}..{y1:.0f})',
                     not blockers, '; '.join(blockers))

    # Lights: movable, no static lighting to build
    for label, component_class in (('T1_Sun', unreal.DirectionalLightComponent), ('T1_SkyLight', unreal.SkyLightComponent)):
        actors = labels.get(label, [])
        component = actors[0].get_component_by_class(component_class) if len(actors) == 1 else None
        mobility = component.get_editor_property('mobility') if component else None
        checks.check(f'{name} {label} is Movable', mobility == unreal.ComponentMobility.MOVABLE, f'{mobility}')
    sun = labels.get('T1_Sun', [None])[0]
    if sun:
        rot = sun.get_actor_rotation()
        checks.check(f'{name} T1_Sun pitch -50, yaw 30', abs(rot.pitch + 50) <= 0.1 and abs(rot.yaw - 30) <= 0.1 and abs(rot.roll) <= 0.1,
                     f'pitch {rot.pitch:.1f}, yaw {rot.yaw:.1f}, roll {rot.roll:.1f}')
    for label in ('T1_SkyAtmosphere', 'T1_HeightFog'):
        checks.check(f'{name} has {label}', len(labels.get(label, [])) == 1)
    captures = [a.get_actor_label() for a in all_actors if isinstance(a, unreal.ReflectionCapture)]
    checks.check(f'{name} has no reflection captures', not captures, f'{captures}')

    # Enemies and pickups
    enemies = [a for a in all_actors if isinstance(a, unreal.MordecaiEnemyCharacter)]
    pickups = [a for a in all_actors if isinstance(a, unreal.MordecaiItemPickup)]
    checks.check(f'{name} has no item pickups', not pickups, f'{[a.get_actor_label() for a in pickups]}')
    checks.check(f'{name} has exactly {len(layout["enemies"])} enemies', len(enemies) == len(layout['enemies']),
                 f'{[a.get_actor_label() for a in enemies]}')
    placed = {}
    for label, blueprint, spot, yaw in layout['enemies']:
        class_name = blueprint.rsplit('/', 1)[1] + '_C'
        matches = [a for a in enemies if a.get_class().get_name() == class_name]
        if not checks.check(f'{name} has exactly one {class_name} ({label})', len(matches) == 1,
                            f'{[a.get_actor_label() for a in matches]}'):
            continue
        enemy = matches[0]
        placed[label] = enemy
        loc = enemy.get_actor_location()
        checks.check(f'{label} stands at ({spot[0]:.0f}, {spot[1]:.0f}, {spot[2]:.0f}), labelled {label}',
                     math.dist((loc.x, loc.y, loc.z), spot) <= SPOT_TOLERANCE_CM and enemy.get_actor_label() == label,
                     f'({loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f}) {enemy.get_actor_label()}')
        defaults = unreal.get_default_object(enemy.get_class())
        overrides = [f'{p} {enemy.get_editor_property(p)} (class {defaults.get_editor_property(p)})'
                     for p in ('default_max_health', 'default_max_posture', 'default_move_speed', 'ai_controller_class',
                               'auto_possess_ai')
                     if enemy.get_editor_property(p) != defaults.get_editor_property(p)]
        checks.check(f'{label} keeps its Blueprint values (no instance override, N13)', not overrides, '; '.join(overrides))

    attacker = placed.get('TestAttacker')
    if attacker is not None:
        aggro = unreal.get_default_object(attacker.get_editor_property('ai_controller_class')).get_editor_property('aggro_range')
        loc = attacker.get_actor_location()
        spot = (loc.x, loc.y)
        for what, other in OUT_OF_AGGRO_SPOTS:
            distance = pawn_distance(spot, other)
            checks.check(f'TestAttacker beyond AggroRange {aggro:.0f} + {AGGRO_MARGIN_CM:.0f} of {what}',
                         distance > aggro + AGGRO_MARGIN_CM, f'{distance:.0f} cm')
        fight = pawn_distance(spot, FIGHT_SPOT)
        checks.check(f'the fight spot ({FIGHT_SPOT[0]:.0f}, {FIGHT_SPOT[1]:.0f}) is in TestAttacker\'s reach',
                     80.0 <= fight <= FIGHT_REACH_CM, f'{fight:.0f} cm')

    # Navigation
    volumes = [a for a in all_actors if isinstance(a, unreal.NavMeshBoundsVolume)]
    navmeshes = [a for a in all_actors if isinstance(a, unreal.RecastNavMesh)]
    if layout['nav_bounds']:
        (x0, x1, y0, y1), (z0, z1) = layout['nav_bounds']
        got = box_of(volumes[0], False) if len(volumes) == 1 else None
        checks.check(f'{name} T1_NavBounds covers the floor {fmt_box((x0, x1, y0, y1, z0, z1))}',
                     got is not None and boxes_equal(got, (x0, x1, y0, y1, z0, z1)), f'{fmt_box(got) if got else volumes}')
        checks.check(f'{name} has a RecastNavMesh', len(navmeshes) >= 1, f'{[a.get_name() for a in navmeshes]}')
        checks.check(f'{name} navigation is not being built', not unreal.NavigationSystemV1.is_navigation_being_built(world))
        for what, (sx, sy), (ex, ey) in NAV_PATHS:
            nav_path = unreal.NavigationSystemV1.find_path_to_location_synchronously(
                world, unreal.Vector(sx, sy, PAWN_CENTRE_Z), unreal.Vector(ex, ey, PAWN_CENTRE_Z))
            valid = nav_path is not None and nav_path.is_valid() and not nav_path.is_partial()
            length = nav_path.get_path_length() if nav_path is not None and nav_path.is_valid() else 0.0
            checks.check(f'{name} navmesh path {what} is valid and complete', valid,
                         f'length {length:.0f} cm, {len(nav_path.path_points) if nav_path else 0} points'
                         + ('' if valid else f', partial {nav_path.is_partial() if nav_path else None}'))
    else:
        checks.check(f'{name} has no NavMeshBoundsVolume', not volumes, f'{[a.get_actor_label() for a in volumes]}')

    dirty = [p for p in dirty_packages() if p == path]
    checks.check(f'{name} is not dirty after loading', not dirty, f'{dirty}')
    print(f'{name} actors: ' + ', '.join(sorted(f'{a.get_actor_label()} ({a.get_class().get_name()})' for a in all_actors)))


def drop_stale_world_refs():
    # A smoke script that kept a PIE actor or world in the editor's Python globals (verify_pie_weapon.py does) keeps
    # that world alive, and the next map load dies with the fatal "World Memory Leaks". Drop those names first
    # (Scripts/run_pie_verify.py does the same before it opens a map).
    def world_bound(value):
        if isinstance(value, (unreal.World, unreal.Level, unreal.Actor, unreal.ActorComponent, unreal.UserWidget,
                              unreal.GameInstance, unreal.Subsystem, unreal.Player)):
            return True
        if isinstance(value, unreal.Object):
            name = value.get_path_name()
            return 'UEDPIE_' in name or ':PersistentLevel' in name or name.startswith('/Temp/')
        if isinstance(value, dict):
            return any(world_bound(v) for v in value.values())
        if isinstance(value, (list, tuple, set, unreal.Array)):
            return any(world_bound(v) for v in value)
        return False

    names = globals()
    stale = [n for n, v in list(names.items()) if not n.startswith('__') and world_bound(v)]
    for name in stale:
        del names[name]
    gc.collect()
    if stale:
        print(f'dropped editor Python globals that held world objects: {sorted(stale)}')


def main():
    checks = Checks()
    print('=' * 60)
    print('US-100: TEST MAPS (MovementTestMap, CombatTestMap)')
    print('=' * 60)
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
        raise RuntimeError('PIE is running: stop it first')
    drop_stale_world_refs()
    built_at = time.time()
    devtest_before = os.path.getmtime(map_file(DEVTEST_MAP))

    try:
        build_map(MOVEMENT_MAP, MOVEMENT_LAYOUT)
        build_map(COMBAT_MAP, COMBAT_LAYOUT)
        verify_map(checks, MOVEMENT_MAP, MOVEMENT_LAYOUT, built_at)
        verify_map(checks, COMBAT_MAP, COMBAT_LAYOUT, built_at)
    finally:
        # run_pie_verify.py plays the open map, and its default suite needs DevTestMap. Never save it here.
        _, level_editor, _ = subsystems()
        dirty = dirty_packages()
        if dirty:
            print(f'unsaved packages before going back to DevTestMap (discarded by the load): {dirty}')
        if not is_open(DEVTEST_MAP) and not level_editor.load_level(DEVTEST_MAP):
            print(f'  FAIL: could not reload {DEVTEST_MAP}')
            checks.failed += 1

    print('')
    checks.check('DevTestMap is the open map again', is_open(DEVTEST_MAP))
    checks.check('DevTestMap.umap was not written', os.path.getmtime(map_file(DEVTEST_MAP)) == devtest_before)
    left = dirty_packages()
    checks.check('no unsaved packages are left', not left, f'{left}')
    print('')
    print(f'RESULT: {"PASS" if checks.failed == 0 else "FAIL"} ({checks.passed} passed, {checks.failed} failed)')
    if checks.failed:
        raise RuntimeError(f'us100_02_create_test_maps: {checks.failed} check(s) failed')


main()
