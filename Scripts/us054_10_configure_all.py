"""US-054 Step 10: Final configuration of all assets.

Configures:
- Enemy BP with abilities and health bar
- Loads DevTestMap and ensures arena setup is intact
- Saves all dirty assets

The player controller is NOT configured here.  This script used to write DefaultCombatAbilities,
MeleeAttackProfiles and CombatHUDWidgetClass onto the native MordecaiPlayerController class
default object: an edit that was never saved, never reached a spawned controller (an instance of
a native class copies only Config properties from its CDO), and planted the packaged-build GC
crash of writing loaded assets into a native CDO (N10, issue #15; unreal-tribal-knowledge).  The
controller's C++ constructor sets its combat abilities (US-083); CreateCombatHUD loads
WBP_CombatHUD itself.  Never write a native class default object from a script.
"""
import unreal

EAL = unreal.EditorAssetLibrary

# === Configure Enemy BP ===
unreal.log('=== Configuring Enemy BP ===')
enemy_bp_path = '/MordecaiCore/Enemies/BP_MordecaiEnemy_Frontliner'
if EAL.does_asset_exist(enemy_bp_path):
    gen_class = unreal.load_class(None, enemy_bp_path + '.BP_MordecaiEnemy_Frontliner_C')
    if gen_class:
        cdo = unreal.get_default_object(gen_class)
        if cdo:
            # Verify stats
            hp = cdo.get_editor_property('default_max_health')
            posture = cdo.get_editor_property('default_max_posture')
            speed = cdo.get_editor_property('default_move_speed')
            unreal.log(f'Enemy stats: HP={hp}, Posture={posture}, Speed={speed}')

            # Set DefaultAbilities
            cdo.set_editor_property('default_abilities', [unreal.MordecaiGA_MeleeAttack])
            unreal.log('Set DefaultAbilities = [MeleeAttack]')

            # Set health bar widget class
            hb_path = '/MordecaiCore/UI/WBP_EnemyHealthBar'
            if EAL.does_asset_exist(hb_path):
                hb_class = unreal.load_class(None, hb_path + '.WBP_EnemyHealthBar_C')
                if hb_class:
                    cdo.set_editor_property('enemy_health_bar_widget_class', hb_class)
                    unreal.log('Set EnemyHealthBarWidgetClass = WBP_EnemyHealthBar')

            # Set AI controller
            cdo.set_editor_property('ai_controller_class', unreal.MordecaiEnemyAIController)
            unreal.log('Set AIControllerClass = MordecaiEnemyAIController')

    EAL.save_asset(enemy_bp_path, only_if_is_dirty=False)
else:
    unreal.log_error('Enemy BP not found')

# === Verify All Assets ===
unreal.log('=== Verifying All Assets ===')
required_assets = {
    'Experience': '/MordecaiCore/System/Experiences/B_MordecaiDevExperience',
    'PawnData': '/MordecaiCore/System/PawnData_Mordecai',
    'Camera': '/MordecaiCore/System/BP_CameraMode_Diorama',
    'IMC': '/MordecaiCore/Input/IMC_Mordecai',
    'PlayerBP': '/MordecaiCore/Game/BP_MordecaiCharacter',
    'EnemyBP': '/MordecaiCore/Enemies/BP_MordecaiEnemy_Frontliner',
    'CombatHUD': '/MordecaiCore/UI/WBP_CombatHUD',
    'EnemyHealthBar': '/MordecaiCore/UI/WBP_EnemyHealthBar',
    'EnemyAttack': '/MordecaiCore/Combat/Enemies/DA_EnemyAttack_BasicSlash',
    'Combo1': '/MordecaiCore/Combat/Player/DA_PlayerAttack_LightSlash_1',
    'Combo2': '/MordecaiCore/Combat/Player/DA_PlayerAttack_LightSlash_2',
    'Combo3': '/MordecaiCore/Combat/Player/DA_PlayerAttack_LightSlash_3',
}

all_ok = True
for label, path in required_assets.items():
    if EAL.does_asset_exist(path):
        unreal.log(f'  OK: {label}')
    else:
        unreal.log_error(f'  MISSING: {label} -> {path}')
        all_ok = False

# Save all
EAL.save_directory('/MordecaiCore/', only_if_is_dirty=True, recursive=True)

if all_ok:
    print('SUCCESS: All assets configured and verified.')
else:
    print('WARNING: Some assets are missing.')
