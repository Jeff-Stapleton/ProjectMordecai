# Clear broken ability entries from AS_MordecaiCombat so the experience loads cleanly
import unreal

EAL = unreal.EditorAssetLibrary
ab = unreal.load_asset('/MordecaiCore/System/AbilitySets/AS_MordecaiCombat')
if ab:
    # Clear the broken entries
    ab.set_editor_property('granted_gameplay_abilities', [])
    EAL.save_asset('/MordecaiCore/System/AbilitySets/AS_MordecaiCombat', only_if_is_dirty=False)
    print('Cleared broken ability entries from AS_MordecaiCombat')

    # Verify
    abilities = ab.get_editor_property('granted_gameplay_abilities')
    print(f'Remaining entries: {len(abilities) if abilities else 0}')
else:
    print('AS_MordecaiCombat not found')
