# Create a fresh Blueprint experience with a new name
import unreal

EAL = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

# Use a clean new name
exp_name = 'B_MordecaiArenaExperience'
exp_dir = '/MordecaiCore/System/Experiences'
exp_path = exp_dir + '/' + exp_name

# Clean
if EAL.does_asset_exist(exp_path):
    EAL.delete_asset(exp_path)
    unreal.SystemLibrary.collect_garbage()

# Create
bf = unreal.BlueprintFactory()
bf.set_editor_property('parent_class', unreal.LyraExperienceDefinition)
new_bp = asset_tools.create_asset(exp_name, exp_dir, unreal.Blueprint, bf)

if not new_bp:
    print(f'ERROR: Could not create at {exp_path}')
    # Try /Game/ instead
    new_bp = asset_tools.create_asset(exp_name, '/Game/System/Experiences', unreal.Blueprint, bf)
    if new_bp:
        exp_path = '/Game/System/Experiences/' + exp_name
        print(f'Created at fallback path: {exp_path}')

if new_bp:
    print(f'Created: {new_bp.get_name()} class={new_bp.get_class().get_name()}')

    # Configure
    pd = unreal.load_asset('/MordecaiCore/System/PawnData_Mordecai')
    gen_class = new_bp.generated_class()
    if gen_class:
        cdo = unreal.get_default_object(gen_class)
        if pd:
            cdo.set_editor_property('default_pawn_data', pd)
        cdo.set_editor_property('game_features_to_enable', ['MordecaiCore'])

    unreal.BlueprintEditorLibrary.compile_blueprint(new_bp)

    # Re-set after compile
    gen_class = new_bp.generated_class()
    if gen_class:
        cdo = unreal.get_default_object(gen_class)
        if pd:
            cdo.set_editor_property('default_pawn_data', pd)
        cdo.set_editor_property('game_features_to_enable', ['MordecaiCore'])
    unreal.BlueprintEditorLibrary.compile_blueprint(new_bp)
    EAL.save_asset(exp_path, only_if_is_dirty=False)

    # Verify
    loaded_class = unreal.load_class(None, exp_path + '.' + exp_name + '_C')
    print(f'_C class: {loaded_class.get_name() if loaded_class else "NOT FOUND"}')
    if loaded_class:
        vcdo = unreal.get_default_object(loaded_class)
        vpd = vcdo.get_editor_property('default_pawn_data')
        vgf = vcdo.get_editor_property('game_features_to_enable')
        print(f'DefaultPawnData: {vpd.get_name() if vpd else "None"}')
        print(f'GameFeatures: {list(vgf) if vgf else []}')

    # Update DeveloperSettings to use the new experience
    ds = unreal.get_default_object(unreal.LyraDeveloperSettings)
    asset_id = unreal.PrimaryAssetId()
    asset_id.primary_asset_type = unreal.PrimaryAssetType('LyraExperienceDefinition')
    asset_id.primary_asset_name = unreal.Name(exp_name)
    ds.set_editor_property('experience_override', asset_id)
    print(f'ExperienceOverride set to: {exp_name}')
    print('SUCCESS')
else:
    print('TOTAL FAILURE - could not create Blueprint anywhere')
