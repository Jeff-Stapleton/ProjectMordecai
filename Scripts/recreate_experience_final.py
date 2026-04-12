# Create experience Blueprint directly at the correct path
import unreal

EAL = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
exp_path = '/MordecaiCore/System/Experiences/B_MordecaiDevExperience'

# Clean up any leftovers
for path in [exp_path, exp_path + '_New']:
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
unreal.SystemLibrary.collect_garbage()
print('Cleaned up old assets')

# Load PawnData
pd = unreal.load_asset('/MordecaiCore/System/PawnData_Mordecai')

# Create Blueprint
bf = unreal.BlueprintFactory()
bf.set_editor_property('parent_class', unreal.LyraExperienceDefinition)
new_bp = asset_tools.create_asset(
    'B_MordecaiDevExperience',
    '/MordecaiCore/System/Experiences',
    unreal.Blueprint,
    bf
)

if not new_bp:
    print('ERROR: Could not create Blueprint at target path')
    print(f'  Asset exists: {EAL.does_asset_exist(exp_path)}')
else:
    print(f'Created: {new_bp.get_name()} type={new_bp.get_class().get_name()}')

    # Get generated class and configure
    gen_class = new_bp.generated_class()
    print(f'Generated class: {gen_class.get_name() if gen_class else "None"}')

    if gen_class:
        cdo = unreal.get_default_object(gen_class)
        if pd:
            cdo.set_editor_property('default_pawn_data', pd)
        cdo.set_editor_property('game_features_to_enable', ['MordecaiCore'])

    # Compile twice (set -> compile -> re-set -> compile to ensure persistence)
    unreal.BlueprintEditorLibrary.compile_blueprint(new_bp)
    if gen_class:
        cdo = unreal.get_default_object(gen_class)
        if pd:
            cdo.set_editor_property('default_pawn_data', pd)
        cdo.set_editor_property('game_features_to_enable', ['MordecaiCore'])
    unreal.BlueprintEditorLibrary.compile_blueprint(new_bp)

    # Save
    EAL.save_asset(exp_path, only_if_is_dirty=False)
    print('Saved')

    # Full verify
    loaded_class = unreal.load_class(None, exp_path + '.B_MordecaiDevExperience_C')
    print(f'_C class: {loaded_class.get_name() if loaded_class else "NOT FOUND"}')
    if loaded_class:
        vcdo = unreal.get_default_object(loaded_class)
        vpd = vcdo.get_editor_property('default_pawn_data')
        vgf = vcdo.get_editor_property('game_features_to_enable')
        print(f'DefaultPawnData: {vpd.get_name() if vpd else "None"}')
        print(f'GameFeatures: {list(vgf) if vgf else []}')
        print('SUCCESS - Experience is now a Blueprint class')
