"""US-089: repair the four existing spell DataAssets in place.

This script never deletes or recreates an asset. It imports each native cooldown tag, assigns
the binding duration, saves the existing package, and verifies an exact load/read-back.
Run inside the editor through Scripts/ue_remote_exec.py.
"""

import unreal


SPELLS = (
    ("/MordecaiCore/Spells/DA_Spell_Fireball",
     "Mordecai.Cooldown.Spell.Fireball", 2.0),
    ("/MordecaiCore/Spells/DA_Spell_Blink",
     "Mordecai.Cooldown.Spell.Blink", 4.0),
    ("/MordecaiCore/Spells/DA_Spell_StoneSkin",
     "Mordecai.Cooldown.Spell.StoneSkin", 15.0),
    ("/MordecaiCore/Spells/DA_Spell_Restoration",
     "Mordecai.Cooldown.Spell.Restoration", 10.0),
)


def import_tag(tag_name):
    tag = unreal.GameplayTag()
    if not tag.import_text(f'(TagName="{tag_name}")'):
        raise RuntimeError(f"Could not import native gameplay tag {tag_name}")
    serialized = tag.export_text()
    if serialized != f'(TagName="{tag_name}")':
        raise RuntimeError(f"GameplayTag import mismatch for {tag_name}: {serialized}")
    return tag


def verify(asset, path, tag_name, duration, phase):
    if asset is None:
        raise RuntimeError(f"{phase}: failed to load existing asset {path}")
    actual_tag = asset.get_editor_property("cooldown_tag").export_text()
    actual_duration = float(asset.get_editor_property("cooldown_duration"))
    expected_tag = f'(TagName="{tag_name}")'
    if actual_tag != expected_tag:
        raise RuntimeError(
            f"{phase}: {path} CooldownTag {actual_tag!r}, expected {expected_tag!r}")
    if abs(actual_duration - duration) >= 0.001:
        raise RuntimeError(
            f"{phase}: {path} CooldownDuration {actual_duration:g}, expected {duration:g}")
    print(f"  PASS {phase}: {path} tag={tag_name} duration={actual_duration:g}")


def main():
    eal = unreal.EditorAssetLibrary
    for path, tag_name, duration in SPELLS:
        if not eal.does_asset_exist(path):
            raise RuntimeError(f"Missing existing spell asset: {path}")
        asset = eal.load_asset(path)
        if asset is None:
            raise RuntimeError(f"Failed to load existing spell asset: {path}")

        asset.set_editor_property("cooldown_tag", import_tag(tag_name))
        asset.set_editor_property("cooldown_duration", float(duration))
        verify(asset, path, tag_name, duration, "assigned")

        if not eal.save_asset(path, only_if_is_dirty=False):
            raise RuntimeError(f"Failed to save {path}")

        # Exact load/read-back after save. Unreal may retain the UObject in its package cache,
        # but this goes back through the asset library after the package serialization completed.
        reloaded = eal.load_asset(path)
        verify(reloaded, path, tag_name, duration, "saved/read-back")

    target_packages = {path for path, _, _ in SPELLS}
    dirty = {
        package.get_name()
        for package in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    }
    still_dirty = sorted(target_packages.intersection(dirty))
    if still_dirty:
        raise RuntimeError(f"Spell packages remain dirty after save: {still_dirty}")

    print("RESULT: PASS -- repaired and saved 4 existing spell cooldown assets in place")


main()
del main
