# Diagnose input state during PIE
import unreal

# Check if PIE is active
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
if not world:
    print('NO PIE WORLD - run this during Play In Editor')
else:
    print(f'PIE World: {world.get_name()}')

    # Get player controller
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    if pc:
        print(f'PlayerController: {pc.get_class().get_name()}')
        pawn = pc.get_pawn()
        if pawn:
            print(f'Pawn: {pawn.get_class().get_name()}')

            # Check components
            comps = pawn.get_components_by_class(unreal.ActorComponent)
            for c in comps:
                name = c.get_class().get_name()
                if 'Hero' in name or 'Input' in name or 'Pawn' in name or 'Extension' in name:
                    print(f'  Component: {name} ({c.get_name()})')

            # Check HeroComponent DefaultInputMappings
            hero_comps = [c for c in comps if 'Hero' in c.get_class().get_name()]
            if hero_comps:
                hero = hero_comps[0]
                try:
                    mappings = hero.get_editor_property('default_input_mappings')
                    print(f'  HeroComponent.DefaultInputMappings: {len(mappings) if mappings else 0}')
                    if mappings:
                        for m in mappings:
                            imc_ref = m.get_editor_property('input_mapping')
                            print(f'    IMC: {imc_ref}')
                except Exception as e:
                    print(f'  Could not read DefaultInputMappings: {e}')

            # Check PawnExtension for PawnData
            ext_comps = [c for c in comps if 'Extension' in c.get_class().get_name()]
            if ext_comps:
                ext = ext_comps[0]
                try:
                    pd = ext.get_editor_property('pawn_data')
                    if pd:
                        print(f'  PawnData: {pd.get_path_name()}')
                        ic = pd.get_editor_property('input_config')
                        print(f'  InputConfig: {ic.get_path_name() if ic else "NOT SET"}')
                    else:
                        print(f'  PawnData: NOT SET (this is the problem!)')
                except Exception as e:
                    print(f'  Could not read PawnData: {e}')

            # Check Enhanced Input subsystem
            local_player = unreal.GameplayStatics.get_player_controller(world, 0).get_local_player()
            if local_player:
                eis = unreal.SubsystemBlueprintLibrary.get_local_player_subsystem(
                    unreal.EnhancedInputLocalPlayerSubsystem, local_player)
                if eis:
                    print(f'  EnhancedInputSubsystem: found')
                else:
                    print(f'  EnhancedInputSubsystem: NOT FOUND')
        else:
            print('Pawn: NONE (no pawn possessed)')
    else:
        print('PlayerController: NONE')
