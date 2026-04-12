"""US-057 Step 2: Create Spell Input Action assets.

Creates 4 InputAction assets in /MordecaiCore/Input/Actions/:
- IA_Mordecai_Spell1 (AC-057.5: LB/Q - Fireball)
- IA_Mordecai_Spell2 (AC-057.5: RB/E - Blink)
- IA_Mordecai_Spell3 (AC-057.5: LB+RB/R - Stone Skin)
- IA_Mordecai_Spell4 (AC-057.5: DPadUp/1 - Restoration)

Key bindings are handled in C++ (MordecaiHeroComponent programmatic IMC).
"""
import unreal

EAL = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

EAL.make_directory('/MordecaiCore/Input/Actions')

spell_actions = [
    'IA_Mordecai_Spell1',
    'IA_Mordecai_Spell2',
    'IA_Mordecai_Spell3',
    'IA_Mordecai_Spell4',
]

for name in spell_actions:
    folder = '/MordecaiCore/Input/Actions'
    path = f'{folder}/{name}'

    if EAL.does_asset_exist(path):
        unreal.log(f'Already exists: {path}')
        continue

    # Create InputAction asset (bool type - press/release)
    ia = asset_tools.create_asset(name, folder, unreal.InputAction, None)
    if ia is None:
        unreal.log_error(f'Failed to create {name}')
        continue

    # Set value type to bool (digital button press)
    try:
        ia.set_editor_property('value_type', unreal.InputActionValueType.BOOLEAN)
    except Exception as e:
        unreal.log_warning(f'Could not set value_type on {name}: {e}')

    EAL.save_asset(path, only_if_is_dirty=False)
    unreal.log(f'Created: {path}')

unreal.log('US-057 Step 2 complete: All spell input actions created.')
print('SUCCESS: 4 spell input action assets created.')
