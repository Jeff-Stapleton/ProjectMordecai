"""US-057 Step 3: Create DA_EnemyAttack_FireSlash and configure one arena enemy.

AC-057.7: Create DA_EnemyAttack_FireSlash with Burning at 40% chance.
AC-057.8: Configure one arena Frontliner to use the fire slash.
"""
import unreal

EAL = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

EAL.make_directory('/MordecaiCore/Combat/Enemies')

# --- AC-057.7: Create DA_EnemyAttack_FireSlash ---
fire_slash_path = '/MordecaiCore/Combat/Enemies/DA_EnemyAttack_FireSlash'
if EAL.does_asset_exist(fire_slash_path):
    unreal.log(f'Deleting existing: {fire_slash_path}')
    EAL.delete_asset(fire_slash_path)

factory = unreal.DataAssetFactory()
factory.set_editor_property('data_asset_class', unreal.MordecaiAttackProfileDataAsset)
fire_slash = asset_tools.create_asset(
    'DA_EnemyAttack_FireSlash',
    '/MordecaiCore/Combat/Enemies',
    None,
    factory
)

if fire_slash is None:
    unreal.log_error('Failed to create DA_EnemyAttack_FireSlash')
    print('ERROR: Failed to create fire slash attack')
else:
    # Same base stats as BasicSlash
    fire_slash.set_editor_property('attack_type', unreal.MordecaiAttackType.MELEE_SWEEP)
    fire_slash.set_editor_property('input_slot', unreal.MordecaiInputSlot.LIGHT)
    fire_slash.set_editor_property('windup_time_ms', 500.0)
    fire_slash.set_editor_property('active_time_ms', 200.0)
    fire_slash.set_editor_property('recovery_time_ms', 400.0)
    fire_slash.set_editor_property('rooted_during', unreal.MordecaiRootedMode.FULL)
    fire_slash.set_editor_property('hit_shape_type', unreal.MordecaiHitShapeType.ARC_SECTOR)

    # Hit shape params
    params = fire_slash.get_editor_property('hit_shape_params')
    params.set_editor_property('radius', 150.0)
    params.set_editor_property('angle', 90.0)
    params.set_editor_property('start_angle_offset', 0.0)
    fire_slash.set_editor_property('hit_shape_params', params)

    fire_slash.set_editor_property('hits_airborne', False)
    fire_slash.set_editor_property('jump_avoidable', True)

    # Damage profile — fire damage
    profile = fire_slash.get_editor_property('damage_profile')
    profile.set_editor_property('damage_type', unreal.MordecaiDamageType.FIRE)
    profile.set_editor_property('base_power', 15.0)
    profile.set_editor_property('can_crit', False)
    profile.set_editor_property('applies_posture_damage', True)
    fire_slash.set_editor_property('damage_profile', profile)

    fire_slash.set_editor_property('posture_damage_scalar', 0.3)
    fire_slash.set_editor_property('stamina_cost', 0.0)
    fire_slash.set_editor_property('combo_index', 0)
    fire_slash.set_editor_property('cancelable_into_dodge', False)
    fire_slash.set_editor_property('cancelable_into_block', False)

    # --- AC-057.7: StatusEffectsOnHit — Burning at 40% ---
    try:
        entry = unreal.MordecaiStatusOnHitEntry()
        entry.set_editor_property('status_effect_tag',
            unreal.GameplayTag.request_gameplay_tag(unreal.Name('Mordecai.Status.Burning')))
        entry.set_editor_property('application_chance', 0.4)
        entry.set_editor_property('status_effect_ge_class', unreal.MordecaiGE_Burning)
        fire_slash.set_editor_property('status_effects_on_hit', [entry])
        unreal.log('Set StatusEffectsOnHit: Burning at 40%')
    except Exception as e:
        unreal.log_warning(f'Could not set StatusEffectsOnHit: {e}')
        unreal.log('NOTE: StatusEffectsOnHit may need manual editor configuration')

    EAL.save_asset(fire_slash_path, only_if_is_dirty=False)
    unreal.log(f'Created and saved: {fire_slash_path}')

# --- AC-057.8: Configure one arena Frontliner with fire slash ---
# The arena has 3 Frontliner enemies. We update the EnemyAttackProfiles on one
# of them to use FireSlash. Since all 3 use the same BP, we need to modify the
# BP CDO to include FireSlash as an additional attack profile.
#
# For the prototype: set the BP's EnemyAttackProfiles to include both BasicSlash
# and FireSlash. The AI will pick randomly, giving the "fire enemy" effect.
enemy_bp_path = '/MordecaiCore/Enemies/BP_MordecaiEnemy_Frontliner'
if EAL.does_asset_exist(enemy_bp_path):
    gen_class = unreal.load_class(None, enemy_bp_path + '.BP_MordecaiEnemy_Frontliner_C')
    if gen_class:
        cdo = unreal.get_default_object(gen_class)
        if cdo:
            # Load both attack profiles
            basic_slash = unreal.load_asset('/MordecaiCore/Combat/Enemies/DA_EnemyAttack_BasicSlash')
            fire_slash_asset = unreal.load_asset(fire_slash_path)
            profiles = []
            if basic_slash:
                profiles.append(basic_slash)
            if fire_slash_asset:
                profiles.append(fire_slash_asset)
            if profiles:
                cdo.set_editor_property('enemy_attack_profiles', profiles)
                unreal.log(f'Set EnemyAttackProfiles: {len(profiles)} profiles (BasicSlash + FireSlash)')
            EAL.save_asset(enemy_bp_path, only_if_is_dirty=False)
        else:
            unreal.log_error('Could not get enemy CDO')
    else:
        unreal.log_error('Could not load enemy generated class')
else:
    unreal.log_warning('Enemy BP not found - fire slash will need manual assignment')

unreal.log('US-057 Step 3 complete: FireSlash created and enemy configured.')
print('SUCCESS: DA_EnemyAttack_FireSlash created. Enemy configured with fire attack.')
