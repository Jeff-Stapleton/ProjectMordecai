# Verify and fix Mordecai GameMode/Experience/PawnData wiring.
#
# Ensures the full pipeline is correctly configured:
#   DevTestMap WorldSettings -> B_MordecaiDevExperience -> PawnData_Mordecai
#   PawnData_Mordecai -> BP_MordecaiCharacter + BP_CameraMode_Diorama + InputData_Mordecai
#   WorldSettings -> MordecaiGameMode (which sets PlayerController, PlayerState, GameState)
import unreal

EAL = unreal.EditorAssetLibrary
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

issues = []
fixes = []


def check(label, actual, expected, fix_fn=None):
    """Compare actual vs expected. If mismatch, log and optionally fix."""
    if actual == expected:
        unreal.log(f'  OK: {label} = {expected}')
        return True
    else:
        msg = f'{label}: expected {expected}, got {actual}'
        unreal.log_warning(f'  MISMATCH: {msg}')
        issues.append(msg)
        if fix_fn:
            fix_fn()
            fixes.append(label)
        return False


# ============================================================================
# 1. PawnData_Mordecai
# ============================================================================
unreal.log('=== 1. PawnData_Mordecai ===')

pd = unreal.load_asset('/MordecaiCore/System/PawnData_Mordecai')
if not pd:
    unreal.log_error('MISSING: PawnData_Mordecai')
    issues.append('PawnData_Mordecai asset missing')
else:
    # PawnClass -> BP_MordecaiCharacter
    bp_char_class = unreal.load_class(
        None, '/MordecaiCore/Game/BP_MordecaiCharacter.BP_MordecaiCharacter_C')
    current_pawn = pd.get_editor_property('pawn_class')
    pawn_name = current_pawn.get_name() if current_pawn else 'None'
    expected_name = bp_char_class.get_name() if bp_char_class else 'BP_MordecaiCharacter_C'
    check('PawnData.PawnClass', pawn_name, expected_name,
          lambda: pd.set_editor_property('pawn_class', bp_char_class) if bp_char_class else None)

    # DefaultCameraMode -> BP_CameraMode_Diorama
    bp_cam_class = unreal.load_class(
        None, '/MordecaiCore/System/BP_CameraMode_Diorama.BP_CameraMode_Diorama_C')
    current_cam = pd.get_editor_property('default_camera_mode')
    cam_name = current_cam.get_name() if current_cam else 'None'
    expected_cam = bp_cam_class.get_name() if bp_cam_class else 'BP_CameraMode_Diorama_C'
    check('PawnData.DefaultCameraMode', cam_name, expected_cam,
          lambda: pd.set_editor_property('default_camera_mode', bp_cam_class) if bp_cam_class else None)

    # InputConfig -> InputData_Mordecai
    ic = unreal.load_asset('/MordecaiCore/Input/InputData_Mordecai')
    current_ic = pd.get_editor_property('input_config')
    ic_name = current_ic.get_name() if current_ic else 'None'
    expected_ic = ic.get_name() if ic else 'InputData_Mordecai'
    check('PawnData.InputConfig', ic_name, expected_ic,
          lambda: pd.set_editor_property('input_config', ic) if ic else None)

    # AbilitySets -> should include AS_MordecaiCombat
    ability_sets = pd.get_editor_property('ability_sets')
    as_names = [a.get_name() for a in ability_sets] if ability_sets else []
    unreal.log(f'  INFO: PawnData.AbilitySets = {as_names}')
    if 'AS_MordecaiCombat' not in as_names:
        as_combat = unreal.load_asset('/MordecaiCore/System/AbilitySets/AS_MordecaiCombat')
        if as_combat:
            new_sets = list(ability_sets) if ability_sets else []
            new_sets.append(as_combat)
            pd.set_editor_property('ability_sets', new_sets)
            fixes.append('PawnData.AbilitySets += AS_MordecaiCombat')
            unreal.log('  FIXED: Added AS_MordecaiCombat to AbilitySets')
        else:
            issues.append('AS_MordecaiCombat asset not found')

# ============================================================================
# 2. B_MordecaiDevExperience
# ============================================================================
unreal.log('=== 2. B_MordecaiDevExperience ===')

exp_path = '/MordecaiCore/System/Experiences/B_MordecaiDevExperience'
if not EAL.does_asset_exist(exp_path):
    unreal.log_error('MISSING: B_MordecaiDevExperience')
    issues.append('B_MordecaiDevExperience asset missing')
else:
    exp = unreal.load_asset(exp_path)
    if exp:
        # DefaultPawnData -> PawnData_Mordecai
        current_pd = exp.get_editor_property('default_pawn_data')
        pd_name = current_pd.get_name() if current_pd else 'None'
        check('Experience.DefaultPawnData', pd_name, 'PawnData_Mordecai',
              lambda: exp.set_editor_property('default_pawn_data', pd) if pd else None)

        # Log GameFeaturesToEnable
        try:
            gf = exp.get_editor_property('game_features_to_enable')
            unreal.log(f'  INFO: GameFeaturesToEnable = {list(gf) if gf else []}')
        except Exception as e:
            unreal.log(f'  INFO: GameFeaturesToEnable check: {e}')

        # Log Actions
        try:
            actions = exp.get_editor_property('actions')
            unreal.log(f'  INFO: Actions count = {len(actions) if actions else 0}')
        except Exception as e:
            unreal.log(f'  INFO: Actions check: {e}')

# ============================================================================
# 3. DevTestMap World Settings
# ============================================================================
unreal.log('=== 3. DevTestMap World Settings ===')

level_sub.load_level('/MordecaiCore/Game/Maps/DevTestMap')

try:
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if world:
        ws = world.get_world_settings()
        if ws:
            ws_type = type(ws).__name__
            unreal.log(f'  WorldSettings type: {ws_type}')

            # GameMode override -> MordecaiGameMode
            current_gm = ws.get_editor_property('default_game_mode')
            gm_name = current_gm.get_name() if current_gm else 'None'
            unreal.log(f'  Current GameMode override: {gm_name}')
            if 'Mordecai' not in gm_name:
                if hasattr(unreal, 'MordecaiGameMode'):
                    ws.set_editor_property('default_game_mode', unreal.MordecaiGameMode)
                    fixes.append('WorldSettings.GameMode = MordecaiGameMode')
                    unreal.log('  FIXED: GameMode = MordecaiGameMode')
                else:
                    unreal.log_warning('  MordecaiGameMode not exposed to Python — set manually in Editor')
                    issues.append('GameMode override needs manual set to MordecaiGameMode')

            # DefaultGameplayExperience -> B_MordecaiDevExperience
            # This is a TSoftClassPtr - try loading as class
            try:
                current_exp = ws.get_editor_property('default_gameplay_experience')
                exp_info = str(current_exp) if current_exp else 'None'
                unreal.log(f'  Current Experience: {exp_info}')

                if not current_exp or 'Mordecai' not in str(current_exp):
                    exp_class = unreal.load_class(
                        None, exp_path + '.B_MordecaiDevExperience_C')
                    if exp_class:
                        ws.set_editor_property('default_gameplay_experience', exp_class)
                        fixes.append('WorldSettings.Experience = B_MordecaiDevExperience')
                        unreal.log('  FIXED: Experience = B_MordecaiDevExperience')
                    else:
                        unreal.log_warning('  Could not load experience class — try setting manually')
                        issues.append('DefaultGameplayExperience needs manual set')
            except Exception as e:
                unreal.log_warning(f'  Experience property: {e}')
                issues.append(f'DefaultGameplayExperience: {e}')
        else:
            unreal.log_error('  No world settings found')
    else:
        unreal.log_error('  No editor world')
except Exception as e:
    unreal.log_error(f'  World settings error: {e}')

# ============================================================================
# 4. Save
# ============================================================================
unreal.log('=== 4. Saving ===')
EAL.save_directory('/MordecaiCore/', only_if_is_dirty=True, recursive=True)
level_sub.save_current_level()

# ============================================================================
# Summary
# ============================================================================
unreal.log('=== SUMMARY ===')
if fixes:
    unreal.log(f'Fixed {len(fixes)} issues:')
    for f in fixes:
        unreal.log(f'  - {f}')

if issues:
    unreal.log_warning(f'{len(issues)} remaining issues:')
    for i in issues:
        unreal.log_warning(f'  - {i}')
    print(f'WARNING: {len(issues)} issues remain. Check Output Log for details.')
else:
    unreal.log('All wiring verified OK!')
    print('SUCCESS: All GameMode/Experience/PawnData wiring is correct.')
