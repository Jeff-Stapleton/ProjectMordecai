# Full diagnostic of PawnData_Mordecai and the Arena experience spawn chain
import unreal

EAL = unreal.EditorAssetLibrary

print('=== PawnData_Mordecai ===')
pd = unreal.load_asset('/MordecaiCore/System/PawnData_Mordecai')
if pd:
    pc = pd.get_editor_property('pawn_class')
    cam = pd.get_editor_property('default_camera_mode')
    ic = pd.get_editor_property('input_config')
    print(f'PawnClass: {pc.get_name() if pc else "NOT SET"}')
    if pc:
        print(f'  PawnClass path: {pc.get_path_name()}')
    print(f'DefaultCameraMode: {cam.get_name() if cam else "NOT SET"}')
    print(f'InputConfig: {ic.get_path_name() if ic else "NOT SET"}')
else:
    print('NOT FOUND')

print('\n=== B_MordecaiArenaExperience ===')
bp_class = unreal.load_class(None, '/MordecaiCore/System/Experiences/B_MordecaiArenaExperience.B_MordecaiArenaExperience_C')
if bp_class:
    cdo = unreal.get_default_object(bp_class)
    if cdo:
        exp_pd = cdo.get_editor_property('default_pawn_data')
        print(f'DefaultPawnData: {exp_pd.get_path_name() if exp_pd else "NOT SET"}')
        if exp_pd:
            exp_pc = exp_pd.get_editor_property('pawn_class')
            print(f'  -> PawnClass: {exp_pc.get_name() if exp_pc else "NOT SET"}')
            if exp_pc:
                print(f'     PawnClass path: {exp_pc.get_path_name()}')

        # Check action sets
        action_sets = cdo.get_editor_property('action_sets')
        if action_sets:
            print(f'ActionSets: {len(action_sets)}')
            for a in action_sets:
                print(f'  {a.get_path_name()}')
        else:
            print('ActionSets: EMPTY')

        # Check actions
        actions = cdo.get_editor_property('actions')
        if actions:
            print(f'Actions: {len(actions)}')
            for a in actions:
                print(f'  {a.get_class().get_name()}: {a.get_path_name()}')
        else:
            print('Actions: EMPTY')

        gf = cdo.get_editor_property('game_features_to_enable')
        print(f'GameFeaturesToEnable: {list(gf) if gf else "EMPTY"}')
else:
    print('Could not load BP class')

print('\n=== DevTestMap WorldSettings ===')
# Check if the map has a game mode override
world_settings = unreal.load_asset('/MordecaiCore/Game/Maps/DevTestMap')
print(f'DevTestMap loaded: {world_settings is not None}')
