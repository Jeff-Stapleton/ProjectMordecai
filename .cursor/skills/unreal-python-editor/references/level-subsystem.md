# LevelEditorSubsystem API Reference

Access: `level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)`

## Level Management

| Method | Returns | Description |
|--------|---------|-------------|
| `new_level(asset_path, is_partitioned_world=False)` | `bool` | Create new blank level, save & load it |
| `new_level_from_template(asset_path, template_path)` | `bool` | Create level from template |
| `load_level(asset_path)` | `bool` | Close current and load level |
| `save_current_level()` | `bool` | Save current level |
| `save_all_dirty_levels()` | `bool` | Save all modified levels |
| `get_current_level()` | `Level` | Get current level |
| `set_current_level_by_name(level_name)` | `bool` | Switch active sub-level |

## Viewport

| Method | Returns | Description |
|--------|---------|-------------|
| `editor_set_game_view(game_view, viewport_key='None')` | `None` | Toggle game view |
| `editor_get_game_view(viewport_key='None')` | `bool` | Check game view state |
| `editor_invalidate_viewports()` | `None` | Force viewport refresh |
| `editor_set_viewport_realtime(realtime, viewport_key='None')` | `None` | Toggle realtime rendering |
| `get_viewport_config_keys()` | `Array[Name]` | List viewport configs |
| `get_active_viewport_config_key()` | `Name` | Get active viewport key |

## Piloting (Camera Lock to Actor)

| Method | Returns | Description |
|--------|---------|-------------|
| `pilot_level_actor(actor, viewport_key='None')` | `None` | Lock viewport to actor |
| `eject_pilot_level_actor(viewport_key='None')` | `None` | Unlock viewport |
| `get_pilot_level_actor(viewport_key='None')` | `Actor` | Get piloted actor |

## Cinematic

| Method | Returns | Description |
|--------|---------|-------------|
| `set_allows_cinematic_control(allow, viewport_key='None')` | `None` | Enable/disable cinematic control |
| `get_allows_cinematic_control(viewport_key='None')` | `bool` | Check cinematic state |
| `set_exact_camera_view(exact, viewport_key='None')` | `None` | Toggle exact camera view |

## PIE (Play In Editor)

| Method | Returns | Description |
|--------|---------|-------------|
| `editor_play_simulate()` | `None` | Start Simulate mode |
| `editor_request_begin_play()` | `None` | Start PIE |
| `editor_request_end_play()` | `None` | Stop PIE |
| `is_in_play_in_editor()` | `bool` | Check if in PIE |

## Selection

| Method | Returns | Description |
|--------|---------|-------------|
| `get_selection_set()` | `TypedElementSelectionSet` | Get full selection set |

## Building

| Method | Returns | Description |
|--------|---------|-------------|
| `build_light_maps(quality=QUALITY_PRODUCTION, with_reflection_captures=False)` | `bool` | Build lighting |

## Events

- `on_map_changed` — fires on map change (may be too early for some scripting)
- `on_map_opened` — fires after map fully opened (preferred)
- `on_editor_camera_moved` — viewport camera moved
- `on_pre_save_world` / `on_post_save_world` — save events
