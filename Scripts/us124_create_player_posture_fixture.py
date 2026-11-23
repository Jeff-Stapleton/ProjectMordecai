"""Create and save US-124's deterministic packaged player-posture fixture.

Run in the editor through Scripts/ue_remote_exec.py. The map is a duplicate of
CombatTestMap's proven greybox/nav fixture, with five stationary children of the existing
TestAttacker. Their real melee profiles have staggered windups so one control hit, one posture
break, two broken-window hits and one knocked-down hit resolve in a deterministic sequence.
"""

import math
import os

import unreal

EAL = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
COMBAT_MAP = "/MordecaiCore/Game/Maps/CombatTestMap"
MAP = "/MordecaiCore/Game/Maps/PlayerPostureTestMap"
DEVTEST_MAP = "/MordecaiCore/Game/Maps/DevTestMap"
ATTACKER_BP = "/MordecaiCore/Enemies/BP_MordecaiEnemy_TestAttacker"
FIXTURE_FOLDER = "/MordecaiCore/Enemies/PlayerPostureTest"
PROFILE_FOLDER = "/MordecaiCore/Combat/Enemies/PlayerPostureTest"
AI_BP = f"{FIXTURE_FOLDER}/BP_MordecaiEnemyAI_PlayerPostureTest"
CONTENT_DIR = "Plugins/GameFeatures/MordecaiCore/Content"
START = (-1200.0, 0.0, 100.0)
FIGHT = (-100.0, 0.0)
ATTACK_COOLDOWN_MS = 10000.0

# name, windup ms, base health damage, posture scalar, polar angle around the fight point.
ATTACKS = (
    ("Control", 200.0, 20.0, 0.0, -80.0),
    ("Breaker", 500.0, 1.0, 100.0, -40.0),
    ("BonusA", 1000.0, 20.0, 0.0, 0.0),
    ("BonusB", 1400.0, 20.0, 0.0, 40.0),
    # Independent AI controllers can begin their attack loops on different frames.  Keep a
    # full second after BonusB so this hit cannot overtake it and land at the broken 1.5x rate.
    ("Down", 2400.0, 5.0, 0.0, 80.0),
)


def asset_file(path, extension=".uasset"):
    relative = path[len("/MordecaiCore/"):]
    return os.path.normpath(os.path.join(
        unreal.SystemLibrary.get_project_directory(), CONTENT_DIR, relative + extension))


def ensure_blueprint(path, parent):
    folder, name = path.rsplit("/", 1)
    EAL.make_directory(folder)
    if not EAL.does_asset_exist(path):
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", parent)
        if TOOLS.create_asset(name, folder, unreal.Blueprint, factory) is None:
            raise RuntimeError(f"could not create {path}")
        print(f"created {path}")
    blueprint = EAL.load_asset(path)
    generated = EAL.load_blueprint_class(path)
    if blueprint is None or generated is None or not unreal.MathLibrary.class_is_child_of(
            generated, parent):
        raise RuntimeError(f"{path} is not a child of {parent.get_name()}")
    return blueprint


def configure_blueprint(path, settings):
    blueprint = EAL.load_asset(path)
    for attempt in (1, 2):
        defaults = unreal.get_default_object(EAL.load_blueprint_class(path))
        for name, value in settings:
            defaults.set_editor_property(name, value)
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
        print(f"compiled {path} ({attempt}/2)")
    defaults = unreal.get_default_object(EAL.load_blueprint_class(path))
    for name, value in settings:
        actual = defaults.get_editor_property(name)
        if isinstance(value, (list, tuple)):
            actual_paths = [item.get_path_name().split(".")[0] for item in actual]
            expected_paths = [item.get_path_name().split(".")[0] for item in value]
            if actual_paths != expected_paths:
                raise RuntimeError(f"{path} {name} is {actual_paths}, expected {expected_paths}")
        elif actual != value:
            raise RuntimeError(f"{path} {name} is {actual}, expected {value}")
    if not EAL.save_loaded_asset(blueprint, only_if_is_dirty=False):
        raise RuntimeError(f"save failed for {path}")


def ensure_profile(name, windup, power, posture_scalar):
    path = f"{PROFILE_FOLDER}/DA_PlayerPostureTest_{name}"
    EAL.make_directory(PROFILE_FOLDER)
    profile = EAL.load_asset(path) if EAL.does_asset_exist(path) else None
    if profile is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.MordecaiAttackProfileDataAsset)
        profile = TOOLS.create_asset(path.rsplit("/", 1)[1], PROFILE_FOLDER, None, factory)
        if profile is None:
            raise RuntimeError(f"could not create {path}")
        print(f"created {path}")

    simple = {
        "attack_type": unreal.MordecaiAttackType.MELEE_SWEEP,
        "input_slot": unreal.MordecaiInputSlot.LIGHT,
        "windup_time_ms": windup,
        "active_time_ms": 100.0,
        "recovery_time_ms": 100.0,
        "rooted_during": unreal.MordecaiRootedMode.FULL,
        "hit_shape_type": unreal.MordecaiHitShapeType.ARC_SECTOR,
        "posture_damage_scalar": posture_scalar,
        "stamina_cost": 0.0,
        "combo_index": 0,
        "cancelable_into_dodge": False,
        "cancelable_into_block": False,
    }
    for key, value in simple.items():
        profile.set_editor_property(key, value)
    shape = profile.get_editor_property("hit_shape_params")
    shape.set_editor_property("radius", 150.0)
    shape.set_editor_property("angle", 120.0)
    shape.set_editor_property("start_angle_offset", 0.0)
    profile.set_editor_property("hit_shape_params", shape)
    damage = profile.get_editor_property("damage_profile")
    damage.set_editor_property("damage_type", unreal.MordecaiDamageType.SLASH)
    damage.set_editor_property("base_power", power)
    damage.set_editor_property("can_crit", False)
    damage.set_editor_property("applies_posture_damage", True)
    profile.set_editor_property("damage_profile", damage)
    if not EAL.save_loaded_asset(profile, only_if_is_dirty=False):
        raise RuntimeError(f"save failed for {path}")
    return profile


def create_fixture_classes():
    attacker_class = EAL.load_blueprint_class(ATTACKER_BP)
    if attacker_class is None:
        raise RuntimeError(f"missing existing TestAttacker {ATTACKER_BP}")

    ensure_blueprint(AI_BP, unreal.MordecaiEnemyAIController.static_class())
    configure_blueprint(AI_BP, (
        ("aggro_range", 900.0),
        ("attack_range", 200.0),
        ("leash_range", 1800.0),
        ("attack_cooldown_ms", ATTACK_COOLDOWN_MS),
    ))
    ai_class = EAL.load_blueprint_class(AI_BP)

    classes = []
    for name, windup, power, posture_scalar, angle in ATTACKS:
        profile = ensure_profile(name, windup, power, posture_scalar)
        path = f"{FIXTURE_FOLDER}/BP_MordecaiEnemy_PostureTest{name}"
        ensure_blueprint(path, attacker_class)
        configure_blueprint(path, (
            ("ai_controller_class", ai_class),
            ("enemy_attack_profiles", [profile]),
        ))
        classes.append((name, EAL.load_blueprint_class(path), angle))
    return classes


def create_map(classes):
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not EAL.does_asset_exist(MAP):
        if not EAL.duplicate_asset(COMBAT_MAP, MAP):
            raise RuntimeError(f"could not duplicate {COMBAT_MAP} to {MAP}")
        if not EAL.save_asset(MAP, only_if_is_dirty=False):
            raise RuntimeError(f"could not save duplicated map {MAP}")
        print(f"duplicated {COMBAT_MAP} -> {MAP}")
        # Loading a newly duplicated UWorld in the same Python command can leave the
        # standalone duplicate's KEEPFLAGS world alive and make Map_Load fatally reject
        # the old package. End this editor command cleanly; a rerun loads the saved map.
        return False
    if not level.load_level(MAP):
        raise RuntimeError(f"could not load {MAP}")

    for actor in list(actors.get_all_level_actors()):
        if isinstance(actor, unreal.MordecaiEnemyCharacter):
            actors.destroy_actor(actor)

    starts = [actor for actor in actors.get_all_level_actors()
              if isinstance(actor, unreal.PlayerStart)]
    if len(starts) != 1:
        raise RuntimeError(f"{MAP} has {len(starts)} PlayerStarts, expected one")
    starts[0].set_actor_location(unreal.Vector(*START), False, False)

    for name, enemy_class, angle in classes:
        radians = math.radians(angle)
        x = FIGHT[0] + 120.0 * math.cos(radians)
        y = FIGHT[1] + 120.0 * math.sin(radians)
        yaw = math.degrees(math.atan2(FIGHT[1] - y, FIGHT[0] - x))
        enemy = actors.spawn_actor_from_class(
            enemy_class, unreal.Vector(x, y, 100.0), unreal.Rotator(yaw=yaw))
        if enemy is None:
            raise RuntimeError(f"could not place PostureTestAttacker_{name}")
        enemy.set_actor_label(f"PostureTestAttacker_{name}")

    settings = unreal.get_editor_subsystem(
        unreal.UnrealEditorSubsystem).get_editor_world().get_world_settings()
    settings.set_editor_property("default_game_mode", unreal.MordecaiGameMode.static_class())
    if not level.save_current_level():
        raise RuntimeError(f"save failed for {MAP}")
    print(f"saved {MAP}")

    if not level.load_level(MAP):
        raise RuntimeError(f"could not reload {MAP}")
    labels = sorted(
        actor.get_actor_label() for actor in actors.get_all_level_actors()
        if isinstance(actor, unreal.MordecaiEnemyCharacter))
    expected = sorted(f"PostureTestAttacker_{name}" for name, *_ in ATTACKS)
    if labels != expected:
        raise RuntimeError(f"{MAP} enemy labels are {labels}, expected {expected}")
    if not os.path.isfile(asset_file(MAP, ".umap")):
        raise RuntimeError(f"{MAP} was not saved on disk")
    return True


def main():
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
        raise RuntimeError("PIE is running: stop it first")
    classes = create_fixture_classes()
    if not create_map(classes):
        print("RESULT: PASS (stage 1/2) -- map duplicated; rerun after restarting the editor")
        return
    if not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(DEVTEST_MAP):
        raise RuntimeError(f"could not return to {DEVTEST_MAP}")
    print("RESULT: PASS -- US-124 player posture fixture saved")


main()
