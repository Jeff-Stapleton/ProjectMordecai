"""Check Blueprint API."""
import unreal

EAL = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

# Create a test BP
factory = unreal.BlueprintFactory()
factory.set_editor_property('parent_class', unreal.MordecaiEnemyCharacter)

EAL.make_directory('/MordecaiCore/Enemies')
bp_path = '/MordecaiCore/Enemies/TestEnemyBP'
if EAL.does_asset_exist(bp_path):
    EAL.delete_asset(bp_path)

bp = asset_tools.create_asset('TestEnemyBP', '/MordecaiCore/Enemies', unreal.Blueprint, factory)
if bp:
    print(f'BP class: {bp.get_class().get_name()}')

    # Check for GeneratedClass
    for prop in ['generated_class', 'GeneratedClass', 'parent_class', 'skeleton_generated_class']:
        try:
            val = bp.get_editor_property(prop)
            print(f'  {prop} = {val}')
        except Exception as e:
            print(f'  {prop} -> ERROR: {e}')

    # Try different ways to get CDO
    # Method 1: load_class
    try:
        cls = unreal.load_class(None, bp_path + '.TestEnemyBP_C')
        if cls:
            print(f'load_class: {cls}')
            cdo = unreal.get_default_object(cls)
            if cdo:
                print(f'CDO via load_class: {cdo.get_class().get_name()}')
                # Try setting properties
                cdo.set_editor_property('default_max_health', 200.0)
                print('  Set default_max_health = 200 -> OK')
    except Exception as e:
        print(f'load_class error: {e}')

    # Method 2: get_editor_property alternatives
    try:
        # In UE5.7, Blueprint might use different property access
        gen = bp.generated_class()
        print(f'bp.generated_class() = {gen}')
    except Exception as e:
        pass

    # Clean up
    EAL.delete_asset(bp_path)
    print('Test complete')
else:
    print('Failed to create test BP')
