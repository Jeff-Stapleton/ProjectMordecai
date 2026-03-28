"""Fix: Configure LyraAbilitySet abilities using property chain approach."""
import unreal

EAL = unreal.EditorAssetLibrary

as_path = '/MordecaiCore/System/AbilitySets/AS_MordecaiCombat'
ability_set = unreal.load_asset(as_path)

if not ability_set:
    unreal.log_error('AS_MordecaiCombat not found')
    print('ERROR')
else:
    # Try approach: Create entries using unreal.Array with struct initialization
    ability_classes = [
        unreal.MordecaiGA_MeleeAttack,
        unreal.MordecaiGA_Dodge,
        unreal.MordecaiGA_Block,
        unreal.MordecaiGA_Parry,
    ]

    # Approach 1: Try to set as array of dicts/structs
    try:
        entries = []
        for cls in ability_classes:
            entry = unreal.LyraAbilitySet_GameplayAbility()
            # Try force setting
            try:
                entry.ability = cls
                entry.ability_level = 1
            except Exception:
                pass
            entries.append(entry)
        ability_set.set_editor_property('granted_gameplay_abilities', entries)
        unreal.log('Approach 1: Set array with entries')
    except Exception as e:
        unreal.log_warning(f'Approach 1 failed: {e}')

    # Approach 2: Set on the array elements after they're part of the asset
    try:
        arr = ability_set.get_editor_property('granted_gameplay_abilities')
        unreal.log(f'Array has {len(arr)} entries')
        for i, entry in enumerate(arr):
            unreal.log(f'  Entry {i}: ability = {entry.get_editor_property("ability") if hasattr(entry, "get_editor_property") else "N/A"}')
    except Exception as e:
        unreal.log_warning(f'Approach 2 inspection failed: {e}')

    # Approach 3: Delete and recreate with the factory, pre-configuring via CDO
    # Since LyraAbilitySet uses EditDefaultsOnly, the data is supposed to be
    # set in the editor details panel. For the prototype, let's try a workaround
    # using Python's direct attribute access instead of set_editor_property
    try:
        entry = unreal.LyraAbilitySet_GameplayAbility()
        # Direct attribute access (bypassing editor property checks)
        entry.ability = unreal.MordecaiGA_MeleeAttack
        entry.ability_level = 1
        unreal.log(f'Direct attr: ability = {entry.ability}')
    except Exception as e:
        unreal.log_warning(f'Direct attr failed: {e}')

    EAL.save_asset(as_path, only_if_is_dirty=False)
    print('Ability set configuration attempted. Check log for details.')
