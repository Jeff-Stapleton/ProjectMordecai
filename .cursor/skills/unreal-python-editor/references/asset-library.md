# EditorAssetLibrary API Reference

All methods are `@classmethod` — call directly: `unreal.EditorAssetLibrary.method_name(...)`

Asset paths: `/Game/MyFolder/MyAsset` or `/Game/MyFolder/MyAsset.MyAsset`

## Existence Checks

| Method | Returns |
|--------|---------|
| `does_asset_exist(asset_path)` | `bool` |
| `do_assets_exist(asset_paths)` | `bool` |
| `does_directory_exist(directory_path)` | `bool` |
| `does_directory_have_assets(directory_path, recursive=True)` | `bool` |

## Listing

| Method | Returns |
|--------|---------|
| `list_assets(directory_path, recursive=True, include_folder=False)` | `Array[str]` |
| `list_asset_by_tag_value(tag_name, tag_value)` | `Array[str]` |

## Loading & Finding

| Method | Returns |
|--------|---------|
| `find_asset_data(asset_path)` | `AssetData` |
| `find_package_referencers_for_asset(asset_path, load_assets_to_confirm=False)` | `Array[str]` |
| `get_path_name_for_loaded_asset(loaded_asset)` | `str` |
| `get_project_root_asset_directory()` | `str` — `/Game/` for .uproject |

## Duplication

| Method | Returns |
|--------|---------|
| `duplicate_asset(source_path, dest_path)` | `Object` |
| `duplicate_loaded_asset(source_asset, dest_path)` | `Object` |
| `duplicate_directory(source_dir, dest_dir)` | `bool` |

## Deletion (Force — no reference check!)

| Method | Returns |
|--------|---------|
| `delete_asset(asset_path)` | `bool` |
| `delete_loaded_asset(asset)` | `bool` |
| `delete_loaded_assets(assets)` | `bool` |
| `delete_directory(directory_path)` | `bool` |

## Source Control

| Method | Returns |
|--------|---------|
| `checkout_asset(asset_path)` | `bool` |
| `checkout_loaded_asset(asset)` | `bool` |
| `checkout_loaded_assets(assets)` | `bool` |
| `checkout_directory(directory_path, recursive=True)` | `bool` |

## Consolidation

| Method | Returns |
|--------|---------|
| `consolidate_assets(target_asset, assets_to_consolidate)` | `bool` — merges references, **deletes** consolidated assets |

## Metadata

| Method | Returns |
|--------|---------|
| `get_metadata_tag(object, tag)` | `str` |
| `get_metadata_tag_values(object)` | `Map[Name, str]` |
| `get_tag_values(asset_path)` | `Map[Name, str]` — from Asset Registry (unloaded) |
| `get_package_for_object(object)` | `Package` |

## Saving

| Method | Returns |
|--------|---------|
| `save_asset(asset_path, only_if_is_dirty=True)` | `bool` |
| `save_loaded_asset(asset, only_if_is_dirty=True)` | `bool` |
| `save_loaded_assets(assets, only_if_is_dirty=True)` | `bool` |
| `save_directory(directory_path, only_if_is_dirty=True, recursive=True)` | `bool` |

## Renaming

| Method | Returns |
|--------|---------|
| `rename_asset(source_path, dest_path)` | `bool` |
| `rename_loaded_asset(asset, dest_path)` | `bool` |
| `rename_directory(source_dir, dest_dir)` | `bool` |

## Making Directories

| Method | Returns |
|--------|---------|
| `make_directory(directory_path)` | `bool` |
