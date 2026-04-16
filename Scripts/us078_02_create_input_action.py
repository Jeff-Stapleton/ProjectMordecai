"""US-078 Step 2: Create IA_Mordecai_WeaponCyclePrev input action.

IA_Mordecai_WeaponSwap already exists (used for cycle-next).
This creates the prev-cycle input action. Key bindings are configured
programmatically in MordecaiHeroComponent.cpp.
"""
import unreal

EAL = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

folder = '/MordecaiCore/Input/Actions'
name = 'IA_Mordecai_WeaponCyclePrev'
path = f'{folder}/{name}'

if EAL.does_asset_exist(path):
    unreal.log(f'Already exists: {path}')
else:
    ia = asset_tools.create_asset(name, folder, unreal.InputAction, None)
    if ia is None:
        unreal.log_error(f'Failed to create {name}')
        raise SystemExit(1)

    try:
        ia.set_editor_property('value_type', unreal.InputActionValueType.BOOLEAN)
    except Exception as e:
        unreal.log_warning(f'Could not set value_type: {e}')

    EAL.save_asset(path, only_if_is_dirty=False)
    unreal.log(f'Created: {path}')

print('SUCCESS: IA_Mordecai_WeaponCyclePrev created.')
