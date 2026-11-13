# US-093: Recovery — Pause Menu Character, Skills and Feats Tabs Show Their Content

**Epic:** Epic R: Playability Recovery
**Priority:** P1
**Depends on:** none
**Paired story:** none

## Overview
The pause menu opens and its tabs switch, but three of them are empty for the player: the Character tab is blank (N4: `UMordecaiCharacterSheetWidget` builds no widget tree) and Skills and Feats show "Coming Soon" (N5: only Character and Inventory are registered). The character sheet (US-066), skill tree (US-067) and feat display (US-068) were all marked done and have never been seen in a packaged build. Features: character_sheet, skill_tree_ui, feat_ui, pause_menu.

## Execution Mode
**HEADLESS**
Native widget trees and tab registration/binding in C++; no editor assets.

## References
- `Specs/agent_playtest_plan_v1.md` — N4, N5; exploratory run note "The Settings tab also shows 'Coming Soon' (an extension of N5)"
- Sweep `Saved/Playtests/20261022-184702-sweep/summary.md`; run `Saved/Playtests/20261022-184913-us069`: `character_tab_content` FAIL known ("MordecaiCharacterSheetWidget visible with 0 text(s); missing attributes ['STR', ... 'CHA'], missing groups ['Physical', 'Resilience', 'Magical']"), `skills_tab_content` FAIL known ("the content area shows 'Coming Soon'"), `feats_tab_content` FAIL known
- Exploratory run `Saved/Playtests/20261022-185207-explore/report.md` (Settings tab also "Coming Soon")
- Done stories: US-066 AC-066.1/.3/.4/.5 (9 primary attributes in Physical/Resilience/Magical groups, resource "x / y" lines), US-067 AC-067.1/.3/.4 (skill categories, "rank / 20"), US-068 AC-068.1/.8 ("Feats: N unlocked" header), US-069 AC-069.3 (Settings placeholder is acceptable)
- `Specs/character_attributes_v1.md`, `Specs/skill_sheet_v1.1.md`
- Existing code: `Source/LyraGame/Mordecai/UI/MordecaiCharacterSheetWidget.cpp:36-40` (no tree), `Source/LyraGame/Mordecai/UI/MordecaiPauseMenuWidget.cpp:60-69` (`InitializeTabs` registers only character + inventory), `:252-276` (`BindTabContent` binds only inventory and character), `MordecaiSkillTreeWidget.*`, `MordecaiFeatDisplayWidget.*`

## Acceptance Criteria
- [x] AC-093.1: `UMordecaiCharacterSheetWidget` builds a scrollable default tree when none is provided: the exact-case localized codes STR, DEX, END, CON, RES, DIS, INT, WIS, CHA in Physical / Resilience / Magical groups with distinct values and `+X%` effective modifiers, and Health / Stamina / Spell Points / Posture "x / y" lines bound to the ASC — verified by `Mordecai.Recovery.CharacterSheet.BuildsDefaultContent`
- [x] AC-093.2: The pause menu registers the skill tree widget for "skills" and the feat display for "feats", and `BindTabContent` binds them to the Skill and Feat components that live on the PlayerState; both build default trees — verified by `Mordecai.Recovery.PauseMenu.SkillsAndFeatsTabsRegisteredAndBound`, `Mordecai.Recovery.SkillTree.BuildsDefaultContent`, `Mordecai.Recovery.FeatDisplay.BuildsDefaultContent`
- [x] AC-093.3: In the packaged build, the player presses Escape and sees the Character tab with all 9 exact-case codes, values and effective modifiers, the three group headings and all four named resource lines — verified by check `character_tab_content` of `Scripts/playtests/us069_pause_menu.py` (Tier 4)
- [x] AC-093.4: The player clicks the Skills tab and sees the real ordered 42-skill catalogue with "rank / 20" rows, then clicks the real Magic category button and sees `Magic · 11 skills` and `Fireball`; visible ranks and Skill Points refresh when the PlayerState component changes — verified by Tier 1 catalogue/category/refresh coverage and checks `skills_tab_content` and `click_magic_category` of `Scripts/playtests/us069_pause_menu.py` (Tier 4)
- [x] AC-093.5: The player clicks the Feats tab and sees a "Feats: N unlocked" header — verified by check `feats_tab_content` of `Scripts/playtests/us069_pause_menu.py` (Tier 4)

## Technical Notes
- Follow the `BuildDefaultContent` pattern (build only when `WidgetTree->RootWidget` is null) used by `UMordecaiStatusEffectIndicatorWidget`; these widgets are created with `CreateWidget` from native classes, so their trees must be built in `NativeOnInitialized`.
- Fit the 1000×640 content area (`MordecaiPauseMenuWidget.cpp:160`); scroll box for long skill lists.
- Settings stays a "Coming Soon" placeholder (AC-069.3 accepts it; `settings_placeholder` PASS). A real Settings tab is split to needs-decision issue #129 and is not scoped here.
- Script bookkeeping (before packaging): remove the N4 `known_issue` from `character_tab_content` and the N5 `known_issue` from `skills_tab_content` and `feats_tab_content` in `us069_pause_menu.py`.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.CharacterSheet.BuildsDefaultContent` — AC-093.1
- `Mordecai.Recovery.PauseMenu.SkillsAndFeatsTabsRegisteredAndBound`, `Mordecai.Recovery.SkillTree.BuildsDefaultContent`, `Mordecai.Recovery.FeatDisplay.BuildsDefaultContent` — AC-093.2
- `Mordecai.Recovery.Skills.StartingSetMatchesSheet` — all 42 names and localized display names match case-sensitively and in explicit sheet order; catalogue objects are transient
- `Mordecai.Recovery.Skills.StartingRegistrationHasNoGameplayEffects` — the spawned production PlayerState path adds no milestone names, `Mordecai.Skill.*` ASC tags or activatable abilities
- `Mordecai.Recovery.Skills.ClientMutatorsAreRejected` — rank, points and allocation mutators reject non-authority owners
- `Mordecai.Recovery.SkillTree.BuildsDefaultContent` — real category-button switching, exact category counts, live rank/points refresh, selected fill, wrapping/width constraints and rebuild-churn guards
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_pause_menu.py` (new or extended): `[LOGIC]` open the menu via the subsystem and switch tabs; each tab's content widget has visible text children (diagnostic only)
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us069_pause_menu.py` checks `character_tab_content`, `skills_tab_content`, `click_magic_category`, `feats_tab_content`

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap` (MovementTestMap → B_MordecaiArenaExperience; us069's `MAP` since US-100); Tier 4: `py Scripts/playtest_story.py US-069` on the same package
- **Player steps:** press Escape (menu opens on Character); click "Skills"; click the "Magic" category; click "Feats"; press Escape
- **Visible on screen:** us069 Character capture with exact-case STR..CHA, numeric values, `+X%` modifiers and four named "x / y" resources; Skills Weapon capture with "/ 20" rows; Skills Magic capture with `Magic · 11 skills` and `Fireball`; Feats capture with "Feats: N unlocked"
- **Proof:** Tier 4 checks `character_tab_content`, `skills_tab_content`, `click_magic_category`, `feats_tab_content` of `Scripts/playtests/us069_pause_menu.py`
- **Tier 4 script:** `Scripts/playtests/us069_pause_menu.py` — run: `py Scripts/playtest_story.py US-069`
- **Steps:**

| Script | Check | Player step (bridge calls) → expected | Round-3 result (f689caf7) |
|---|---|---|---|
| us069 | `character_tab_content` | state pause_menu, widgets -> on the character tab a visible MordecaiCharacterSheetWidget with all 9 primary attributes (STR..CHA), the groups Physical/Resilience/Magical and an 'x / y' resource line | PASS |
| us069 | `skills_tab_content` | state pause_menu, widgets -> a visible MordecaiSkillTreeWidget with texts including "Weapon" and "/ 20" | PASS |
| us069 | `click_magic_category` | click_button(button_id="magic") -> Magic category shows exact texts "Magic · 11 skills" and "Fireball" | PASS |
| us069 | `feats_tab_content` | state pause_menu, widgets -> a visible MordecaiFeatDisplayWidget with a text starting "Feats:" | PASS |

- **Playable alone:** YES

## Definition of Done
- [x] All ACs checked off
- [x] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [x] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [x] Content: none; `py Scripts/check_content_committed.py` exits 0
- [x] Work committed FIRST (code + script edits), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [x] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap` exits 0 — never `--allow-stale`
- [x] On that same fresh package `py Scripts/playtest_story.py US-069`: the three checks go from FAIL to PASS; no check that PASSed in the sweep now FAILs; `--lint` exits 0 — never `--allow-stale`
- [x] Known issues N4 and N5 accurately marked PARTIAL in `Specs/agent_playtest_plan_v1.md`: Character base/resources/effective modifiers and Skills/Feats/category switching are implemented; AC-066.6/.7 derived stats and AC-067.9 Allocate remain tracked, with the server RPC deferred to #130
- [x] Every screenshot (`engine_NN.png`, `bridge_NN.png`) analyzed by the coordinator-required Pillow script and preserved in the checksummed bundle; no laptop builder opens an image with the file reader or claims human visual inspection. Mordecai Bot opens and describes the bundled captures before merge.
- [x] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-093]` closing commit
- [x] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
- Round-3 RED commits: `8a56572e` (D1-D7/D10 Tier 1) and `4934e7fc` (strict Tier 4 mutation). `Saved/TestLogs/tests-20261113-074930.log` records the intended pre-implementation failures, including case-sensitive attribute text, effective modifiers, category switching, live refresh, authority, rebuild churn and ordered transient catalogue coverage.
- Delivered/package commit: `f689caf755344f3e630b75c81505f734198ca6d7`. `PackagedBuilds/Playtest/build_info.json` records `dirty=false` and `source_changed_during_build=false`.
- Character rows now use localized `FText` codes/display names, exact-case codes, distinct Tier 1 values and `+X%` effective modifiers. Skills use the ordered 42-row transient catalogue, localized display names, real focusable category buttons, live PlayerState-component refresh and authority-guarded mutators. The source typo is corrected to `Magic Missile`. Registration adds no milestone tags, `Mordecai.Skill.*` ASC tags or abilities.
- Tier 1: compile PASS and full `Mordecai.*` suite 843/843 PASS (`Saved/TestLogs/tests-20261113-083404.log`). Python tooling: 666/666 PASS. Tier 2: `py Scripts/run_pie_verify.py` 20/20 PASS. Content guard, LFS pointer check and `py Scripts/playtest_story.py US-069 --lint` PASS.
- Tier 3: `Saved/Playtests/20261113-093249-boot` PASS on MovementTestMap at `f689caf755`: fresh clean package, correct Mordecai experience, cooked content and loading screen, 3/3 engine frames, no crash or log errors.
- Tier 4: `Saved/Playtests/20261113-093311-us069` PASS, 37 PASS / 0 FAIL and 19/19 ACs accepted on the same fresh package. `character_tab_content` reports all exact-case codes, nine numeric values, nine `+8.5%` modifiers and all four named resource lines. `skills_tab_content`, real click `button_id="magic"` (`Magic · 11 skills`, `Fireball`) and `feats_tab_content` PASS. Captures are `bridge_01.png` Character, `bridge_02.png` Skills/Weapon, `bridge_03.png` Skills/Magic and `bridge_04.png` Feats.
- Full packaged sweep: final `Saved/Playtests/20261113-093348-sweep` compared with fresh main `8abfa991` baseline `Saved/Playtests/20261113-090752-sweep`. Across 437 shared checks there are zero PASS→FAIL and three FAIL→PASS (`character_tab_content`, `skills_tab_content`, `feats_tab_content`); two new Magic checks PASS. The four unrelated baseline failures remain unchanged (`facing_mouse`, `facing_stick`, `enemies_present`, `cycle_prev_shift_tab`).
- Known issues N4/N5 are PARTIAL, not FIXED: Character base/resources/effective modifiers and Skills/Feats/category switching are implemented; AC-066.6/.7 derived stats, AC-067.9 Allocate and the server RPC remain deferred to #130.
- The coordinator-required Pillow-only inspection analyzed 184 bundled images: 0 uniform and 0 near-uniform. The builder did not open or visually describe any image. Mordecai Bot must open and describe the bundled packaged Character/Skills Weapon/Skills Magic/Feats captures plus the Diorama/High Contrast Character and Skills captures at default and diagnostic 1.5× text before merge.
- Checksummed evidence bundle (389 hashed files, 300,102,479 bytes, hashes verified): `C:\Users\jeffd\AppData\Local\Cursor\AgentStores\cursor_agent_stores\bc-8d1ea2a5-920b-5e14-b563-40c753a87db0\files\us093-r3-f689caf7`. It contains `SHA256SUMS.txt`, package metadata, sweep comparison, baseline/final summaries, reports, logs, all sweep images, required packaged captures, diagnostic theme/text captures and scripted image analysis.
- Playable status: pause_menu, character_sheet, skill_tree_ui and feat_ui — PLAYABLE (input) (`Saved/Playtests/20261113-093311-us069` + `Saved/Playtests/20261113-093249-boot` @ `f689caf755`).
