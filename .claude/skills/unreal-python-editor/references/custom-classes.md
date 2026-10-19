# Working with Custom C++ Classes

## Loading Custom Classes by Path

```python
import unreal

# Load a Blueprint generated from a C++ class
bp_class = unreal.load_class(None, '/Game/Blueprints/BP_MyActor.BP_MyActor_C')

# Load the C++ class directly (if exposed to Python)
# Most C++ classes with UCLASS() are auto-exposed
actor_class = unreal.find_class('MyCustomActor')
```

## Discovering Properties on a Class

```python
# Get the CDO (Class Default Object) to inspect defaults
cdo = unreal.get_default_object(bp_class)

# List all editable properties via EditorAssetLibrary metadata
# Or inspect in Python:
asset = unreal.load_asset('/Game/Blueprints/BP_MyActor')
tags = unreal.EditorAssetLibrary.get_metadata_tag_values(asset)

# For a spawned actor, use get/set_editor_property
actor = actor_sub.spawn_actor_from_class(bp_class, unreal.Vector(0,0,0))
# get_editor_property / set_editor_property work on any UObject
value = actor.get_editor_property('MyFloatVar')
actor.set_editor_property('MyFloatVar', 42.0)
```

## Common Property Types & How to Set Them

```python
# Bool
actor.set_editor_property('bIsActive', True)

# Int / Float
actor.set_editor_property('Health', 100)
actor.set_editor_property('Speed', 600.0)

# String / Name / Text
actor.set_editor_property('DisplayName', 'My Actor')
actor.set_editor_property('TagName', unreal.Name('Enemy'))

# Vector / Rotator
actor.set_editor_property('SpawnOffset', unreal.Vector(100, 0, 50))
actor.set_editor_property('InitialRotation', unreal.Rotator(0, 90, 0))

# Color
actor.set_editor_property('TeamColor', unreal.LinearColor(1, 0, 0, 1))

# Enum (use the Python enum value)
actor.set_editor_property('MovementMode', unreal.EMovementMode.MOVE_FLYING)

# Object reference (e.g., mesh, material, data table)
mesh = unreal.load_asset('/Game/Meshes/SM_Cube')
actor.get_editor_property('StaticMeshComponent').set_editor_property('static_mesh', mesh)

# Soft object reference
actor.set_editor_property('MeshReference', unreal.SoftObjectPath('/Game/Meshes/SM_Cube.SM_Cube'))

# Array properties
actor.set_editor_property('PatrolPoints', [
    unreal.Vector(0, 0, 0),
    unreal.Vector(500, 0, 0),
    unreal.Vector(500, 500, 0),
])
```

## Working with Components

```python
# Get components from a spawned actor
components = actor.get_components_by_class(unreal.StaticMeshComponent)
for comp in components:
    comp.set_editor_property('static_mesh', mesh)
    comp.set_editor_property('relative_location', unreal.Vector(0, 0, 100))

# Add a component (runtime-style, works in editor scripts)
# For editor placement, prefer Blueprints or spawn pre-configured actors

# Get root component
root = actor.get_editor_property('root_component')
```

## Setting Actor Tags & Labels

```python
# Actor tags (gameplay tags array)
actor.set_editor_property('tags', ['Enemy', 'Boss'])

# Actor label (display name in editor)
actor.set_actor_label('BossSpawner_01')
```

## Finding Actors of Custom Class

```python
# Filter all level actors by class
all_actors = actor_sub.get_all_level_actors()
my_actors = [a for a in all_actors if a.get_class().get_name() == 'BP_MyActor_C']

# Or using ActorIterator
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
for actor in unreal.ActorIterator(world, bp_class):
    print(actor.get_actor_label(), actor.get_actor_location())
```

## Modifying Existing Actors by Name/Label

```python
# Find by label
target = None
for a in actor_sub.get_all_level_actors():
    if a.get_actor_label() == 'MySpecificActor':
        target = a
        break

if target:
    target.set_editor_property('Health', 200)
    actor_sub.set_actor_transform(target, unreal.Transform(
        location=[100, 200, 0],
        rotation=[0, 45, 0],
        scale=[2, 2, 2]
    ))
```

## Blueprint-Generated Classes from C++

When a C++ class is used as a parent for a Blueprint:
- The Blueprint class path ends with `_C`: `/Game/BP/BP_MyActor.BP_MyActor_C`
- Properties defined in C++ with `UPROPERTY(EditAnywhere)` are accessible via `get/set_editor_property`
- Blueprint-added variables are also accessible by their variable name
- Call Blueprint-implemented functions with `call_method('FunctionName', args...)`
