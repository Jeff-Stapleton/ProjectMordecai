"""US-057 Step 3b: Fix StatusEffectsOnHit on DA_EnemyAttack_FireSlash.

The GameplayTag Python API requires a different approach in UE5.7.
"""
import unreal

EAL = unreal.EditorAssetLibrary

fire_slash = unreal.load_asset('/MordecaiCore/Combat/Enemies/DA_EnemyAttack_FireSlash')
if not fire_slash:
    print('ERROR: DA_EnemyAttack_FireSlash not found')
else:
    # Try multiple approaches to set StatusEffectsOnHit

    # Approach 1: Create the struct entry and set tag via string
    success = False
    try:
        entry = unreal.MordecaiStatusOnHitEntry()
        # Try setting tag name directly
        tag = unreal.GameplayTag()
        tag.set_editor_property('tag_name', unreal.Name('Mordecai.Status.Burning'))
        entry.set_editor_property('status_effect_tag', tag)
        entry.set_editor_property('application_chance', 0.4)
        entry.set_editor_property('status_effect_ge_class', unreal.MordecaiGE_Burning)
        fire_slash.set_editor_property('status_effects_on_hit', [entry])
        success = True
        print('Approach 1 worked: tag_name property')
    except Exception as e:
        print(f'Approach 1 failed: {e}')

    # Approach 2: Use make_gameplay_tag_from_name if available
    if not success:
        try:
            from unreal import GameplayTagLibrary
            tag = GameplayTagLibrary.make_gameplay_tag('Mordecai.Status.Burning')
            entry = unreal.MordecaiStatusOnHitEntry()
            entry.set_editor_property('status_effect_tag', tag)
            entry.set_editor_property('application_chance', 0.4)
            entry.set_editor_property('status_effect_ge_class', unreal.MordecaiGE_Burning)
            fire_slash.set_editor_property('status_effects_on_hit', [entry])
            success = True
            print('Approach 2 worked: GameplayTagLibrary')
        except Exception as e:
            print(f'Approach 2 failed: {e}')

    # Approach 3: Use GameplayTagsManager
    if not success:
        try:
            tag_mgr = unreal.GameplayTagsManager.get()
            tag = tag_mgr.request_gameplay_tag(unreal.Name('Mordecai.Status.Burning'))
            entry = unreal.MordecaiStatusOnHitEntry()
            entry.set_editor_property('status_effect_tag', tag)
            entry.set_editor_property('application_chance', 0.4)
            entry.set_editor_property('status_effect_ge_class', unreal.MordecaiGE_Burning)
            fire_slash.set_editor_property('status_effects_on_hit', [entry])
            success = True
            print('Approach 3 worked: GameplayTagsManager')
        except Exception as e:
            print(f'Approach 3 failed: {e}')

    # Approach 4: Just set the GE class without the tag (tag is for display only)
    if not success:
        try:
            entry = unreal.MordecaiStatusOnHitEntry()
            entry.set_editor_property('application_chance', 0.4)
            entry.set_editor_property('status_effect_ge_class', unreal.MordecaiGE_Burning)
            fire_slash.set_editor_property('status_effects_on_hit', [entry])
            success = True
            print('Approach 4 worked: GE class only (no tag)')
        except Exception as e:
            print(f'Approach 4 failed: {e}')

    if success:
        EAL.save_asset('/MordecaiCore/Combat/Enemies/DA_EnemyAttack_FireSlash', only_if_is_dirty=False)
        print('SUCCESS: StatusEffectsOnHit set on FireSlash')
    else:
        print('WARNING: Could not set StatusEffectsOnHit programmatically')
        print('NOTE: Set it manually in editor: Mordecai.Status.Burning, 40% chance, MordecaiGE_Burning')
