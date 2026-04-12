# Deep camera diagnostic during PIE
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
if not world:
    print('NO PIE WORLD')
else:
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    if pawn:
        comps = pawn.get_components_by_class(unreal.ActorComponent)

        # Check what PawnData the pawn actually has
        ext_comps = [c for c in comps if 'Extension' in c.get_class().get_name()]
        if ext_comps:
            ext = ext_comps[0]
            try:
                pd = ext.get_editor_property('pawn_data')
                if pd:
                    print(f'Pawn PawnData: {pd.get_path_name()}')
                    print(f'Pawn PawnData class: {pd.get_class().get_name()}')
                    cam = pd.get_editor_property('default_camera_mode')
                    print(f'Pawn PawnData.DefaultCameraMode: {cam.get_name() if cam else "None"}')
                    if cam:
                        print(f'  Camera class path: {cam.get_path_name()}')
                else:
                    print('Pawn PawnData: None')
            except Exception as e:
                print(f'PawnData error: {e}')

        # Check the asset directly for comparison
        pd_asset = unreal.load_asset('/MordecaiCore/System/PawnData_Mordecai')
        if pd_asset:
            cam_asset = pd_asset.get_editor_property('default_camera_mode')
            print(f'\nAsset PawnData.DefaultCameraMode: {cam_asset.get_name() if cam_asset else "None"}')
            if cam_asset:
                print(f'  Asset camera class path: {cam_asset.get_path_name()}')

        # Check controller rotation
        pc = unreal.GameplayStatics.get_player_controller(world, 0)
        if pc:
            rot = pc.get_control_rotation()
            print(f'\nControlRotation: pitch={rot.pitch:.1f} yaw={rot.yaw:.1f}')

        # List ALL camera-related components
        print('\nAll components:')
        for c in comps:
            name = c.get_class().get_name()
            if 'Camera' in name or 'Hero' in name:
                print(f'  {name} ({c.get_name()})')
    else:
        print('No pawn')
