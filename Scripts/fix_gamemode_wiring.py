# Fix Mordecai GameMode wiring issues.
#
# Issue 1: BP_CameraMode_Diorama is broken (parent class in wrong module).
#   Fix: Set PawnData.DefaultCameraMode to the C++ class directly.
#
# Issue 2: DevTestMap Experience needs manual set in World Settings.
#   Fix: Try to set it via Python, fall back to manual instructions.
import unreal

EAL = unreal.EditorAssetLibrary
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

# ============================================================================
# 1. Fix PawnData DefaultCameraMode — use C++ class directly
# ============================================================================
unreal.log('=== 1. Fixing PawnData.DefaultCameraMode ===')

pd = unreal.load_asset('/MordecaiCore/System/PawnData_Mordecai')
if pd:
    # Set to C++ class directly (bypasses broken BP_CameraMode_Diorama)
    if hasattr(unreal, 'MordecaiCameraMode_Diorama'):
        pd.set_editor_property('default_camera_mode', unreal.MordecaiCameraMode_Diorama)
        # Verify
        cam = pd.get_editor_property('default_camera_mode')
        cam_name = cam.get_name() if cam else 'None'
        unreal.log(f'  PawnData.DefaultCameraMode = {cam_name}')
        if cam:
            unreal.log('  [OK] Camera mode set to C++ class MordecaiCameraMode_Diorama')
        else:
            unreal.log_error('  [FAIL] Camera mode is still None')
    else:
        unreal.log_error('  MordecaiCameraMode_Diorama not exposed to Python')
        unreal.log('  Manual fix: Open PawnData_Mordecai, set DefaultCameraMode to MordecaiCameraMode_Diorama')
else:
    unreal.log_error('  PawnData_Mordecai not found')

# Save PawnData
EAL.save_asset('/MordecaiCore/System/PawnData_Mordecai', only_if_is_dirty=False)

# ============================================================================
# 2. Fix DevTestMap Experience — try multiple approaches
# ============================================================================
unreal.log('=== 2. Fixing DevTestMap DefaultGameplayExperience ===')

# Load DevTestMap
level_sub.load_level('/MordecaiCore/Game/Maps/DevTestMap')

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
ws = world.get_world_settings() if world else None

if ws:
    # DefaultGameplayExperience is a TSoftClassPtr<ULyraExperienceDefinition>
    # Try setting via SoftClassPath string
    exp_path = '/MordecaiCore/System/Experiences/B_MordecaiDevExperience.B_MordecaiDevExperience_C'

    # Approach 1: Try loading the asset as-is and see what type it is
    exp_asset = unreal.load_asset('/MordecaiCore/System/Experiences/B_MordecaiDevExperience')
    if exp_asset:
        exp_type = type(exp_asset).__name__
        exp_class_name = exp_asset.get_class().get_name()
        unreal.log(f'  Experience asset type: {exp_type}, class: {exp_class_name}')

        # If it's a Blueprint, get its generated class
        if hasattr(exp_asset, 'generated_class'):
            try:
                gen_class = exp_asset.get_editor_property('generated_class')
                unreal.log(f'  Generated class: {gen_class}')
                if gen_class:
                    ws.set_editor_property('default_gameplay_experience', gen_class)
                    unreal.log('  [OK] Experience set via generated_class')
            except Exception as e:
                unreal.log(f'  generated_class attempt: {e}')

    # Approach 2: Try SoftClassPath
    try:
        soft_path = unreal.SoftClassPath(exp_path)
        ws.set_editor_property('default_gameplay_experience', soft_path)
        unreal.log('  [OK] Experience set via SoftClassPath')
    except Exception as e:
        unreal.log(f'  SoftClassPath attempt: {e}')

    # Approach 3: Try loading class directly
    try:
        exp_class = unreal.load_class(None, exp_path)
        if exp_class:
            ws.set_editor_property('default_gameplay_experience', exp_class)
            unreal.log('  [OK] Experience set via load_class')
        else:
            unreal.log('  load_class returned None')
    except Exception as e:
        unreal.log(f'  load_class attempt: {e}')

    # Verify final state
    try:
        final_exp = ws.get_editor_property('default_gameplay_experience')
        unreal.log(f'  Final DefaultGameplayExperience = {final_exp}')
    except Exception as e:
        unreal.log(f'  Could not read final value: {e}')

    # If all programmatic approaches failed, print manual instructions
    unreal.log('')
    unreal.log('  If Experience is still None, set it manually:')
    unreal.log('  1. Open DevTestMap in Editor')
    unreal.log('  2. Window > World Settings')
    unreal.log('  3. Find "Default Gameplay Experience"')
    unreal.log('  4. Set it to B_MordecaiDevExperience')
    unreal.log('  5. Save the map (Ctrl+S)')
else:
    unreal.log_error('  Could not get World Settings')

# Save map
level_sub.save_current_level()
unreal.log('=== Done ===')
print('Script complete — check Output Log for results.')
