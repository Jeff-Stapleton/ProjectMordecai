# US-091: Recovery — Weapon HUD and D20 Combat HUD Root

**Epic:** Epic R: Playability Recovery  
**Issue:** #8  
**Priority:** P1  
**Depends on:** none  
**Paired story:** none

## Overview
The equipped-weapon state and cycling work, but `UMordecaiEquippedWeaponWidget` has no widget tree and draws nothing (N3). Implement the Tin Tally loadout plate's functional content and move the whole combat HUD to the D20 CommonUI root architecture. Visual polish, motion, icons, and replication are out of scope.

Owner decisions:
- Functional Tin Tally content/layout only; no visual-polish work.
- Complete the combat-HUD layer migration now.
- Local player only; multiplayer replication is tracked by #83.

## Execution Mode
**HEADLESS**

Native widgets and test/playtest scripts only; no editor assets.

## References
- `Specs/agent_playtest_plan_v1.md` — N3
- `Design/TinTally/ds/project/components/LoadoutPlate/README.md`
- `Design/TinTally/ds/project/components/HUDLayout/README.md` — D20
- Baseline sweep: `Saved/Playtests/20261104-072646-sweep`

## Acceptance Criteria
- [ ] **AC-091.A — Loadout plate:** `UMordecaiEquippedWeaponWidget` builds a native fallback only for an empty tree. `WeaponNameText` shows the weapon display name or exactly `Unarmed`; `DamageSubtypeText` shows `Slash`, `Pierce`, or `Blunt` from the weapon's attack profile (`Unarmed` = `Blunt`); `SwapKey` shows `Tab`. It updates from `OnWeaponChanged` for MainHand/TwoHand with no polling and no stale text.
- [ ] **AC-091.B — D20 root:** `UMordecaiCombatHUDWidget` derives from `ULyraActivatableWidget` (not `ULyraHUDLayout`) and is pushed once on `UI.Layer.Game`. The loadout plate is its child at bottom-right anchor `(1,1)`, offset `(-32,-32)`, alignment `(1,1)`, fixed size `340x84`. The character owns no standalone weapon widget.
- [ ] **AC-091.C — Layering:** the death screen, as the top widget on `UI.Layer.Game`, hides the whole HUD including the weapon name; respawn removes it and reactivates/refreshed the HUD. `UI.Layer.Menu` remains above the HUD and Escape still opens/closes pause exactly as before.
- [ ] **AC-091.D — Player-visible result:** the fresh packaged build starts with `Longsword` visible; after four real Tab presses `Mace` is visible. `weapon_hud_text` and `weapon_hud_updates` pass and only accept text from `MordecaiEquippedWeaponWidget`.
- [ ] **AC-091.E — Regression:** no check that passed in baseline sweep `Saved/Playtests/20261104-072646-sweep` becomes a failure.

## Tests Required
### Tier 1 — Automation
- `Mordecai.Recovery.WeaponHUD.BuildsDefaultContent`
- `Mordecai.Recovery.WeaponHUD.UpdatesOnWeaponChanged`
- `Mordecai.Recovery.WeaponHUD.IsCombatHUDChildAtMapBox`
- `Mordecai.Recovery.WeaponHUD.CharacterOwnsNoStandaloneWidget`

### Tier 2 — PIE
- `Scripts/verify_pie_hud.py`: one visible equipped-weapon widget, owned by the activated combat HUD root, reading `Longsword`.

### Tier 3 — Packaged
- Boot `CombatTestMap`; the bottom-right loadout plate visibly reads `Longsword`.

### Tier 4 — Real input
- `Scripts/playtests/us078_weapons.py`: `weapon_hud_text` and `weapon_hud_updates`.
- `Scripts/playtests/us054_combat.py`: weapon plate hidden under the death screen and visible as `Longsword` after respawn.

## Playtest
- **Launch path:** CombatTestMap → `B_MordecaiArenaExperience`; `py Scripts/playtest_story.py US-078` and the full regression sweep on the same fresh package.
- **Player steps:** observe `Longsword`; tap Tab four times (Dagger → Greatsword → Spear → Mace); complete the existing death/respawn path; open and close pause during the regression sweep.
- **Visible on screen:** `bridge_01.png` shows `Longsword` in the bottom-right loadout plate; `bridge_02.png` shows `Mace`. The death frame has no loadout plate; the respawn frame shows it again.
- **Proof:** Tier 3 boot, Tier 4 `weapon_hud_text`, `weapon_hud_updates`, extended death-screen checks, and full sweep comparison.
- **Playable alone:** YES

## Definition of Done
- [ ] All ACs checked off.
- [ ] Red tests recorded failing for the intended missing-tree/ownership/layout reasons.
- [ ] Serial compile and all `Mordecai.*` Tier 1 tests pass.
- [ ] Python tooling tests and story lint pass.
- [ ] Tier 2 PIE smoke passes.
- [ ] `py Scripts/check_content_committed.py` exits 0.
- [ ] Code/scripts committed and pushed before packaging.
- [ ] Fresh package of the exact clean commit passes Tier 3 and Tier 4 without `--allow-stale`.
- [ ] Every `engine_NN.png` and `bridge_NN.png` opened and described.
- [ ] Full sweep has no baseline PASS → FAIL.
- [x] N3 marked FIXED after packaged evidence from `3273068` (`Saved/Playtests/20261104-185928-us078` and `Saved/Playtests/20261104-185741-us054combat`).
- [ ] Evidence recorded here and in the closing `[US-091]` commit; closing commit touches only `stories/`.

## Follow-ups
- #83: replicate equipped-weapon state for remote clients.
- Move the remaining existing HUD regions to their final Tin Tally map boxes; this issue moves only the new loadout plate.
- Loadout plate icons, gamepad glyph switching, motion, and visual styling.

## Completion Notes
<Filled in after packaged evidence.>
