# Create experience Blueprint with temp name, configure, then rename
import unreal

EAL = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
exp_dir = '/MordecaiCore/System/Experiences'
final_path = exp_dir + '/B_MordecaiDevExperience'
temp_path = exp_dir + '/B_MordecaiDevExperience_New'

# Load PawnData reference
pd = unreal.load_asset('/MordecaiCore/System/PawnData_Mordecai')

# Delete old if still around
if EAL.does_asset_exist(final_path):
    EAL.delete_asset(final_path)
    unreal.SystemLibrary.collect_garbage()
    print('Deleted old asset')

# Delete temp if leftover
if EAL.does_asset_exist(temp_path):
    EAL.delete_asset(temp_path)
    unreal.SystemLibrary.collect_garbage()

# Create Blueprint at temp path
bf = unreal.BlueprintFactory()
bf.set_editor_property('parent_class', unreal.LyraExperienceDefinition)
new_bp = asset_tools.create_asset('B_MordecaiDevExperience_New', exp_dir, unreal.Blueprint, bf)

if not new_bp:
    print('ERROR: Could not create Blueprint')
else:
    print(f'Created temp Blueprint: {new_bp.get_name()}')

    # Configure via CDO
    gen_class = new_bp.generated_class()
    if gen_class:
        cdo = unreal.get_default_object(gen_class)
        if pd:
            cdo.set_editor_property('default_pawn_data', pd)
        cdo.set_editor_property('game_features_to_enable', ['MordecaiCore'])
        print('Set properties on CDO')

    # Compile
    unreal.BlueprintEditorLibrary.compile_blueprint(new_bp)

    # Re-set after compile
    gen_class = new_bp.generated_class()
    if gen_class:
        cdo = unreal.get_default_object(gen_class)
        if pd:
            cdo.set_editor_property('default_pawn_data', pd)
        cdo.set_editor_property('game_features_to_enable', ['MordecaiCore'])
    unreal.BlueprintEditorLibrary.compile_blueprint(new_bp)

    # Save temp
    EAL.save_asset(temp_path, only_if_is_dirty=False)

    # Rename to final name
    EAL.rename_asset(temp_path, final_path)
    print(f'Renamed to {final_path}')

    # Save final
    EAL.save_asset(final_path, only_if_is_dirty=False)

    # Verify
    verify = unreal.load_asset(final_path)
    if verify:
        print(f'Final type: {verify.get_class().get_name()}')
        vclass = unreal.load_class(None, final_path + '.B_MordecaiDevExperience_C')
        if vclass:
            vcdo = unreal.get_default_object(vclass)
            vpd = vcdo.get_editor_property('default_pawn_data')
            vgf = vcdo.get_editor_property('game_features_to_enable')
            print(f'DefaultPawnData: {vpd.get_name() if vpd else "None"}')
            print(f'GameFeatures: {list(vgf) if vgf else []}')
            print('SUCCESS')
        else:
            print('WARNING: _C class not found after rename')
    else:
        print('ERROR: Could not load final asset')
