import unreal

# List all experience assets
experiences = unreal.EditorAssetLibrary.list_assets('/MordecaiCore/System/Experiences/', recursive=True)
print('MordecaiCore experience assets:')
for exp in experiences:
    print(f'  {exp}')

lyra_experiences = unreal.EditorAssetLibrary.list_assets('/Game/System/Experiences/', recursive=True)
print('\nLyra experience assets:')
for exp in lyra_experiences:
    print(f'  {exp}')

# Try loading each
dev_exp = unreal.load_asset('/MordecaiCore/System/Experiences/B_MordecaiDevExperience')
if dev_exp:
    print(f'\nB_MordecaiDevExperience: {dev_exp.get_path_name()} class={dev_exp.get_class().get_name()}')
else:
    print('\nB_MordecaiDevExperience: NOT FOUND')

arena_exp = unreal.load_asset('/MordecaiCore/System/Experiences/B_MordecaiArenaExperience')
if arena_exp:
    print(f'B_MordecaiArenaExperience: {arena_exp.get_path_name()} class={arena_exp.get_class().get_name()}')
else:
    print('B_MordecaiArenaExperience: NOT FOUND')

# Check what the asset manager knows about
am = unreal.SystemLibrary.get_engine_subsystem(unreal.AssetManager)
if am:
    print(f'\nAssetManager found')
else:
    print('\nAssetManager not found via subsystem')
