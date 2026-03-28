"""US-054: Verify arena setup and re-apply if needed."""
import unreal

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
EAL = unreal.EditorAssetLibrary

# Load DevTestMap
map_path = '/MordecaiCore/Game/Maps/DevTestMap'
level_sub.load_level(map_path)

# Check what actors exist
all_actors = actor_sub.get_all_level_actors()
arena_actors = [a for a in all_actors if a.get_actor_label().startswith('Arena_')]
frontliner_actors = [a for a in all_actors if a.get_actor_label().startswith('Frontliner_')]

print(f'Arena actors: {len(arena_actors)}')
for a in arena_actors:
    print(f'  {a.get_actor_label()} at {a.get_actor_location()}')

print(f'Frontliner enemies: {len(frontliner_actors)}')
for a in frontliner_actors:
    print(f'  {a.get_actor_label()} at {a.get_actor_location()}')

if len(frontliner_actors) < 3:
    print('WARNING: Need to re-run arena setup (enemies missing)')
else:
    print('SUCCESS: Arena setup verified.')
