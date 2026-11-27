# US-095: Recovery — Pause Menu Input: I Closes the Menu, LB/RB Switch Tabs

**Epic:** Epic R: Playability Recovery
**Priority:** P1
**Depends on:** N18 decided "keep" (Jeff asked to fix issue #23, 2026-10-30)
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
- [x] AC-095.1: While the pause menu is open, I closes it (the same close path as Escape: `NativeOnDeactivated` → subsystem close, game unpaused) — verified by `Mordecai.Recovery.PauseMenu.InventoryKeyClosesMenu`
- [x] AC-095.2: In the packaged build, the player presses I (menu opens on Inventory) and presses I again (menu closes) — verified by checks `i_opens_inventory` and `i_closes` in `Saved/Playtests/20261127-014050-us069`
- [x] AC-095.3 (per TODO(DECISION) N18): While the menu is open, LB selects the previous tab and RB the next (wrapping), via CommonUI action bindings — verified by `Mordecai.Recovery.PauseMenu.ShoulderButtonsCycleTabs`
- [x] AC-095.4 (per TODO(DECISION) N18): In the packaged build, LB selects the previous tab and RB the next — verified by `shoulder_switches_tab` and `shoulder_rb_switches_tab` in `Saved/Playtests/20261127-014050-us069`
- [x] AC-095.5 (added during implementation; found by the review of this story): In the packaged build, a player who holds I in the open menu keeps it open while the key is down (its key repeats included), the key-up closes it, and it stays closed: a held I never reopens the menu — verified by `i_held_closes_on_key_up` in `Saved/Playtests/20261127-014050-us069`; logic in `Mordecai.Recovery.PauseMenu.InventoryKeyClosesMenu`

## Technical Notes
- N18 **decided: keep** LB/RB tab switching (AC-069.4). Jeff asked to fix issue #23 (2026-10-30), which keeps it; recorded in `Specs/design_decisions_log.md`. It does not conflict with LB/RB spells because game input is blocked in Menu mode. (The alternative was to drop it and amend AC-069.4.)
- Implementation: native UI action tags `UI.Action.Mordecai.PauseMenu.{PreviousTab,NextTab,Inventory}` (`MordecaiGameplayTags`; `FUIActionTag` only accepts tags under `UI.Action`), keyed in `Config/DefaultInput.ini` `[/Script/CommonUI.CommonUIInputSettings]` `+InputActions` (LB, RB, I; config, not a content asset, so the story stays HEADLESS). `UMordecaiPauseMenuWidget::NativeOnInitialized` registers them once (Lyra's `LyraHUDLayout` pattern): CommonUI keeps the handles for the pooled widget's lifetime and re-attaches them on every Slate rebuild. LB/RB fire on the press. I is two bindings: the press arms, the key-up closes (if armed). So the menu consumes the whole I press, a held I's key repeats never reach the game's Inventory action (which would reopen the menu, found by the review of this change), and the key-up of the I press that opened the menu closes nothing.
- Also TODO(DECISION): add "I — Inventory" (and the gamepad equivalent, if any) to `Specs/control_bindings_v1.1.md` (doc change by Jeff); the code already binds I.
- Use `RegisterUIActionBinding(FBindUIActionArgs(FUIActionTag / key, ...))`; verify in Tier 1 that Escape and Gamepad B still close the menu.
- Script bookkeeping (before packaging): remove the `known_issue` from `i_closes`; update `shoulder_switches_tab`'s `does` text (drop "expected FAIL today"); add `shoulder_rb_switches_tab` (`proves` `US-095:AC-095.4`; US-095 added to `COVERS`), offline-tested against `Scripts/tests/fake_bridge.py`.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
All in `Source/LyraGame/Tests/Recovery/MordecaiRecoveryPauseMenuInputTests.cpp`. Tier 1 has no CommonUI action router, so the tests create the menu's bindings with `FUIActionBinding::TryCreate` (what the router calls) and emulate the router's key matching:
- `Mordecai.Recovery.PauseMenu.InventoryKeyClosesMenu` — AC-095.1 (an I press then its key-up closes through the subsystem; a held I stays open until the key-up; the opening press's key-up/repeats, a press from before the last activation and the back keys close nothing)
- `Mordecai.Recovery.PauseMenu.ShoulderButtonsCycleTabs` — AC-095.3 (LB/RB both ways with wrap; tab-bar highlight and content follow; key-ups do nothing)
- `Mordecai.Recovery.PauseMenu.TabModelCyclesWithWrap` — AC-095.3 (model: wrap, unknown tab, one tab, no tabs)
- `Mordecai.Recovery.PauseMenu.UIActionKeysConfigured` — AC-095.1/095.3 config (each action maps exactly its key as a press, no back key)
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_pause_menu.py`: the open menu registered the three UI actions (I, LB, RB) with the live CommonUI action router, once each; `[LOGIC]` open / next / previous / close
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us069_pause_menu.py` checks `i_closes`, `shoulder_switches_tab`, new `shoulder_rb_switches_tab`

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap` (MovementTestMap → B_MordecaiArenaExperience; us069's `MAP` since US-100); Tier 4: `py Scripts/playtest_story.py US-095` (US-095 is in us069's `COVERS`) on the same package
- **Player steps:** press Escape, click Settings, tap LB (previous tab: Inventory), tap RB (next tab: Settings), Escape; press I (inventory), press I (closes); press I (opens), hold I 1.2 s (stays open), release (closes and stays closed)
- **Visible on screen:** the tab highlight moves on LB (to Inventory, with the inventory list) and on RB (to Settings, "Coming Soon"): `bridge_NN.png` labelled `menu_lb` / `menu_rb`; after the second I the MovementTestMap grid floor with the HUD and no menu (`menu_closed_by_i`)
- **Proof:** Tier 4 checks `shoulder_switches_tab`, `shoulder_rb_switches_tab` (AC-095.4), `i_opens_inventory` + `i_closes` (AC-095.2), `i_held_closes_on_key_up` (AC-095.5) of `Scripts/playtests/us069_pause_menu.py`
- **Tier 4 script:** `Scripts/playtests/us069_pause_menu.py` — run: `py Scripts/playtest_story.py US-095`
- **Steps** (`py Scripts/playtest_story.py US-095 --plan`; US-095's checks are 13, 14, 20, 21 and 22):

<!-- Scripts/playtests/us069_pause_menu.py -->
| # | Check | Player step (bridge calls) → expected | Proves |
|---|---|---|---|
| 1 | `hud_visible` | wait_until pickups.prompt.hint_visible==true && pickups.prompt.hint_text=="[I] Inventory" -> a combat HUD widget in state.widgets | - |
| 2 | `escape_opens` | tap Escape -> wait_until pause_menu.open==true && pause_menu.active_tab=="character" && game.paused==true | AC-069.6, AC-069.7 |
| 3 | `menu_input_mode` | state focus, widgets -> focus.commonui_input_mode=="Menu" && focus.mouse_cursor_shown==true and the pause menu widget shows "Paused" | AC-069.1 |
| 4 | `tabs_listed` | state pause_menu, widgets -> pause_menu.tabs is exactly character/Character, skills/Skills, feats/Feats, inventory/Inventory, settings/Settings and 5 enabled tab buttons with those ids are on screen | AC-069.2 |
| 5 | `game_input_blocked` | tap W (0.3s), tap Q, wait 1s -> player.location moved < 5 cm and spell_points unchanged, menu still open | AC-069.1 |
| 6 | `character_tab_content` | state pause_menu, widgets -> on the character tab a visible MordecaiCharacterSheetWidget with exact-case STR..CHA codes, all 9 numeric values and '+X%' effective modifiers, the Physical/Resilience/Magical groups, and all four named Health/Stamina/Spell Points/Posture 'x / y' resource lines | US-066:AC-066.1, US-066:AC-066.3, US-066:AC-066.4, US-066:AC-066.5 |
| 7 | `click_skills_tab` | click_button(button_id="skills") -> wait_until pause_menu.active_tab=="skills" | AC-069.4 |
| 8 | `skills_tab_content` | state pause_menu, widgets -> a visible MordecaiSkillTreeWidget with texts including "Weapon" and "/ 20" | US-067:AC-067.1, US-067:AC-067.4 |
| 9 | `click_magic_category` | click_button(button_id="magic") -> wait for exact texts 'Magic · 11 skills' and 'Fireball' | US-067:AC-067.3 |
| 10 | `click_feats_tab` | click_button(button_id="feats") -> wait_until pause_menu.active_tab=="feats" | AC-069.4 |
| 11 | `feats_tab_content` | state pause_menu, widgets -> a visible MordecaiFeatDisplayWidget with a text starting "Feats:" | US-068:AC-068.1, US-068:AC-068.8 |
| 12 | `settings_placeholder` | click_button(button_id="settings") -> active_tab=="settings" and a visible widget shows "Coming Soon" | AC-069.3 |
| 13 | `shoulder_switches_tab` | tap Gamepad_LeftShoulder -> wait_until pause_menu.open==true && pause_menu.active_tab == the previous tab in tab-bar order, wrapping (settings -> inventory) | AC-069.4, US-095:AC-095.4 |
| 14 | `shoulder_rb_switches_tab` | tap Gamepad_RightShoulder -> wait_until pause_menu.open==true && pause_menu.active_tab == the next tab in tab-bar order, wrapping (inventory -> settings) | AC-069.4, US-095:AC-095.4 |
| 15 | `escape_closes` | tap Escape -> wait_until pause_menu.open==false && game.paused==false | AC-069.10, AC-069.6 |
| 16 | `reopen_restores_tab` | tap Escape -> wait_until pause_menu.open==true && pause_menu.active_tab == the tab active before escape_closes | AC-069.11 |
| 17 | `gamepad_back_closes` | tap Gamepad_FaceButton_Right -> wait_until pause_menu.open==false && game.paused==false | AC-069.10 |
| 18 | `start_button_opens` | tap Gamepad_Special_Right -> wait_until pause_menu.open==true && game.paused==true | AC-069.7 |
| 19 | `escape_closes_again` | tap Escape -> wait_until pause_menu.open==false | - |
| 20 | `i_opens_inventory` | tap I -> wait_until pause_menu.open==true && pause_menu.active_tab=="inventory" | US-095:AC-095.2 |
| 21 | `i_closes` | tap I (the menu closes on its key-up) -> wait_until pause_menu.open==false && game.paused==false within 3s | US-095:AC-095.2 |
| 22 | `i_held_closes_on_key_up` | tap I -> wait_until pause_menu.open==true; press I, wait 1.2s (key repeats from 0.5s) -> pause_menu.open==true while held; release I -> wait_until pause_menu.open==false && game.paused==false; wait 1s -> still closed (a held I never reopens the menu) | US-095:AC-095.5 |
| 23 | `final_close` | if the menu is still open: tap Escape -> wait_until pause_menu.open==false (runs even when i_closes or i_held_closes_on_key_up failed) | - |

- **Before this story** (sweep on `8ea853b`, run `Saved/Playtests/20261022-184913-us069`): `shoulder_switches_tab` FAIL (NEW, then "active_tab changes"), `i_closes` FAIL (known); `shoulder_rb_switches_tab` did not exist.
- **Playable alone:** YES

## Definition of Done
- [x] All ACs checked off (N18 decided "keep"); AC-095.2/095.4/095.5 accepted by the fresh-main Tier 4 run `Saved/Playtests/20261127-014050-us069`
- [x] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [x] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [x] Content: none expected; `py Scripts/check_content_committed.py` exits 0
- [x] Work committed FIRST (code + script edits), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [x] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap` exits 0 — never `--allow-stale`
- [x] On that same fresh package `py Scripts/playtest_story.py US-095` exits 0: all five US-095 checks PASS and all 20 covered ACs are accepted (`Saved/Playtests/20261127-014032-story-us095`)
- [x] Known issues "I does not close the pause menu" and N18 are marked FIXED/decided in `Specs/agent_playtest_plan_v1.md`
- [x] Every screenshot was verified numerically under the 2026-11-26 media-viewing decision; coordinator viewing remains separate
- [x] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-095]` closing commit
- [x] Closing commits pushed; story moved to `stories/done/` after the fresh-main story run and full sweep passed

## Completion Notes

**Historical status (2026-10-30):** Every US-095 check PASSed, but the shared `us069_pause_menu.py` still failed on N4/N5 owned by US-066/067/068. Those blockers are now fixed; see the 2026-11-27 close-out below.

**What changed** (commits `9be2071`, `54e7775`, `3294f0a`; packaged `3294f0a`):
- `UMordecaiPauseMenuWidget` registers CommonUI UI action bindings once in `NativeOnInitialized`:
  - LB / RB: previous / next tab, wrapping (`FMordecaiPauseMenuTabModel::CycleActiveTab`).
  - I: the press arms the close, the key-up closes through `DeactivateWidget`, the back action's path.
  - Tags `UI.Action.Mordecai.PauseMenu.*`, keys in `Config/DefaultInput.ini`.
- The review of this change found that a close on the I press let a held I's key repeats reopen the menu. That led to the key-up close and AC-095.5, which was added during implementation.
- N18 decided "keep" (`Specs/design_decisions_log.md`, 2026-10-30).

**Tier 1:** 762 / 762 Mordecai tests pass (`py Scripts/run_tests.py`, final run after the review fix). The four new `Mordecai.Recovery.PauseMenu.*` tests were red before the fix: 4 FAIL, while the existing `RebindsContentOnReactivate` passed.

**Tier 2:** `py Scripts/run_pie_verify.py`, 16 / 16 scripts PASS. `verify_pie_pause_menu.py` shows the open menu registered with the live CommonUI router PreviousTab 1x, NextTab 1x and Inventory 2x (press + key-up), with no duplicates. Its `[LOGIC]` next / previous / close checks PASS. No `LogUIActionRouter` error in the editor log.

**Python tooling:** `py Scripts/run_python_tests.py`, 549 / 549. After the last script edits, `-k us069` is 19 / 19. `py Scripts/playtest_story.py US-069 --lint` and `US-095 --lint` PASS.

**Tier 3** (fresh package of `3294f0a`, dirty=False): `Saved/Playtests/20261030-235145-boot`, exit 0.
- Checks: `build_fresh`, `launch`, `experience_identified` / `experience_loaded` (B_MordecaiArenaExperience at 6.3 s), `no_crash`, `content_cooked`, `loading_screen` (Mordecai, 2.25 s), `game_window`, `engine_screenshots` 3/3 and `screenshots` 3/3 all PASS. `log_errors`: 0 errors, 8 warnings.
- `engine_loading.png`: "PROJECT MORDECAI / Loading..." on dark grey (the Mordecai loading screen).
- `engine_01.png` – `engine_03.png`: MovementTestMap's grey grid floor with the player capsule centred, the HUD bars top-left (SP "10 / 10"), "Kills: 0" top-right, the step block top-right, "[I] Inventory" bottom-centre, and the yellow Steam / loading-screen debug lines (issue #29).
- `shot_01.png` – `shot_03.png`: the same frames as desktop captures at 1920×1080.

**Tier 4** (same package): `py Scripts/playtest_story.py US-095` → `Saved/Playtests/20261030-235145-story-us095/summary.md`; us069 run `Saved/Playtests/20261030-235207-us069`, exit 1, 32 PASS / 0 WARN / 3 FAIL / 1 SKIP. The window was not focused (`app_has_focus` false, `viewport_has_focus` true); the run never needed the virtual cursor (`virtual_cursor` false).
- US-095 checks, all PASS:
  - `shoulder_switches_tab`: LB on Settings → `active_tab=="inventory"`.
  - `shoulder_rb_switches_tab`: RB → `"settings"`.
  - `i_opens_inventory`.
  - `i_closes`: `open==false && paused==false`.
  - `i_held_closes_on_key_up`: "pressed I in the open menu and held it 1.2s: pause_menu.open=True; released: closed after 0s; still closed 1s later".
- AC status: AC-095.2, AC-095.4 and AC-095.5 PASS; AC-069.4 PASS.
- Known FAILs: `character_tab_content` (N4), `skills_tab_content` (N5) and `feats_tab_content` (N5). No check that PASSed before now FAILs. `final_close` SKIP: I already closed the menu.
- `bridge_01.png`: "Paused", tab bar Character / Skills / Feats / Inventory / Settings, Character highlighted, empty content area (N4).
- `bridge_02.png`: Skills highlighted, "Coming Soon" (N5).
- `bridge_03.png`: Feats highlighted, "Coming Soon" (N5).
- `bridge_04.png` (`menu_lb`, after LB on Settings): Inventory highlighted. The content area shows the filter bar All / Weapons / Armor / Trinkets / Consumables / Materials / Quest / Magical, "Inventory is empty." and "No resources stored." The filter bar overlaps the "Stored Resources" header (issue #37).
- `bridge_05.png` (`menu_rb`, after RB): Settings highlighted, "Coming Soon".
- `bridge_06.png` (`menu_closed`): the grid floor with the player and HUD, no menu (closed by Escape).
- `bridge_07.png` (`menu_closed_by_i`): the grid floor with the player and HUD, no menu (closed by I).

**Not done / open:**
- `control_bindings_v1.1.md` still lists neither "I — Inventory" nor in-menu LB/RB (TODO(DECISION), doc change by Jeff).
- A held Escape probably reopens the menu after about 0.5 s, the same repeat path I had. That comes from reading the code; it was not reproduced, and the back action is engine code.
- If an I key-up is ever lost while the menu stays active (window focus loss, a modal over the menu), the next I key-up closes the menu. Low impact.

### 2026-11-27 close-out on current main

**Status: DONE.** Re-proofed the shipped implementation on clean `origin/main`
`3aece9dc225f995c35d8b30ca0f6daa14f2a9f99`; no gameplay code regressed and
no code, content, config or playtest script changed in this close-out.

- Compile: `LyraEditor Win64 Development` PASS.
- Tier 1 targeted: `Mordecai.Recovery.PauseMenu` 7 / 7 PASS
  (`Saved/TestLogs/tests-20261127-002803.log`).
- Tier 1 overall: 984 / 984 `Mordecai.*` PASS
  (`Saved/TestLogs/tests-20261127-002955.log`).
- Python tooling: 817 / 817 PASS.
- Tier 2: 23 / 23 PIE scripts PASS. `verify_pie_pause_menu.py` is 44 / 44:
  PreviousTab and NextTab registered once, Inventory registered twice (press and
  key-up), and the `[LOGIC]` next / previous / close diagnostics pass.
- Package: `PackagedBuilds/Playtest/build_info.json` records commit
  `3aece9dc`, `dirty=false`, `source_changed_during_build=false`; UAT log
  `Saved/Playtests/package-20261127-012809.log`.
- Tier 3: `Saved/Playtests/20261127-013954-boot` PASS (fresh package,
  B_MordecaiArenaExperience, no crash, no missing cooked content, 0 errors,
  all 3 engine and 3 desktop frames non-black/non-uniform). The story runner's
  second Tier 3 boot also PASSed at `Saved/Playtests/20261127-014032-boot`.
- Tier 4: `Saved/Playtests/20261127-014050-us069` PASS, exit 0,
  37 PASS / 0 WARN / 0 FAIL / 1 SKIP; 20 / 20 covered ACs accepted.
  Story summary: `Saved/Playtests/20261127-014032-story-us095/summary.md`.
  - `i_opens_inventory`: PASS — tapped I; menu open on Inventory after 0 s.
  - `i_closes`: PASS — tapped I; menu closed and game unpaused after 0 s.
  - `shoulder_switches_tab`: PASS — LB changed Settings to Inventory after 0 s.
  - `shoulder_rb_switches_tab`: PASS — RB changed Inventory to Settings after 0 s.
  - `i_held_closes_on_key_up`: PASS — menu stayed open during the 1.2 s hold,
    closed on release after 0 s, and remained closed 1 s later.
- Tier 4 numeric frame checks: all eight `bridge_NN.png` files are 1280x720,
  non-black and non-uniform. US-095-specific paths:
  `bridge_05.png` (LB/Inventory, stddev 22.9), `bridge_06.png`
  (RB/Settings, stddev 18.0), and `bridge_08.png` (closed by I, stddev 32.0).
  Per the 2026-11-26 decision, the builder did not open or delegate media.
- Full sweep (required because the close-out adds LFS marketing stills):
  `Saved/Playtests/20261127-014334-sweep` PASS, 6 / 6 Tier 3 boots and
  22 / 22 Tier 4 scripts, no failures. Compared with the prior all-PASS sweep
  `Saved/Playtests/20261126-223825-sweep`, `pass_to_fail=[]`.
- Marketing: `marketing/us095/` contains three 1920x1080 LFS stills for
  Character, Inventory and Settings. ffprobe, blackdetect, non-uniform/black
  checks, green/yellow debug-colour pixel checks, and runtime actor-name-label
  checks all PASS in `content-check.json`. The optional clip was omitted because
  a paused static menu cannot honestly meet the >=90% mpdecimate unique-frame
  rule without artificial rapid tab changes.

**Open items (not fixed here):**
- Held Escape may reopen the menu after about 0.5 s; this remains unreproduced.
- `Specs/control_bindings_v1.1.md` still lacks “I — Inventory” and in-menu
  LB/RB; that documentation change belongs to Jeff and was not edited here.
