# Fix PawnData to use C++ class directly (has HeroComponent) instead of stale Blueprint
import unreal

EAL = unreal.EditorAssetLibrary
pd = unreal.load_asset('/MordecaiCore/System/PawnData_Mordecai')

if pd:
    # Set PawnClass to C++ AMordecaiCharacter directly
    pd.set_editor_property('pawn_class', unreal.MordecaiCharacter)
    EAL.save_asset('/MordecaiCore/System/PawnData_Mordecai', only_if_is_dirty=False)

    # Verify
    pc = pd.get_editor_property('pawn_class')
    print(f'PawnData.PawnClass = {pc.get_name() if pc else "None"}')

    # Check C++ CDO has HeroComponent
    cdo = unreal.get_default_object(unreal.MordecaiCharacter)
    comps = cdo.get_components_by_class(unreal.ActorComponent)
    hero = [c for c in comps if 'Hero' in c.get_class().get_name()]
    print(f'C++ CDO HeroComponent: {hero[0].get_class().get_name() if hero else "NOT FOUND"}')
else:
    print('PawnData not found')
