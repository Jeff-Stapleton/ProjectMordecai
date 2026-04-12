import unreal
imc = unreal.load_asset('/MordecaiCore/Input/IMC_Mordecai')
if imc:
    mappings = imc.get_editor_property('mappings')
    for m in mappings:
        action = m.get_editor_property('action')
        key = m.get_editor_property('key')
        key_name = key.get_editor_property('key_name') if key else 'NO KEY'
        action_name = action.get_name() if action else 'NO ACTION'
        mods = m.get_editor_property('modifiers')
        mod_names = []
        if mods:
            for mod in mods:
                if mod:
                    mod_names.append(mod.get_class().get_name())
                else:
                    mod_names.append('NULL')
        print(f'{action_name} -> {key_name} mods={mod_names}')
