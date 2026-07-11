"""US-079: Place the 6 test pickups in DevTestMap (AC-079.6 / AC-079.10).

Spawns one AMordecaiItemPickup per test item DataAsset in a short loop around
the arena player spawn (origin), each with a distinct placeholder shape +
colored material instance so they read at a glance. Rerunnable: deletes any
existing 'TestPickup_*' actors first. Saves the level and all created assets.

Prereq: run create_test_pickups.py first.
Run inside the editor via:
  py ue_remote_exec.py "exec(open(r'Scripts/place_test_pickups.py').read())"
"""
import unreal

EAL = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

MAP_PATH = '/MordecaiCore/Game/Maps/DevTestMap'
ITEM_FOLDER = '/MordecaiCore/Items/TestPickups'
MAT_FOLDER = f'{ITEM_FOLDER}/Materials'
BASE_MATERIAL = '/Engine/BasicShapes/BasicShapeMaterial'

# (actor label, item asset, shape, color RGBA, world location)
# Loop layout: reachable arc ~4m from the spawn at origin, < 30s to collect all.
PICKUPS = [
    ('TestPickup_HealthPotion',     'DA_TestItem_HealthPotion',     '/Engine/BasicShapes/Cylinder', (0.9, 0.1, 0.1, 1.0), unreal.Vector(400, 0, 30)),
    ('TestPickup_IronOre',          'DA_TestItem_IronOre',          '/Engine/BasicShapes/Cube',     (0.4, 0.4, 0.45, 1.0), unreal.Vector(300, 300, 30)),
    ('TestPickup_TownStone',        'DA_TestItem_TownStone',        '/Engine/BasicShapes/Cube',     (0.6, 0.5, 0.3, 1.0), unreal.Vector(0, 420, 30)),
    ('TestPickup_UpgradeKey',       'DA_TestItem_UpgradeKey',       '/Engine/BasicShapes/Cone',     (0.15, 0.4, 1.0, 1.0), unreal.Vector(-300, 300, 30)),
    ('TestPickup_MysteriousAmulet', 'DA_TestItem_MysteriousAmulet', '/Engine/BasicShapes/Sphere',   (0.6, 0.2, 0.9, 1.0), unreal.Vector(-400, 0, 30)),
    ('TestPickup_RustySword',       'DA_TestItem_RustySword',       '/Engine/BasicShapes/Cylinder', (0.55, 0.35, 0.2, 1.0), unreal.Vector(-300, -300, 30)),
]


def ensure_map_loaded():
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if world and MAP_PATH.split('/')[-1] in world.get_name():
        return
    level_sub.load_level(MAP_PATH)
    unreal.log(f'Loaded map: {MAP_PATH}')


def make_color_material(name, rgba):
    path = f'{MAT_FOLDER}/{name}'
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    factory = unreal.MaterialInstanceConstantFactoryNew()
    mi = asset_tools.create_asset(name, MAT_FOLDER, unreal.MaterialInstanceConstant, factory)
    if mi is None:
        unreal.log_warning(f'Could not create material instance {name}')
        return None
    parent = unreal.load_asset(BASE_MATERIAL)
    unreal.MaterialEditingLibrary.set_material_instance_parent(mi, parent)
    try:
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
            mi, 'Color', unreal.LinearColor(*rgba))
    except Exception as e:
        unreal.log_warning(f'Could not set Color parameter on {name}: {e}')
    EAL.save_asset(path, only_if_is_dirty=False)
    return mi


def main():
    ensure_map_loaded()
    EAL.make_directory(MAT_FOLDER)

    # Rerunnable: clear previous placements
    for actor in actor_sub.get_all_level_actors():
        label = actor.get_actor_label()
        if label and label.startswith('TestPickup_'):
            actor_sub.destroy_actor(actor)

    placed = 0
    with unreal.ScopedEditorTransaction('Place US-079 test pickups'):
        for label, item_asset_name, shape_path, rgba, location in PICKUPS:
            item_def = unreal.load_asset(f'{ITEM_FOLDER}/{item_asset_name}')
            if item_def is None:
                unreal.log_error(f'Missing item DataAsset {item_asset_name} — run create_test_pickups.py first')
                continue

            pickup = actor_sub.spawn_actor_from_class(unreal.MordecaiItemPickup, location)
            if pickup is None:
                unreal.log_error(f'Failed to spawn pickup {label}')
                continue
            pickup.set_actor_label(label)
            pickup.set_editor_property('item_definition', item_def)
            pickup.set_editor_property('quantity', 1)

            mesh_comp = pickup.get_editor_property('display_mesh')
            shape = unreal.load_asset(shape_path)
            if mesh_comp and shape:
                mesh_comp.set_static_mesh(shape)
                mesh_comp.set_relative_scale3d(unreal.Vector(0.35, 0.35, 0.35))
                mi = make_color_material(f'MI_{label}', rgba)
                if mi:
                    mesh_comp.set_material(0, mi)

            placed += 1
            unreal.log(f'Placed {label} at {location}')

    level_sub.save_current_level()
    print(f'RESULT: placed {placed}/{len(PICKUPS)} test pickups in DevTestMap')
    if placed != len(PICKUPS):
        raise RuntimeError('place_test_pickups failed for one or more pickups')


main()
