# Set DefaultInputMappings on the MordecaiCharacter CDO's HeroComponent
# to include IMC_Mordecai
import unreal

EAL = unreal.EditorAssetLibrary

# Get C++ CDO of MordecaiCharacter
cdo = unreal.get_default_object(unreal.MordecaiCharacter)
if not cdo:
    print('FATAL: Could not get MordecaiCharacter CDO')
else:
    # Find the HeroComponent on the CDO
    comps = cdo.get_components_by_class(unreal.ActorComponent)
    print(f'CDO components: {[c.get_class().get_name() for c in comps]}')

    hero_comps = [c for c in comps if 'Hero' in c.get_class().get_name()]
    if not hero_comps:
        print('FATAL: No HeroComponent found on CDO')
    else:
        hero = hero_comps[0]
        print(f'Found: {hero.get_class().get_name()}')

        # Check current DefaultInputMappings
        try:
            mappings = hero.get_editor_property('default_input_mappings')
            print(f'Current DefaultInputMappings: {len(mappings) if mappings else 0}')
        except Exception as e:
            print(f'Could not read DefaultInputMappings: {e}')
            mappings = None

        # Load IMC_Mordecai
        imc = unreal.load_asset('/MordecaiCore/Input/IMC_Mordecai')
        if not imc:
            print('FATAL: IMC_Mordecai not found')
        else:
            # Check if already present
            has_imc = False
            if mappings:
                for m in mappings:
                    try:
                        ref = m.get_editor_property('input_mapping')
                        if ref and 'IMC_Mordecai' in str(ref):
                            has_imc = True
                            break
                    except Exception:
                        pass

            if has_imc:
                print('IMC_Mordecai already in DefaultInputMappings')
            else:
                try:
                    new_mapping = unreal.InputMappingContextAndPriority()
                    new_mapping.set_editor_property('input_mapping', imc)
                    new_mapping.set_editor_property('priority', 1)
                    new_mapping.set_editor_property('register_with_settings', True)

                    if not mappings:
                        mappings = [new_mapping]
                    else:
                        mappings.append(new_mapping)

                    hero.set_editor_property('default_input_mappings', mappings)
                    print(f'Added IMC_Mordecai to DefaultInputMappings')
                except Exception as e:
                    print(f'Failed to set DefaultInputMappings: {e}')
                    print('Will need C++ approach instead.')
