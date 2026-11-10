# US-108: Tin Tally Pause Shell and Tabs

**Epic:** Epic Tin Tally: In-engine UI
**Priority:** P1
**Depends on:** US-103, US-104, US-093
**Paired story:** none

## Overview
Restyle the working pause menu into the Tin Tally shell and apply final layouts to Character, Skills, Feats, and Inventory. US-093 remains the source story for restoring missing tab content; this story owns presentation, focus, hint bar, and the inventory wrap defect.

## Execution Mode
**HEADLESS**
Native fallback trees use foundation assets; no new WBP is required.

## References
- `Design/TinTally/ds/project/components/{PauseShell,CharacterTab,SkillsTab,FeatsTab,InventoryTab}/README.md`
- `Design/TinTally/ds/BUILD.md` — D4-D10, D13-D14, D23-D25
- `Specs/control_bindings_v1.1.md` — pause/menu input used in Tier 4
- `stories/backlog/US-093-recovery-pause-menu-tab-contents.md`
- Existing `MordecaiPauseMenuWidget.*`, `MordecaiInventoryWidget.*`

## Acceptance Criteria
- [ ] AC-108.1: pause displays the exact nameplate, 56 px tab bar, `1000x584` shell, `948x540` content box, veil, and footer at the documented 1920x1080 boxes.
- [ ] AC-108.2: `TitleText`, `TabBar`, `TabContentBorder` and all inventory optional names remain bindable; labels/order are exactly Character, Skills, Feats, Inventory, Settings.
- [ ] AC-108.3: selected tabs use enamel plus focus; keyboard/mouse and gamepad navigation preserve CommonUI focus/back/close behavior without advertising undecided keyboard tab cycling.
- [ ] AC-108.4: Character, Skills, and Feats content from US-093 is visible and styled; Inventory's `FilterBar` is panel-compatible/WrapBox; labels are exactly `"All"`, `"Weapons"`, `"Armor"`, `"Trinkets"`, `"Consumables"`, `"Materials"`, `"Quest"`, and `"Magical"` and avoid the `"Stored Resources"` header.
- [ ] AC-108.5: empty states use the single enamel-tag style with exact copy `"Inventory is empty."`, `"No items match this filter."`, and `"No resources stored."`; Settings remains exact `"Coming Soon"` pending US-113.
- [ ] AC-108.6: the real HUD remains under the veil, no hint floats above the menu, and both themes pass preview comparisons.

## Technical Notes
- Keep `UMordecaiPauseMenuWidget` on `UI.Layer.Menu`; do not redraw or hide the HUD.
- Retype `FilterBar` to `UPanelWidget*`; preserve every other inventory binding.
- The pause spike shell may be present empty, but ticket history/behavior belongs to US-114.
- TODO(DECISION): Q/E tab cycling and real Settings are explicitly deferred.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.UI.PauseShell.FinalGeometryAndBindings`
- `Mordecai.UI.PauseShell.FocusAndSelection`
- `Mordecai.UI.Inventory.FilterWrapNoLedgerOverlap`
### Tier 2 — PIE smoke (editor integration)
- Extend `Scripts/verify_pie_pause_menu.py` for shell geometry, focus, and each content class.
### Tier 3 — Packaged playtest
- MovementTestMap pause captures for every tab and both themes.
### Tier 4 — Real-input playtest
- `us069_pause_menu.py` and `us079_inventory.py`, updated only for intentional presentation changes.

## Playtest
- **Launch path:** MovementTestMap and DevTestMap inventory path
- **Player steps:** Escape; click/focus every tab; exercise filters; back/close with keyboard and gamepad
- **Visible on screen:** Tin Tally nameplate/folder tabs/shell/footer, all restored content, wrapped filters, exact placeholder
- **Proof:** existing Tier 4 tab checks plus new focus/layout checks and both-theme gallery comparison
- **Tier 4 script:** extend US-069 and US-079; run with `playtest_story.py`
- **Steps:** generated plans and `--lint`
- **Playable alone:** YES

## Definition of Done
- [ ] All ACs checked; red tests fail for intended reasons first
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation and affected Python tests pass
- [ ] Tier 2: `py Scripts/run_pie_verify.py` passes; direct-call checks are labeled LOGIC
- [ ] Content guard: `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then `py Scripts/package_playtest.py` exits 0 for that exact clean commit
- [ ] MovementTestMap and DevTestMap Tier 3 boots exit 0 — never `--allow-stale`
- [ ] US-069/US-079 and their `--lint` checks pass on that package; full sweep has no baseline PASS to FAIL
- [ ] #37 is evidenced without issue mutation; US-093 owns #9/#10 tab recovery; both-theme previews/diffs and every gameplay screenshot are opened/described
- [ ] Evidence run dirs appear in Completion Notes and closing `[US-108]` commit body; closing commit touches only `stories/`
- [ ] Pushed; story moves to `stories/done/` only after all checks pass

## Completion Notes
<Filled in by the implementing agent.>
