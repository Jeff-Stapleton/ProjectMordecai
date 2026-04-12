# Verify AssetManager can resolve B_MordecaiDevExperience
import unreal

level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
level_sub.load_level('/MordecaiCore/Game/Maps/DevTestMap')

# Search via AssetRegistry for all LyraExperienceDefinition assets
ar = unreal.AssetRegistryHelpers.get_asset_registry()
if ar:
    # Try TopLevelAssetPath
    try:
        tlap = unreal.TopLevelAssetPath('/Script/LyraGame', 'LyraExperienceDefinition')
        results = ar.get_assets_by_class(tlap)
        print(f'LyraExperienceDefinition assets in registry: {len(results)}')
        for r in results:
            print(f'  {r.asset_name} at {r.package_name}')
    except Exception as e:
        print(f'TopLevelAssetPath search: {e}')

    # Also try searching by path
    mordecai_exp = ar.get_asset_by_object_path('/MordecaiCore/System/Experiences/B_MordecaiDevExperience.B_MordecaiDevExperience')
    print(f'Direct path lookup: {mordecai_exp.asset_name if mordecai_exp and mordecai_exp.is_valid() else "NOT FOUND"}')
else:
    print('AssetRegistry not available')

print('Done')
