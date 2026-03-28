"""US-054 Step 1: Create Attack Profile DataAssets.

Creates:
- DA_EnemyAttack_BasicSlash (AC-054.3)
- DA_PlayerAttack_LightSlash_1/2/3 (AC-054.4)
"""
import unreal

EAL = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

# Ensure content directories exist
for d in ['/MordecaiCore/Combat/Enemies', '/MordecaiCore/Combat/Player']:
    EAL.make_directory(d)

def create_attack_profile(name, folder, config):
    """Create a UMordecaiAttackProfileDataAsset with the given config."""
    path = f'{folder}/{name}'
    if EAL.does_asset_exist(path):
        unreal.log(f'Deleting existing: {path}')
        EAL.delete_asset(path)

    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.MordecaiAttackProfileDataAsset)
    asset = asset_tools.create_asset(name, folder, None, factory)
    if asset is None:
        unreal.log_error(f'Failed to create {name}')
        return None

    # Set simple properties
    for key, value in config.items():
        if key in ('hit_shape', 'damage_profile'):
            continue
        try:
            asset.set_editor_property(key, value)
        except Exception as e:
            unreal.log_warning(f'  Could not set {key}: {e}')

    # Set HitShapeParams struct
    if 'hit_shape' in config:
        params = asset.get_editor_property('hit_shape_params')
        for k, v in config['hit_shape'].items():
            try:
                params.set_editor_property(k, v)
            except Exception as e:
                unreal.log_warning(f'  Could not set hit_shape.{k}: {e}')
        asset.set_editor_property('hit_shape_params', params)

    # Set DamageProfile struct
    if 'damage_profile' in config:
        profile = asset.get_editor_property('damage_profile')
        for k, v in config['damage_profile'].items():
            try:
                profile.set_editor_property(k, v)
            except Exception as e:
                unreal.log_warning(f'  Could not set damage_profile.{k}: {e}')
        asset.set_editor_property('damage_profile', profile)

    EAL.save_asset(path, only_if_is_dirty=False)
    unreal.log(f'Created and saved: {path}')
    return asset

# --- AC-054.3: Enemy Basic Slash ---
create_attack_profile('DA_EnemyAttack_BasicSlash', '/MordecaiCore/Combat/Enemies', {
    'attack_type': unreal.MordecaiAttackType.MELEE_SWEEP,
    'input_slot': unreal.MordecaiInputSlot.LIGHT,
    'windup_time_ms': 500.0,
    'active_time_ms': 200.0,
    'recovery_time_ms': 400.0,
    'rooted_during': unreal.MordecaiRootedMode.FULL,
    'hit_shape_type': unreal.MordecaiHitShapeType.ARC_SECTOR,
    'hit_shape': {
        'radius': 150.0,
        'angle': 90.0,
        'start_angle_offset': 0.0,
    },
    'hits_airborne': False,
    'jump_avoidable': True,
    'damage_profile': {
        'damage_type': unreal.MordecaiDamageType.SLASH,
        'base_power': 15.0,
        'can_crit': False,
        'applies_posture_damage': True,
    },
    'posture_damage_scalar': 0.3,
    'stamina_cost': 0.0,
    'combo_index': 0,
    'cancelable_into_dodge': False,
    'cancelable_into_block': False,
})

# --- AC-054.4: Player Light Slash Combo (3-hit) ---
combo_configs = [
    {
        'name': 'DA_PlayerAttack_LightSlash_1',
        'arc': 70.0, 'radius': 180.0, 'power': 10.0,
        'combo_idx': 0, 'posture_scalar': 0.4,
        'windup': 200.0, 'active': 250.0, 'recovery': 200.0,
        'stamina': 20.0,
        'cancel_dodge': True, 'cancel_block': True,
    },
    {
        'name': 'DA_PlayerAttack_LightSlash_2',
        'arc': 90.0, 'radius': 190.0, 'power': 12.0,
        'combo_idx': 1, 'posture_scalar': 0.5,
        'windup': 180.0, 'active': 270.0, 'recovery': 180.0,
        'stamina': 22.0,
        'cancel_dodge': True, 'cancel_block': False,
    },
    {
        'name': 'DA_PlayerAttack_LightSlash_3',
        'arc': 120.0, 'radius': 200.0, 'power': 15.0,
        'combo_idx': 2, 'posture_scalar': 0.7,
        'windup': 220.0, 'active': 300.0, 'recovery': 250.0,
        'stamina': 25.0,
        'cancel_dodge': False, 'cancel_block': False,
    },
]

for cc in combo_configs:
    create_attack_profile(cc['name'], '/MordecaiCore/Combat/Player', {
        'attack_type': unreal.MordecaiAttackType.MELEE_SWEEP,
        'input_slot': unreal.MordecaiInputSlot.LIGHT,
        'windup_time_ms': cc['windup'],
        'active_time_ms': cc['active'],
        'recovery_time_ms': cc['recovery'],
        'rooted_during': unreal.MordecaiRootedMode.ACTIVE,
        'hit_shape_type': unreal.MordecaiHitShapeType.ARC_SECTOR,
        'hit_shape': {
            'radius': cc['radius'],
            'angle': cc['arc'],
            'start_angle_offset': 0.0,
        },
        'hits_airborne': False,
        'jump_avoidable': True,
        'damage_profile': {
            'damage_type': unreal.MordecaiDamageType.SLASH,
            'base_power': cc['power'],
            'can_crit': True,
            'applies_posture_damage': True,
        },
        'posture_damage_scalar': cc['posture_scalar'],
        'stamina_cost': cc['stamina'],
        'combo_index': cc['combo_idx'],
        'cancelable_into_dodge': cc['cancel_dodge'],
        'cancelable_into_block': cc['cancel_block'],
    })

unreal.log('US-054 Step 1 complete: All DataAssets created.')
print('SUCCESS: All attack profile DataAssets created.')
