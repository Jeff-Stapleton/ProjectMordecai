# Diagnose camera state during PIE
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
if not world:
    print('NO PIE WORLD')
else:
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    if pawn:
        comps = pawn.get_components_by_class(unreal.ActorComponent)

        # Check HeroComponent camera mode
        hero_comps = [c for c in comps if 'Hero' in c.get_class().get_name()]
        if hero_comps:
            hero = hero_comps[0]
            # Check AbilityCameraMode (set by abilities)
            try:
                acm = hero.get_editor_property('ability_camera_mode')
                print(f'AbilityCameraMode: {acm.get_name() if acm else "None"}')
            except Exception as e:
                print(f'AbilityCameraMode: error ({e})')

            # DetermineCameraMode result
            try:
                cam_mode = hero.determine_camera_mode()
                print(f'DetermineCameraMode(): {cam_mode.get_name() if cam_mode else "None"}')
            except Exception as e:
                print(f'DetermineCameraMode(): error ({e})')

        # Check PawnExtension PawnData camera mode
        ext_comps = [c for c in comps if 'Extension' in c.get_class().get_name()]
        if ext_comps:
            ext = ext_comps[0]
            try:
                pd = ext.get_editor_property('pawn_data')
                if pd:
                    cam = pd.get_editor_property('default_camera_mode')
                    print(f'PawnData.DefaultCameraMode: {cam.get_name() if cam else "NOT SET"}')
            except Exception as e:
                print(f'PawnData error: {e}')

        # Check LyraCameraComponent
        cam_comps = [c for c in comps if 'Camera' in c.get_class().get_name()]
        for cc in cam_comps:
            print(f'CameraComponent: {cc.get_class().get_name()} fov={cc.get_editor_property("field_of_view")}')

        # Check controller rotation
        pc = unreal.GameplayStatics.get_player_controller(world, 0)
        if pc:
            rot = pc.get_control_rotation()
            print(f'ControlRotation: pitch={rot.pitch:.1f} yaw={rot.yaw:.1f} roll={rot.roll:.1f}')
    else:
        print('No pawn')
