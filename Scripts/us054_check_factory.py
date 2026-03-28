"""Check DataAssetFactory properties for setting subclass."""
import unreal

f = unreal.DataAssetFactory()

# Try to find the data_asset_class property
props_to_try = [
    'data_asset_class', 'DataAssetClass', 'asset_class',
    'supported_class', 'parent_class',
]
for p in props_to_try:
    try:
        val = f.get_editor_property(p)
        print(f'get_editor_property({p}) = {val}')
    except Exception as e:
        print(f'get_editor_property({p}) -> ERROR: {e}')

# Try setting data_asset_class
try:
    f.set_editor_property('data_asset_class', unreal.MordecaiAttackProfileDataAsset)
    print('set data_asset_class with class -> OK')
except Exception as e:
    print(f'set data_asset_class with class -> ERROR: {e}')

# Try with static_class
try:
    f.set_editor_property('data_asset_class', unreal.MordecaiAttackProfileDataAsset.static_class())
    print('set data_asset_class with static_class() -> OK')
except Exception as e:
    print(f'set data_asset_class with static_class() -> ERROR: {e}')

# Try creating directly with asset tools
at = unreal.AssetToolsHelpers.get_asset_tools()
unreal.EditorAssetLibrary.make_directory('/MordecaiCore/Combat/Test')

# Method 1: Basic factory
try:
    test1 = at.create_asset('TestDA_1', '/MordecaiCore/Combat/Test', None, f)
    if test1:
        print(f'Method 1 (basic factory): Created {test1.get_class().get_name()}')
        unreal.EditorAssetLibrary.delete_asset('/MordecaiCore/Combat/Test/TestDA_1')
except Exception as e:
    print(f'Method 1 error: {e}')

# Method 2: Try to create MordecaiAttackProfileDataAsset directly
try:
    # Some UE5 versions support creating typed DataAssets this way
    test2 = at.create_asset('TestDA_2', '/MordecaiCore/Combat/Test',
                           unreal.MordecaiAttackProfileDataAsset, f)
    if test2:
        print(f'Method 2 (typed class): Created {test2.get_class().get_name()}')
        unreal.EditorAssetLibrary.delete_asset('/MordecaiCore/Combat/Test/TestDA_2')
except Exception as e:
    print(f'Method 2 error: {e}')

# Clean up test dir
unreal.EditorAssetLibrary.delete_directory('/MordecaiCore/Combat/Test')
