"""US-057 Step 1: Create Spell DataAssets.

Creates 4 SpellDataAssets in /MordecaiCore/Spells/:
- DA_Spell_Fireball (AC-057.1)
- DA_Spell_Blink (AC-057.2)
- DA_Spell_StoneSkin (AC-057.3)
- DA_Spell_Restoration (AC-057.4)
"""
import unreal

EAL = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

EAL.make_directory('/MordecaiCore/Spells')


def create_spell_data_asset(name, config):
    """Create a UMordecaiSpellDataAsset with the given config."""
    folder = '/MordecaiCore/Spells'
    path = f'{folder}/{name}'
    if EAL.does_asset_exist(path):
        unreal.log(f'Deleting existing: {path}')
        EAL.delete_asset(path)

    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.MordecaiSpellDataAsset)
    asset = asset_tools.create_asset(name, folder, None, factory)
    if asset is None:
        unreal.log_error(f'Failed to create {name}')
        return None

    for key, value in config.items():
        try:
            asset.set_editor_property(key, value)
        except Exception as e:
            unreal.log_warning(f'  Could not set {key}: {e}')

    EAL.save_asset(path, only_if_is_dirty=False)
    unreal.log(f'Created and saved: {path}')
    return asset


# --- AC-057.1: DA_Spell_Fireball ---
create_spell_data_asset('DA_Spell_Fireball', {
    'spell_id': 'Fireball',
    'display_name': unreal.Text('Fireball'),
    'spell_point_cost': 3,
    'cooldown_duration': 2.0,
    'cast_time': 0.3,
    'windup_time': 0.2,
    'recovery_time': 0.3,
    'targeting_type': unreal.MordecaiSpellTargetingType.PROJECTILE,
    'base_power': 25.0,
    'range': 1500.0,
    'delivery_type': unreal.MordecaiSpellDeliveryType.SPAWN_PROJECTILE,
    'movement_policy': unreal.MordecaiMovementPolicy.SLOW_WHILE_CASTING,
    'projectile_speed': 2000.0,
    'projectile_lifetime': 3.0,
    'projectile_collision_radius': 30.0,
    'projectile_count': 1,
})

# --- AC-057.2: DA_Spell_Blink ---
create_spell_data_asset('DA_Spell_Blink', {
    'spell_id': 'Blink',
    'display_name': unreal.Text('Blink'),
    'spell_point_cost': 2,
    'cooldown_duration': 4.0,
    'cast_time': 0.0,
    'windup_time': 0.0,
    'recovery_time': 0.2,
    'targeting_type': unreal.MordecaiSpellTargetingType.DASH,
    'range': 500.0,
    'delivery_type': unreal.MordecaiSpellDeliveryType.DASH_MOVEMENT,
    'movement_policy': unreal.MordecaiMovementPolicy.FREE_MOVE,
    'buff_duration': 0.3,  # invulnerability duration
})

# --- AC-057.3: DA_Spell_StoneSkin ---
create_spell_data_asset('DA_Spell_StoneSkin', {
    'spell_id': 'StoneSkin',
    'display_name': unreal.Text('Stone Skin'),
    'spell_point_cost': 4,
    'cooldown_duration': 15.0,
    'cast_time': 0.5,
    'windup_time': 0.3,
    'recovery_time': 0.3,
    'targeting_type': unreal.MordecaiSpellTargetingType.SELF,
    'base_power': 50.0,  # shield HP / damage reduction
    'buff_duration': 10.0,
    'delivery_type': unreal.MordecaiSpellDeliveryType.INSTANT_APPLY,
    'movement_policy': unreal.MordecaiMovementPolicy.ROOT_WHILE_CASTING,
})

# --- AC-057.4: DA_Spell_Restoration ---
create_spell_data_asset('DA_Spell_Restoration', {
    'spell_id': 'Restoration',
    'display_name': unreal.Text('Restoration'),
    'spell_point_cost': 3,
    'cooldown_duration': 10.0,
    'cast_time': 0.4,
    'windup_time': 0.2,
    'recovery_time': 0.2,
    'targeting_type': unreal.MordecaiSpellTargetingType.SELF,
    'base_power': 40.0,  # total heal
    'buff_duration': 5.0,
    'delivery_type': unreal.MordecaiSpellDeliveryType.INSTANT_APPLY,
    'movement_policy': unreal.MordecaiMovementPolicy.SLOW_WHILE_CASTING,
})

unreal.log('US-057 Step 1 complete: All SpellDataAssets created.')
print('SUCCESS: All 4 SpellDataAssets created in /MordecaiCore/Spells/')
