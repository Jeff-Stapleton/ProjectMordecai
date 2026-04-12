# Diagnose what components are on the MordecaiCharacter
import unreal

# Check ALyraCharacter base components
char_cdo = unreal.get_default_object(unreal.LyraCharacter)
if char_cdo:
    comps = char_cdo.get_components_by_class(unreal.ActorComponent)
    print('ALyraCharacter components:')
    for c in comps:
        print(f'  {c.get_class().get_name()} ({c.get_name()})')

# Check AMordecaiCharacter components
if hasattr(unreal, 'MordecaiCharacter'):
    mc_cdo = unreal.get_default_object(unreal.MordecaiCharacter)
    if mc_cdo:
        comps = mc_cdo.get_components_by_class(unreal.ActorComponent)
        print('AMordecaiCharacter components:')
        for c in comps:
            print(f'  {c.get_class().get_name()} ({c.get_name()})')

# Check BP_MordecaiCharacter components
bp_class = unreal.load_class(None, '/MordecaiCore/Game/BP_MordecaiCharacter.BP_MordecaiCharacter_C')
if bp_class:
    bp_cdo = unreal.get_default_object(bp_class)
    if bp_cdo:
        comps = bp_cdo.get_components_by_class(unreal.ActorComponent)
        print('BP_MordecaiCharacter components:')
        for c in comps:
            print(f'  {c.get_class().get_name()} ({c.get_name()})')

# Check if LyraHeroComponent exists
print(f'LyraHeroComponent available: {hasattr(unreal, "LyraHeroComponent")}')
