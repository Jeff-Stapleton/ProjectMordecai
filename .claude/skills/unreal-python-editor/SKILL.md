---
name: unreal-python-editor
description: >
  Automate Unreal Engine Editor tasks via its Python API (UE 5.x). Use when:
  (1) spawning/placing/transforming actors in a level,
  (2) creating or modifying Blueprints, materials, or assets,
  (3) level setup — loading, saving, creating new levels,
  (4) managing editor selection and viewport,
  (5) asset management — importing, duplicating, renaming, deleting,
  (6) batch operations on level actors or content browser assets,
  (7) building lighting or running editor utilities via Python.
  NOT for: C++ plugin development, runtime gameplay code, or Blueprints-only workflows.
---

# Unreal Python Editor Automation

## Prerequisites

Enable the **Python Editor Script Plugin** and **Editor Scripting Utilities** plugin in the project.
All scripts run inside the editor process — either via the Output Log Python console, `Execute Python Script` in the editor, or Unreal's remote execution protocol.

## Execution Methods

1. **Output Log console** — switch to Python mode, paste/type commands
2. **File execution** — `unreal.PythonScriptLibrary.execute_python_command("py path/to/script.py")` or use `Edit > Execute Python Script`
3. **Startup scripts** — place `.py` files in `Content/Python/` for auto-execution
4. **Remote execution** — UDP-based protocol on port 6766 for external tool integration (see [references/remote-execution.md](references/remote-execution.md))

## Persisting AND Committing Content (Project Mordecai) — MANDATORY

Content is real only when it is **saved to disk AND committed**. An asset or level edit that lives only in editor memory, or only on your disk, does not exist for the packaged build, the next agent, or the player.

**Why this rule exists (US-079):** US-079 "playable inventory arena" committed the C++ plus `Scripts/create_test_pickups.py` / `Scripts/place_test_pickups.py`, but those scripts were never successfully run and saved — the commits contained **zero** `.uasset`/`.umap` files (newest MordecaiCore asset on disk: 2026-04-16). PIE tests passed against unsaved in-editor memory; the shipped DevTestMap had nothing to pick up.

### DO (every asset-creating or level-editing script)

1. **Save inside the script and check the return value** (all three return `bool`; `create_asset` returns `None` on failure):
   ```python
   EAL = unreal.EditorAssetLibrary
   level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

   if not EAL.save_asset(path, only_if_is_dirty=False):              # by package path
       raise RuntimeError(f'save_asset failed: {path}')
   if not EAL.save_loaded_asset(asset_obj, only_if_is_dirty=False):  # when you hold the object
       raise RuntimeError(f'save_loaded_asset failed: {asset_obj.get_path_name()}')
   if not level_sub.save_current_level():                            # after placing/editing actors
       raise RuntimeError('save_current_level failed')

   # Nothing you touched may still be dirty when the script ends:
   dirty = (unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
            + unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages())
   print('DIRTY:', [p.get_name() for p in dirty])
   ```
2. **Verify the save landed on disk** (from the shell, after the script returns). Package path → file: `/Game/X/Y` → `Content/X/Y.uasset`; `/MordecaiCore/X/Y` → `Plugins/GameFeatures/MordecaiCore/Content/X/Y.uasset` (levels are `.umap`; One-File-Per-Actor levels also write `__ExternalActors__/`/`__ExternalObjects__/` `.uasset` files). The file must exist with a `LastWriteTime` from this run, and git must see it:
   ```powershell
   Get-Item "Plugins\GameFeatures\MordecaiCore\Content\Game\Maps\DevTestMap.umap" | Select-Object FullName, LastWriteTime
   git status --porcelain -- "*.uasset" "*.umap"
   ```
   A script that "saved" but produces no new/modified line in `git status` saved nothing — fix the script and re-run; do not move on.
3. **`git add` the resulting assets in the same branch as the code** (same PR as the C++/scripts that need them). `.uasset`/`.umap` are Git LFS-tracked via `.gitattributes`; a normal `git add` routes them through LFS:
   ```powershell
   git add "Plugins/GameFeatures/MordecaiCore/Content/<folder>" "Content/<folder>"
   git lfs status   # staged assets must show (LFS: ...), not (Git: ...)
   ```
4. **Run the content guard before calling the story done** (repo root or any cwd):
   ```powershell
   py Scripts/check_content_committed.py                  # 0 = pass, 1 = fail, 2 = git error / bad ref
   py Scripts/check_content_committed.py --base origin/main --json
   ```
   It FAILS if any `.uasset`/`.umap` is modified/untracked/deleted but uncommitted, or if the branch (`merge-base(--base, HEAD)..HEAD`, default `origin/main`) adds/modifies a `Scripts/*.py` that writes assets (`save_asset(`, `save_loaded_asset`, `save_current_level`, `save_directory(`, `save_packages`, `create_asset(`, `save_dirty_packages`, `save_all_dirty_levels`, `save_map(`, `import_asset_tasks(`) — committed, or modified/untracked in the working tree — while adding/modifying zero `.uasset`/`.umap` files. `[no-content]` exempts a script from the second check only if EVERY commit in the range that touched it carries the tag and it has no uncommitted changes — use it only when the script legitimately leaves nothing to commit, and say why in the commit body.
5. **Commit, then package.** The packaged playtest (`py Scripts/package_playtest.py`, then `py Scripts/playtest_boot.py`) is the proof the content works for a player; `playtest_boot.py`'s staleness gate rejects a package that does not match HEAD.

### DO NOT

- Do not rely on assets or level edits that exist only in editor memory — PIE runs against unsaved memory and will pass anyway.
- Do not ignore `save_*` return values or a `None` from `create_asset`.
- Do not commit the C++ and leave the content "for later" — the feature is not playable, so the story is not done.
- Do not make gameplay setup depend on editor-only/PIE-only state (e.g. `LyraDeveloperSettings` `ExperienceOverride`, which Lyra honors only in PIE). Content must resolve correctly in a packaged build.

### Running project editor scripts (`ue_remote_exec`)

Project editor scripts in `Scripts/` (`Scripts/place_test_pickups.py`, `Scripts/create_test_pickups.py`, `Scripts/create_arena_playlist.py`, …) run **inside the editor** via remote exec, exactly like the existing pattern:
```powershell
py "Scripts/ue_remote_exec.py" "exec(open(r'C:/Users/jeffd/Documents/Gamedev/ProjectMordecai/Scripts/create_arena_playlist.py').read())" --timeout 120
```
- Prefer the absolute path (as `Scripts/run_pie_verify.py` does): the relative form `r'Scripts/...'` resolves against the editor process's working directory, not your shell's.
- Scripts run in the editor's persistent interpreter: wrap the work in a function (`main()`) so no UObject refs leak, print `PASS`/`FAIL` lines and a `RESULT: ...` line, raise `RuntimeError` on hard failure, and keep backslashes out of docstrings (`exec()` gotcha).
- `Scripts/create_arena_playlist.py` creates/updates `/MordecaiCore/System/Playlists/DA_Mordecai_Arena` (front-end playlist → DevTestMap + `B_MordecaiArenaExperience`). The asset manager only resolves it after an editor restart that picks up the `DefaultGame.ini` scan paths (see **When to restart**). It is content: steps 2–4 above apply.

### `verify_pie_*` scripts are logic checks, not playability proof

`Scripts/verify_pie_*.py` scripts that call gameplay functions directly (teleport the pawn + `interaction.try_pickup_focused()`, `subsystem.toggle_pause_menu_to_tab('inventory')`) never press a key, never exercise Enhanced Input mappings, and run against unsaved editor memory. Label them **logic/diagnostic** in their output and in story notes. They never accept a player-facing AC: that requires the packaged playtest now (`py Scripts/package_playtest.py` → `py Scripts/playtest_boot.py` → open every screenshot with the Read tool and describe what is on screen) and, in Phase 2 (planned, not available yet), real-input playtests through the in-game agent bridge.

## Core Patterns

### Getting Subsystems

```python
import unreal

# Modern subsystem approach (preferred over deprecated EditorLevelLibrary)
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor_sub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
```

### Spawning Actors

```python
location = unreal.Vector(0, 0, 0)
rotation = unreal.Rotator(0, 0, 0)

# From class
actor = actor_sub.spawn_actor_from_class(unreal.PointLight, location, rotation)

# From asset (Blueprint, StaticMesh, etc.)
asset = unreal.load_asset('/Game/MyFolder/MyBlueprint')
actor = actor_sub.spawn_actor_from_object(asset, location, rotation)
```

### Transforms

```python
actor_sub.set_actor_transform(actor, unreal.Transform(
    location=[100, 200, 0],
    rotation=[0, 0, 45],
    scale=[1, 1, 1]
))
```

### Selection

```python
actors = actor_sub.get_all_level_actors()
actor_sub.set_selected_level_actors([actors[0]])
selected = actor_sub.get_selected_level_actors()
actor_sub.select_nothing()
```

### Destroying Actors

```python
actor_sub.destroy_actor(actor)
actor_sub.destroy_actors([a1, a2])
```

### Duplicating Actors

```python
copy = actor_sub.duplicate_actor(actor, offset=unreal.Vector(200, 0, 0))
```

## Asset Management

Use `unreal.EditorAssetLibrary` (static methods):

```python
EAL = unreal.EditorAssetLibrary

# List, check, load
assets = EAL.list_assets('/Game/MyFolder/', recursive=True)
exists = EAL.does_asset_exist('/Game/MyFolder/MyMesh')
asset = unreal.load_asset('/Game/MyFolder/MyMesh')

# Duplicate / rename / delete
EAL.duplicate_asset('/Game/Old', '/Game/New')
EAL.rename_asset('/Game/Old', '/Game/Renamed')
EAL.delete_asset('/Game/ToDelete')  # force delete — use with caution

# Metadata
tags = EAL.get_tag_values('/Game/MyFolder/MyMesh')
```

## Level Management

```python
level_sub.new_level('/Game/Maps/NewMap')
level_sub.load_level('/Game/Maps/ExistingMap')
level_sub.save_current_level()
level_sub.save_all_dirty_levels()
level_sub.set_current_level_by_name('SubLevelName')
```

## Viewport & Camera

```python
# Get/set viewport camera
info = editor_sub.get_level_viewport_camera_info()  # returns (location, rotation) or None
editor_sub.set_level_viewport_camera_info(
    unreal.Vector(0, -500, 300),
    unreal.Rotator(-20, 0, 0)
)

# Game view toggle
level_sub.editor_set_game_view(True)

# PIE controls
level_sub.editor_request_begin_play()   # PIE as the player (what run_pie_verify.py uses)
level_sub.editor_play_simulate()        # Simulate: spectator mode, no player pawn; NOT for smoke tests
level_sub.editor_request_end_play()
```

## Transactions (Undo Support)

Wrap modifications in a scoped transaction for undo support:

```python
with unreal.ScopedEditorTransaction("My Operation") as trans:
    actor_sub.spawn_actor_from_class(unreal.PointLight, unreal.Vector(0,0,0))
```

## Progress Feedback

```python
total = 100
with unreal.ScopedSlowTask(total, "Processing...") as task:
    task.make_dialog(True)
    for i in range(total):
        if task.should_cancel():
            break
        task.enter_progress_frame(1, f"Step {i}")
        # ... do work
```

## Creating Blueprints & Generated Types

Define UClasses, UStructs, UEnums from Python:

```python
@unreal.uclass()
class MyEditorUtility(unreal.PlacedEditorUtilityBase):
    pass

@unreal.ustruct()
class MyStruct(unreal.StructBase):
    x = unreal.uproperty(float)
    y = unreal.uproperty(float)

@unreal.uenum()
class MyEnum(unreal.EnumBase):
    VALUE_A = unreal.uvalue(0)
    VALUE_B = unreal.uvalue(1)
```

## Building

```python
level_sub.build_light_maps(
    quality=unreal.LightingBuildQuality.QUALITY_PRODUCTION,
    with_reflection_captures=True
)
```

## Working with Custom C++ / Blueprint Classes

When a user story introduces a new C++ class, use `get/set_editor_property` to configure instances:

```python
# Load a Blueprint derived from a C++ class
bp_class = unreal.load_class(None, '/Game/Blueprints/BP_EnemySpawner.BP_EnemySpawner_C')

# Spawn and configure
spawner = actor_sub.spawn_actor_from_class(bp_class, unreal.Vector(500, 0, 0))
spawner.set_editor_property('MaxEnemies', 5)
spawner.set_editor_property('SpawnRadius', 800.0)
spawner.set_actor_label('BossRoom_Spawner')

# Discover properties on an actor
cdo = unreal.get_default_object(bp_class)
```

For full patterns (property types, components, finding by class/label): see [references/custom-classes.md](references/custom-classes.md)

## Level Blockout

For blocking out levels with geometry, rooms, corridors, lighting, volumes, and batch operations: see [references/level-blockout.md](references/level-blockout.md)

Key helpers available:
- `make_room()` — box room from wall segments
- `make_corridor()` — hallway between two points
- `setup_basic_lighting()` — sun, sky light, fog, atmosphere
- `blockout_arena()` — full example arena with cover and perimeter walls

## Key API Quick Reference

For detailed API docs per subsystem, see:
- [references/actor-subsystem.md](references/actor-subsystem.md) — EditorActorSubsystem full API
- [references/asset-library.md](references/asset-library.md) — EditorAssetLibrary full API
- [references/level-subsystem.md](references/level-subsystem.md) — LevelEditorSubsystem full API
- [references/custom-classes.md](references/custom-classes.md) — Working with C++/Blueprint classes
- [references/level-blockout.md](references/level-blockout.md) — Level blockout patterns & helpers
- [references/remote-execution.md](references/remote-execution.md) — Remote execution protocol
- [references/common-classes.md](references/common-classes.md) — Frequently used UE classes

## Editor Lifecycle (Start / Stop / Wait)

Agents should manage the editor lifecycle themselves rather than asking the user to open or close it.

### Launch the editor
```bash
"C:/Users/jeffd/Documents/Gamedev/UnrealEngine/Engine/Binaries/Win64/UnrealEditor.exe" "C:/Users/jeffd/Documents/Gamedev/ProjectMordecai/ProjectMordecai.uproject" &
```

### Wait for the editor to be ready
Poll with the remote exec script until it responds:
```bash
for i in $(seq 1 60); do
  result=$(py "Scripts/ue_remote_exec.py" "print('ready')" --timeout 5 2>&1)
  if echo "$result" | grep -q "ready"; then echo "Editor ready"; break; fi
  sleep 5
done
```

### Stop the editor
```bash
taskkill //F //IM UnrealEditor.exe
```

**Important:** After killing the editor, Live Coding locks are cleared and a full rebuild can proceed. Always kill the editor before running `Build.bat` if Live Coding is active.

### When to restart
- Config file changes (`DefaultGame.ini`, `DefaultEngine.ini`, `DefaultEditorPerProjectUserSettings.ini`) require a restart
- Adding new C++ default subobjects to a class requires a restart (Blueprints derived from it need recompilation)
- The agent should kill, rebuild if needed, and relaunch automatically

## Agent Behavior: Self-Service First

When a story requires editor work (creating DataAssets, configuring Blueprints, setting up levels):
1. **Launch the editor yourself** if it's not running
2. **Run Python scripts via remote exec** to do the work
3. **Verify the results** via remote exec
4. **Save, confirm on disk, `git add`, and run `py Scripts/check_content_committed.py`** — see **Persisting AND Committing Content** above
5. **Only ask the user** if remote exec is unreachable after troubleshooting, or if manual GUI interaction is truly required (e.g. visual layout review)

Never ask the user to "open the editor and set X manually" when it can be done via Python remote exec.

## Important Notes

- Always use **EditorActorSubsystem** over deprecated `EditorLevelLibrary` methods
- The editor must **not** be in PIE mode for most operations
- Asset paths use forward slashes: `/Game/MyFolder/MyAsset`
- Use `unreal.load_asset()` to load by path, `unreal.find_asset()` for already-loaded
- Wrap bulk operations in `ScopedEditorTransaction` for undo support
- Use `ScopedSlowTask` for long operations to show progress and allow cancellation
- A save is not done until the `.uasset`/`.umap` is committed in the same branch as the code — see **Persisting AND Committing Content**
