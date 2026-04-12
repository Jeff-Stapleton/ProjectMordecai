"""US-057 Step 5: Verify all magic arena assets exist and are configured.

Checks all SpellDataAssets, input actions, fire slash, and HUD widgets.
"""
import unreal

EAL = unreal.EditorAssetLibrary

pass_count = 0
fail_count = 0


def check(name, condition, detail=""):
    global pass_count, fail_count
    if condition:
        pass_count += 1
        print(f"  PASS: {name}")
    else:
        fail_count += 1
        msg = f"  FAIL: {name}"
        if detail:
            msg += f" -- {detail}"
        print(msg)


print("=" * 60)
print("US-057 ASSET VERIFICATION")
print("=" * 60)

# --- SpellDataAssets (AC-057.1-4) ---
spell_assets = [
    '/MordecaiCore/Spells/DA_Spell_Fireball',
    '/MordecaiCore/Spells/DA_Spell_Blink',
    '/MordecaiCore/Spells/DA_Spell_StoneSkin',
    '/MordecaiCore/Spells/DA_Spell_Restoration',
]
for path in spell_assets:
    check(f'SpellDataAsset: {path.split("/")[-1]}', EAL.does_asset_exist(path))

# Verify Fireball config
fireball = unreal.load_asset('/MordecaiCore/Spells/DA_Spell_Fireball')
if fireball:
    check('Fireball SpellPointCost=3', fireball.get_editor_property('spell_point_cost') == 3)
    check('Fireball CooldownDuration=2.0', abs(fireball.get_editor_property('cooldown_duration') - 2.0) < 0.01)
    check('Fireball BasePower=25.0', abs(fireball.get_editor_property('base_power') - 25.0) < 0.01)
    check('Fireball Range=1500.0', abs(fireball.get_editor_property('range') - 1500.0) < 0.01)

# --- Spell Input Actions (AC-057.5) ---
input_actions = [
    '/MordecaiCore/Input/Actions/IA_Mordecai_Spell1',
    '/MordecaiCore/Input/Actions/IA_Mordecai_Spell2',
    '/MordecaiCore/Input/Actions/IA_Mordecai_Spell3',
    '/MordecaiCore/Input/Actions/IA_Mordecai_Spell4',
]
for path in input_actions:
    check(f'InputAction: {path.split("/")[-1]}', EAL.does_asset_exist(path))

# --- Fire Slash (AC-057.7) ---
check('DA_EnemyAttack_FireSlash exists',
      EAL.does_asset_exist('/MordecaiCore/Combat/Enemies/DA_EnemyAttack_FireSlash'))

fire_slash = unreal.load_asset('/MordecaiCore/Combat/Enemies/DA_EnemyAttack_FireSlash')
if fire_slash:
    try:
        status_entries = fire_slash.get_editor_property('status_effects_on_hit')
        check('FireSlash has StatusEffectsOnHit',
              status_entries is not None and len(status_entries) > 0,
              f'Got {len(status_entries) if status_entries else 0} entries')
    except Exception as e:
        check('FireSlash StatusEffectsOnHit accessible', False, str(e))

# --- HUD Widgets (AC-057.9) ---
hud_widgets = [
    '/MordecaiCore/UI/WBP_CombatHUD',
    '/MordecaiCore/UI/WBP_SpellPointsBar',
    '/MordecaiCore/UI/WBP_StatusEffectBar',
    '/MordecaiCore/UI/WBP_SpellCooldownA',
    '/MordecaiCore/UI/WBP_SpellCooldownB',
    '/MordecaiCore/UI/WBP_StatusEffectIndicator',
]
for path in hud_widgets:
    check(f'Widget: {path.split("/")[-1]}', EAL.does_asset_exist(path))

# --- Existing required assets ---
required_assets = [
    '/MordecaiCore/Game/BP_MordecaiCharacter',
    '/MordecaiCore/Enemies/BP_MordecaiEnemy_Frontliner',
    '/MordecaiCore/Combat/Enemies/DA_EnemyAttack_BasicSlash',
    '/MordecaiCore/Input/IMC_Mordecai',
]
for path in required_assets:
    check(f'Required: {path.split("/")[-1]}', EAL.does_asset_exist(path))

print("")
print(f"RESULT: {pass_count} passed, {fail_count} failed")
if fail_count > 0:
    print('WARNING: Some assets are missing or misconfigured!')
else:
    print('ALL CHECKS PASSED')
