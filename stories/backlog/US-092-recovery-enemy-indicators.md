# US-092: Recovery — Enemy Indicators Appear for Enemies in Aggro Range

**Epic:** Epic R: Playability Recovery
**Priority:** P1
**Depends on:** none (US-085 makes the enemies visible; indicators work on invisible ones too)
**Paired story:** none

## Overview
US-064 migrated enemy indicators to Lyra's IndicatorSystem, but in the packaged arena no `EnemyIndicator` widget is ever on screen, even with three Frontliners in aggro range (N17). Source reading shows three gaps: nothing calls `NotifyAggroActivated`, a failed registration is never retried, and the Mordecai arena has neither the `ULyraIndicatorManagerComponent` on the player controller nor an `UIndicatorLayer` in the HUD that Lyra's ShooterCore experience normally adds. Feature: enemy_indicators.

## Execution Mode
**HEADLESS**
C++ only: component wiring, the PC's indicator manager component and an `UIndicatorLayer` in `UMordecaiCombatHUDWidget::BuildDefaultLayout` (`WBP_CombatHUD` has no widget tree, so the C++ default layout is what ships).

## References
- `Specs/agent_playtest_plan_v1.md` — N17
- Sweep `Saved/Playtests/20261022-184702-sweep/summary.md`; run `Saved/Playtests/20261022-184803-us054`: `enemy_indicators` FAIL NEW ("not met within 3s: 0 EnemyIndicator widget(s) on screen (expected >= 3, one per enemy in aggro range)"); `enemy_aggro` PASS (all three not Idle)
- Done stories: US-064 AC-064.2 (register on damage or aggro, 120 cm offset), AC-064.4 (visibility range), AC-064.10 (one per enemy); US-054 AC-054.9 / US-052 AC-052.10 (the old `WBP_EnemyHealthBar` widget components, still shown — check `enemy_health_bars` PASS)
- Existing code:
  - `Source/LyraGame/Mordecai/UI/MordecaiEnemyIndicatorComponent.cpp:65-71` (`NotifyAggroActivated`, no caller in `Source/LyraGame/Mordecai/`), `:95-108` (`bIndicatorRegistered = true` is set before the manager null-check, so a registration attempted before the PC has a manager is lost forever), `:193-208` (`FindIndicatorManager` → `ULyraIndicatorManagerComponent::GetComponent(PC)`)
  - `Source/LyraGame/Mordecai/Enemy/MordecaiEnemyCharacter.cpp:47,186-188,233-235` (damage/death notifications only)
  - `Source/LyraGame/UI/IndicatorSystem/LyraIndicatorManagerComponent.*`, `IndicatorLayer.*` (Lyra)
  - `Source/LyraGame/Mordecai/UI/MordecaiCombatHUDWidget.cpp:34-127` (no indicator layer)

## Acceptance Criteria
- [ ] AC-092.1: When an enemy's AI enters aggro on the player, its indicator component registers an indicator — verified by `Mordecai.Recovery.EnemyIndicator.RegistersOnAggro`
- [ ] AC-092.2: A registration attempted while no indicator manager is available is retried (the flag is set only after `AddIndicator` succeeds) — verified by `Mordecai.Recovery.EnemyIndicator.RetriesWhenManagerMissing`
- [ ] AC-092.3: `AMordecaiPlayerController` has a `ULyraIndicatorManagerComponent` and the default combat HUD contains an `UIndicatorLayer`, without depending on the ShooterCore experience — verified by `Mordecai.Recovery.EnemyIndicator.ArenaHasManagerAndLayer`
- [ ] AC-092.4: In the packaged build, standing at the spawn with three Frontliners in aggro range, the player sees one indicator per enemy within 3 s — verified by check `enemy_indicators` of `Scripts/playtests/us054_arena.py` (Tier 4; state read while the player stands — no input needed beyond the launch)

## Technical Notes
- Call `NotifyAggroActivated()` from the enemy AI's transition out of Idle (the same place `ai_state` changes to Chase/Attack).
- The indicator widget class (`IndicatorWidgetClass`, soft class) must resolve in the packaged build; if it defaults to the native `UMordecaiEnemyIndicatorWidget`, make sure that widget builds a default tree (`BuildDefaultContent` exists at `MordecaiEnemyIndicatorWidget.cpp:46`).
- TODO(DECISION) (Jeff): with indicators working, each enemy shows two health readouts (the old `WBP_EnemyHealthBar` widget component and the new indicator). US-064 was a "migration"; should the widget-component bar be removed? If yes, `enemy_health_bars` (AC-054.9, AC-052.10) must be rewritten to check the indicator instead — out of scope until decided; do not remove it here.
- Script bookkeeping: `enemy_indicators` was NEW (no `known_issue`).
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.EnemyIndicator.RegistersOnAggro` — AC-092.1
- `Mordecai.Recovery.EnemyIndicator.RetriesWhenManagerMissing` — AC-092.2
- `Mordecai.Recovery.EnemyIndicator.ArenaHasManagerAndLayer` — AC-092.3
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_hud.py`: the PC has an indicator manager component; within 3 s of PIE start three enemy indicators are registered
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us054_arena.py` check `enemy_indicators`

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py` (DevTestMap → B_MordecaiArenaExperience); Tier 4: `py Scripts/playtest_story.py US-054` on the same package
- **Player steps:** stand at the spawn
- **Visible on screen:** an indicator (health bar above the head, 120 cm offset) over each of the three enemies in the us054 `start` screenshot and in Tier 3 `engine_NN.png`
- **Proof:** Tier 3 screenshots; Tier 4 check `enemy_indicators` of `Scripts/playtests/us054_arena.py`
- **Tier 4 script:** `Scripts/playtests/us054_arena.py` — run: `py Scripts/playtest_story.py US-054`
- **Steps:**

| Script | Check | Player step (bridge calls) → expected | Sweep result (8ea853b) |
|---|---|---|---|
| us054 | `enemy_indicators` | state widgets -> one EnemyIndicator widget per enemy in aggro range within 3s | FAIL (NEW): 0 |

- **Playable alone:** YES

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: none; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py` exits 0 — never `--allow-stale`
- [ ] On that same fresh package `py Scripts/playtest_story.py US-054`: `enemy_indicators` goes from FAIL to PASS; no check that PASSed in the sweep now FAILs; `--lint` exits 0 — never `--allow-stale`
- [ ] Known issue N17 marked FIXED in `Specs/agent_playtest_plan_v1.md`
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen vs. "Visible on screen"
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-092]` closing commit
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
<Filled in by the coding agent.>
