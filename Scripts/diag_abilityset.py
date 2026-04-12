# Check AS_MordecaiCombat ability set entries
import unreal
ab = unreal.load_asset('/MordecaiCore/System/AbilitySets/AS_MordecaiCombat')
if ab:
    print(f'Type: {ab.get_class().get_name()}')
    props = [x for x in dir(ab) if 'abilit' in x.lower() or 'granted' in x.lower() or 'effect' in x.lower()]
    print(f'Props: {props}')
    for prop_name in ['granted_gameplay_abilities', 'granted_gameplay_effects', 'granted_attributes']:
        try:
            val = ab.get_editor_property(prop_name)
            if val is not None:
                print(f'{prop_name}: {len(val)} entries')
                for i, entry in enumerate(val):
                    try:
                        entry_props = [x for x in dir(entry) if not x.startswith('_') and not x.startswith('get') and not x.startswith('set')]
                        # Try to read ability/effect class
                        for ep in ['ability', 'gameplay_effect', 'attribute_set']:
                            try:
                                v = entry.get_editor_property(ep)
                                print(f'  [{i}].{ep} = {v.get_name() if v else "None/Invalid"} ({v.get_path_name() if v else ""})')
                            except:
                                pass
                    except Exception as e:
                        print(f'  [{i}] Error: {e}')
        except Exception as e:
            print(f'{prop_name}: {e}')
else:
    print('NOT FOUND')
