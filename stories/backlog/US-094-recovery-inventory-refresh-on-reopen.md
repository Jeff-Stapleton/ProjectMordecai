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
- [ ] AC-094.1: Each time the pause menu is activated (including a CommonUI pooled reuse), the active tab's content widget is rebound to the current pawn's components and refreshed — verified by `Mordecai.Recovery.PauseMenu.RebindsContentOnReactivate`
- [ ] AC-094.2: `UMordecaiInventoryWidget` refreshes its rows and ledger rows when the inventory or ledger changes while it is bound, and unbinds cleanly on deactivation (no "--" placeholders after repeated opens) — verified by `Mordecai.Recovery.Inventory.WidgetRefreshesOnChange` and `Mordecai.Recovery.Inventory.RepeatedOpenStaysBound`
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
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: none; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py` exits 0 — never `--allow-stale`
- [ ] On that same fresh package `py Scripts/playtest_story.py US-079`: the nine checks above PASS (from FAIL/SKIP in sweep `Saved/Playtests/20261022-184702-sweep`); no check that PASSed in the sweep now FAILs; `pickup_labels` may remain FAIL (known: bridge gap B1); `--lint` exits 0 — never `--allow-stale`
- [ ] The CommonUI pool known issue marked FIXED in `Specs/agent_playtest_plan_v1.md`; the planner/worker is told US-079 and US-071 can resume
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen vs. "Visible on screen"
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-094]` closing commit
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
<Filled in by the coding agent.>
