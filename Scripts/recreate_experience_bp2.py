# Recreate B_MordecaiDevExperience as a Blueprint class
import unreal

EAL = unreal.EditorAssetLibrary
exp_path = '/MordecaiCore/System/Experiences/B_MordecaiDevExperience'

# Read current config
old_exp = unreal.load_asset(exp_path)
old_pawn_data = old_exp.get_editor_property('default_pawn_data') if old_exp else None
old_gf_list = list(old_exp.get_editor_property('game_features_to_enable') or []) if old_exp else ['MordecaiCore']
print(f'Captured: PawnData={old_pawn_data.get_name() if old_pawn_data else "None"}, GF={old_gf_list}')

# Try different creation approaches
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

# First see what factories are available
factories = [x for x in dir(unreal) if 'Factory' in x and 'Blueprint' in x]
print(f'Blueprint factories: {factories}')

# Check BlueprintFactory properties
bf = unreal.BlueprintFactory()
bf_props = [x for x in dir(bf) if not x.startswith('_') and ('parent' in x.lower() or 'type' in x.lower() or 'class' in x.lower())]
print(f'BlueprintFactory props: {bf_props}')

# Try setting up factory properly
bf.set_editor_property('parent_class', unreal.LyraExperienceDefinition)
# Check blueprint_type
try:
    bt = bf.get_editor_property('blueprint_type')
    print(f'BlueprintType: {bt}')
except:
    pass

# Delete old asset first
if EAL.does_asset_exist(exp_path):
    # Unload first
    unreal.EditorLoadingAndSavingUtils.unload_packages(['/MordecaiCore/System/Experiences/B_MordecaiDevExperience'])
    EAL.delete_asset(exp_path)
    print('Deleted old asset')

# Force garbage collect
unreal.SystemLibrary.collect_garbage()

# Create new Blueprint
new_bp = asset_tools.create_asset(
    'B_MordecaiDevExperience',
    '/MordecaiCore/System/Experiences/',
    unreal.Blueprint,
    bf
)

if not new_bp:
    # Try alternate path without trailing slash
    new_bp = asset_tools.create_asset(
        'B_MordecaiDevExperience',
        '/MordecaiCore/System/Experiences',
        unreal.Blueprint,
        bf
    )

if not new_bp:
    # Try with DataOnlyBlueprintFactory if it exists
    if hasattr(unreal, 'DataOnlyBlueprintFactory'):
        df = unreal.DataOnlyBlueprintFactory()
        df.set_editor_property('parent_class', unreal.LyraExperienceDefinition)
        new_bp = asset_tools.create_asset(
            'B_MordecaiDevExperience',
            '/MordecaiCore/System/Experiences',
            unreal.Blueprint,
            df
        )
        print(f'DataOnlyBlueprintFactory result: {new_bp}')

if new_bp:
    print(f'Created: {new_bp.get_name()} type={new_bp.get_class().get_name()}')
else:
    print('All creation attempts failed')
    # Check what went wrong
    print(f'Asset exists after delete: {EAL.does_asset_exist(exp_path)}')
    # Try creating at a different path to test
    test_bp = asset_tools.create_asset(
        'B_TestExperience',
        '/MordecaiCore/System/Experiences',
        unreal.Blueprint,
        bf
    )
    print(f'Test create at different name: {test_bp}')
    if test_bp:
        print(f'Test type: {test_bp.get_class().get_name()}')
        # Clean up test
        EAL.delete_asset('/MordecaiCore/System/Experiences/B_TestExperience')
