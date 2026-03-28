"""US-054 Step 4: Set up DevTestMap arena.

AC-054.10: Flat arena floor ~2000x2000, walls/boundaries, lighting
AC-054.11: 3 Frontliner enemies at varied positions
"""
import math
import unreal

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
EAL = unreal.EditorAssetLibrary

# Load the DevTestMap
map_path = '/MordecaiCore/Game/Maps/DevTestMap'
if not EAL.does_asset_exist(map_path):
    unreal.log_error(f'DevTestMap not found at {map_path}')
    print('ERROR: DevTestMap not found')
else:
    level_sub.load_level(map_path)
    unreal.log(f'Loaded: {map_path}')

# Clean up existing arena actors (if re-running)
for actor in actor_sub.get_all_level_actors():
    label = actor.get_actor_label()
    if label and (label.startswith('Arena_') or label.startswith('Frontliner_')):
        actor_sub.destroy_actor(actor)

# --- Arena Floor ---
with unreal.ScopedEditorTransaction('Arena Floor'):
    floor_mesh = unreal.load_asset('/Engine/BasicShapes/Plane')
    if floor_mesh:
        floor = actor_sub.spawn_actor_from_object(floor_mesh, unreal.Vector(0, 0, 0))
        # Plane is 100x100 by default; scale to 2000x2000
        floor.set_actor_scale3d(unreal.Vector(20, 20, 1))
        floor.set_actor_label('Arena_Floor')
        unreal.log('Created arena floor 2000x2000')
    else:
        unreal.log_warning('Could not load Plane mesh, trying Cube for floor')
        cube = unreal.load_asset('/Engine/BasicShapes/Cube')
        floor = actor_sub.spawn_actor_from_object(cube, unreal.Vector(0, 0, -25))
        floor.set_actor_scale3d(unreal.Vector(20, 20, 0.5))
        floor.set_actor_label('Arena_Floor')

# --- Boundary Walls (4 sides) ---
with unreal.ScopedEditorTransaction('Arena Walls'):
    cube = unreal.load_asset('/Engine/BasicShapes/Cube')
    if cube:
        wall_height = 200
        wall_thickness = 50
        arena_half = 1000  # 2000/2

        walls = [
            ('Arena_Wall_N', unreal.Vector(0, arena_half, wall_height/2), unreal.Vector(20, 0.5, wall_height/100)),
            ('Arena_Wall_S', unreal.Vector(0, -arena_half, wall_height/2), unreal.Vector(20, 0.5, wall_height/100)),
            ('Arena_Wall_E', unreal.Vector(arena_half, 0, wall_height/2), unreal.Vector(0.5, 20, wall_height/100)),
            ('Arena_Wall_W', unreal.Vector(-arena_half, 0, wall_height/2), unreal.Vector(0.5, 20, wall_height/100)),
        ]
        for label, loc, scale in walls:
            wall = actor_sub.spawn_actor_from_object(cube, loc)
            wall.set_actor_scale3d(scale)
            wall.set_actor_label(label)
        unreal.log('Created 4 boundary walls')

# --- Lighting ---
with unreal.ScopedEditorTransaction('Arena Lighting'):
    # Directional light (sun)
    sun = actor_sub.spawn_actor_from_class(
        unreal.DirectionalLight,
        unreal.Vector(0, 0, 1000),
        unreal.Rotator(-50, 30, 0)
    )
    sun.set_actor_label('Arena_Sun')

    # Sky light for ambient
    sky = actor_sub.spawn_actor_from_class(
        unreal.SkyLight,
        unreal.Vector(0, 0, 500)
    )
    sky.set_actor_label('Arena_SkyLight')

    # Sky atmosphere
    try:
        atmo = actor_sub.spawn_actor_from_class(
            unreal.SkyAtmosphere,
            unreal.Vector(0, 0, 0)
        )
        atmo.set_actor_label('Arena_Atmosphere')
    except Exception:
        unreal.log_warning('SkyAtmosphere not available')

    unreal.log('Created lighting setup')

# --- Player Start ---
with unreal.ScopedEditorTransaction('Player Start'):
    # Check if a player start already exists
    has_player_start = False
    for actor in actor_sub.get_all_level_actors():
        if actor.get_class().get_name() == 'PlayerStart':
            has_player_start = True
            break

    if not has_player_start:
        ps = actor_sub.spawn_actor_from_class(
            unreal.PlayerStart,
            unreal.Vector(0, 0, 100)
        )
        ps.set_actor_label('Arena_PlayerStart')
        unreal.log('Created PlayerStart')
    else:
        unreal.log('PlayerStart already exists, skipping')

# --- NavMesh Bounds Volume (for AI pathfinding) ---
with unreal.ScopedEditorTransaction('NavMesh'):
    try:
        nav = actor_sub.spawn_actor_from_class(
            unreal.NavMeshBoundsVolume,
            unreal.Vector(0, 0, 100)
        )
        nav.set_actor_label('Arena_NavBounds')
        nav.set_actor_scale3d(unreal.Vector(25, 25, 5))
        unreal.log('Created NavMeshBoundsVolume')
    except Exception:
        unreal.log_warning('NavMeshBoundsVolume not available')

# --- AC-054.11: Place 3 Frontliner Enemies ---
enemy_bp_path = '/MordecaiCore/Enemies/BP_MordecaiEnemy_Frontliner'
if EAL.does_asset_exist(enemy_bp_path):
    with unreal.ScopedEditorTransaction('Place Enemies'):
        enemy_bp = unreal.load_asset(enemy_bp_path)
        if enemy_bp:
            enemy_positions = [
                unreal.Vector(600, 0, 100),      # Front center
                unreal.Vector(-400, 500, 100),    # Back left
                unreal.Vector(-400, -500, 100),   # Back right
            ]
            for i, pos in enumerate(enemy_positions):
                enemy = actor_sub.spawn_actor_from_object(enemy_bp, pos)
                if enemy:
                    enemy.set_actor_label(f'Frontliner_{i+1}')
                    unreal.log(f'Placed Frontliner_{i+1} at {pos}')
                else:
                    unreal.log_error(f'Failed to spawn Frontliner at {pos}')
        else:
            unreal.log_error('Could not load enemy BP')
else:
    unreal.log_error(f'Enemy BP not found at {enemy_bp_path} - run step 2 first')

# Save the level
level_sub.save_current_level()
unreal.log('US-054 Step 4 complete: Arena set up with 3 enemies.')
print('SUCCESS: DevTestMap arena configured with floor, walls, lighting, and 3 enemies.')
