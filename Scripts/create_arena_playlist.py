"""Create or update the Mordecai Arena playlist (LyraUserFacingExperienceDefinition).

Phase 1 playtest tooling. Lyra's front end (L_LyraFrontEnd, the packaged
GameDefaultMap) lists the LyraUserFacingExperienceDefinition assets the
AssetManager knows about. MordecaiCore had none, so a player could never reach
DevTestMap from the menu. This script makes one:

  /MordecaiCore/System/Playlists/DA_Mordecai_Arena
    MapID                = Map:/MordecaiCore/Game/Maps/DevTestMap
    ExperienceID         = LyraExperienceDefinition:B_MordecaiArenaExperience
    TileTitle            = 'Mordecai Arena'
    TileSubTitle         = 'Dev test arena'
    TileDescription      = 'Project Mordecai diorama arena (DevTestMap)'
    bShowInFrontEnd      = True
    bIsDefaultExperience = False
    MaxPlayerCount / ExtraArgs / TileIcon are left at their defaults.

The Map primary asset name is the FULL package path, not the short name:
UWorld::GetPrimaryAssetId and UAssetManager::ExtractPrimaryAssetIdFromData use
the package name for the Map type, DevTestMap.umap is saved with that
PrimaryAssetName tag, and Lyra's own playlists store e.g.
Map:/ShooterCore/Maps/L_ShooterGym. A short Map:DevTestMap never resolves, so
UCommonSession_HostSessionRequest::GetMapName() returns an empty string and the
front-end tile fails with "Can't find asset data for MapID". The script checks
both targets' saved PrimaryAssetName tags before writing anything.

Idempotent: creates the asset when it is missing, otherwise updates the existing
asset in place so its identity and references survive. Saves it with
EditorAssetLibrary.save_asset(only_if_is_dirty=False), checks that the .uasset
file was written to disk by this run, that the package is clean after the save
and that the file bytes contain the MapID, ExperienceID and tile texts, then
re-loads the asset, prints every field and verifies the values. It then reports
whether the AssetManager resolves the playlist, the map and the experience.
An id registered under a different name is a FAIL; a plain miss is WARN only:
the Config/DefaultGame.ini scan paths (Map: /MordecaiCore/Game/Maps, playlists:
/MordecaiCore/System/Playlists) are read at editor startup, so a restart is
needed after that ini change.

Runs INSIDE the Unreal editor (needs the `unreal` module), not from a shell:
  py C:/Users/jeffd/.openclaw/workspace/skills/unreal-python-editor/scripts/ue_remote_exec.py "exec(open(r'C:/Users/jeffd/Documents/Gamedev/ProjectMordecai/Scripts/create_arena_playlist.py').read())" --timeout 60

Stop PIE first (the script refuses to run during PIE). After it passes, commit
Plugins/GameFeatures/MordecaiCore/Content/System/Playlists/DA_Mordecai_Arena.uasset;
Scripts/check_content_committed.py fails while it is saved but uncommitted.

Exit codes (ue_remote_exec.py reports the in-editor outcome):
  0 - RESULT: PASS. Asset saved to disk and every field verified.
      WARN lines (AssetManager not yet registered, etc.) are informational.
  1 - RESULT: FAIL. RuntimeError raised: PIE running, class or factory missing,
      target map/experience asset missing or saved under a different
      PrimaryAssetName, create/save failed, file not on disk or missing the
      values, package still dirty, a field did not read back, or the
      AssetManager registered a target under a different id. Also returned
      when the editor is unreachable.

Note: this file is exec()'d in the editor, so it deliberately contains no
backslashes and keeps every UObject reference inside _create_arena_playlist().
"""
import os
import time

import unreal

PLAYLIST_FOLDER = '/MordecaiCore/System/Playlists'
PLAYLIST_NAME = 'DA_Mordecai_Arena'
PLAYLIST_PATH = f'{PLAYLIST_FOLDER}/{PLAYLIST_NAME}'
PLAYLIST_CLASS_NAME = 'LyraUserFacingExperienceDefinition'
PLAYLIST_ID = (PLAYLIST_CLASS_NAME, PLAYLIST_NAME)
# Fallback on-disk location (relative to the project dir) if get_system_path fails.
PLAYLIST_REL_FILE = 'Plugins/GameFeatures/MordecaiCore/Content/System/Playlists/DA_Mordecai_Arena.uasset'

MAP_ASSET_PATH = '/MordecaiCore/Game/Maps/DevTestMap'
# Map ids use the full package path as the name (see the module docstring).
MAP_ID = ('Map', MAP_ASSET_PATH)
EXPERIENCE_ASSET_PATH = '/MordecaiCore/System/Experiences/B_MordecaiArenaExperience'
EXPERIENCE_ID = ('LyraExperienceDefinition', 'B_MordecaiArenaExperience')

TILE_TITLE = 'Mordecai Arena'
TILE_SUB_TITLE = 'Dev test arena'
TILE_DESCRIPTION = 'Project Mordecai diorama arena (DevTestMap)'
SHOW_IN_FRONT_END = True
IS_DEFAULT_EXPERIENCE = False

# Python snake_case spelling first, C++ UPROPERTY spelling as the fallback.
PROP_MAP_ID = ('map_id', 'MapID')
PROP_EXPERIENCE_ID = ('experience_id', 'ExperienceID')
PROP_TILE_TITLE = ('tile_title', 'TileTitle')
PROP_TILE_SUB_TITLE = ('tile_sub_title', 'TileSubTitle')
PROP_TILE_DESCRIPTION = ('tile_description', 'TileDescription')
PROP_SHOW_IN_FRONT_END = ('show_in_front_end', 'bShowInFrontEnd')
PROP_IS_DEFAULT_EXPERIENCE = ('is_default_experience', 'bIsDefaultExperience')
# Fields this script deliberately leaves alone; dumped for the record only.
PROP_UNTOUCHED = (
    ('max_player_count', 'MaxPlayerCount'),
    ('extra_args', 'ExtraArgs'),
    ('tile_icon', 'TileIcon'),
    ('loading_screen_widget', 'LoadingScreenWidget'),
    ('record_replay', 'bRecordReplay'),
)
# UDataAssetFactory::DataAssetClass
FACTORY_CLASS_PROPS = ('data_asset_class', 'DataAssetClass')


def log(msg):
    print(f'[ArenaPlaylist] {msg}')


# ---------------------------------------------------------------------------
# Property / PrimaryAssetId helpers (no UObject state kept at module level)
# ---------------------------------------------------------------------------
def _set_prop(obj, names, value):
    # Set the first property spelling that works; return the spelling used.
    errors = []
    for name in names:
        try:
            obj.set_editor_property(name, value)
            return name
        except Exception as e:
            errors.append(f'{name}: {e}')
    raise RuntimeError(f'could not set {names[0]} ({"; ".join(errors)})')


def _get_prop(obj, names):
    errors = []
    for name in names:
        try:
            return obj.get_editor_property(name)
        except Exception as e:
            errors.append(f'{name}: {e}')
    raise RuntimeError(f'could not read {names[0]} ({"; ".join(errors)})')


def _make_asset_type(type_name):
    try:
        return unreal.PrimaryAssetType(type_name)
    except Exception:
        asset_type = unreal.PrimaryAssetType()
        asset_type.set_editor_property('name', unreal.Name(type_name))
        return asset_type


def aid_to_str(aid):
    # 'Type:Name' for a PrimaryAssetId (same format as FPrimaryAssetId::ToString).
    if aid is None:
        return 'None'
    try:
        return str(unreal.SystemLibrary.conv_primary_asset_id_to_string(aid))
    except Exception:
        pass
    try:
        asset_type = aid.get_editor_property('primary_asset_type')
        return f"{asset_type.get_editor_property('name')}:{aid.get_editor_property('primary_asset_name')}"
    except Exception:
        return str(aid)


def make_primary_asset_id(type_name, asset_name):
    # Tribal-knowledge pattern first (attribute assignment), constructor forms as fallbacks.
    expected = f'{type_name}:{asset_name}'

    def by_attributes():
        aid = unreal.PrimaryAssetId()
        aid.primary_asset_type = _make_asset_type(type_name)
        aid.primary_asset_name = unreal.Name(asset_name)
        return aid

    def by_positional_ctor():
        return unreal.PrimaryAssetId(_make_asset_type(type_name), unreal.Name(asset_name))

    def by_keyword_ctor():
        return unreal.PrimaryAssetId(primary_asset_type=_make_asset_type(type_name),
                                     primary_asset_name=unreal.Name(asset_name))

    def by_set_editor_property():
        aid = unreal.PrimaryAssetId()
        aid.set_editor_property('primary_asset_type', _make_asset_type(type_name))
        aid.set_editor_property('primary_asset_name', unreal.Name(asset_name))
        return aid

    attempts = []
    for label, build in (('attributes', by_attributes),
                         ('positional ctor', by_positional_ctor),
                         ('keyword ctor', by_keyword_ctor),
                         ('set_editor_property', by_set_editor_property)):
        try:
            aid = build()
        except Exception as e:
            attempts.append(f'{label}: {e}')
            continue
        got = aid_to_str(aid)
        if got == expected:
            return aid
        attempts.append(f'{label}: built {got!r}')
    raise RuntimeError(f'could not build PrimaryAssetId {expected} ({"; ".join(attempts)})')


def _describe(value):
    if value is None:
        return 'None'
    if isinstance(value, unreal.PrimaryAssetId):
        return aid_to_str(value)
    if isinstance(value, unreal.Object):
        try:
            return value.get_path_name()
        except Exception:
            pass
    if isinstance(value, unreal.Text):
        return repr(str(value))
    return repr(value)


def _create_playlist_asset(playlist_cls):
    # Returns (asset or None, factory property spelling used or None, error or '').
    factory = unreal.DataAssetFactory()
    used = None
    for cls_value in (playlist_cls, playlist_cls.static_class()):
        try:
            used = _set_prop(factory, FACTORY_CLASS_PROPS, cls_value)
            break
        except RuntimeError as e:
            log(f'DataAssetFactory: {e}')
    # The class is also passed to create_asset: UAssetToolsImpl::CreateAsset uses
    # it as ClassToUse, so the asset is typed even if the factory property failed.
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    try:
        asset = asset_tools.create_asset(PLAYLIST_NAME, PLAYLIST_FOLDER, playlist_cls, factory)
    except Exception as e:
        return None, used, str(e)
    return asset, used, '' if asset else 'create_asset returned None'


def _disk_path(asset):
    try:
        path = str(unreal.SystemLibrary.get_system_path(asset))
        if path:
            return os.path.normpath(path)
    except Exception:
        pass
    try:
        project_dir = str(unreal.SystemLibrary.get_project_directory())
        return os.path.normpath(os.path.join(project_dir, PLAYLIST_REL_FILE))
    except Exception:
        return ''


def _saved_primary_asset_name(asset_path):
    # The PrimaryAssetName asset-registry tag the asset was saved with, i.e. the
    # name the AssetManager registers it under (independent of scan-path state).
    # Returns (name or None, detail).
    try:
        asset_data = unreal.EditorAssetLibrary.find_asset_data(asset_path)
    except Exception as e:
        return None, f'find_asset_data error: {e}'
    value = None
    errors = []
    for getter in (lambda: asset_data.get_tag_value('PrimaryAssetName'),
                   lambda: unreal.AssetRegistryHelpers.get_tag_value(asset_data, 'PrimaryAssetName')):
        try:
            value = getter()
            break
        except Exception as e:
            errors.append(str(e))
    else:
        return None, 'get_tag_value error: ' + '; '.join(errors)
    if isinstance(value, tuple):
        # (found, value) if the bool return is not folded into None by this build
        value = value[-1] if len(value) > 1 and value[0] else None
    if not value:
        return None, 'no PrimaryAssetName tag'
    return str(value), ''


# ---------------------------------------------------------------------------
# Main (all UObject references live and die inside this function)
# ---------------------------------------------------------------------------
def _create_arena_playlist():
    state = {'pass': 0, 'fail': [], 'warn': []}

    def check(name, ok, detail=''):
        suffix = f' -- {detail}' if detail else ''
        if ok:
            state['pass'] += 1
            print(f'  PASS: {name}{suffix}')
        else:
            state['fail'].append(name)
            print(f'  FAIL: {name}{suffix}')
        return bool(ok)

    def warn(name, detail=''):
        state['warn'].append(name)
        suffix = f' -- {detail}' if detail else ''
        print(f'  WARN: {name}{suffix}')

    def finish():
        print('')
        n_pass, n_fail, n_warn = state['pass'], len(state['fail']), len(state['warn'])
        if n_fail:
            print(f'RESULT: FAIL - {n_fail} failed, {n_pass} passed, {n_warn} warning(s) ({PLAYLIST_PATH})')
            raise RuntimeError('create_arena_playlist failed: ' + '; '.join(state['fail']))
        print(f'RESULT: PASS - {PLAYLIST_PATH} saved and verified ({n_pass} checks, {n_warn} warning(s))')

    EAL = unreal.EditorAssetLibrary
    start_time = time.time()

    print('=' * 60)
    print(f'CREATE ARENA PLAYLIST: {PLAYLIST_PATH}')
    print('=' * 60)

    # --- Preconditions ---------------------------------------------------
    log('Preconditions')
    editor_sub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    pie_world = editor_sub.get_game_world() if editor_sub else None
    in_pie = pie_world is not None
    pie_world = None
    if not check('Editor is not in PIE', not in_pie,
                 'stop PIE before creating/saving assets' if in_pie else ''):
        return finish()

    playlist_cls = getattr(unreal, PLAYLIST_CLASS_NAME, None)
    check(f'unreal.{PLAYLIST_CLASS_NAME} is exposed', playlist_cls is not None,
          '' if playlist_cls is not None else 'LyraGame module not loaded in this editor?')
    check('unreal.DataAssetFactory is exposed', hasattr(unreal, 'DataAssetFactory'))
    check(f'Target map exists: {MAP_ASSET_PATH}', EAL.does_asset_exist(MAP_ASSET_PATH))
    check(f'Target experience exists: {EXPERIENCE_ASSET_PATH}', EAL.does_asset_exist(EXPERIENCE_ASSET_PATH))
    if state['fail']:
        return finish()

    # The ids written into the playlist must equal the names the targets are
    # registered under, or the front end cannot resolve them (no restart fixes that).
    for label, asset_path, (type_name, asset_name) in (('map', MAP_ASSET_PATH, MAP_ID),
                                                      ('experience', EXPERIENCE_ASSET_PATH, EXPERIENCE_ID)):
        saved_name, detail = _saved_primary_asset_name(asset_path)
        if saved_name is None:
            warn(f'Could not read the {label} PrimaryAssetName tag; id not cross-checked', detail)
        else:
            check(f'{type_name} id name matches the {label} saved PrimaryAssetName tag',
                  saved_name == asset_name, f'tag={saved_name!r}, playlist uses {asset_name!r}')

    map_aid = experience_aid = None
    try:
        map_aid = make_primary_asset_id(*MAP_ID)
        experience_aid = make_primary_asset_id(*EXPERIENCE_ID)
        check('Built PrimaryAssetIds', True, f'{aid_to_str(map_aid)}, {aid_to_str(experience_aid)}')
    except RuntimeError as e:
        check('Built PrimaryAssetIds', False, str(e))
    if state['fail']:
        return finish()

    # --- Create or update ------------------------------------------------
    log('Create or update')
    asset = None
    if EAL.does_asset_exist(PLAYLIST_PATH):
        asset = EAL.load_asset(PLAYLIST_PATH)
        found_cls = asset.get_class().get_name() if asset else 'None'
        if asset is not None and found_cls == PLAYLIST_CLASS_NAME:
            log(f'Updating existing asset in place: {PLAYLIST_PATH}')
        else:
            warn('Existing asset has the wrong class; deleting and recreating it', f'class={found_cls}')
            asset = None
            if not check('Deleted wrong-class asset', EAL.delete_asset(PLAYLIST_PATH)):
                return finish()

    if asset is None:
        EAL.make_directory(PLAYLIST_FOLDER)
        asset, factory_prop, error = _create_playlist_asset(playlist_cls)
        if factory_prop is None:
            warn('Could not set DataAssetFactory data asset class (either spelling)',
                 'relied on the create_asset asset_class argument')
        else:
            log(f'DataAssetFactory class property spelling: {factory_prop}')
        if not check(f'Created {PLAYLIST_PATH}', asset is not None, error):
            return finish()

    check(f'Asset class is {PLAYLIST_CLASS_NAME}', asset.get_class().get_name() == PLAYLIST_CLASS_NAME,
          f'got {asset.get_class().get_name()}')

    fields = (
        (PROP_MAP_ID, map_aid),
        (PROP_EXPERIENCE_ID, experience_aid),
        (PROP_TILE_TITLE, unreal.Text(TILE_TITLE)),
        (PROP_TILE_SUB_TITLE, unreal.Text(TILE_SUB_TITLE)),
        (PROP_TILE_DESCRIPTION, unreal.Text(TILE_DESCRIPTION)),
        (PROP_SHOW_IN_FRONT_END, SHOW_IN_FRONT_END),
        (PROP_IS_DEFAULT_EXPERIENCE, IS_DEFAULT_EXPERIENCE),
    )
    for names, value in fields:
        try:
            used = _set_prop(asset, names, value)
            log(f'  set {used} = {_describe(value)}')
        except RuntimeError as e:
            check(f'Set {names[0]}', False, str(e))
    if state['fail']:
        return finish()

    # --- Save --------------------------------------------------------------
    log('Save')
    saved = EAL.save_asset(PLAYLIST_PATH, only_if_is_dirty=False)
    if not check('save_asset returned True', bool(saved)):
        return finish()

    disk_path = _disk_path(asset)
    on_disk = bool(disk_path) and os.path.isfile(disk_path)
    check('.uasset exists on disk', on_disk, disk_path or 'could not resolve file path')
    if on_disk:
        mtime = os.path.getmtime(disk_path)
        check('.uasset was written by this run', mtime >= start_time - 2.0,
              time.strftime('mtime %Y-%m-%d %H:%M:%S', time.localtime(mtime)))
        # load_asset below returns the same in-memory object, so also check the
        # bytes on disk: FNames (name map) and FText source strings are stored as
        # plain ASCII in an uncooked package.
        try:
            with open(disk_path, 'rb') as f:
                blob = f.read()
            wanted = (MAP_ID[1], EXPERIENCE_ID[1], TILE_TITLE, TILE_SUB_TITLE, TILE_DESCRIPTION)
            missing = [s for s in wanted if s.encode('utf-8') not in blob]
            check('.uasset on disk contains the MapID, ExperienceID and tile texts', not missing,
                  ('missing: ' + ', '.join(missing)) if missing else f'{len(blob)} bytes')
        except OSError as e:
            check('.uasset on disk is readable', False, str(e))
    try:
        dirty = {str(p.get_name()) for p in (unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages() or [])}
        check('Package is clean after save (disk matches memory)', PLAYLIST_PATH not in dirty)
    except Exception as e:
        warn('Could not list dirty packages', str(e))
    asset = None

    # --- Re-load and verify fields ---------------------------------------
    log('Re-load and verify')
    reloaded = EAL.load_asset(PLAYLIST_PATH)
    if not check('Re-loaded asset after save', reloaded is not None):
        return finish()

    def read(names):
        try:
            return _get_prop(reloaded, names)
        except RuntimeError as e:
            check(f'Read {names[0]}', False, str(e))
            return None

    log('Fields on the saved asset:')
    expected_fields = (
        (PROP_MAP_ID, aid_to_str, f'{MAP_ID[0]}:{MAP_ID[1]}'),
        (PROP_EXPERIENCE_ID, aid_to_str, f'{EXPERIENCE_ID[0]}:{EXPERIENCE_ID[1]}'),
        (PROP_TILE_TITLE, str, TILE_TITLE),
        (PROP_TILE_SUB_TITLE, str, TILE_SUB_TITLE),
        (PROP_TILE_DESCRIPTION, str, TILE_DESCRIPTION),
        (PROP_SHOW_IN_FRONT_END, bool, SHOW_IN_FRONT_END),
        (PROP_IS_DEFAULT_EXPERIENCE, bool, IS_DEFAULT_EXPERIENCE),
    )
    for names, normalize, expected in expected_fields:
        value = read(names)
        got = normalize(value) if value is not None else None
        log(f'  {names[1]} = {_describe(value)}')
        check(f'{names[1]} == {expected!r}', got == expected, f'got {got!r}')
    for names in PROP_UNTOUCHED:
        try:
            log(f'  {names[1]} = {_describe(_get_prop(reloaded, names))} (left at default)')
        except RuntimeError as e:
            log(f'  {names[1]} = <unreadable: {e}>')

    try:
        asset_data = EAL.find_asset_data(PLAYLIST_PATH)
        if asset_data is not None and asset_data.is_valid():
            try:
                class_name = str(asset_data.asset_class_path.asset_name)
            except Exception:
                class_name = str(asset_data.get_editor_property('asset_class'))
            if class_name == PLAYLIST_CLASS_NAME:
                check('Asset registry lists the playlist', True, f'class={class_name}')
            else:
                warn('Asset registry lists the playlist with an unexpected class', f'class={class_name}')
        else:
            warn('Asset registry has no entry for the playlist yet')
    except Exception as e:
        warn('Asset registry lookup failed', str(e))

    # --- AssetManager resolution (WARN only) -----------------------------
    log('AssetManager resolution (WARN only; restart the editor after DefaultGame.ini scan-path changes)')
    log('  note: Python resolves soft references only for objects already in memory,')
    log('  so soft/object None can mean "not loaded"; the id-list membership is the registration test.')
    sl = unreal.SystemLibrary

    try:
        playlist_aid = sl.get_primary_asset_id_from_object(reloaded)
        playlist_aid_str = aid_to_str(playlist_aid) if sl.is_valid_primary_asset_id(playlist_aid) else 'invalid'
    except Exception as e:
        playlist_aid_str = f'error: {e}'
    log(f'  get_primary_asset_id_from_object(playlist) -> {playlist_aid_str}')
    reloaded = None
    # UPrimaryDataAsset::GetPrimaryAssetId is Class:ObjectName; it does not depend
    # on scan paths, so a mismatch is a real defect.
    expected_playlist = f'{PLAYLIST_ID[0]}:{PLAYLIST_ID[1]}'
    if playlist_aid_str.startswith('error:'):
        warn('Could not read the playlist primary asset id', playlist_aid_str)
    else:
        check(f'Playlist primary asset id is {expected_playlist}', playlist_aid_str == expected_playlist,
              f'got {playlist_aid_str}')

    targets = (
        ('playlist', PLAYLIST_ID, False),
        ('map', MAP_ID, False),
        ('experience', EXPERIENCE_ID, True),
    )
    for label, (type_name, asset_name), is_blueprint_class in targets:
        expected = f'{type_name}:{asset_name}'
        try:
            aid = make_primary_asset_id(type_name, asset_name)
        except RuntimeError as e:
            warn(f'AssetManager check skipped for {expected}', str(e))
            continue

        short_name = asset_name.rsplit('/', 1)[-1]
        near = []
        try:
            ids = sl.get_primary_asset_id_list(_make_asset_type(type_name))
            registered_names = {aid_to_str(i) for i in (ids or [])}
            registered = expected in registered_names
            id_detail = f'{len(registered_names)} {type_name} id(s) registered'
            # Same asset registered under another name = wrong id, not a missing scan path.
            near = sorted(n for n in registered_names
                          if n != expected and n.rsplit('/', 1)[-1].split(':')[-1] == short_name)
        except Exception as e:
            registered = False
            id_detail = f'get_primary_asset_id_list error: {e}'

        try:
            soft = sl.get_soft_object_reference_from_primary_asset_id(aid)
        except Exception as e:
            soft = f'error: {e}'
        try:
            obj = sl.get_object_from_primary_asset_id(aid)
        except Exception as e:
            obj = f'error: {e}'
        soft_desc = soft if isinstance(soft, str) else _describe(soft)
        obj_desc = obj if isinstance(obj, str) else _describe(obj)
        log(f'  {expected}: get_soft_object_reference_from_primary_asset_id -> {soft_desc}')
        log(f'  {expected}: get_object_from_primary_asset_id -> {obj_desc}')
        if is_blueprint_class:
            try:
                soft_cls = sl.get_soft_class_reference_from_primary_asset_id(aid)
                log(f'  {expected}: get_soft_class_reference_from_primary_asset_id -> {_describe(soft_cls)}')
            except Exception as e:
                log(f'  {expected}: get_soft_class_reference_from_primary_asset_id -> error: {e}')
        soft = obj = None

        if registered:
            check(f'AssetManager knows {label} {expected}', True, id_detail)
        elif near:
            check(f'AssetManager id for {label} matches {expected}', False,
                  f'registered as {", ".join(near)}; the playlist id is wrong')
        else:
            warn(f'AssetManager does not list {label} {expected}',
                 f'{id_detail}; restart the editor so the DefaultGame.ini scan paths apply')

    print('')
    log(f'NEXT: commit {PLAYLIST_REL_FILE}')
    if state['warn']:
        log('NEXT: restart the editor and re-run this script if any AssetManager WARN remains')
    return finish()


try:
    _create_arena_playlist()
finally:
    del _create_arena_playlist
