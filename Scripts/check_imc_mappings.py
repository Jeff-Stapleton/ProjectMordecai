import unreal
imc = unreal.load_asset('/MordecaiCore/Input/IMC_Mordecai')
if imc:
    mappings = imc.get_editor_property('mappings')
    count = len(mappings) if mappings else 0
    print(f'IMC_Mordecai key mappings: {count}')
    if count == 0:
        print('EMPTY - this is why movement does not work!')
else:
    print('IMC_Mordecai NOT FOUND')
