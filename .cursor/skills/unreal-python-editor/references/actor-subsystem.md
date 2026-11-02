# EditorActorSubsystem API Reference

Access: `actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)`

## Actor Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `get_all_level_actors()` | `Array[Actor]` | All loaded actors (excludes pending kill, PIE, preview) |
| `get_all_level_actors_components()` | `Array[ActorComponent]` | All components from loaded actors |
| `get_selected_level_actors()` | `Array[Actor]` | Currently selected actors |
| `get_actor_reference(path_to_actor)` | `Actor` | Find actor by path (e.g. `"PersistentLevel.PlayerStart"`) |

## Spawning

| Method | Returns | Description |
|--------|---------|-------------|
| `spawn_actor_from_class(actor_class, location, rotation=[0,0,0], transient=False)` | `Actor` | Spawn from class or Blueprint class |
| `spawn_actor_from_object(object_to_use, location, rotation=[0,0,0], transient=False)` | `Actor` | Spawn from asset (Factory, Archetype, Blueprint, Class, Asset) |

## Destruction

| Method | Returns | Description |
|--------|---------|-------------|
| `destroy_actor(actor)` | `bool` | Destroy single actor |
| `destroy_actors(actors)` | `bool` | Destroy multiple actors |
| `delete_selected_actors(world)` | `None` | Delete selected actors in world |

## Duplication

| Method | Returns | Description |
|--------|---------|-------------|
| `duplicate_actor(actor, to_world=None, offset=[0,0,0])` | `Actor` | Duplicate with optional offset |
| `duplicate_actors(actors, to_world=None, offset=[0,0,0])` | `Array[Actor]` | Duplicate multiple |
| `duplicate_selected_actors(world)` | `None` | Duplicate selection |

## Selection

| Method | Returns | Description |
|--------|---------|-------------|
| `set_actor_selection_state(actor, should_be_selected)` | `None` | Select/deselect one actor |
| `set_selected_level_actors(actors)` | `None` | Replace selection with list |
| `clear_actor_selection_set()` | `None` | Clear selection |
| `select_nothing()` | `None` | Clear selection (alias) |
| `select_all(world)` | `None` | Select all visible actors |
| `select_all_children(recurse_children)` | `None` | Select children of selection |
| `invert_selection(world)` | `None` | Invert selection |

## Transforms

| Method | Returns | Description |
|--------|---------|-------------|
| `set_actor_transform(actor, world_transform)` | `bool` | Set actor world transform |
| `set_component_transform(scene_component, world_transform)` | `bool` | Set component world transform |

## Conversion

| Method | Returns | Description |
|--------|---------|-------------|
| `convert_actors(actors, actor_class, static_mesh_package_path)` | `Array[Actor]` | Replace actors with new type |

## Events (Delegates)

- `on_new_actors_placed` — actors placed via editor
- `on_new_actors_dropped` — actors dropped into viewport
- `on_delete_actors_begin/end` — deletion events
- `on_duplicate_actors_begin/end` — duplication events
- `on_edit_copy_actors_begin/end` — copy events
- `on_edit_cut_actors_begin/end` — cut events
- `on_edit_paste_actors_begin/end` — paste events
- `on_actor_label_changed` — actor label changed
