# Set ExperienceOverride in LyraDeveloperSettings for PIE
# This has higher priority than WorldSettings for Play-In-Editor
import unreal

if hasattr(unreal, 'LyraDeveloperSettings'):
    dev_settings = unreal.get_default_object(unreal.LyraDeveloperSettings)
    if dev_settings:
        # Build the PrimaryAssetId for B_MordecaiDevExperience
        asset_id = unreal.PrimaryAssetId()
        asset_id.primary_asset_type = unreal.PrimaryAssetType('LyraExperienceDefinition')
        asset_id.primary_asset_name = unreal.Name('B_MordecaiDevExperience')
        dev_settings.set_editor_property('experience_override', asset_id)

        # Verify
        result = dev_settings.get_editor_property('experience_override')
        print(f'ExperienceOverride = {result}')
        unreal.log(f'ExperienceOverride set to: {result}')

        # Save the setting to config
        dev_settings.save_config()
        print('Saved to config. PIE will now use B_MordecaiDevExperience.')
    else:
        print('ERROR: Could not get LyraDeveloperSettings CDO')
else:
    print('ERROR: LyraDeveloperSettings not exposed to Python')
