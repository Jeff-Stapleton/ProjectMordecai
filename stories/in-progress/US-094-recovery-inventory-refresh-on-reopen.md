# US-094: Recovery — Inventory Tab Shows Current Items on Every Menu Open (CommonUI pool)

**Epic:** Epic R: Playability Recovery
**Priority:** P0
**Depends on:** none
**Paired story:** none

## Overview
A player who opens the inventory, closes it, picks things up and opens it again sees a stale, empty list: "Inventory is empty." and "No resources stored." although the inventory holds 4 items and 2 resources. After several reopens the tab even shows "--" placeholders (unbound). This was a latent bug (CommonUI pools the pause-menu widget and the content area keeps the old inventory widget instance) that the first Phase 3 sweep confirmed. It is the blocker for the in-progress US-071 / US-079 inventory slice. Features: inventory, identification, pause_menu.

## Execution Mode
**HEADLESS**
C++ only: content-area refresh on activation and inventory-widget change subscriptions.

## References
- `Specs/agent_playtest_plan_v1.md` — "The CommonUI pool may leave inventory rows stale" and "The CommonUI pool bug is CONFIRMED"
- Sweep `Saved/Playtests/20261022-184702-sweep/summary.md`; run `Saved/Playtests/20261022-184957-us079`: `empty_state` PASS, all six pickups PASS, `inventory_count`==4 and `ledger_count`==2 PASS (the data is right), then `menu_rows` FAIL NEW ("0 rendered rows (expected 4)"), `unidentified_badge`, `menu_ledger_rows`, `identify_button`, `filter_weapons`, `filter_armor_empty` (placeholder '--'), `filter_all` FAIL NEW; `identify_click`/`amulet_identified` SKIP
- Exploratory run `Saved/Playtests/20261022-185207-explore/report.md` — after several reopens the tab shows "--" placeholders (`bridge_14.png`)
- In-progress stories: `stories/in-progress/US-071-inventory-widget.md`, `stories/in-progress/US-079-playable-inventory-arena.md` (ACs AC-079.9, AC-079.11; US-071 AC-071.4/.7/.8/.10/.12/.13/.15)
- Existing code: `Source/LyraGame/Mordecai/UI/MordecaiPauseMenuWidget.cpp:237-241` (keeps the existing content instance when the class is unchanged), `:252-262` (`BindTabContent` → `UMordecaiInventoryWidget::BindToInventory`), `MordecaiInventoryWidget.*`, `MordecaiInventoryComponent.*`, `MordecaiResourceLedger.*`

## Acceptance Criteria
- [x] AC-094.1: Each time the pause menu is activated (including a CommonUI pooled reuse), the active tab's content widget is rebound to the current pawn's components and refreshed — verified by `Mordecai.Recovery.PauseMenu.RebindsContentOnReactivate`
- [x] AC-094.2: `UMordecaiInventoryWidget` refreshes its rows and ledger rows when the inventory or ledger changes while it is bound, and unbinds cleanly on deactivation (no "--" placeholders after repeated opens) — verified by `Mordecai.Recovery.Inventory.WidgetRefreshesOnChange` and `Mordecai.Recovery.Inventory.RepeatedOpenStaysBound`
- [ ] AC-094.3: In the packaged build, the player opens the inventory (I) before any pickup, closes it (Escape), picks up all six items (walk + F), presses I again and sees 4 item rows including "? Unidentified Purple Amulet" and 2 stored resources — verified by checks `menu_rows`, `unidentified_badge`, `menu_ledger_rows` of `Scripts/playtests/us079_inventory.py` (Tier 4)
- [ ] AC-094.4: In that second open, the amulet row has an enabled Identify button and the filter buttons work (Weapons → only Rusty Sword; Armor → "No items match this filter."; All → 4 rows), and identifying renames the amulet — verified by checks `identify_button`, `filter_weapons`, `filter_armor_empty`, `filter_all`, `identify_click`, `amulet_identified` of `Scripts/playtests/us079_inventory.py` (Tier 4)

## Technical Notes
- Override `NativeOnActivated` in `UMordecaiPauseMenuWidget` to re-run `BindTabContent(CurrentContentWidget)` (and refresh) for the active tab; keep the "same class → keep instance" optimisation for tab clicks, but never skip the rebind on activation.
- In `UMordecaiInventoryWidget`, subscribe to the inventory component's / ledger's change delegates in `BindToInventory`, unsubscribe in `NativeDestruct`/on rebind; rebuild rows on change.
- After this lands, US-079 and US-071 resume: their acceptance comes from `py Scripts/playtest_story.py US-079` (US-071 is in its `COVERS`). `pickup_labels` (bridge gap B1) is not a game bug and stays known.
- Out of scope (Known issues, cosmetic): the filter bar overlapping the "Stored Resources" header; rotated pickup labels.
- Script bookkeeping: these checks were NEW (no `known_issue`).
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.PauseMenu.RebindsContentOnReactivate` — AC-094.1
- `Mordecai.Recovery.Inventory.WidgetRefreshesOnChange`, `Mordecai.Recovery.Inventory.RepeatedOpenStaysBound` — AC-094.2
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_inventory.py`: `[LOGIC]` open/close/add item/reopen via the subsystem → rows reflect the inventory (tag the existing direct calls `[LOGIC]` while touching it)
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us079_inventory.py` (unchanged plan)

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py` (DevTestMap → B_MordecaiArenaExperience); Tier 4: `py Scripts/playtest_story.py US-079` on the same package
- **Player steps:** press I (empty inventory), Escape; walk (WASD) to each of the six pickups and press F; press I; click Weapons, Armor, All; click Identify on the amulet; Escape
- **Visible on screen:** us079 `bridge_02.png` the empty inventory ("Inventory is empty."); `bridge_03.png` 4 item rows (Upgrade Key, Rusty Sword, "? Unidentified Purple Amulet" with an Identify button, Health Potion) and Stored Resources Iron Ore x1, Town Stone x1; `bridge_04.png` the arena with the menu closed
- **Proof:** Tier 4 checks `menu_rows`, `unidentified_badge`, `menu_ledger_rows`, `identify_button`, `filter_weapons`, `filter_armor_empty`, `filter_all`, `identify_click`, `amulet_identified` of `Scripts/playtests/us079_inventory.py`
- **Tier 4 script:** `Scripts/playtests/us079_inventory.py` — run: `py Scripts/playtest_story.py US-079`
- **Steps:** (rows 35-43 of `py Scripts/playtest_story.py US-079 --plan`)

| Script | Check | Player step (bridge calls) → expected | Sweep result (8ea853b) |
|---|---|---|---|
| us079 | `menu_rows` | wait_until len(pause_menu.inventory_tab.rows)==4 (rendered item rows) | FAIL (NEW): 0 |
| us079 | `unidentified_badge` | state pause_menu -> the amulet row's text starts with "?" and unidentified==true | FAIL (NEW) |
| us079 | `menu_ledger_rows` | state pause_menu -> 2 rendered pause_menu.inventory_tab.ledger_rows | FAIL (NEW): 0 |
| us079 | `identify_button` | state pause_menu -> the amulet row has an enabled 'Identify' button on screen | FAIL (NEW) |
| us079 | `filter_weapons` | click_button(button_id="weapons") -> wait_until len(rows)==1 && rows[0].text=="Rusty Sword" | FAIL (NEW) |
| us079 | `filter_armor_empty` | click_button(button_id="armor") -> wait_until list_placeholder=="No items match this filter." && len(rows)==0 | FAIL (NEW): '--' |
| us079 | `filter_all` | click_button(button_id="all") -> wait_until len(rows)==4 | FAIL (NEW) |
| us079 | `identify_click` | re-read the amulet row's 'Identify' button, click Left at its center | SKIP |
| us079 | `amulet_identified` | the amulet row reads 'Mysterious Amulet' with no Identify button and the item is identified | SKIP |

- **Playable alone:** YES

## Definition of Done
- [ ] All ACs checked off
- [x] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [x] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [x] Content: none; `py Scripts/check_content_committed.py` exits 0
- [x] Work committed FIRST, then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [x] `py Scripts/playtest_boot.py` exits 0 — never `--allow-stale`
- [ ] On that same fresh package `py Scripts/playtest_story.py US-079`: the nine checks above PASS (from FAIL/SKIP in sweep `Saved/Playtests/20261022-184702-sweep`); no check that PASSed in the sweep now FAILs; `pickup_labels` may remain FAIL (known: bridge gap B1); `--lint` exits 0 — never `--allow-stale`
- [ ] The CommonUI pool known issue marked FIXED in `Specs/agent_playtest_plan_v1.md`; the planner/worker is told US-079 and US-071 can resume
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen vs. "Visible on screen"
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-094]` closing commit
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes

**Status: stays in `stories/in-progress/` (2026-10-28).**
- The code fix is delivered: issue #28, branch `fix/issue-28-inventory-reopen`, commits `cdeec5a`, `35898b3`, `a8aaa51`.
- AC-094.1 and AC-094.2 pass at Tier 1 and Tier 2.
- AC-094.3 and AC-094.4 (Tier 4) are NOT accepted. In all 3 us079 runs on the fresh package of `a8aaa51`, the DevTestMap Frontliners killed the player during the pickup route (T1, issue #46). The checks that need 4 items and 2 resources therefore never had them.
- On every pooled reopen, the tab drew exactly what the player held. That includes `menu_ledger_rows` PASS (run 2) and `filter_armor_empty` PASS (run 3); both FAILed in every sweep from `8ea853b` to `f21525f`.
- Closing waits on Jeff's call, or on T1 for a clean run.

**Root cause.** CommonUI pools `UMordecaiPauseMenuWidget` on `UI.Layer.Menu`.
- On close, its Slate tree is released one core tick later. `~SObjectWidget` runs the content's `NativeDestruct`, and `UMordecaiInventoryWidget::Unbind()` unbinds the tab.
- A reopen reuses the same instance. `NativeOnInitialized` does not run again, and `SetActiveTab` keeps the same-class content (`RebuildContentArea`'s early return), so nothing rebound the tab.
- `Unbind()` keeps the caches and does not redraw, so the first open's "Inventory is empty." stayed on screen until a filter click drew "--".
- The inventory widget's own change subscriptions were already right (Technical Note 2 was already implemented).

**Fix (`35898b3`, `MordecaiPauseMenuWidget.{h,cpp}`):**
- `NativeOnActivated` rebinds the active tab's content to the current pawn's inventory and ledger (or the player state ASC for the character sheet) before `Super`. This runs on every push, fresh or pooled. Lyra forces the layer transition to 0, so the push is synchronous and comes before the subsystem's `SetActiveTab`.
- `NativeOnDeactivated` unbinds the content.
- `RebuildContentArea` unbinds a content widget it replaces.
- The same-class optimisation for tab clicks is kept.

**Tier 1.** `Source/LyraGame/Tests/Recovery/MordecaiRecoveryInventoryMenuTests.cpp` builds the real menu: `NewObject` + player context + `Initialize`, a spawned PC + `ULocalPlayer`, and the FRecoveryTestEnv player. It drives the menu as the stack does and reads the rendered rows through `MordecaiAgentBridge::State::GatherInventoryTab`.
- `Mordecai.Recovery.PauseMenu.RebindsContentOnReactivate` (AC-094.1), `Mordecai.Recovery.Inventory.WidgetRefreshesOnChange` and `Mordecai.Recovery.Inventory.RepeatedOpenStaysBound` (AC-094.2).
- RED before the fix (`Saved/TestLogs/tests-20261028-173042.log`): after the reopen the tab was unbound, still read "Inventory is empty.", and drew "--" after a filter click.
- GREEN after the fix: 708/708 Mordecai tests (`Saved/TestLogs/tests-20261028-193333.log`).
- Headless there is no Slate tree, so the tests call the content's unbind in place of `~SObjectWidget`.

**Tier 2.** `run_pie_verify.py` re-runs a script that prints `PIE_NEXT_PHASE` after the editor has ticked, because one remote exec is one editor frame. `verify_pie_inventory.py` runs three phases:
1. open the inventory empty, then close it;
2. 6 pickups, then reopen: the same pooled menu must be bound, show 4 items and 2 resources, and identify must work;
3. change the inventory while closed, then reopen: the current rows, a redraw on change, the filters.

Every direct call is tagged `[LOGIC]`.
- RED on the unfixed binary: phase 2 failed 8 checks (unbound, '--', 0 rows, 0 ledger rows, TryIdentify false).
- GREEN on the fix: 12/12 smoke scripts. Inventory passed 23 + 63 + 32 checks, and CommonUI reused the pooled instance in phases 2 and 3.
- PIE painted nothing during these runs because the workstation was locked: every widget's cached geometry was 0x0. So Tier 2 does not check painted geometry.

**Tier 3** (fresh package of `a8aaa51`, clean tree, 10.8 min, `Saved/Playtests/package-20261028-194336.log`): `Saved/Playtests/20261028-195434-boot`, exit 0.
- Checks: build_fresh, launch, experience_identified (B_MordecaiArenaExperience, OptionsString, 6.7 s), experience_loaded, no_crash, content_cooked, game_window and engine_screenshots all PASS. screenshots is WARN (window covered). log_errors: 0 errors, 14 warnings.
- `engine_01.png`: the Mordecai diorama arena from the fixed overhead camera; the player at the centre with three red Frontliners around it; the HUD bars top-left, 'Kills: 0' top-right, '[I] Inventory' bottom-centre; the six labelled pickups; the yellow Steam debug text.
- `engine_02.png`, `engine_03.png`: the same view with the health bar empty. The standing player was killed by the swarm (T1).
- `shot_01..03.png`: the Windows lock-screen photo, not the game. The workstation was locked.

**Tier 4** (same package; `py Scripts/playtest_story.py US-079`, then `py Scripts/playtests/us079_inventory.py` twice, unchanged). All three runs exit 1. `pickup_labels` is FAIL (known, B1) in all three, and `empty_state`, `menu_opens_inventory` and `menu_closes_escape` PASS in all three. Environment: `app_has_focus` false, virtual cursor at the end, locked desktop.
1. `Saved/Playtests/20261028-195457-us079` (summary `Saved/Playtests/20261028-195434-story-us079/summary.md`).
   - `walk_HealthPotion` timed out boxed in at (37, 67), then the player died there. Inventory 0, ledger 0.
   - The pooled reopen drew 0 rows. After the Armor click it read "Inventory is empty.", not the unbound "--".
   - `bridge_01`: the player boxed in by the three Frontliners, health about 55%. `bridge_02`: the first open, "Inventory is empty." / "No resources stored.". `bridge_03`: the pooled reopen with the player dead (health empty), "Inventory is empty." / "No resources stored.", which is correct for an empty inventory. `bridge_04`: the arena after Escape, the dead player beside the enemies.
2. `Saved/Playtests/20261028-195608-us079`.
   - Iron Ore and Town Stone were picked up (`ledger_count`, `ledger_entries` PASS); then the player died at (33, 432).
   - **`menu_ledger_rows` PASS:** "Iron Ore  x1", "Town Stone  x1". Every sweep since `8ea853b` had 0.
   - `menu_rows` 0 is the real inventory (0 items).
   - `bridge_01`: the start, swarmed, about 55% health. `bridge_02`: the first open, empty. `bridge_03`: the pooled reopen, "Stored Resources: Iron Ore x1, Town Stone x1", no item rows, player dead. `bridge_04`: the arena after Escape; Iron Ore and Town Stone gone, the other four pickups still there, the player dead near the Town Stone spot.
3. `Saved/Playtests/20261028-195731-us079`.
   - The Health Potion was picked up; then the player died at (275, 133).
   - `menu_rows` drew 1 row, ['Health Potion'].
   - **`filter_armor_empty` PASS:** "No items match this filter.". It drew '--' in every sweep since `8ea853b`.
   - `filter_weapons` drew 0 rows and `filter_all` 1 row, both correct for that inventory.
   - `bridge_01`: the start, the player engulfed by the three Frontliners. `bridge_02`: the first open, empty. `bridge_03`: the pooled reopen with the row "Health Potion" and "No resources stored.", player dead. `bridge_04`: the arena after Escape, the potion gone, the dead player beside two Frontliners.
- Not run because of the deaths: `unidentified_badge`, `identify_button`, `identify_click` and `amulet_identified` (no amulet was picked up), and `menu_rows` / `filter_*` with the full 4 items. Tier 1 and Tier 2 cover those paths.
- Compared with sweep `20261028-090808` (`f21525f`):
  - The only checks that went PASS → FAIL are the route checks (`walk_*`, `pickup_*`, `inventory_*`, `ledger_*`). Each one failed on a dead or boxed-in player (the swarm, T1). This fix changes only the pause menu's activation, nothing that moves or damages the player.
  - Every other check that ran passed as before: `hud_visible`, `empty_state`, `menu_opens_inventory`, `menu_closes_escape`, `no_crash` and `content_cooked`.
  - Earlier packages survived the route 3 of 3 times at 25 HP (`20261027-085342`, `20261027-203022`, `20261028-091034`). This package lost it 3 of 3 times.

**Other.**
- `py Scripts/check_content_committed.py`: PASS.
- `py Scripts/run_python_tests.py`: 447 run, 1 failed (`test_us057_playtest.TestUs057PassPath.test_rewalks_when_the_enemy_moved`). That test is a timing flake. It failed in 2 of 4 runs, passed on a clean origin/main tree and on this branch, and this branch does not touch it. `-k us079`: 33/33.
- An adversarial review (4 lenses, each finding verified) found no defects in the C++ fix or the tests. Its two minor PIE-script findings (a vacuous check, and "ALL CHECKS PASSED" printed after phase 1) are fixed in `a8aaa51`.
- US-079 and US-071 can resume once US-094 closes: the pooled reopen is fixed. Their remaining blocker is T1 (the swarm); B1 (`pickup_labels`) is fixed (issues #30 and #38, `92e7575`).
- 2026-10-28 (issues #30 and #38, not this story's run): on the fresh package of `92e7575`, which includes this story's `a8aaa51`, `py Scripts/playtest_story.py US-079` exited 0 (`Saved/Playtests/20261028-232530-story-us079`; Tier 4 `20261028-232550-us079`, 55/55 PASS). All nine checks above PASS there with four items held: `menu_rows`, `unidentified_badge`, `menu_ledger_rows`, `identify_button`, `filter_weapons`, `filter_armor_empty`, `filter_all`, `identify_click`, `amulet_identified`. The player outran the swarm (T1) that run. Closing on it is Jeff's call.
- 2026-10-29 (issue #46, US-100): T1 does not change this story. Jeff decided that `us079_inventory.py` and its six pickups stay on DevTestMap and that us069 moves to MovementTestMap (`Specs/design_decisions_log.md`); per issue #46 and US-100, us001 and us083 also move to MovementTestMap and the combat scripts (us054_combat, us057, us078) run on CombatTestMap. So AC-094.3 and AC-094.4 still need a us079 run in which the player outruns the Frontliner swarm on the pickup route. Closing stays Jeff's call.
- 2026-10-29 (US-100 sweep, fresh package of `9904cde`): `Saved/Playtests/20261029-175841-us079` exits 0, 56/56, on DevTestMap. The player won the race against the swarm, and every pooled-reopen and four-item check PASSes. That is evidence for AC-094.3/094.4; ticking them stays Jeff's call.
