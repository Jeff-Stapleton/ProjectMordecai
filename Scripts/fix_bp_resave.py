# Re-save BP_MordecaiCharacter to pick up new C++ default subobject (LyraHeroComponent)
import unreal

EAL = unreal.EditorAssetLibrary
bp_path = '/MordecaiCore/Game/BP_MordecaiCharacter'
bp = unreal.load_asset(bp_path)

if bp:
    # Find the compile function
    compile_funcs = [x for x in dir(unreal) if 'compile' in x.lower() or 'blueprint' in x.lower()]
    bp_methods = [x for x in dir(bp) if 'compile' in x.lower() or 'mark' in x.lower() or 'dirty' in x.lower()]
    print(f'Blueprint compile utilities: {compile_funcs[:20]}')
    print(f'BP methods: {bp_methods}')

    # Try BlueprintEditorLibrary
    if hasattr(unreal, 'BlueprintEditorLibrary'):
        methods = [x for x in dir(unreal.BlueprintEditorLibrary) if not x.startswith('_')]
        print(f'BlueprintEditorLibrary: {methods}')

    # Try direct compile
    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        print('Compiled via BlueprintEditorLibrary')
    except Exception as e:
        print(f'BlueprintEditorLibrary.compile_blueprint: {e}')

    # Force save regardless - UE will recompile on load
    EAL.save_asset(bp_path, only_if_is_dirty=False)
    print('Force-saved BP_MordecaiCharacter')

    # Verify
    bp_class = unreal.load_class(None, bp_path + '.BP_MordecaiCharacter_C')
    if bp_class:
        cdo = unreal.get_default_object(bp_class)
        comps = cdo.get_components_by_class(unreal.ActorComponent)
        hero_found = False
        print('Components:')
        for c in comps:
            name = c.get_class().get_name()
            print(f'  {name} ({c.get_name()})')
            if 'Hero' in name:
                hero_found = True
        print(f'HeroComponent present: {hero_found}')
else:
    print('BP not found')
