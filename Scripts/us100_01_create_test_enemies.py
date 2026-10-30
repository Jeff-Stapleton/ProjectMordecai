# US-100 (issue #46, T1): create and SAVE the CombatTestMap's two test enemies (run in the editor via remote exec).
#
# Five data-only assets, no C++ (stories/in-progress/US-100-recovery-playtest-test-maps.md, Technical Notes):
# - /MordecaiCore/Enemies/BP_MordecaiEnemyAI_TestDummy: an AI controller Blueprint (parent MordecaiEnemyAIController)
#   with AggroRange 0, so it never leaves Idle and never targets a decoy. The other AI values stay default.
# - /MordecaiCore/Enemies/BP_MordecaiEnemy_TestDummy: an enemy Blueprint (parent MordecaiEnemyCharacter, like the
#   Frontliner, not a child of it). The AI above, DefaultMoveSpeed 0, DefaultMaxHealth 200, a blue-grey body.
# - /MordecaiCore/Enemies/BP_MordecaiEnemy_TestAttacker: an enemy Blueprint with the native AI (AggroRange 900,
#   AttackRange 200, LeashRange 1800, AttackCooldownMs 1500), DefaultMoveSpeed 0 (it holds its spot),
#   DefaultMaxHealth 200, an orange body. Its only attack profile is DA_EnemyAttack_BasicSlash: the AI swings
#   profile 0 only (N24), so a second profile would never be used.
# - /MordecaiCore/Enemies/Materials/MI_Enemy_TestDummy and MI_Enemy_TestAttacker, made like MI_Enemy_Frontliner.
# Both enemies keep the melee ability, an attack profile, a health bar and a body, so ValidateContent and
# TryEnterAttackState log nothing; the dummy is made harmless by its AI, not by taking those away.
# Base stats live on the Blueprint class defaults only. Never write them on a placed actor: the level saves an
# invisible instance override (N13) and ValidateContent logs it. Scripts/us100_02_create_test_maps.py places them.
#
# Rerunnable: it creates what is missing, writes only the values that differ, compiles twice (a first compile can
# reset properties), saves only what changed, then reads every asset back. Prints PASS/FAIL lines and a RESULT line,
# and raises RuntimeError on any FAIL.
#
# Usage (an absolute path: the editor's working directory is not the project root):
#   py Scripts/ue_remote_exec.py "exec(open(r'C:/Users/jeffd/Documents/Gamedev/ProjectMordecai/Scripts/us100_01_create_test_enemies.py').read())" --timeout 300
import os

import unreal

ENEMY_FOLDER = '/MordecaiCore/Enemies'
MATERIAL_FOLDER = '/MordecaiCore/Enemies/Materials'
AI_DUMMY_BP = f'{ENEMY_FOLDER}/BP_MordecaiEnemyAI_TestDummy'
DUMMY_BP = f'{ENEMY_FOLDER}/BP_MordecaiEnemy_TestDummy'
ATTACKER_BP = f'{ENEMY_FOLDER}/BP_MordecaiEnemy_TestAttacker'
FRONTLINER_BP = f'{ENEMY_FOLDER}/BP_MordecaiEnemy_Frontliner'
BASIC_SLASH = '/MordecaiCore/Combat/Enemies/DA_EnemyAttack_BasicSlash'
HEALTH_BAR = '/MordecaiCore/UI/WBP_EnemyHealthBar'
BODY_MESH = '/Engine/BasicShapes/Cylinder'
BASE_MATERIAL = '/Engine/BasicShapes/BasicShapeMaterial'
MORDECAI_CORE_CONTENT = 'Plugins/GameFeatures/MordecaiCore/Content'

MAX_HEALTH = 200.0
MOVE_SPEED = 0.0
DUMMY_AGGRO_RANGE = 0.0
# The attacker keeps the native AI's C++ defaults (the "known cadence": one BasicSlash about every 2.6 s)
NATIVE_AI = (('aggro_range', 900.0), ('attack_range', 200.0), ('leash_range', 1800.0), ('attack_cooldown_ms', 1500.0))
BODY_SCALE = (0.8, 0.8, 1.8)  # the enemy capsule (radius 40, half height 90) on the 100 cm engine cylinder
DUMMY_MATERIAL = (f'{MATERIAL_FOLDER}/MI_Enemy_TestDummy', (0.25, 0.35, 0.75, 1.0))  # blue-grey
ATTACKER_MATERIAL = (f'{MATERIAL_FOLDER}/MI_Enemy_TestAttacker', (1.0, 0.45, 0.05, 1.0))  # orange
TOLERANCE = 0.01

EAL = unreal.EditorAssetLibrary


class Checks:
    def __init__(self):
        self.passed = 0
        self.failed = 0

    def check(self, name, condition, detail=''):
        if condition:
            self.passed += 1
            print(f'  PASS: {name}' + (f' -- {detail}' if detail else ''))
        else:
            self.failed += 1
            print(f'  FAIL: {name}' + (f' -- {detail}' if detail else ''))
        return condition


def dirty_package_names():
    return [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()]


def asset_file(package_path, extension='.uasset'):
    # /MordecaiCore/X/Y -> <project>/Plugins/GameFeatures/MordecaiCore/Content/X/Y.uasset
    relative = package_path[len('/MordecaiCore/'):]
    return os.path.normpath(os.path.join(unreal.SystemLibrary.get_project_directory(), MORDECAI_CORE_CONTENT,
                                         relative + extension))


def describe(value):
    if isinstance(value, unreal.Object):
        return value.get_path_name()
    if isinstance(value, (list, tuple, unreal.Array)):
        return [describe(v) for v in value]
    return value


def same(a, b):
    if isinstance(a, float) or isinstance(b, float):
        return a is not None and b is not None and abs(a - b) <= TOLERANCE
    return describe(a) == describe(b)


def vectors_near(a, b):
    return abs(a.x - b.x) <= TOLERANCE and abs(a.y - b.y) <= TOLERANCE and abs(a.z - b.z) <= TOLERANCE


def colors_near(a, b):
    return all(abs(x - y) <= TOLERANCE for x, y in ((a.r, b.r), (a.g, b.g), (a.b, b.b), (a.a, b.a)))


def require_properties(obj, names, what):
    # A renamed or missing C++ property must fail loudly, not silently create nothing
    missing = []
    for name in names:
        try:
            obj.get_editor_property(name)
        except Exception as e:
            missing.append(f'{name} ({e})')
    if missing:
        raise RuntimeError(f'{what} has no Python property {missing}: is the editor built from this branch?')


def load_class_or_fail(path, what):
    generated = EAL.load_blueprint_class(path) if EAL.does_asset_exist(path) else None
    if generated is None:
        raise RuntimeError(f'{what} {path} not found')
    return generated


# --- the body materials -----------------------------------------------------------------------

def ensure_material(path, rgba):
    # BasicShapeMaterial with a Color, like MI_Enemy_Frontliner. Created once, then only updated, so the Blueprint's
    # reference to it survives a rerun. Returns (material, saved).
    mel = unreal.MaterialEditingLibrary
    folder, name = path.rsplit('/', 1)
    color = unreal.LinearColor(*rgba)
    mi = EAL.load_asset(path) if EAL.does_asset_exist(path) else None
    changed = False
    if mi is None:
        EAL.make_directory(folder)
        mi = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        if mi is None:
            raise RuntimeError(f'could not create {path}')
        print(f'created {path}')
        changed = True
    parent = unreal.load_asset(BASE_MATERIAL)
    if parent is None:
        raise RuntimeError(f'{BASE_MATERIAL} not found')
    if mi.get_editor_property('parent') != parent:
        mel.set_material_instance_parent(mi, parent)
        changed = True
    if not colors_near(mel.get_material_instance_vector_parameter_value(mi, 'Color'), color):
        # Its bool result is always False (UE 5.7 never sets it): read the value back instead
        mel.set_material_instance_vector_parameter_value(mi, 'Color', color)
        actual = mel.get_material_instance_vector_parameter_value(mi, 'Color')
        if not colors_near(actual, color):
            raise RuntimeError(f'{path} Color is {actual} after setting {color}')
        changed = True
    saved = False
    if changed or path in dirty_package_names() or not os.path.isfile(asset_file(path)):
        if not EAL.save_loaded_asset(mi, only_if_is_dirty=False):
            raise RuntimeError(f'save failed for {path}')
        print(f'saved {path}: Color {rgba}')
        saved = True
    else:
        print(f'{path} already set up')
    return mi, saved


# --- the Blueprints ---------------------------------------------------------------------------

def ensure_blueprint(path, parent_class):
    # Returns (blueprint, created). An existing asset must already derive from parent_class.
    folder, name = path.rsplit('/', 1)
    created = False
    if not EAL.does_asset_exist(path):
        factory = unreal.BlueprintFactory()
        factory.set_editor_property('parent_class', parent_class)
        blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Blueprint, factory)
        if blueprint is None:
            raise RuntimeError(f'could not create {path}')
        print(f'created {path} (parent {parent_class.get_name()})')
        created = True
    blueprint = EAL.load_asset(path)
    generated = EAL.load_blueprint_class(path)
    if blueprint is None or generated is None:
        raise RuntimeError(f'{path} does not load as a Blueprint class')
    if not unreal.MathLibrary.class_is_child_of(generated, parent_class):
        raise RuntimeError(f'{path} is not a {parent_class.get_name()}: delete it and rerun')
    return blueprint, created


def body_transform(class_defaults, mesh):
    # Scale and offset that make the mesh fill the capsule: same radius, same height, centred (us085's body)
    capsule = class_defaults.get_editor_property('capsule_component')
    radius = capsule.get_unscaled_capsule_radius()
    half_height = capsule.get_unscaled_capsule_half_height()
    origin, extent = mesh.get_bounds().origin, mesh.get_bounds().box_extent
    scale = unreal.Vector(radius / extent.x, radius / extent.y, half_height / extent.z)
    location = unreal.Vector(-origin.x * scale.x, -origin.y * scale.y, -origin.z * scale.z)
    if not vectors_near(scale, unreal.Vector(*BODY_SCALE)):
        raise RuntimeError(f'body scale {scale} for capsule {radius}/{half_height}, expected {BODY_SCALE}')
    return scale, location


def body_problems(body, mesh, material, scale, location):
    if body is None:
        return ['no BodyMesh component (is the editor built with issue #19?)']
    problems = []
    if body.get_editor_property('static_mesh') != mesh:
        problems.append(f"mesh {body.get_editor_property('static_mesh')}")
    if list(body.get_editor_property('override_materials')) != [material]:
        problems.append(f"materials {describe(body.get_editor_property('override_materials'))}")
    if not vectors_near(body.get_editor_property('relative_scale3d'), scale):
        problems.append(f"scale {body.get_editor_property('relative_scale3d')}")
    if not vectors_near(body.get_editor_property('relative_location'), location):
        problems.append(f"location {body.get_editor_property('relative_location')}")
    if body.get_collision_enabled() != unreal.CollisionEnabled.NO_COLLISION:
        problems.append(f'collision {body.get_collision_enabled()}')
    if not body.is_visible() or body.get_editor_property('hidden_in_game'):
        problems.append('hidden')
    return problems


def configure_blueprint(path, settings, body=None):
    # settings: ((python property name, value), ...) on the class defaults; body: (mesh, material, scale, location)
    # for the BodyMesh template. Writes only what differs, compiles twice, saves. Returns True when it saved.
    blueprint = EAL.load_asset(path)

    def class_defaults():
        # Looked up again each time: a compile makes a new class default object
        return unreal.get_default_object(EAL.load_blueprint_class(path))

    require_properties(class_defaults(), [name for name, _ in settings] + (['body_mesh'] if body else []), path)

    def problems():
        defaults = class_defaults()
        found = [f'{name} {describe(defaults.get_editor_property(name))} (want {describe(value)})'
                 for name, value in settings if not same(defaults.get_editor_property(name), value)]
        if body:
            found += [f'BodyMesh {p}' for p in body_problems(defaults.get_editor_property('body_mesh'), *body)]
        return found

    left = problems()
    if not left and path not in dirty_package_names() and os.path.isfile(asset_file(path)):
        print(f'{path} already set up: nothing to save')
        return False
    for attempt in (1, 2):  # tribal knowledge: compile twice, properties can reset on the first compile
        defaults = class_defaults()
        for name, value in settings:
            if not same(defaults.get_editor_property(name), value):
                defaults.set_editor_property(name, value)
        if body:
            mesh, material, scale, location = body
            template = defaults.get_editor_property('body_mesh')
            if template is None:
                raise RuntimeError(f'{path} has no BodyMesh: build the editor with issue #19 first')
            template.set_editor_property('static_mesh', mesh)
            template.set_editor_property('override_materials', [material])
            template.set_editor_property('relative_scale3d', scale)
            template.set_editor_property('relative_location', location)
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
        left = problems()
        print(f'{path} compile {attempt}: {left or "all values set"}')
        if not left and attempt == 2:
            break
    if left:
        raise RuntimeError(f'{path} did not keep its values: {left}')
    if not EAL.save_loaded_asset(blueprint, only_if_is_dirty=False):
        raise RuntimeError(f'save failed for {path}')
    print(f'saved {path}')
    return True


# --- read back --------------------------------------------------------------------------------

def verify_enemy(checks, path, ai_class, aggro_range, material):
    # What Mordecai.Level.TestMaps.TestDummyNeverMovesOrAggroes / TestAttackerHoldsItsSpotAndSwings check, read from
    # the reloaded class defaults
    name = path.rsplit('/', 1)[1]
    enemy_class = load_class_or_fail(path, 'enemy Blueprint')
    defaults = unreal.get_default_object(enemy_class)
    checks.check(f'{name} is a MordecaiEnemyCharacter, not a Frontliner',
                 unreal.MathLibrary.class_is_child_of(enemy_class, unreal.MordecaiEnemyCharacter.static_class())
                 and not unreal.MathLibrary.class_is_child_of(enemy_class, load_class_or_fail(FRONTLINER_BP, 'Frontliner')))
    checks.check(f'{name} DefaultMoveSpeed is {MOVE_SPEED:g}', same(defaults.get_editor_property('default_move_speed'), MOVE_SPEED),
                 f"{defaults.get_editor_property('default_move_speed')}")
    checks.check(f'{name} DefaultMaxHealth is {MAX_HEALTH:g}', same(defaults.get_editor_property('default_max_health'), MAX_HEALTH),
                 f"{defaults.get_editor_property('default_max_health')}")
    possess = defaults.get_editor_property('auto_possess_ai')
    checks.check(f'{name} is possessed by its AI when placed',
                 possess in (unreal.AutoPossessAI.PLACED_IN_WORLD, unreal.AutoPossessAI.PLACED_IN_WORLD_OR_SPAWNED), f'{possess}')
    actual_ai = defaults.get_editor_property('ai_controller_class')
    checks.check(f'{name} AIControllerClass is {ai_class.get_name()}', same(actual_ai, ai_class), describe(actual_ai))
    if actual_ai:
        ai_defaults = unreal.get_default_object(actual_ai)
        checks.check(f'{name} AI AggroRange is {aggro_range:g}', same(ai_defaults.get_editor_property('aggro_range'), aggro_range),
                     f"{ai_defaults.get_editor_property('aggro_range')}")
        cooldown = ai_defaults.get_editor_property('attack_cooldown_ms')
        checks.check(f'{name} AI AttackCooldownMs is 1500', same(cooldown, 1500.0), f'{cooldown}')
        melee_class = ai_defaults.get_editor_property('melee_attack_ability_class')
        checks.check(f'{name} AI swings MordecaiGA_MeleeAttack',
                     melee_class is not None and unreal.MathLibrary.class_is_child_of(melee_class, unreal.MordecaiGA_MeleeAttack.static_class()),
                     describe(melee_class))
    abilities = defaults.get_editor_property('default_abilities')
    checks.check(f'{name} DefaultAbilities is [MordecaiGA_MeleeAttack]',
                 same(abilities, [unreal.MordecaiGA_MeleeAttack.static_class()]), f'{describe(abilities)}')
    profiles = defaults.get_editor_property('enemy_attack_profiles')
    checks.check(f'{name} EnemyAttackProfiles is [DA_EnemyAttack_BasicSlash]',
                 [p.get_path_name().split('.')[0] for p in profiles if p] == [BASIC_SLASH] and len(profiles) == 1,
                 f'{describe(profiles)}')
    health_bar = defaults.get_editor_property('enemy_health_bar_widget_class')
    checks.check(f'{name} EnemyHealthBarWidgetClass is WBP_EnemyHealthBar',
                 health_bar is not None and health_bar.get_path_name().startswith(HEALTH_BAR + '.'), describe(health_bar))
    body = defaults.get_editor_property('body_mesh')
    mesh = body.get_editor_property('static_mesh') if body else None
    materials = list(body.get_editor_property('override_materials')) if body else []
    checks.check(f'{name} BodyMesh is the engine cylinder with {material.get_name()}, no collision',
                 mesh is not None and mesh.get_path_name().startswith(BODY_MESH + '.') and materials == [material]
                 and body.get_collision_enabled() == unreal.CollisionEnabled.NO_COLLISION,
                 f'{describe(mesh)}, {describe(materials)}, scale {body.get_editor_property("relative_scale3d") if body else None}')


def main():
    checks = Checks()
    print('=' * 60)
    print('US-100: TEST ENEMIES (TestDummy, TestAttacker)')
    print('=' * 60)

    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
        raise RuntimeError('PIE is running: stop it first')
    # The cook reads assets from disk: an unsaved edit made elsewhere would be saved by this script unseen
    ours = [AI_DUMMY_BP, DUMMY_BP, ATTACKER_BP, DUMMY_MATERIAL[0], ATTACKER_MATERIAL[0]]
    dirty = [p for p in dirty_package_names() if p in ours]
    if dirty:
        raise RuntimeError(f'{dirty} have unsaved changes: save or revert them first')

    mesh = unreal.load_asset(BODY_MESH)
    slash = unreal.load_asset(BASIC_SLASH)
    health_bar = load_class_or_fail(HEALTH_BAR, 'health bar widget')
    if mesh is None or slash is None:
        raise RuntimeError(f'{BODY_MESH} or {BASIC_SLASH} not found')
    melee = unreal.MordecaiGA_MeleeAttack.static_class()
    native_ai = unreal.MordecaiEnemyAIController.static_class()
    enemy_parent = unreal.MordecaiEnemyCharacter.static_class()
    placed = unreal.AutoPossessAI.PLACED_IN_WORLD_OR_SPAWNED

    dummy_material, _ = ensure_material(*DUMMY_MATERIAL)
    attacker_material, _ = ensure_material(*ATTACKER_MATERIAL)

    # 1. The dummy's AI: never aggroes. Configured first: the dummy Blueprint references its class.
    ensure_blueprint(AI_DUMMY_BP, native_ai)
    configure_blueprint(AI_DUMMY_BP, (('aggro_range', DUMMY_AGGRO_RANGE),))
    dummy_ai = load_class_or_fail(AI_DUMMY_BP, 'dummy AI Blueprint')

    # 2. The two enemies: the same base, a different AI and colour
    for path, ai_class, material in ((DUMMY_BP, dummy_ai, dummy_material), (ATTACKER_BP, native_ai, attacker_material)):
        ensure_blueprint(path, enemy_parent)
        if unreal.MathLibrary.class_is_child_of(EAL.load_blueprint_class(path), load_class_or_fail(FRONTLINER_BP, 'Frontliner')):
            raise RuntimeError(f'{path} is a child of the Frontliner: delete it and rerun')
        scale, location = body_transform(unreal.get_default_object(EAL.load_blueprint_class(path)), mesh)
        configure_blueprint(path, (
            ('ai_controller_class', ai_class),
            ('auto_possess_ai', placed),
            ('default_move_speed', MOVE_SPEED),
            ('default_max_health', MAX_HEALTH),
            ('default_abilities', [melee]),
            ('enemy_attack_profiles', [slash]),
            ('enemy_health_bar_widget_class', health_bar),
        ), body=(mesh, material, scale, location))

    # 3. Read back
    print('')
    for path in ours:
        on_disk = asset_file(path)
        checks.check(f'{path} saved on disk', os.path.isfile(on_disk), on_disk)
    left_dirty = [p for p in dirty_package_names() if p in ours]
    checks.check('nothing this script owns is left unsaved', not left_dirty, f'{left_dirty}')

    ai_defaults = unreal.get_default_object(load_class_or_fail(AI_DUMMY_BP, 'dummy AI Blueprint'))
    checks.check('BP_MordecaiEnemyAI_TestDummy is a MordecaiEnemyAIController',
                 unreal.MathLibrary.class_is_child_of(dummy_ai, native_ai))
    for name, value in NATIVE_AI:
        want = DUMMY_AGGRO_RANGE if name == 'aggro_range' else value
        checks.check(f'BP_MordecaiEnemyAI_TestDummy {name} is {want:g}', same(ai_defaults.get_editor_property(name), want),
                     f'{ai_defaults.get_editor_property(name)}')
    native_defaults = unreal.get_default_object(native_ai)
    for name, value in NATIVE_AI:
        checks.check(f'native MordecaiEnemyAIController {name} is {value:g} (the attacker AI)',
                     same(native_defaults.get_editor_property(name), value), f'{native_defaults.get_editor_property(name)}')

    verify_enemy(checks, DUMMY_BP, dummy_ai, DUMMY_AGGRO_RANGE, dummy_material)
    verify_enemy(checks, ATTACKER_BP, native_ai, 900.0, attacker_material)
    for path, rgba in (DUMMY_MATERIAL, ATTACKER_MATERIAL):
        mi = EAL.load_asset(path)
        color = unreal.MaterialEditingLibrary.get_material_instance_vector_parameter_value(mi, 'Color')
        checks.check(f'{path.rsplit("/", 1)[1]} Color is {rgba}', colors_near(color, unreal.LinearColor(*rgba)), f'{color}')

    print('')
    print(f'RESULT: {"PASS" if checks.failed == 0 else "FAIL"} ({checks.passed} passed, {checks.failed} failed)')
    if checks.failed:
        raise RuntimeError(f'us100_01_create_test_enemies: {checks.failed} check(s) failed')


main()
