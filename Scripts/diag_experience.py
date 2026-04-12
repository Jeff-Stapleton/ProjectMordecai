# Diagnose the B_MordecaiDevExperience asset type and try to set it on World Settings
import unreal

EAL = unreal.EditorAssetLibrary

exp = unreal.load_asset('/MordecaiCore/System/Experiences/B_MordecaiDevExperience')
if exp:
    print(f'Asset type: {type(exp).__name__}')
    print(f'Asset class: {exp.get_class().get_name()}')
    print(f'Asset class path: {exp.get_class().get_path_name()}')

    # Check if it is a Blueprint (has generated_class)
    if hasattr(exp, 'generated_class'):
        try:
            gc = exp.generated_class()
            print(f'Generated class: {gc}')
        except Exception as e:
            print(f'generated_class error: {e}')

    # Try get_primary_asset_id
    if hasattr(exp, 'get_primary_asset_id'):
        try:
            paid = exp.get_primary_asset_id()
            print(f'PrimaryAssetId: {paid}')
        except Exception as e:
            print(f'PrimaryAssetId error: {e}')
else:
    print('Experience asset not found')

# Check if it loads as a Blueprint
bp = unreal.load_asset('/MordecaiCore/System/Experiences/B_MordecaiDevExperience')
print(f'Is Blueprint: {isinstance(bp, unreal.Blueprint)}')

# Try load_class
cls = unreal.load_class(None, '/MordecaiCore/System/Experiences/B_MordecaiDevExperience.B_MordecaiDevExperience_C')
print(f'load_class _C result: {cls}')

# Now try to set the property on WorldSettings using force approach
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
level_sub.load_level('/MordecaiCore/Game/Maps/DevTestMap')

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
ws = world.get_world_settings() if world else None
if ws:
    print(f'WorldSettings class: {ws.get_class().get_name()}')

    # List all properties with 'experience' or 'game' in name
    ws_class = ws.get_class()
    print(f'WS class path: {ws_class.get_path_name()}')

    # Try setting via set_editor_property with PropertyAccessChangeNotifyMode
    try:
        ws.set_editor_property('default_gameplay_experience', exp)
        print('Approach 1 (set asset directly): OK')
    except Exception as e:
        print(f'Approach 1: {e}')

    # Try casting to LyraWorldSettings and using specific method
    if hasattr(unreal, 'LyraWorldSettings'):
        lws = unreal.LyraWorldSettings.cast(ws)
        if lws:
            try:
                lws.set_editor_property('default_gameplay_experience', exp)
                print('Approach 2 (cast to LyraWorldSettings): OK')
            except Exception as e:
                print(f'Approach 2: {e}')

    # Try using EditorAssetLibrary set_metadata
    # Try using call_method
    try:
        # Get the property value to see its current type
        val = ws.get_editor_property('default_gameplay_experience')
        print(f'Current experience value: {val}, type: {type(val).__name__}')
    except Exception as e:
        print(f'get_editor_property: {e}')

level_sub.save_current_level()
print('Done')
