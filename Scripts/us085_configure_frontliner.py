"""US-085: configure the arena's Frontliners and SAVE the content (run in the editor via remote exec).

1. The body (issue #19 / N14).  The packaged Frontliners had no mesh: players saw only their red
   health bars, and their capsules were an invisible wall.  AMordecaiEnemyCharacter has an empty,
   non-colliding BodyMesh; this sets it on BP_MordecaiEnemy_Frontliner to a dark red engine
   cylinder (MI_Enemy_Frontliner) the size of the enemy's capsule, so the body shows exactly what
   blocks the player.  The Blueprint is compiled, saved and read back.
2. The base stats (issue #18 / N13).  The three Frontliners placed in DevTestMap take their base
   stats from the Blueprint again.  The Blueprint has had DefaultMaxHealth = 200 (AC-054.2) since
   US-054, but the level saved DefaultMaxHealth = 100 on each placed actor, an instance override,
   so the packaged enemies spawned with 100/100.
   The stats are EditDefaultsOnly, so Python cannot write them on a placed actor ("cannot be
   edited on instances").  A Python write to a template, however, also writes every loaded
   instance that still holds the template's value.  So the class default object is set to the
   stale value (every placed actor now matches it) and back: all of them follow it back, equal
   their archetype, and the level save drops the override.  The writes on the class default object
   do not notify, so the Blueprint asset is not dirtied by this part.
3. The attack profiles.  The base Frontliner uses BasicSlash only.  A Blueprint child uses
   FireSlash only, and exactly one of DevTestMap's three existing Frontliners is replaced by that
   child without changing its transform.  EnemyAttackProfiles remains EditDefaultsOnly.

Every part is rerunnable: it saves only what it changed.

Usage (an absolute path: the editor's working directory is not the project root):
    py Scripts/ue_remote_exec.py "exec(open(r'C:/Users/jeffd/Documents/Gamedev/ProjectMordecai/Scripts/us085_configure_frontliner.py').read())"
"""
import unreal

FRONTLINER_BP = '/MordecaiCore/Enemies/BP_MordecaiEnemy_Frontliner'
FIRE_FRONTLINER_BP = '/MordecaiCore/Enemies/BP_MordecaiEnemy_Frontliner_Fire'
ARENA_MAP = '/MordecaiCore/Game/Maps/DevTestMap'
ATTACK_PROFILE_FOLDER = '/MordecaiCore/Combat/Enemies'
BASIC_SLASH = f'{ATTACK_PROFILE_FOLDER}/DA_EnemyAttack_BasicSlash'
FIRE_SLASH = f'{ATTACK_PROFILE_FOLDER}/DA_EnemyAttack_FireSlash'
EXPECTED_MAX_HEALTH = 200.0  # AC-054.2
STATS = ('default_max_health', 'default_max_posture', 'default_move_speed')
NO_NOTIFY = unreal.PropertyAccessChangeNotifyMode.NEVER

BODY_MESH = '/Engine/BasicShapes/Cylinder'
BASE_MATERIAL = '/Engine/BasicShapes/BasicShapeMaterial'
BODY_MATERIAL_FOLDER = '/MordecaiCore/Enemies/Materials'
BODY_MATERIAL_NAME = 'MI_Enemy_Frontliner'
BODY_MATERIAL = f'{BODY_MATERIAL_FOLDER}/{BODY_MATERIAL_NAME}'
BODY_COLOR = unreal.LinearColor(0.55, 0.04, 0.04, 1.0)  # dark red: not the player's grey, not the potion's bright red
TOLERANCE = 0.01

EAL = unreal.EditorAssetLibrary


def dirty_package_names():
    return [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()]


def near(a, b):
    return abs(a - b) <= TOLERANCE


def vectors_near(a, b):
    return near(a.x, b.x) and near(a.y, b.y) and near(a.z, b.z)


def colors_near(a, b):
    return near(a.r, b.r) and near(a.g, b.g) and near(a.b, b.b) and near(a.a, b.a)


# --- 1. the body ------------------------------------------------------------------------------

def ensure_body_material():
    """MI_Enemy_Frontliner: BasicShapeMaterial with a dark red Color.  Created once, then only
    updated, so the Blueprint's reference to it survives a rerun."""
    mel = unreal.MaterialEditingLibrary
    mi = EAL.load_asset(BODY_MATERIAL) if EAL.does_asset_exist(BODY_MATERIAL) else None
    changed = False
    if mi is None:
        EAL.make_directory(BODY_MATERIAL_FOLDER)
        mi = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            BODY_MATERIAL_NAME, BODY_MATERIAL_FOLDER, unreal.MaterialInstanceConstant,
            unreal.MaterialInstanceConstantFactoryNew())
        if mi is None:
            raise RuntimeError(f'could not create {BODY_MATERIAL}')
        changed = True
    parent = unreal.load_asset(BASE_MATERIAL)
    if parent is None:
        raise RuntimeError(f'{BASE_MATERIAL} not found')
    if mi.get_editor_property('parent') != parent:
        mel.set_material_instance_parent(mi, parent)
        changed = True
    if not colors_near(mel.get_material_instance_vector_parameter_value(mi, 'Color'), BODY_COLOR):
        # Its bool result is always False (UE 5.7 never sets it): read the value back instead
        mel.set_material_instance_vector_parameter_value(mi, 'Color', BODY_COLOR)
        color = mel.get_material_instance_vector_parameter_value(mi, 'Color')
        if not colors_near(color, BODY_COLOR):
            raise RuntimeError(f"{BODY_MATERIAL} Color is {color} after setting {BODY_COLOR}")
        changed = True
    if changed or BODY_MATERIAL in dirty_package_names():
        if not EAL.save_loaded_asset(mi, only_if_is_dirty=False):
            raise RuntimeError(f'save failed for {BODY_MATERIAL}')
        print(f'saved {BODY_MATERIAL}: Color {BODY_COLOR}')
    else:
        print(f'{BODY_MATERIAL} already set up')
    return mi


def body_transform(capsule, mesh):
    """Scale and offset that make the mesh fill the capsule: same radius, same height, centred."""
    radius = capsule.get_unscaled_capsule_radius()
    half_height = capsule.get_unscaled_capsule_half_height()
    origin, extent = mesh.get_bounds().origin, mesh.get_bounds().box_extent
    scale = unreal.Vector(radius / extent.x, radius / extent.y, half_height / extent.z)
    location = unreal.Vector(-origin.x * scale.x, -origin.y * scale.y, -origin.z * scale.z)
    return scale, location, radius, half_height


def body_problems(body, mesh, material, scale, location):
    problems = []
    if body is None:
        return ['no BodyMesh component (is the editor built with US-085 / issue #19?)']
    if body.get_editor_property('static_mesh') != mesh:
        problems.append(f"mesh {body.get_editor_property('static_mesh')}")
    if list(body.get_editor_property('override_materials')) != [material]:
        problems.append(f"materials {list(body.get_editor_property('override_materials'))}")
    if not vectors_near(body.get_editor_property('relative_scale3d'), scale):
        problems.append(f"scale {body.get_editor_property('relative_scale3d')}")
    if not vectors_near(body.get_editor_property('relative_location'), location):
        problems.append(f"location {body.get_editor_property('relative_location')}")
    if body.get_collision_enabled() != unreal.CollisionEnabled.NO_COLLISION:
        problems.append(f'collision {body.get_collision_enabled()}')
    if not body.is_visible() or body.get_editor_property('hidden_in_game'):
        problems.append('hidden')
    return problems


def configure_body(frontliner_class):
    mesh = unreal.load_asset(BODY_MESH)
    if mesh is None:
        raise RuntimeError(f'{BODY_MESH} not found')
    material = ensure_body_material()

    class_defaults = unreal.get_default_object(frontliner_class)
    scale, location, radius, half_height = body_transform(
        class_defaults.get_editor_property('capsule_component'), mesh)
    print(f'capsule radius {radius:g}, half height {half_height:g} -> body scale {scale}, '
          f'location {location}')

    def template():
        # Looked up again each time: a compile makes a new class default object
        return unreal.get_default_object(EAL.load_blueprint_class(FRONTLINER_BP)).get_editor_property('body_mesh')

    if not body_problems(template(), mesh, material, scale, location):
        print(f'{FRONTLINER_BP} BodyMesh already set: nothing to save')
        return

    blueprint = unreal.load_asset(FRONTLINER_BP)
    for attempt in (1, 2):  # tribal knowledge: compile twice, properties can reset on the first compile
        body = template()
        if body is None:
            raise RuntimeError(f'{FRONTLINER_BP} has no BodyMesh: build the editor with issue #19 first')
        body.set_editor_property('static_mesh', mesh)
        body.set_editor_property('override_materials', [material])
        body.set_editor_property('relative_scale3d', scale)
        body.set_editor_property('relative_location', location)
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
        problems = body_problems(template(), mesh, material, scale, location)
        print(f'compile {attempt}: {problems or "BodyMesh set"}')
        if not problems:
            break
    else:
        raise RuntimeError(f'{FRONTLINER_BP} BodyMesh did not keep its values: {problems}')

    if not EAL.save_loaded_asset(blueprint, only_if_is_dirty=False):
        raise RuntimeError(f'save failed for {FRONTLINER_BP}')
    print(f'saved {FRONTLINER_BP}')


# --- 2. the attack profiles -------------------------------------------------------------------

def ensure_fire_frontliner(base_class):
    if not EAL.does_asset_exist(FIRE_FRONTLINER_BP):
        folder, name = FIRE_FRONTLINER_BP.rsplit('/', 1)
        factory = unreal.BlueprintFactory()
        factory.set_editor_property('parent_class', base_class)
        created = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, folder, unreal.Blueprint, factory)
        if created is None:
            raise RuntimeError(f'could not create {FIRE_FRONTLINER_BP}')
        print(f'created {FIRE_FRONTLINER_BP}')
    blueprint = EAL.load_asset(FIRE_FRONTLINER_BP)
    generated = EAL.load_blueprint_class(FIRE_FRONTLINER_BP)
    if blueprint is None or generated is None:
        raise RuntimeError(f'{FIRE_FRONTLINER_BP} does not load as a Blueprint class')
    if not unreal.MathLibrary.class_is_child_of(generated, base_class):
        raise RuntimeError(f'{FIRE_FRONTLINER_BP} is not a child of {FRONTLINER_BP}')
    return blueprint


def configure_attack_profiles(base_class):
    profiles = {
        FRONTLINER_BP: [unreal.load_asset(BASIC_SLASH)],
        FIRE_FRONTLINER_BP: [unreal.load_asset(FIRE_SLASH)],
    }
    if any(value[0] is None for value in profiles.values()):
        raise RuntimeError(f'attack profile missing: {profiles}')
    ensure_fire_frontliner(base_class)

    for path, expected in profiles.items():
        blueprint = EAL.load_asset(path)

        def current():
            generated = EAL.load_blueprint_class(path)
            return list(unreal.get_default_object(generated).get_editor_property('enemy_attack_profiles'))

        if current() == expected and path not in dirty_package_names():
            print(f'{path} already uses {[p.get_name() for p in expected]}')
            continue
        for attempt in (1, 2):
            generated = EAL.load_blueprint_class(path)
            unreal.get_default_object(generated).set_editor_property('enemy_attack_profiles', expected)
            unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
            if current() == expected:
                break
            print(f'{path} compile {attempt} did not retain the attack profile')
        else:
            raise RuntimeError(f'{path} attack profiles are {current()}, expected {expected}')
        if not EAL.save_loaded_asset(blueprint, only_if_is_dirty=False):
            raise RuntimeError(f'save failed for {path}')
        print(f'saved {path}: {[p.get_name() for p in expected]}')

    return EAL.load_blueprint_class(FIRE_FRONTLINER_BP)


# --- 3. the base stats and placement -----------------------------------------------------------

def placed_frontliners(frontliner_class):
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    return [a for a in actors
            if unreal.MathLibrary.class_is_child_of(a.get_class(), frontliner_class)]


def stale_stats(actors, class_defaults):
    """[(actor, stat, value, class_value)] for every stat an actor overrides."""
    return [(a, s, a.get_editor_property(s), class_defaults.get_editor_property(s))
            for a in actors for s in STATS
            if a.get_editor_property(s) != class_defaults.get_editor_property(s)]


def clear_stat_overrides(frontliner_class, level_editor):
    base_defaults = unreal.get_default_object(frontliner_class)
    bp_health = base_defaults.get_editor_property('default_max_health')
    if bp_health != EXPECTED_MAX_HEALTH:
        raise RuntimeError(f'{FRONTLINER_BP} DefaultMaxHealth is {bp_health}, expected {EXPECTED_MAX_HEALTH}')
    print(f'{FRONTLINER_BP}: DefaultMaxHealth {bp_health} (AC-054.2)')

    actors = placed_frontliners(frontliner_class)
    if len(actors) != 3:
        raise RuntimeError(f'{len(actors)} Frontliners placed in {ARENA_MAP}, expected 3')

    stale = []
    for actor in actors:
        stale += stale_stats([actor], unreal.get_default_object(actor.get_class()))
    if not stale:
        print(f'no placed Frontliner overrides {STATS}: nothing to save')
        return actors
    for actor, stat, value, class_value in stale:
        print(f'{actor.get_actor_label()} ({actor.get_name()}) overrides {stat}: {value} (Blueprint {class_value})')
        actor.modify()  # the propagated write does not notify: dirty the level for the save

    for actor_class in {actor.get_class() for actor, _, _, _ in stale}:
        class_defaults = unreal.get_default_object(actor_class)
        for stat in STATS:
            class_value = class_defaults.get_editor_property(stat)
            for value in {v for actor, s, v, _ in stale
                          if actor.get_class() == actor_class and s == stat}:
                class_defaults.set_editor_property(stat, value, NO_NOTIFY)
                class_defaults.set_editor_property(stat, class_value, NO_NOTIFY)

    left = []
    for actor in actors:
        left += stale_stats([actor], unreal.get_default_object(actor.get_class()))
    if left:
        raise RuntimeError('still overridden, level NOT saved: '
                           + '; '.join(f'{a.get_name()} {s} {v} (Blueprint {c})' for a, s, v, c in left))
    for actor in actors:
        loc = actor.get_actor_location()
        print(f'{actor.get_actor_label()} at ({loc.x:.0f}, {loc.y:.0f}): '
              + ', '.join(f'{s} {actor.get_editor_property(s)}' for s in STATS))

    if not level_editor.save_current_level():
        raise RuntimeError(f'save_current_level failed for {ARENA_MAP}')
    print(f'saved {ARENA_MAP}')
    return actors


def ensure_one_fire_frontliner(frontliner_class, fire_class, level_editor):
    actors = placed_frontliners(frontliner_class)
    base = [a for a in actors if a.get_class() == frontliner_class]
    fire = [a for a in actors if a.get_class() == fire_class]
    if len(base) == 2 and len(fire) == 1:
        print(f'{fire[0].get_actor_label()} already uses {FIRE_FRONTLINER_BP}')
        return actors
    if len(base) != 3 or fire:
        raise RuntimeError(f'expected 3 base and 0 fire Frontliners, got {len(base)} base/{len(fire)} fire')

    old = sorted(base, key=lambda actor: actor.get_actor_label())[0]
    transform = old.get_actor_transform()
    label = old.get_actor_label()
    actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    replacement = actor_sub.spawn_actor_from_class(
        fire_class, transform.translation, transform.rotation.rotator())
    if replacement is None:
        raise RuntimeError(f'could not replace {label} with {FIRE_FRONTLINER_BP}')
    replacement.set_actor_transform(transform, sweep=False, teleport=False)
    replacement.set_actor_label(label)
    if not vectors_near(replacement.get_actor_location(), transform.translation):
        actor_sub.destroy_actor(replacement)
        raise RuntimeError(f'{label} replacement did not retain its transform')
    actor_sub.destroy_actor(old)
    if not level_editor.save_current_level():
        raise RuntimeError(f'save_current_level failed for {ARENA_MAP}')
    print(f'saved {ARENA_MAP}: {label} now uses {FIRE_FRONTLINER_BP} at {transform.translation}')
    return placed_frontliners(frontliner_class)


def main():
    # The cook reads the Blueprint from disk: an unsaved edit would make the placed actors match a value no player gets
    if FRONTLINER_BP in dirty_package_names():
        raise RuntimeError(f'{FRONTLINER_BP} has unsaved changes: save or revert it first')

    frontliner_class = EAL.load_blueprint_class(FRONTLINER_BP)
    configure_body(frontliner_class)
    frontliner_class = EAL.load_blueprint_class(FRONTLINER_BP)  # the compile made a new class
    fire_class = configure_attack_profiles(frontliner_class)
    frontliner_class = EAL.load_blueprint_class(FRONTLINER_BP)
    fire_class = EAL.load_blueprint_class(FIRE_FRONTLINER_BP)

    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if not world or not world.get_path_name().startswith(ARENA_MAP + '.'):
        if not level_editor.load_level(ARENA_MAP):
            raise RuntimeError(f'could not load {ARENA_MAP}')
    actors = clear_stat_overrides(frontliner_class, level_editor)
    actors = ensure_one_fire_frontliner(frontliner_class, fire_class, level_editor)

    # The placed Frontliners have no BodyMesh data of their own: they show their Blueprint's body
    for actor in actors:
        body = actor.get_editor_property('body_mesh')
        mesh = body.get_editor_property('static_mesh') if body else None
        print(f"{actor.get_actor_label()}: BodyMesh {mesh.get_path_name() if mesh else None}")
        if mesh is None:
            raise RuntimeError(f'{actor.get_actor_label()} in {ARENA_MAP} has no body mesh')

    print(f'dirty content packages: {dirty_package_names()}')


main()
