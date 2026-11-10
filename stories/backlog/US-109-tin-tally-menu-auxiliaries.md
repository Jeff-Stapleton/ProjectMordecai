# US-109: Tin Tally Death, Dialog, Cards, and Status Details

**Epic:** Epic Tin Tally: In-engine UI
**Priority:** P1
**Depends on:** US-104, US-108
**Paired story:** none

## Overview
Finish the supporting in-game menu surfaces: World-voice death, Mordecai dialogs, item/spell cards, and status tooltip. Visual/read-only behavior is ready; Equip/Use/Identify, campaign death, missing spell fields, and codex placement remain explicit blocked portions.

## Execution Mode
**HEADLESS**
Native fallback widgets consume the foundation and existing gameplay data.

## References
- `Design/TinTally/ds/project/components/{DeathScreen,Dialog,ItemCard,SpellCard,StatusTooltip}/README.md`
- `Design/TinTally/ds/BUILD.md` — D4, D8, D11, D14
- `Specs/control_bindings_v1.1.md` — menu/back/confirm input
- `Source/LyraGame/Mordecai/UI/MordecaiDeathScreenWidget.*`, inventory/status code
- Issues #27 and #62

## Acceptance Criteria
- [ ] AC-109.1: DeathScreen keeps exact `"You Died"`, countdown and kills, uses World voice/family frame, remains on `UI.Layer.Game`, and preserves automatic arena respawn/layering.
- [ ] AC-109.2: Mordecai Dialog uses `UI.Layer.Modal`, risky action left/safe action right, safe default focus, exact button text, and real keyboard/gamepad focus/back behavior.
- [ ] AC-109.3: ItemCard renders decided inventory fields, rarity word+badge+color, comparison redundancy and empty/unidentified states without enabling nonexistent actions.
- [ ] AC-109.4: SpellCard renders available spell fields and visibly omits undecided description/icon/role/live-state fields rather than fabricating copy.
- [ ] AC-109.5: StatusTooltip uses the same display-name/icon source as StatusChip and exposes tagline/counterplay only when approved data exists.
- [ ] AC-109.6: all five components pass packaged preview comparison in both themes.

## Technical Notes
- Preserve `DeathText`, `CountdownText`, `KillCountText`.
- Add exact `UMordecaiConfirmationDialog`, `UMordecaiControllerDisconnectedScreen`, `UMordecaiItemCardWidget`, `UMordecaiSpellCardWidget`, and `UMordecaiStatusTooltipWidget`. Preserve dialog bindings `Text_Title`, `RichText_Description`, `EntryBox_Buttons`, `Border_TapToCloseZone`, `CancelAction`; preserve controller bindings `HBox_SwitchUser`, `Button_ChangeUser`; labels remain inspectable TextBlocks.
- TODO(DECISION): #27 Equip/Use; field versus Mage Tower Identify; campaign death; spell Description/Icon/AbilityType/scaled power; tooltip codex location/static status data.
- Blocked actions are hidden or unavailable per D24; they are not accepted by this story.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.UI.DeathScreen.CopyAndTheme`
- `Mordecai.UI.Dialog.SafeDefaultFocus`
- `Mordecai.UI.ItemCard.RarityAndUnavailableActions`
- `Mordecai.UI.SpellCard.AvailableDataOnly`
- `Mordecai.UI.StatusTooltip.SharesDisplayData`
### Tier 2 — PIE smoke (editor integration)
- Death/modal layer smoke and inventory/status focus smoke.
### Tier 3 — Packaged playtest
- Combat death, inventory cards, and dialog route captures.
### Tier 4 — Real-input playtest
- US-091 death layering, US-079 inventory, plus a new dialog focus/choice script.

## Playtest
- **Launch path:** CombatTestMap death path, DevTestMap inventory, and front-end quit dialog when available
- **Player steps:** die/respawn; focus item/status/spell; open dialog and choose safe/risky paths
- **Visible on screen:** exact death copy, safe-focused dialog, truthful cards/tooltips with no fake action
- **Proof:** Tier 4 checks and both-theme component comparisons
- **Tier 4 script:** extend existing death/inventory scripts; add dialog script
- **Steps:** generated and linted after routes are available
- **Playable alone:** YES for unblocked surfaces; blocked actions remain unclaimed

## Definition of Done
- [ ] Decisions for any claimed blocked action are logged; all claimed ACs checked; red tests fail first
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation and affected Python tests pass
- [ ] Tier 2: `py Scripts/run_pie_verify.py` passes; direct-call checks are labeled LOGIC
- [ ] Content guard: `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then `py Scripts/package_playtest.py` exits 0 for that exact clean commit
- [ ] CombatTestMap/DevTestMap/front-end Tier 3 routes used by claimed ACs exit 0 — never `--allow-stale`
- [ ] Affected death/inventory/dialog Tier 4 stories and `--lint` pass; full sweep has no baseline PASS to FAIL
- [ ] #27 is not claimed unless decided/implemented/tested; both-theme previews/diffs and every gameplay screenshot are opened/described
- [ ] Evidence run dirs appear in Completion Notes and closing `[US-109]` commit body; closing commit touches only `stories/`
- [ ] Pushed; story moves to `stories/done/` only after all claimed ACs pass

## Completion Notes
<Filled in by the implementing agent; list every still-blocked behavior.>
