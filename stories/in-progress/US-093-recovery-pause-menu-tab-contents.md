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
- [ ] AC-093.1: `UMordecaiCharacterSheetWidget` builds a default tree when none is provided: the 9 primary attributes (STR, DEX, END, CON, RES, DIS, INT, WIS, CHA) in Physical / Resilience / Magical groups with values, and Health / Stamina / Spell Points / Posture "x / y" lines bound to the ASC — verified by `Mordecai.Recovery.CharacterSheet.BuildsDefaultContent`
- [ ] AC-093.2: The pause menu registers the skill tree widget for "skills" and the feat display for "feats", and `BindTabContent` binds them to the pawn's skill and feat components; both build default trees — verified by `Mordecai.Recovery.PauseMenu.SkillsAndFeatsTabsRegisteredAndBound`, `Mordecai.Recovery.SkillTree.BuildsDefaultContent`, `Mordecai.Recovery.FeatDisplay.BuildsDefaultContent`
- [ ] AC-093.3: In the packaged build, the player presses Escape and sees the Character tab with all 9 attributes, the three group headings and the resource lines — verified by check `character_tab_content` of `Scripts/playtests/us069_pause_menu.py` (Tier 4)
- [ ] AC-093.4: The player clicks the Skills tab and sees skill categories including "Weapon" and "rank / 20" rows — verified by check `skills_tab_content` of `Scripts/playtests/us069_pause_menu.py` (Tier 4)
- [ ] AC-093.5: The player clicks the Feats tab and sees a "Feats: N unlocked" header — verified by check `feats_tab_content` of `Scripts/playtests/us069_pause_menu.py` (Tier 4)

## Technical Notes
- Follow the `BuildDefaultContent` pattern (build only when `WidgetTree->RootWidget` is null) used by `UMordecaiStatusEffectIndicatorWidget`; these widgets are created with `CreateWidget` from native classes, so their trees must be built in `NativeOnInitialized`.
- Fit the 1000×640 content area (`MordecaiPauseMenuWidget.cpp:160`); scroll box for long skill lists.
- Settings stays a "Coming Soon" placeholder (AC-069.3 accepts it; `settings_placeholder` PASS). TODO(DECISION) (Jeff): is a real Settings tab wanted in the recovery epic? Not scoped here.
- Script bookkeeping (before packaging): remove the N4 `known_issue` from `character_tab_content` and the N5 `known_issue` from `skills_tab_content` and `feats_tab_content` in `us069_pause_menu.py`.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.CharacterSheet.BuildsDefaultContent` — AC-093.1
- `Mordecai.Recovery.PauseMenu.SkillsAndFeatsTabsRegisteredAndBound`, `Mordecai.Recovery.SkillTree.BuildsDefaultContent`, `Mordecai.Recovery.FeatDisplay.BuildsDefaultContent` — AC-093.2
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_pause_menu.py` (new or extended): `[LOGIC]` open the menu via the subsystem and switch tabs; each tab's content widget has visible text children (diagnostic only)
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us069_pause_menu.py` checks `character_tab_content`, `skills_tab_content`, `feats_tab_content`

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap` (MovementTestMap → B_MordecaiArenaExperience; us069's `MAP` since US-100); Tier 4: `py Scripts/playtest_story.py US-069` on the same package
- **Player steps:** press Escape (menu opens on Character); click "Skills"; click "Feats"; press Escape
- **Visible on screen:** us069 `bridge_01.png` the Character tab with STR..CHA under Physical/Resilience/Magical and "x / y" resource lines; `bridge_02.png` the Skills tab with "Weapon" and "/ 20" rows; `bridge_03.png` the Feats tab with "Feats: N unlocked"
- **Proof:** Tier 4 checks `character_tab_content`, `skills_tab_content`, `feats_tab_content` of `Scripts/playtests/us069_pause_menu.py`
- **Tier 4 script:** `Scripts/playtests/us069_pause_menu.py` — run: `py Scripts/playtest_story.py US-069`
- **Steps:**

| Script | Check | Player step (bridge calls) → expected | Sweep result (8ea853b) |
|---|---|---|---|
| us069 | `character_tab_content` | state pause_menu, widgets -> on the character tab a visible MordecaiCharacterSheetWidget with all 9 primary attributes (STR..CHA), the groups Physical/Resilience/Magical and an 'x / y' resource line | FAIL (known N4) |
| us069 | `skills_tab_content` | state pause_menu, widgets -> a visible MordecaiSkillTreeWidget with texts including "Weapon" and "/ 20" | FAIL (known N5) |
| us069 | `feats_tab_content` | state pause_menu, widgets -> a visible MordecaiFeatDisplayWidget with a text starting "Feats:" | FAIL (known N5) |

- **Playable alone:** YES

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: none; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST (code + script edits), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap` exits 0 — never `--allow-stale`
- [ ] On that same fresh package `py Scripts/playtest_story.py US-069`: the three checks go from FAIL to PASS; no check that PASSed in the sweep now FAILs; `--lint` exits 0 — never `--allow-stale`
- [ ] Known issues N4 and N5 marked FIXED in `Specs/agent_playtest_plan_v1.md`
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen vs. "Visible on screen"
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-093]` closing commit
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
<Filled in by the coding agent.>
