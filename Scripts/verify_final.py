# Final verification of the full Mordecai GameMode pipeline
import unreal

print('=== Final Verification ===')

# 1. Developer Settings - ExperienceOverride
ds = unreal.get_default_object(unreal.LyraDeveloperSettings)
exp_override = ds.get_editor_property('experience_override')
print(f'1. ExperienceOverride: {exp_override}')

# 2. PawnData
pd = unreal.load_asset('/MordecaiCore/System/PawnData_Mordecai')
if pd:
    pc = pd.get_editor_property('pawn_class')
    cm = pd.get_editor_property('default_camera_mode')
    ic = pd.get_editor_property('input_config')
    ab = pd.get_editor_property('ability_sets')
    print(f'2. PawnData.PawnClass: {pc.get_name() if pc else "None"}')
    print(f'   PawnData.CameraMode: {cm.get_name() if cm else "None"}')
    print(f'   PawnData.InputConfig: {ic.get_name() if ic else "None"}')
    print(f'   PawnData.AbilitySets: {[a.get_name() for a in ab] if ab else []}')
else:
    print('2. PawnData: MISSING')

# 3. Experience
exp = unreal.load_asset('/MordecaiCore/System/Experiences/B_MordecaiDevExperience')
if exp:
    dpd = exp.get_editor_property('default_pawn_data')
    gf = exp.get_editor_property('game_features_to_enable')
    print(f'3. Experience.DefaultPawnData: {dpd.get_name() if dpd else "None"}')
    print(f'   Experience.GameFeatures: {list(gf) if gf else []}')
else:
    print('3. Experience: MISSING')

# 4. GameMode CDO defaults
if hasattr(unreal, 'MordecaiGameMode'):
    gm = unreal.get_default_object(unreal.MordecaiGameMode)
    gs = gm.get_editor_property('game_state_class')
    pcc = gm.get_editor_property('player_controller_class')
    psc = gm.get_editor_property('player_state_class')
    dpc = gm.get_editor_property('default_pawn_class')
    print(f'4. GameMode.GameState: {gs.get_name() if gs else "None"}')
    print(f'   GameMode.PlayerController: {pcc.get_name() if pcc else "None"}')
    print(f'   GameMode.PlayerState: {psc.get_name() if psc else "None"}')
    print(f'   GameMode.DefaultPawn: {dpc.get_name() if dpc else "None"}')
else:
    print('4. MordecaiGameMode not exposed to Python')

print('')
print('=== Ready for PIE on DevTestMap ===')
