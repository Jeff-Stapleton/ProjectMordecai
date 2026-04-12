import unreal
imc = unreal.load_asset('/MordecaiCore/Input/IMC_Mordecai')
if imc:
    mappings = imc.get_editor_property('mappings')
    for m in mappings:
        action = m.get_editor_property('action')
        key = m.get_editor_property('key')
        key_name = key.get_editor_property('key_name') if key else 'NO KEY'
        action_name = action.get_name() if action else 'NO ACTION'
        print(f'{action_name} -> key={key_name}')
