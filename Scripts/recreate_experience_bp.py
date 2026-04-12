# Recreate B_MordecaiDevExperience as a Blueprint class (not a DataAsset)
# Lyra's AssetManager expects bHasBlueprintClasses=True for experiences
import unreal

EAL = unreal.EditorAssetLibrary
exp_path = '/MordecaiCore/System/Experiences/B_MordecaiDevExperience'

# Step 1: Read current config from the old DataAsset
old_exp = unreal.load_asset(exp_path)
old_pawn_data = None
old_gf_list = []
if old_exp:
    old_pawn_data = old_exp.get_editor_property('default_pawn_data')
    old_gf_list = list(old_exp.get_editor_property('game_features_to_enable') or [])
    print(f'Old experience - PawnData: {old_pawn_data.get_name() if old_pawn_data else "None"}')
    print(f'Old experience - GameFeatures: {old_gf_list}')

# Step 2: Delete the old DataAsset
if EAL.does_asset_exist(exp_path):
    EAL.delete_asset(exp_path)
    print(f'Deleted old DataAsset at {exp_path}')

# Step 3: Create a new Blueprint derived from LyraExperienceDefinition
factory = unreal.BlueprintFactory()
factory.set_editor_property('parent_class', unreal.LyraExperienceDefinition)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
new_bp = asset_tools.create_asset(
    'B_MordecaiDevExperience',
    '/MordecaiCore/System/Experiences',
    unreal.Blueprint,
    factory
)

if new_bp:
    print(f'Created Blueprint: {new_bp.get_name()} ({new_bp.get_class().get_name()})')

    # Step 4: Get the CDO of the generated class and set properties
    gen_class = new_bp.generated_class()
    if gen_class:
        print(f'Generated class: {gen_class.get_name()}')
        cdo = unreal.get_default_object(gen_class)
        if cdo:
            # Set DefaultPawnData
            if old_pawn_data:
                cdo.set_editor_property('default_pawn_data', old_pawn_data)
                print(f'Set DefaultPawnData = {old_pawn_data.get_name()}')

            # Set GameFeaturesToEnable
            if old_gf_list:
                cdo.set_editor_property('game_features_to_enable', old_gf_list)
                print(f'Set GameFeaturesToEnable = {old_gf_list}')

    # Step 5: Compile and save
    unreal.BlueprintEditorLibrary.compile_blueprint(new_bp)
    print('Compiled Blueprint')

    # Re-set properties after compile (compile can reset CDO)
    gen_class = new_bp.generated_class()
    if gen_class:
        cdo = unreal.get_default_object(gen_class)
        if cdo:
            if old_pawn_data:
                cdo.set_editor_property('default_pawn_data', old_pawn_data)
            if old_gf_list:
                cdo.set_editor_property('game_features_to_enable', old_gf_list)

    # Compile again after setting properties
    unreal.BlueprintEditorLibrary.compile_blueprint(new_bp)

    EAL.save_asset(exp_path, only_if_is_dirty=False)
    print('Saved Blueprint')

    # Verify
    verify = unreal.load_asset(exp_path)
    if verify:
        print(f'Verify type: {verify.get_class().get_name()}')
        vclass = unreal.load_class(None, exp_path + '.B_MordecaiDevExperience_C')
        print(f'Verify _C class: {vclass.get_name() if vclass else "NOT FOUND"}')
        if vclass:
            vcdo = unreal.get_default_object(vclass)
            vpd = vcdo.get_editor_property('default_pawn_data')
            vgf = vcdo.get_editor_property('game_features_to_enable')
            print(f'Verify DefaultPawnData: {vpd.get_name() if vpd else "None"}')
            print(f'Verify GameFeatures: {list(vgf) if vgf else []}')
else:
    print('ERROR: Failed to create Blueprint')
