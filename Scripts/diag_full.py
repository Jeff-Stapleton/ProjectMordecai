# Full diagnostic of the Mordecai GameMode pipeline
# Run this BEFORE pressing Play to check editor-time state
import unreal

EAL = unreal.EditorAssetLibrary
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

print('============================================')
print('  FULL MORDECAI GAMEMODE DIAGNOSTIC')
print('============================================')

# 1. What map is loaded?
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if world:
    print(f'1. Current map: {world.get_name()}')
else:
    print('1. ERROR: No editor world')

# 2. World Settings
print('')
print('2. WORLD SETTINGS')
if world:
    ws = world.get_world_settings()
    if ws:
        ws_class = ws.get_class().get_name()
        print(f'   WorldSettings class: {ws_class}')

        # GameMode Override
        try:
            gm = ws.get_editor_property('default_game_mode')
            print(f'   GameMode Override: {gm.get_name() if gm else "None (uses project default)"}')
            if gm:
                print(f'   GameMode Override path: {gm.get_path_name()}')
        except Exception as e:
            print(f'   GameMode Override: error - {e}')

        # Experience (EditDefaultsOnly so just try to read)
        try:
            exp = ws.get_editor_property('default_gameplay_experience')
            print(f'   DefaultGameplayExperience: {exp}')
        except Exception as e:
            print(f'   DefaultGameplayExperience: error - {e}')

# 3. Project Default GameMode
print('')
print('3. PROJECT SETTINGS')
try:
    game_maps = unreal.get_default_object(unreal.GameMapsSettings)
    if game_maps:
        dgm = game_maps.get_editor_property('global_default_game_mode')
        print(f'   GlobalDefaultGameMode: {dgm}')
except Exception as e:
    print(f'   GlobalDefaultGameMode: {e}')

# 4. Developer Settings
print('')
print('4. DEVELOPER SETTINGS (PIE overrides)')
ds = unreal.get_default_object(unreal.LyraDeveloperSettings)
if ds:
    exp_override = ds.get_editor_property('experience_override')
    print(f'   ExperienceOverride: {exp_override}')

# 5. GameMode CDO
print('')
print('5. GAMEMODE CDO')
if hasattr(unreal, 'MordecaiGameMode'):
    gm_cdo = unreal.get_default_object(unreal.MordecaiGameMode)
    print(f'   GameStateClass: {gm_cdo.get_editor_property("game_state_class")}')
    print(f'   PlayerControllerClass: {gm_cdo.get_editor_property("player_controller_class")}')
    print(f'   PlayerStateClass: {gm_cdo.get_editor_property("player_state_class")}')
    print(f'   DefaultPawnClass: {gm_cdo.get_editor_property("default_pawn_class")}')
    print(f'   HUDClass: {gm_cdo.get_editor_property("hud_class")}')
else:
    print('   MordecaiGameMode not in Python')

# Also check LyraGameMode CDO
gm_lyra = unreal.get_default_object(unreal.LyraGameMode)
if gm_lyra:
    print(f'   [Lyra base] DefaultPawnClass: {gm_lyra.get_editor_property("default_pawn_class")}')

# 6. PawnData
print('')
print('6. PAWNDATA')
pd = unreal.load_asset('/MordecaiCore/System/PawnData_Mordecai')
if pd:
    pc = pd.get_editor_property('pawn_class')
    cm = pd.get_editor_property('default_camera_mode')
    ic = pd.get_editor_property('input_config')
    ab = pd.get_editor_property('ability_sets')
    print(f'   PawnClass: {pc.get_name() if pc else "None"} ({pc.get_path_name() if pc else ""})')
    print(f'   CameraMode: {cm.get_name() if cm else "None"} ({cm.get_path_name() if cm else ""})')
    print(f'   InputConfig: {ic.get_name() if ic else "None"}')
    print(f'   AbilitySets: {[a.get_name() for a in ab] if ab else []}')
else:
    print('   PawnData_Mordecai NOT FOUND')

# 7. Experience
print('')
print('7. EXPERIENCE')
exp = unreal.load_asset('/MordecaiCore/System/Experiences/B_MordecaiDevExperience')
if exp:
    dpd = exp.get_editor_property('default_pawn_data')
    gf = exp.get_editor_property('game_features_to_enable')
    actions = exp.get_editor_property('actions')
    action_sets = exp.get_editor_property('action_sets')
    print(f'   Type: {exp.get_class().get_name()}')
    print(f'   DefaultPawnData: {dpd.get_name() if dpd else "None"}')
    print(f'   GameFeaturesToEnable: {list(gf) if gf else []}')
    print(f'   Actions: {len(actions) if actions else 0}')
    print(f'   ActionSets: {len(action_sets) if action_sets else 0}')
    if action_sets:
        for i, aset in enumerate(action_sets):
            print(f'     ActionSet[{i}]: {aset.get_name() if aset else "None"}')
else:
    print('   B_MordecaiDevExperience NOT FOUND')

# 8. MordecaiCore Game Feature Plugin state
print('')
print('8. GAME FEATURE PLUGIN')
# Check if MordecaiCore is registered/active
gf_sub = unreal.get_engine_subsystem(unreal.GameFeaturesSubsystem) if hasattr(unreal, 'GameFeaturesSubsystem') else None
if gf_sub:
    print(f'   GameFeaturesSubsystem available')
    methods = [x for x in dir(gf_sub) if 'state' in x.lower() or 'status' in x.lower() or 'loaded' in x.lower() or 'active' in x.lower()]
    print(f'   State methods: {methods}')
else:
    print('   GameFeaturesSubsystem not available')

# Check if MordecaiCore .uasset exists
mc_path = '/MordecaiCore/MordecaiCore'
if EAL.does_asset_exist(mc_path):
    mc = unreal.load_asset(mc_path)
    print(f'   MordecaiCore plugin asset: {mc.get_class().get_name() if mc else "None"}')
else:
    print(f'   MordecaiCore plugin asset: not found at {mc_path}')

# 9. Character CDO components
print('')
print('9. CHARACTER CDO COMPONENTS')
cdo = unreal.get_default_object(unreal.MordecaiCharacter)
if cdo:
    comps = cdo.get_components_by_class(unreal.ActorComponent)
    for c in comps:
        print(f'   {c.get_class().get_name()} ({c.get_name()})')

# 10. Check if PrimaryAssetId resolves
print('')
print('10. ASSET RESOLUTION')
try:
    asset_manager = unreal.AssetRegistryHelpers.get_asset_registry() if hasattr(unreal, 'AssetRegistryHelpers') else None
except:
    asset_manager = None
# Try to check if the experience resolves as a PrimaryAsset
print(f'   Experience asset exists: {EAL.does_asset_exist("/MordecaiCore/System/Experiences/B_MordecaiDevExperience")}')
print(f'   PawnData asset exists: {EAL.does_asset_exist("/MordecaiCore/System/PawnData_Mordecai")}')
print(f'   Camera asset exists: {EAL.does_asset_exist("/MordecaiCore/System/BP_CameraMode_Diorama")}')

print('')
print('============================================')
print('  DIAGNOSTIC COMPLETE')
print('============================================')
