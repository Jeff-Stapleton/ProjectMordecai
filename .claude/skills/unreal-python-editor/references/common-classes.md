# Common Unreal Python Classes & Functions

## Core Module Functions (`unreal.*`)

### Object Loading
- `load_asset(name, type=Object, follow_redirectors=True)` — load asset by path
- `find_asset(name, type=Object, follow_redirectors=True)` — find already-loaded asset
- `load_object(outer, name, type=Object)` — load object with outer
- `find_object(outer, name, type=Object)` — find already-loaded object
- `load_class(outer, name, type=Object)` — load a UClass
- `load_package(name)` / `find_package(name)` — package operations

### Object Creation
- `new_object(type, outer=None, name='', base_type=None)` — create UObject instance

### Type System
- `@uclass()` — decorator to define UClass from Python
- `@ustruct()` — decorator to define UStruct from Python
- `@uenum()` — decorator to define UEnum from Python
- `@ufunction(meta, ret, params, override, static, pure, getter, setter)` — define UFunction
- `uproperty(type, meta, getter, setter)` — define UProperty
- `uvalue(val, meta)` — define enum constant
- `generate_class(type)` / `generate_struct(type)` / `generate_enum(type)` — manual generation

### Type Queries
- `get_type_from_class(class_)` — Python type for UClass
- `get_type_from_struct(struct)` — Python type for UStruct  
- `get_type_from_enum(enum)` — Python type for UEnum
- `get_default_object(type)` — CDO for class
- `get_blueprint_generated_types(paths)` — Python types from Blueprint paths

### Subsystems
- `get_editor_subsystem(type)` — get editor subsystem instance
- `get_engine_subsystem(type)` — get engine subsystem instance

### Logging
- `log(arg)` — info level
- `log_warning(arg)` — warning level
- `log_error(arg)` — error level
- `log_flush()` — flush to disk

### Iterators
- `ActorIterator(world, type=Actor)` — iterate actors in world
- `ObjectIterator(type=Object)` — iterate all UObjects
- `ClassIterator(type=Class)` — iterate UClasses
- `SelectedActorIterator(world)` — iterate selected actors

### Utilities
- `is_editor()` — True if running in editor
- `collect_garbage()` — run GC
- `load_module(name)` — load UE module and generate Python bindings
- `reload(module)` — reload Python module

## Commonly Used Classes

### Transform Types
- `unreal.Vector(x, y, z)`
- `unreal.Rotator(pitch, yaw, roll)`
- `unreal.Transform(location, rotation, scale)`
- `unreal.Quat(x, y, z, w)`
- `unreal.Color(r, g, b, a)`
- `unreal.LinearColor(r, g, b, a)`

### Actor Types
- `unreal.Actor` — base actor
- `unreal.StaticMeshActor` — placed static mesh
- `unreal.PointLight` / `unreal.SpotLight` / `unreal.DirectionalLight`
- `unreal.CameraActor`
- `unreal.PlayerStart`
- `unreal.DecalActor`
- `unreal.ExponentialHeightFog`
- `unreal.SkyLight`
- `unreal.PostProcessVolume`
- `unreal.TriggerBox` / `unreal.TriggerSphere`
- `unreal.BlockingVolume`

### Component Types
- `unreal.StaticMeshComponent`
- `unreal.SkeletalMeshComponent`
- `unreal.SceneComponent`
- `unreal.AudioComponent`

### Asset Types
- `unreal.StaticMesh`
- `unreal.SkeletalMesh`
- `unreal.MaterialInterface` / `unreal.Material` / `unreal.MaterialInstanceConstant`
- `unreal.Texture2D`
- `unreal.Blueprint`
- `unreal.SoundWave` / `unreal.SoundCue`
- `unreal.AnimSequence`
- `unreal.DataTable`
- `unreal.CurveFloat` / `unreal.CurveLinearColor`
- `unreal.NiagaraSystem` / `unreal.NiagaraEmitter`

### Editor Subsystems
- `unreal.EditorActorSubsystem` — actor operations
- `unreal.LevelEditorSubsystem` — level & viewport
- `unreal.UnrealEditorSubsystem` — editor world & camera
- `unreal.StaticMeshEditorSubsystem` — mesh operations
- `unreal.EditorAssetLibrary` — content browser operations
- `unreal.AssetToolsHelpers.get_asset_tools()` — asset import/creation tools
- `unreal.EditorUtilityLibrary` — editor utility functions
- `unreal.EditorFilterLibrary` — actor filtering
- `unreal.MaterialEditingLibrary` — material parameter editing

### Utility Classes
- `unreal.ScopedEditorTransaction(description)` — undo-able operation block
- `unreal.ScopedSlowTask(total, description)` — progress dialog
- `unreal.AssetData` — lightweight asset metadata
- `unreal.AssetRegistryHelpers` — asset registry queries
