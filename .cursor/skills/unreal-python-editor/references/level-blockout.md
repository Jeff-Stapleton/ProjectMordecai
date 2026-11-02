# Level Blockout Workflows

Patterns for blocking out level designs with Python — geometry, lighting, volumes, and layout.

## Grid-Based Placement

```python
import unreal

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Place a grid of actors
GRID_SIZE = 5
SPACING = 300  # units

mesh = unreal.load_asset('/Engine/BasicShapes/Cube')

with unreal.ScopedEditorTransaction("Grid Blockout"):
    for x in range(GRID_SIZE):
        for y in range(GRID_SIZE):
            loc = unreal.Vector(x * SPACING, y * SPACING, 0)
            actor = actor_sub.spawn_actor_from_object(mesh, loc)
            actor.set_actor_label(f'Floor_{x}_{y}')
            # Scale to create floor tiles
            actor.set_actor_scale3d(unreal.Vector(3, 3, 0.1))
```

## Room Layout Helper

```python
def make_wall(label, location, rotation, scale):
    """Spawn a scaled cube as a wall segment."""
    mesh = unreal.load_asset('/Engine/BasicShapes/Cube')
    actor = actor_sub.spawn_actor_from_object(mesh, location, rotation)
    actor.set_actor_scale3d(scale)
    actor.set_actor_label(label)
    return actor

def make_room(name, origin, width, depth, height, wall_thickness=20):
    """Create a simple box room from wall segments."""
    w, d, h, t = width, depth, height, wall_thickness
    ox, oy, oz = origin.x, origin.y, origin.z

    with unreal.ScopedEditorTransaction(f"Room: {name}"):
        # Floor
        make_wall(f'{name}_Floor',
            unreal.Vector(ox, oy, oz),
            unreal.Rotator(0,0,0),
            unreal.Vector(w/100, d/100, t/100))

        # Ceiling
        make_wall(f'{name}_Ceiling',
            unreal.Vector(ox, oy, oz + h),
            unreal.Rotator(0,0,0),
            unreal.Vector(w/100, d/100, t/100))

        # Walls (4 sides)
        # North
        make_wall(f'{name}_Wall_N',
            unreal.Vector(ox, oy + d/2, oz + h/2),
            unreal.Rotator(0,0,0),
            unreal.Vector(w/100, t/100, h/100))
        # South
        make_wall(f'{name}_Wall_S',
            unreal.Vector(ox, oy - d/2, oz + h/2),
            unreal.Rotator(0,0,0),
            unreal.Vector(w/100, t/100, h/100))
        # East
        make_wall(f'{name}_Wall_E',
            unreal.Vector(ox + w/2, oy, oz + h/2),
            unreal.Rotator(0,0,0),
            unreal.Vector(t/100, d/100, h/100))
        # West
        make_wall(f'{name}_Wall_W',
            unreal.Vector(ox - w/2, oy, oz + h/2),
            unreal.Rotator(0,0,0),
            unreal.Vector(t/100, d/100, h/100))

# Usage:
make_room('MainHall', unreal.Vector(0,0,0), 2000, 3000, 500)
```

## Corridor / Hallway

```python
def make_corridor(name, start, end, width=300, height=400):
    """Create a corridor between two points."""
    mid = (start + end) / 2
    diff = end - start
    length = diff.length()
    yaw = unreal.MathLibrary.atan2(diff.y, diff.x) * 180 / 3.14159

    with unreal.ScopedEditorTransaction(f"Corridor: {name}"):
        # Floor
        make_wall(f'{name}_Floor', mid, unreal.Rotator(0, yaw, 0),
                  unreal.Vector(length/100, width/100, 0.1))
        # Left wall
        left_offset = unreal.Vector(-diff.y, diff.x, 0)
        left_offset.normalize()
        left_offset *= width/2
        make_wall(f'{name}_Wall_L',
            mid + left_offset + unreal.Vector(0,0,height/2),
            unreal.Rotator(0, yaw, 0),
            unreal.Vector(length/100, 0.1, height/100))
        # Right wall
        make_wall(f'{name}_Wall_R',
            mid - left_offset + unreal.Vector(0,0,height/2),
            unreal.Rotator(0, yaw, 0),
            unreal.Vector(length/100, 0.1, height/100))
```

## Lighting Setup

```python
def setup_basic_lighting(location=unreal.Vector(0,0,0)):
    """Add standard lighting setup for a blockout level."""
    with unreal.ScopedEditorTransaction("Basic Lighting"):
        # Directional light (sun)
        sun = actor_sub.spawn_actor_from_class(
            unreal.DirectionalLight,
            location + unreal.Vector(0, 0, 1000),
            unreal.Rotator(-45, 30, 0))
        sun.set_actor_label('Sun')

        # Sky light
        sky = actor_sub.spawn_actor_from_class(
            unreal.SkyLight,
            location + unreal.Vector(0, 0, 500))
        sky.set_actor_label('SkyLight')

        # Exponential height fog
        fog = actor_sub.spawn_actor_from_class(
            unreal.ExponentialHeightFog,
            location)
        fog.set_actor_label('Fog')

        # Sky atmosphere (UE5)
        try:
            atmo = actor_sub.spawn_actor_from_class(
                unreal.SkyAtmosphere, location)
            atmo.set_actor_label('Atmosphere')
        except:
            pass  # May not exist in all configs

    return sun, sky, fog
```

## Volume Placement

```python
# Trigger volume
trigger = actor_sub.spawn_actor_from_class(
    unreal.TriggerBox, unreal.Vector(500, 0, 100))
trigger.set_actor_label('CombatTrigger')
trigger.set_actor_scale3d(unreal.Vector(5, 5, 3))

# Blocking volume (invisible wall)
blocker = actor_sub.spawn_actor_from_class(
    unreal.BlockingVolume, unreal.Vector(0, -1000, 0))
blocker.set_actor_label('InvisibleWall')
blocker.set_actor_scale3d(unreal.Vector(20, 0.5, 5))

# Post-process volume
pp = actor_sub.spawn_actor_from_class(
    unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
pp.set_actor_label('GlobalPostProcess')
pp.set_editor_property('unbound', True)
```

## Player Start & Navigation

```python
# Place player start
ps = actor_sub.spawn_actor_from_class(
    unreal.PlayerStart, unreal.Vector(0, 0, 100))
ps.set_actor_label('PlayerStart')

# Nav mesh bounds volume (for AI navigation)
try:
    nav = actor_sub.spawn_actor_from_class(
        unreal.NavMeshBoundsVolume, unreal.Vector(0, 0, 0))
    nav.set_actor_label('NavBounds')
    nav.set_actor_scale3d(unreal.Vector(50, 50, 10))
except:
    unreal.log_warning("NavMeshBoundsVolume not available")
```

## Batch Operations

```python
# Tag all actors in an area
center = unreal.Vector(0, 0, 0)
radius = 2000

for actor in actor_sub.get_all_level_actors():
    loc = actor.get_actor_location()
    dist = (loc - center).length()
    if dist < radius:
        tags = list(actor.get_editor_property('tags'))
        tags.append('Zone_A')
        actor.set_editor_property('tags', tags)

# Delete all actors with a specific label prefix
to_delete = [a for a in actor_sub.get_all_level_actors()
             if a.get_actor_label().startswith('TEMP_')]
if to_delete:
    with unreal.ScopedEditorTransaction("Cleanup temp actors"):
        actor_sub.destroy_actors(to_delete)

# Replace all instances of one mesh with another
old_mesh = unreal.load_asset('/Game/Meshes/OldCube')
new_mesh = unreal.load_asset('/Game/Meshes/NewCube')
for actor in actor_sub.get_all_level_actors():
    comps = actor.get_components_by_class(unreal.StaticMeshComponent)
    for comp in comps:
        if comp.get_editor_property('static_mesh') == old_mesh:
            comp.set_editor_property('static_mesh', new_mesh)
```

## Full Blockout Example: Simple Arena

```python
def blockout_arena():
    """Create a simple combat arena for testing."""
    with unreal.ScopedEditorTransaction("Arena Blockout"):
        # Floor
        floor_mesh = unreal.load_asset('/Engine/BasicShapes/Plane')
        floor = actor_sub.spawn_actor_from_object(floor_mesh, unreal.Vector(0,0,0))
        floor.set_actor_scale3d(unreal.Vector(30, 30, 1))
        floor.set_actor_label('Arena_Floor')

        # Perimeter walls (octagonal)
        import math
        RADIUS = 3000
        WALL_COUNT = 8
        for i in range(WALL_COUNT):
            angle = (2 * math.pi * i) / WALL_COUNT
            x = RADIUS * math.cos(angle)
            y = RADIUS * math.sin(angle)
            yaw = math.degrees(angle) + 90

            wall_mesh = unreal.load_asset('/Engine/BasicShapes/Cube')
            wall = actor_sub.spawn_actor_from_object(
                wall_mesh,
                unreal.Vector(x, y, 250),
                unreal.Rotator(0, yaw, 0))
            wall.set_actor_scale3d(unreal.Vector(25, 1, 5))
            wall.set_actor_label(f'Arena_Wall_{i}')

        # Lighting
        setup_basic_lighting()

        # Player start
        ps = actor_sub.spawn_actor_from_class(
            unreal.PlayerStart, unreal.Vector(0, 0, 100))
        ps.set_actor_label('Arena_PlayerStart')

        # Cover objects
        cover_positions = [
            unreal.Vector(800, 0, 75),
            unreal.Vector(-800, 0, 75),
            unreal.Vector(0, 800, 75),
            unreal.Vector(0, -800, 75),
        ]
        cube = unreal.load_asset('/Engine/BasicShapes/Cube')
        for i, pos in enumerate(cover_positions):
            cover = actor_sub.spawn_actor_from_object(cube, pos)
            cover.set_actor_scale3d(unreal.Vector(2, 2, 1.5))
            cover.set_actor_label(f'Arena_Cover_{i}')

blockout_arena()
```
