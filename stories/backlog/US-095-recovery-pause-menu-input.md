# US-095: Recovery — Pause Menu Input: I Closes the Menu, LB/RB Switch Tabs

**Epic:** Epic R: Playability Recovery
**Priority:** P1
**Depends on:** TODO(DECISION) N18 for AC-095.3/095.4 only
**Paired story:** none

## Overview
Two menu inputs a player expects do nothing in the packaged game. I opens the pause menu on the Inventory tab but pressing I again does not close it, because the menu's CommonUI Menu input mode blocks the Enhanced Input `Inventory` action; it needs a CommonUI UI action binding. And AC-069.4 promises LB/RB tab switching on a controller, but LB does not change the active tab (N18) — there is no CommonUI tab-list action binding. Features: pause_menu, inventory.

## Execution Mode
**HEADLESS**
CommonUI action bindings registered from C++ (`RegisterUIActionBinding` with `FBindUIActionArgs`) on the pause menu widget; if the project needs a new row in a CommonUI input action DataTable instead, that is an asset and this story must be split (record it, do not guess).

## References
- `Specs/agent_playtest_plan_v1.md` — "I does not close the pause menu either", N18
- Sweep `Saved/Playtests/20261022-184702-sweep/summary.md`; run `Saved/Playtests/20261022-184913-us069`: `i_opens_inventory` PASS, `i_closes` FAIL known ("tapped I; but pause_menu.open==false was not true within 3s"), `shoulder_switches_tab` FAIL NEW ("tapped Gamepad_LeftShoulder on the 'settings' tab; ... active_tab='settings'"); `escape_closes`, `gamepad_back_closes`, `start_button_opens` PASS (back handler works)
- Done stories: US-069 AC-069.4 ("Tab switching via LB/RB (controller) or clicking tab headers (mouse)"), AC-069.10; US-079 (I opens the inventory)
- `Specs/control_bindings_v1.1.md` — Pause / Menu: Start / Esc. It lists no "I" binding and no in-menu tab switching (LB/RB are Skill B / Skill A in game)
- Existing code: `Source/LyraGame/Mordecai/UI/MordecaiPauseMenuWidget.cpp:34-45` (Menu input mode, back handler), `:108-113` (`SetActiveTab`); `Source/LyraGame/Mordecai/MordecaiHeroComponent.cpp:113-119,246,362` (programmatic `IA_Mordecai_Inventory_Runtime` on I)

## Acceptance Criteria
- [ ] AC-095.1: While the pause menu is open, I closes it (the same close path as Escape: `NativeOnDeactivated` → subsystem close, game unpaused) — verified by `Mordecai.Recovery.PauseMenu.InventoryKeyClosesMenu`
- [ ] AC-095.2: In the packaged build, the player presses I (menu opens on Inventory) and presses I again (menu closes) — verified by check `i_closes` of `Scripts/playtests/us069_pause_menu.py` (Tier 4)
- [ ] AC-095.3 (per TODO(DECISION) N18): While the menu is open, LB selects the previous tab and RB the next (wrapping), via CommonUI action bindings — verified by `Mordecai.Recovery.PauseMenu.ShoulderButtonsCycleTabs`
- [ ] AC-095.4 (per TODO(DECISION) N18): In the packaged build, the player taps LB on the Settings tab and the active tab changes — verified by check `shoulder_switches_tab` of `Scripts/playtests/us069_pause_menu.py` (Tier 4), plus a NEW check `shoulder_rb_switches_tab` (tap Gamepad_RightShoulder → next tab) added to the script

## Technical Notes
- TODO(DECISION) N18 (Jeff): keep LB/RB tab switching (AC-069.4) — it does not conflict with LB/RB spells because game input is blocked in Menu mode — or drop it and amend AC-069.4 (then `shoulder_switches_tab` is removed from the us069 plan, AC-095.3/095.4 are dropped, and the change is recorded in `Specs/design_decisions_log.md`). If Jeff does not answer, ship AC-095.1/095.2 alone and leave AC-095.3/095.4 unchecked.
- Also TODO(DECISION): add "I — Inventory" (and the gamepad equivalent, if any) to `Specs/control_bindings_v1.1.md` (doc change by Jeff); the code already binds I.
- Use `RegisterUIActionBinding(FBindUIActionArgs(FUIActionTag / key, ...))`; verify in Tier 1 that Escape and Gamepad B still close the menu.
- Script bookkeeping (before packaging): remove the `known_issue` from `i_closes`; update `shoulder_switches_tab`'s `does` text (drop "expected FAIL today"); add `shoulder_rb_switches_tab` (`proves` `US-095:AC-095.4`; US-095 added to `COVERS`), offline-tested against `Scripts/tests/fake_bridge.py`.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.PauseMenu.InventoryKeyClosesMenu` — AC-095.1
- `Mordecai.Recovery.PauseMenu.ShoulderButtonsCycleTabs` — AC-095.3
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_pause_menu.py`: the menu widget has UI action bindings for I (and LB/RB if decided)
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us069_pause_menu.py` checks `i_closes`, `shoulder_switches_tab`, new `shoulder_rb_switches_tab`

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py` (DevTestMap → B_MordecaiArenaExperience); Tier 4: `py Scripts/playtest_story.py US-069` (or `US-095` once in `COVERS`) on the same package
- **Player steps:** press Escape, click Settings, tap LB (previous tab), tap RB (next tab), Escape; press I (inventory), press I (closes)
- **Visible on screen:** the tab highlight moves on LB/RB; after the second I the arena with the HUD and no menu
- **Proof:** Tier 4 checks `shoulder_switches_tab`, `shoulder_rb_switches_tab`, `i_closes` of `Scripts/playtests/us069_pause_menu.py`
- **Tier 4 script:** `Scripts/playtests/us069_pause_menu.py` — run: `py Scripts/playtest_story.py US-069`
- **Steps:**

| Script | Check | Player step (bridge calls) → expected | Sweep result (8ea853b) |
|---|---|---|---|
| us069 | `shoulder_switches_tab` | tap Gamepad_LeftShoulder -> wait_until pause_menu.open==true && pause_menu.active_tab changes | FAIL (NEW) |
| us069 | `shoulder_rb_switches_tab` (new) | tap Gamepad_RightShoulder -> wait_until pause_menu.active_tab changes to the next tab | — |
| us069 | `i_closes` | tap I -> wait_until pause_menu.open==false within 3s | FAIL (known) |

- **Playable alone:** YES

## Definition of Done
- [ ] All ACs checked off (AC-095.3/095.4 only if N18 is decided "keep")
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: none expected; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST (code + script edits), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py` exits 0 — never `--allow-stale`
- [ ] On that same fresh package `py Scripts/playtest_story.py US-069`: `i_closes` (and `shoulder_switches_tab`, `shoulder_rb_switches_tab` if decided) PASS; no check that PASSed in the sweep now FAILs; `--lint` exits 0 — never `--allow-stale`
- [ ] Known issues "I does not close the pause menu" and N18 marked FIXED/decided in `Specs/agent_playtest_plan_v1.md`
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-095]` closing commit
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
<Filled in by the coding agent.>
