# Diagnose input state during PIE
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
if not world:
    print('NO PIE WORLD')
else:
    print(f'PIE World: {world.get_name()}')

    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    if pc:
        print(f'PlayerController: {pc.get_class().get_name()}')

        pawn = unreal.GameplayStatics.get_player_pawn(world, 0)

        if pawn:
            print(f'Pawn: {pawn.get_class().get_name()} at {pawn.get_actor_location()}')

            comps = pawn.get_components_by_class(unreal.ActorComponent)
            for c in comps:
                name = c.get_class().get_name()
                if 'Hero' in name or 'Input' in name or 'Extension' in name:
                    print(f'  Component: {name}')

            # Check HeroComponent
            hero_comps = [c for c in comps if 'Hero' in c.get_class().get_name()]
            if hero_comps:
                hero = hero_comps[0]
                try:
                    mappings = hero.get_editor_property('default_input_mappings')
                    count = len(mappings) if mappings else 0
                    print(f'  DefaultInputMappings: {count}')
                except Exception as e:
                    print(f'  DefaultInputMappings error: {e}')

            # Check PawnExtension
            ext_comps = [c for c in comps if 'Extension' in c.get_class().get_name()]
            if ext_comps:
                ext = ext_comps[0]
                try:
                    pd = ext.get_editor_property('pawn_data')
                    if pd:
                        print(f'  PawnData: {pd.get_path_name()}')
                        ic = pd.get_editor_property('input_config')
                        print(f'  InputConfig: {ic.get_path_name() if ic else "NOT SET"}')
                        cam = pd.get_editor_property('default_camera_mode')
                        print(f'  CameraMode: {cam.get_name() if cam else "NOT SET"}')
                    else:
                        print(f'  PawnData: NOT SET *** THIS IS THE PROBLEM ***')
                except Exception as e:
                    print(f'  PawnData error: {e}')

            # Check movement component
            move_comps = [c for c in comps if 'Movement' in c.get_class().get_name()]
            if move_comps:
                mc = move_comps[0]
                speed = mc.get_editor_property('max_walk_speed')
                mode = mc.get_editor_property('movement_mode')
                print(f'  MovementComponent: speed={speed} mode={mode}')
        else:
            print('Pawn: NONE')
    else:
        print('PlayerController: NONE')
