"""US-078 Step 1: Create weapon DataAssets + attack profile DataAssets.

Creates 5 weapon DataAssets (Longsword, Dagger, Greatsword, Spear, Mace) matching
the UMordecaiWeaponProfileFactory output from US-025/026, plus their attack
profile DataAssets (17 total).

Outputs to /MordecaiCore/Weapons/
"""
import unreal

EAL = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

# Ensure content directories exist
EAL.make_directory('/MordecaiCore/Weapons')
EAL.make_directory('/MordecaiCore/Weapons/AttackProfiles')

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def create_attack_profile(name, config):
    folder = '/MordecaiCore/Weapons/AttackProfiles'
    path = f'{folder}/{name}'
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)

    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.MordecaiAttackProfileDataAsset)
    asset = asset_tools.create_asset(name, folder, None, factory)
    if asset is None:
        unreal.log_error(f'Failed to create {name}')
        return None

    for key, value in config.items():
        if key in ('hit_shape', 'damage_profile'):
            continue
        try:
            asset.set_editor_property(key, value)
        except Exception as e:
            unreal.log_warning(f'  Could not set {key}: {e}')

    if 'hit_shape' in config:
        params = asset.get_editor_property('hit_shape_params')
        for k, v in config['hit_shape'].items():
            params.set_editor_property(k, v)
        asset.set_editor_property('hit_shape_params', params)

    if 'damage_profile' in config:
        profile = asset.get_editor_property('damage_profile')
        for k, v in config['damage_profile'].items():
            profile.set_editor_property(k, v)
        asset.set_editor_property('damage_profile', profile)

    EAL.save_asset(path, only_if_is_dirty=False)
    unreal.log(f'Created AP: {path}')
    return asset

def create_weapon(name, config, light_profiles, heavy_profile):
    folder = '/MordecaiCore/Weapons'
    path = f'{folder}/{name}'
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)

    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.MordecaiWeaponDataAsset)
    asset = asset_tools.create_asset(name, folder, None, factory)
    if asset is None:
        unreal.log_error(f'Failed to create {name}')
        return None

    for key, value in config.items():
        try:
            asset.set_editor_property(key, value)
        except Exception as e:
            unreal.log_warning(f'  Could not set {key}: {e}')

    asset.set_editor_property('light_attack_profiles', light_profiles)
    asset.set_editor_property('heavy_attack_profile', heavy_profile)

    # Add GrantedTags
    if 'granted_tag' in config:
        try:
            tag_container = unreal.GameplayTagContainer()
            tag = unreal.GameplayTag(config['granted_tag'])
            tag_container = unreal.GameplayTagLibrary.add_gameplay_tag_to_container(tag_container, tag)
            asset.set_editor_property('granted_tags', tag_container)
        except Exception as e:
            unreal.log_warning(f'  Could not set granted_tag: {e}')

    EAL.save_asset(path, only_if_is_dirty=False)
    unreal.log(f'Created Weapon: {path}')
    return asset

def sweep(radius, angle, wm, am, rm, bp, sc, ps, dmg='Slash', rooted='None', cancel=True):
    """Build a sweep attack profile config dict."""
    return {
        'attack_type': unreal.MordecaiAttackType.MELEE_SWEEP,
        'input_slot': unreal.MordecaiInputSlot.LIGHT,
        'hit_shape_type': unreal.MordecaiHitShapeType.ARC_SECTOR,
        'windup_time_ms': float(wm),
        'active_time_ms': float(am),
        'recovery_time_ms': float(rm),
        'rooted_during': getattr(unreal.MordecaiRootedMode, rooted.upper()),
        'jump_avoidable': True,
        'hits_airborne': False,
        'cancelable_into_dodge': cancel,
        'cancelable_into_block': cancel,
        'posture_damage_scalar': float(ps),
        'stamina_cost': float(sc),
        'hit_shape': {'radius': float(radius), 'angle': float(angle)},
        'damage_profile': {'damage_type': getattr(unreal.MordecaiDamageType, dmg.upper()), 'base_power': float(bp)},
    }

def thrust(length, width, wm, am, rm, bp, sc, ps, dmg='Pierce', rooted='None', cancel=True):
    return {
        'attack_type': unreal.MordecaiAttackType.MELEE_THRUST,
        'input_slot': unreal.MordecaiInputSlot.LIGHT,
        'hit_shape_type': unreal.MordecaiHitShapeType.CAPSULE,
        'windup_time_ms': float(wm),
        'active_time_ms': float(am),
        'recovery_time_ms': float(rm),
        'rooted_during': getattr(unreal.MordecaiRootedMode, rooted.upper()),
        'jump_avoidable': False,
        'hits_airborne': True,
        'cancelable_into_dodge': cancel,
        'cancelable_into_block': cancel,
        'posture_damage_scalar': float(ps),
        'stamina_cost': float(sc),
        'hit_shape': {'length': float(length), 'width': float(width)},
        'damage_profile': {'damage_type': getattr(unreal.MordecaiDamageType, dmg.upper()), 'base_power': float(bp)},
    }

def slam(radius, wm, am, rm, bp, sc, ps, dmg='Blunt', rooted='Active', cancel=False):
    return {
        'attack_type': unreal.MordecaiAttackType.MELEE_SLAM,
        'input_slot': unreal.MordecaiInputSlot.HEAVY,
        'hit_shape_type': unreal.MordecaiHitShapeType.CIRCLE,
        'windup_time_ms': float(wm),
        'active_time_ms': float(am),
        'recovery_time_ms': float(rm),
        'rooted_during': getattr(unreal.MordecaiRootedMode, rooted.upper()),
        'jump_avoidable': False,
        'hits_airborne': True,
        'cancelable_into_dodge': cancel,
        'cancelable_into_block': False,
        'posture_damage_scalar': float(ps),
        'stamina_cost': float(sc),
        'hit_shape': {'radius': float(radius)},
        'damage_profile': {'damage_type': getattr(unreal.MordecaiDamageType, dmg.upper()), 'base_power': float(bp)},
    }

def heavy_sweep(radius, angle, wm, am, rm, bp, sc, ps, dmg='Slash', rooted='Active'):
    cfg = sweep(radius, angle, wm, am, rm, bp, sc, ps, dmg, rooted, cancel=False)
    cfg['input_slot'] = unreal.MordecaiInputSlot.HEAVY
    return cfg

def heavy_thrust(length, width, wm, am, rm, bp, sc, ps, dmg='Pierce', rooted='None'):
    cfg = thrust(length, width, wm, am, rm, bp, sc, ps, dmg, rooted, cancel=False)
    cfg['input_slot'] = unreal.MordecaiInputSlot.HEAVY
    return cfg

# ---------------------------------------------------------------------------
# Longsword (from US-025 CreateLongsword)
# ---------------------------------------------------------------------------
ls_l1 = create_attack_profile('DA_AP_Longsword_L1', sweep(200, 120, 200, 150, 250, 12, 8, 0.5, 'Slash'))
ls_l2 = create_attack_profile('DA_AP_Longsword_L2', sweep(200, 150, 180, 170, 250, 14, 10, 0.5, 'Slash'))
ls_l3 = create_attack_profile('DA_AP_Longsword_L3', sweep(220, 180, 250, 200, 350, 18, 14, 0.8, 'Slash'))
ls_h = create_attack_profile('DA_AP_Longsword_H', heavy_sweep(220, 360, 500, 250, 500, 28, 25, 1.5, 'Slash'))

create_weapon('DA_Weapon_Longsword', {
    'weapon_id': 'Longsword',
    'display_name': 'Longsword',
    'weapon_type': unreal.MordecaiWeaponType.LONGSWORD,
    'equip_slot': unreal.MordecaiEquipSlot.MAIN_HAND,
    'rarity': unreal.MordecaiItemRarity.COMMON,
    'base_damage': 4.0,
    'attack_speed_multiplier': 1.0,
    'range': 200.0,
    'posture_damage_bonus': 1.0,
    'granted_tag': 'Mordecai.Weapon.Type.Sword',
}, [ls_l1, ls_l2, ls_l3], ls_h)

# ---------------------------------------------------------------------------
# Dagger (from US-025 CreateDagger)
# ---------------------------------------------------------------------------
dg_l1 = create_attack_profile('DA_AP_Dagger_L1', thrust(120, 40, 100, 80, 120, 5, 3, 0.2, 'Pierce'))
dg_l2 = create_attack_profile('DA_AP_Dagger_L2', thrust(120, 40, 90, 80, 120, 5, 3, 0.2, 'Pierce'))
dg_l3 = create_attack_profile('DA_AP_Dagger_L3', thrust(120, 45, 80, 80, 110, 6, 4, 0.2, 'Pierce'))
dg_l4 = create_attack_profile('DA_AP_Dagger_L4', thrust(125, 45, 80, 80, 110, 6, 4, 0.3, 'Pierce'))
dg_l5 = create_attack_profile('DA_AP_Dagger_L5', thrust(150, 50, 120, 100, 200, 10, 8, 0.5, 'Pierce'))
dg_h = create_attack_profile('DA_AP_Dagger_H', heavy_thrust(160, 50, 250, 120, 300, 22, 15, 1.2, 'Pierce'))

create_weapon('DA_Weapon_Dagger', {
    'weapon_id': 'Dagger',
    'display_name': 'Dagger',
    'weapon_type': unreal.MordecaiWeaponType.DAGGER,
    'equip_slot': unreal.MordecaiEquipSlot.MAIN_HAND,
    'rarity': unreal.MordecaiItemRarity.COMMON,
    'base_damage': 1.0,
    'attack_speed_multiplier': 1.4,
    'range': 120.0,
    'posture_damage_bonus': 0.0,
    'granted_tag': 'Mordecai.Weapon.Type.Dagger',
}, [dg_l1, dg_l2, dg_l3, dg_l4, dg_l5], dg_h)

# ---------------------------------------------------------------------------
# Greatsword (from US-025 CreateGreatsword)
# ---------------------------------------------------------------------------
gs_l1 = create_attack_profile('DA_AP_Greatsword_L1', sweep(250, 180, 350, 200, 400, 20, 14, 0.8, 'Slash'))
gs_l2 = create_attack_profile('DA_AP_Greatsword_L2', sweep(260, 200, 400, 250, 500, 26, 18, 1.0, 'Slash'))
gs_h = create_attack_profile('DA_AP_Greatsword_H', slam(180, 600, 200, 600, 35, 30, 2.0, 'Slash', 'Active'))

create_weapon('DA_Weapon_Greatsword', {
    'weapon_id': 'Greatsword',
    'display_name': 'Greatsword',
    'weapon_type': unreal.MordecaiWeaponType.GREATSWORD,
    'equip_slot': unreal.MordecaiEquipSlot.TWO_HAND,
    'rarity': unreal.MordecaiItemRarity.COMMON,
    'base_damage': 8.0,
    'attack_speed_multiplier': 0.75,
    'range': 250.0,
    'posture_damage_bonus': 4.0,
    'granted_tag': 'Mordecai.Weapon.Type.Sword',
}, [gs_l1, gs_l2], gs_h)

# ---------------------------------------------------------------------------
# Spear (from US-026 CreateSpear)
# ---------------------------------------------------------------------------
sp_l1 = create_attack_profile('DA_AP_Spear_L1', thrust(280, 50, 200, 150, 280, 12, 8, 0.5, 'Pierce'))
sp_l2 = create_attack_profile('DA_AP_Spear_L2', thrust(290, 55, 220, 160, 280, 14, 10, 0.6, 'Pierce'))
sp_l3 = create_attack_profile('DA_AP_Spear_L3', sweep(250, 160, 280, 200, 350, 18, 14, 0.8, 'Pierce'))
sp_h = create_attack_profile('DA_AP_Spear_H', heavy_thrust(320, 60, 400, 180, 450, 26, 22, 1.3, 'Pierce', 'Active'))

create_weapon('DA_Weapon_Spear', {
    'weapon_id': 'Spear',
    'display_name': 'Spear',
    'weapon_type': unreal.MordecaiWeaponType.SPEAR,
    'equip_slot': unreal.MordecaiEquipSlot.TWO_HAND,
    'rarity': unreal.MordecaiItemRarity.COMMON,
    'base_damage': 5.0,
    'attack_speed_multiplier': 0.9,
    'range': 280.0,
    'posture_damage_bonus': 2.0,
    'granted_tag': 'Mordecai.Weapon.Type.Spear',
}, [sp_l1, sp_l2, sp_l3], sp_h)

# ---------------------------------------------------------------------------
# Mace (from US-026 CreateMace)
# ---------------------------------------------------------------------------
mc_l1 = create_attack_profile('DA_AP_Mace_L1', sweep(160, 150, 300, 180, 400, 14, 12, 1.0, 'Blunt'))
# Mace L2 is a slam with light input slot + cancelable
mc_l2_cfg = slam(140, 320, 200, 450, 18, 14, 1.2, 'Blunt', 'None', cancel=True)
mc_l2_cfg['input_slot'] = unreal.MordecaiInputSlot.LIGHT
mc_l2_cfg['cancelable_into_block'] = True
mc_l2 = create_attack_profile('DA_AP_Mace_L2', mc_l2_cfg)
mc_h = create_attack_profile('DA_AP_Mace_H', slam(180, 600, 200, 600, 28, 28, 2.5, 'Blunt', 'Active'))

create_weapon('DA_Weapon_Mace', {
    'weapon_id': 'Mace',
    'display_name': 'Mace',
    'weapon_type': unreal.MordecaiWeaponType.MACE,
    'equip_slot': unreal.MordecaiEquipSlot.MAIN_HAND,
    'rarity': unreal.MordecaiItemRarity.COMMON,
    'base_damage': 5.0,
    'attack_speed_multiplier': 0.8,
    'range': 160.0,
    'posture_damage_bonus': 5.0,
    'granted_tag': 'Mordecai.Weapon.Type.Mace',
}, [mc_l1, mc_l2], mc_h)

unreal.log('US-078 Step 1 complete: 5 weapons + 17 attack profiles created.')
print('SUCCESS: 5 weapon DataAssets + 17 attack profile DataAssets created in /MordecaiCore/Weapons/')
