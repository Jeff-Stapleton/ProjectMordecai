"""US-085: configure the arena's Frontliners and SAVE the content (run in the editor via remote exec).

Done so far (issue #18 / N13): the three Frontliners placed in DevTestMap take their base stats from
BP_MordecaiEnemy_Frontliner again.  The Blueprint has had DefaultMaxHealth = 200 (AC-054.2) since
US-054, but the level saved DefaultMaxHealth = 100 on each placed actor, an instance override, so
the packaged enemies spawned with 100/100.
Still to do in this script (US-085): the mesh, the attack profiles and the placement.

The stats are EditDefaultsOnly, so Python cannot write them on a placed actor ("cannot be edited on
instances").  A Python write to a template, however, also writes every loaded instance that still
holds the template's value.  So the class default object is set to the stale value (every placed
actor now matches it) and back: all of them follow it back, equal their archetype, and the level
save drops the override.  The writes on the class default object do not notify, so the Blueprint
asset is not dirtied and is not saved.

Usage:
    py Scripts/ue_remote_exec.py "exec(open(r'Scripts/us085_configure_frontliner.py').read())"
"""
import unreal

FRONTLINER_BP = '/MordecaiCore/Enemies/BP_MordecaiEnemy_Frontliner'
ARENA_MAP = '/MordecaiCore/Game/Maps/DevTestMap'
EXPECTED_MAX_HEALTH = 200.0  # AC-054.2
STATS = ('default_max_health', 'default_max_posture', 'default_move_speed')
NO_NOTIFY = unreal.PropertyAccessChangeNotifyMode.NEVER


def placed_frontliners(frontliner_class):
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    return [a for a in actors if a.get_class() == frontliner_class]


def stale_stats(actors, class_defaults):
    """[(actor, stat, value, class_value)] for every stat an actor overrides."""
    return [(a, s, a.get_editor_property(s), class_defaults.get_editor_property(s))
            for a in actors for s in STATS
            if a.get_editor_property(s) != class_defaults.get_editor_property(s)]


def main():
    # The cook reads the Blueprint from disk: an unsaved edit would make the placed actors match a value no player gets
    dirty = [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()]
    if FRONTLINER_BP in dirty:
        raise RuntimeError(f'{FRONTLINER_BP} has unsaved changes: save or revert it first')

    frontliner_class = unreal.EditorAssetLibrary.load_blueprint_class(FRONTLINER_BP)
    class_defaults = unreal.get_default_object(frontliner_class)
    bp_health = class_defaults.get_editor_property('default_max_health')
    if bp_health != EXPECTED_MAX_HEALTH:
        raise RuntimeError(f'{FRONTLINER_BP} DefaultMaxHealth is {bp_health}, expected {EXPECTED_MAX_HEALTH}')
    print(f'{FRONTLINER_BP}: DefaultMaxHealth {bp_health} (AC-054.2)')

    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if not world or not world.get_path_name().startswith(ARENA_MAP + '.'):
        if not level_editor.load_level(ARENA_MAP):
            raise RuntimeError(f'could not load {ARENA_MAP}')

    actors = placed_frontliners(frontliner_class)
    if len(actors) != 3:
        raise RuntimeError(f'{len(actors)} Frontliners placed in {ARENA_MAP}, expected 3')

    stale = stale_stats(actors, class_defaults)
    if not stale:
        print(f'no placed Frontliner overrides {STATS}: nothing to save')
        return
    for actor, stat, value, class_value in stale:
        print(f'{actor.get_actor_label()} ({actor.get_name()}) overrides {stat}: {value} (Blueprint {class_value})')
        actor.modify()  # the propagated write does not notify: dirty the level for the save

    for stat in STATS:
        class_value = class_defaults.get_editor_property(stat)
        for value in {v for _, s, v, _ in stale if s == stat}:
            class_defaults.set_editor_property(stat, value, NO_NOTIFY)
            class_defaults.set_editor_property(stat, class_value, NO_NOTIFY)

    left = stale_stats(actors, class_defaults)
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
    dirty = unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    print(f'dirty content packages after the save: {[p.get_name() for p in dirty]}')


main()
