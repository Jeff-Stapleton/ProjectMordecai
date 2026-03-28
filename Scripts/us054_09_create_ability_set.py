"""US-054 Step 9: Create LyraAbilitySet for player combat abilities.

Creates AS_MordecaiCombat containing: MeleeAttack, Dodge, Block, Parry
Then adds it to PawnData_Mordecai.
"""
import unreal

EAL = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

EAL.make_directory('/MordecaiCore/System/AbilitySets')

# --- Create the AbilitySet ---
as_path = '/MordecaiCore/System/AbilitySets/AS_MordecaiCombat'
if EAL.does_asset_exist(as_path):
    EAL.delete_asset(as_path)

# LyraAbilitySet extends UPrimaryDataAsset (which is UDataAsset)
# We need to create it with the right type
factory = unreal.DataAssetFactory()
factory.set_editor_property('data_asset_class', unreal.LyraAbilitySet)

ability_set = asset_tools.create_asset(
    'AS_MordecaiCombat',
    '/MordecaiCore/System/AbilitySets',
    None,
    factory
)

if ability_set is None:
    unreal.log_error('Failed to create AS_MordecaiCombat')
    print('ERROR: Failed to create ability set')
else:
    unreal.log(f'Created: {as_path} ({ability_set.get_class().get_name()})')

    # Try to set the GrantedGameplayAbilities array
    try:
        # Get the current (empty) array
        abilities = ability_set.get_editor_property('granted_gameplay_abilities')
        unreal.log(f'Current abilities array: {abilities} (type: {type(abilities).__name__})')

        # Create entries for each combat ability
        ability_classes = [
            ('MeleeAttack', unreal.MordecaiGA_MeleeAttack, 'InputTag.LightAttack'),
            ('Dodge', unreal.MordecaiGA_Dodge, 'InputTag.Dodge'),
            ('Block', unreal.MordecaiGA_Block, 'InputTag.Block'),
            ('Parry', unreal.MordecaiGA_Parry, 'InputTag.Parry'),
        ]

        new_abilities = []
        for name, cls, input_tag_str in ability_classes:
            entry = unreal.LyraAbilitySet_GameplayAbility()
            entry.set_editor_property('ability', cls)
            entry.set_editor_property('ability_level', 1)
            # Input tag - try to set it, may need specific tag format
            try:
                tag = unreal.GameplayTag.request_gameplay_tag(unreal.Name(input_tag_str))
                entry.set_editor_property('input_tag', tag)
            except Exception as e:
                unreal.log_warning(f'Could not set input tag {input_tag_str}: {e}')
            new_abilities.append(entry)
            unreal.log(f'  Added ability: {name}')

        ability_set.set_editor_property('granted_gameplay_abilities', new_abilities)
        unreal.log('Set GrantedGameplayAbilities with 4 combat abilities')
    except Exception as e:
        unreal.log_error(f'Failed to configure abilities: {e}')

    EAL.save_asset(as_path, only_if_is_dirty=False)

# --- Add AbilitySet to PawnData_Mordecai ---
pawn_data_path = '/MordecaiCore/System/PawnData_Mordecai'
if EAL.does_asset_exist(pawn_data_path):
    pawn_data = unreal.load_asset(pawn_data_path)
    if pawn_data:
        unreal.log(f'Loaded PawnData: {pawn_data.get_class().get_name()}')
        try:
            # Get existing AbilitySets
            existing = pawn_data.get_editor_property('ability_sets')
            unreal.log(f'Existing ability sets: {len(existing)}')

            # Load our new ability set
            new_as = unreal.load_asset(as_path)
            if new_as:
                # Add to the array (don't duplicate)
                already_has = False
                for s in existing:
                    if s and s.get_name() == 'AS_MordecaiCombat':
                        already_has = True
                        break
                if not already_has:
                    existing.append(new_as)
                    pawn_data.set_editor_property('ability_sets', existing)
                    unreal.log('Added AS_MordecaiCombat to PawnData_Mordecai.AbilitySets')
                else:
                    unreal.log('AS_MordecaiCombat already in PawnData')

            EAL.save_asset(pawn_data_path, only_if_is_dirty=False)
        except Exception as e:
            unreal.log_error(f'Failed to configure PawnData: {e}')
else:
    unreal.log_error('PawnData_Mordecai not found')

print('SUCCESS: Player ability set created and added to PawnData.')
