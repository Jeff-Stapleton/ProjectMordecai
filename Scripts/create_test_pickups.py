"""US-079: Create the 6 test-pickup item DataAssets (AC-079.4 / AC-079.5).

Covers every major routing case:
  - DA_TestItem_HealthPotion    Consumable, Common, stackable x10
  - DA_TestItem_IronOre         Material, Common, auto-store -> TownStorage
  - DA_TestItem_TownStone       TownResource, Common, auto-store
  - DA_TestItem_UpgradeKey      UpgradeKey, Rare (Blue), SortPriority=Critical
  - DA_TestItem_MysteriousAmulet MagicalItem, Epic (Purple), unidentified
  - DA_TestItem_RustySword      Weapon, Common

Outputs to /MordecaiCore/Items/TestPickups/
Run inside the editor via:
  py ue_remote_exec.py "exec(open(r'Scripts/create_test_pickups.py').read())"
"""
import unreal

EAL = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

FOLDER = '/MordecaiCore/Items/TestPickups'
EAL.make_directory(FOLDER)

ITEMS = {
    'DA_TestItem_HealthPotion': {
        'item_id': 'TestItem_HealthPotion',
        'display_name': 'Health Potion',
        'description': 'Restores a chunk of health. The Alchemist limits how many you can field.',
        'item_type': unreal.MordecaiItemType.CONSUMABLE,
        'rarity': unreal.MordecaiItemRarity.COMMON,
        'stackable': True,
        'max_stack_size': 10,
    },
    'DA_TestItem_IronOre': {
        'item_id': 'TestItem_IronOre',
        'display_name': 'Iron Ore',
        'description': 'Raw ore for town smithing. Auto-stored on pickup.',
        'item_type': unreal.MordecaiItemType.MATERIAL,
        'rarity': unreal.MordecaiItemRarity.COMMON,
        'auto_store_on_pickup': True,
        'storage_domain': unreal.MordecaiStorageDomain.TOWN_STORAGE,
        'stackable': True,
        'max_stack_size': 99,
    },
    'DA_TestItem_TownStone': {
        'item_id': 'TestItem_TownStone',
        'display_name': 'Town Stone',
        'description': 'Building material for village reconstruction. Auto-stored on pickup.',
        'item_type': unreal.MordecaiItemType.TOWN_RESOURCE,
        'rarity': unreal.MordecaiItemRarity.COMMON,
        'auto_store_on_pickup': True,
        'storage_domain': unreal.MordecaiStorageDomain.TOWN_STORAGE,
        'stackable': True,
        'max_stack_size': 99,
    },
    'DA_TestItem_UpgradeKey': {
        'item_id': 'TestItem_UpgradeKey',
        'display_name': 'Upgrade Key',
        'description': 'Unlocks a town progression upgrade. Sorts to the top of the inventory.',
        'item_type': unreal.MordecaiItemType.UPGRADE_KEY,
        'rarity': unreal.MordecaiItemRarity.BLUE,
        'sort_priority': unreal.MordecaiSortPriority.CRITICAL,
    },
    'DA_TestItem_MysteriousAmulet': {
        'item_id': 'TestItem_MysteriousAmulet',
        'display_name': 'Mysterious Amulet',
        'description': 'A faintly humming amulet. The Mage Tower can identify it.',
        'short_description': 'An amulet of unknown power.',
        'subtype': 'Amulet',
        'item_type': unreal.MordecaiItemType.MAGICAL_ITEM,
        'rarity': unreal.MordecaiItemRarity.PURPLE,
        'uses_identification': True,
        'default_identification_state': unreal.MordecaiIdentificationState.UNIDENTIFIED,
        'requires_identification_to_equip': True,
    },
    'DA_TestItem_RustySword': {
        'item_id': 'TestItem_RustySword',
        'display_name': 'Rusty Sword',
        'description': 'A worn blade. Barely holds an edge, but it is honest work.',
        'subtype': 'Sword',
        'item_type': unreal.MordecaiItemType.WEAPON,
        'rarity': unreal.MordecaiItemRarity.COMMON,
    },
}


def create_item(name, config):
    path = f'{FOLDER}/{name}'
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)

    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.MordecaiItemDefinition)
    asset = asset_tools.create_asset(name, FOLDER, None, factory)
    if asset is None:
        unreal.log_error(f'Failed to create {name}')
        return None

    for key, value in config.items():
        try:
            if key in ('display_name', 'description', 'short_description'):
                asset.set_editor_property(key, unreal.Text(value))
            else:
                asset.set_editor_property(key, value)
        except Exception as e:
            unreal.log_warning(f'  Could not set {key} on {name}: {e}')

    EAL.save_asset(path, only_if_is_dirty=False)
    unreal.log(f'Created item: {path}')
    return asset


created = 0
for asset_name, asset_config in ITEMS.items():
    if create_item(asset_name, asset_config):
        created += 1

print(f'RESULT: created {created}/{len(ITEMS)} test pickup item DataAssets')
if created != len(ITEMS):
    raise RuntimeError('create_test_pickups failed for one or more assets')
