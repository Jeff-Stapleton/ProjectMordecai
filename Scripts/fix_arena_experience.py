# Revert ExperienceOverride to B_MordecaiArenaExperience (which the asset manager can find)
# and ensure it points to PawnData_Mordecai
import unreal

EAL = unreal.EditorAssetLibrary

# 1. Set ExperienceOverride back to B_MordecaiArenaExperience
dev_settings = unreal.get_default_object(unreal.LyraDeveloperSettings)
exp_id = unreal.PrimaryAssetId()
exp_id.primary_asset_type = unreal.PrimaryAssetType('LyraExperienceDefinition')
exp_id.primary_asset_name = unreal.Name('B_MordecaiArenaExperience')
dev_settings.set_editor_property('experience_override', exp_id)
result = dev_settings.get_editor_property('experience_override')
print(f'ExperienceOverride: {result}')

# 2. Check what B_MordecaiArenaExperience has for DefaultPawnData
arena_exp = unreal.load_asset('/MordecaiCore/System/Experiences/B_MordecaiArenaExperience')
if arena_exp:
    print(f'ArenaExperience class: {arena_exp.get_class().get_name()}')

    # For Blueprint experiences, we need the generated class CDO
    bp_class = unreal.load_class(None, '/MordecaiCore/System/Experiences/B_MordecaiArenaExperience.B_MordecaiArenaExperience_C')
    if bp_class:
        cdo = unreal.get_default_object(bp_class)
        if cdo:
            pd = cdo.get_editor_property('default_pawn_data')
            print(f'DefaultPawnData: {pd.get_path_name() if pd else "NOT SET"}')

            gf = cdo.get_editor_property('game_features_to_enable')
            print(f'GameFeaturesToEnable: {list(gf) if gf else "EMPTY"}')

            # Fix DefaultPawnData if not set
            if not pd:
                pd_mordecai = unreal.load_asset('/MordecaiCore/System/PawnData_Mordecai')
                if pd_mordecai:
                    cdo.set_editor_property('default_pawn_data', pd_mordecai)
                    EAL.save_asset('/MordecaiCore/System/Experiences/B_MordecaiArenaExperience', only_if_is_dirty=False)
                    print('Fixed: Set DefaultPawnData to PawnData_Mordecai')
        else:
            print('Could not get CDO')
    else:
        print('Could not load B_MordecaiArenaExperience_C class')
else:
    print('B_MordecaiArenaExperience NOT FOUND')
