# US-092: Recovery — Enemy Indicators Appear for Enemies in Aggro Range

**Epic:** Epic R: Playability Recovery
**Priority:** P1
**Depends on:** none (US-085 makes the enemies visible; indicators work on invisible ones too)
**Paired story:** none

## Overview
US-064 migrated enemy indicators to Lyra's IndicatorSystem, but in the packaged arena no `EnemyIndicator` widget was ever on screen (N17). The first recovery restored manager/layer wiring and aggro registration. Jeff's final product call now makes the replacement behave like the retired bar: one always-on indicator for every living enemy from spawn, including an Idle enemy whose AggroRange is zero. Feature: enemy_indicators.

## Execution Mode
**HEADLESS**
C++ only: component wiring, the PC's indicator manager component and an `UIndicatorLayer` in `UMordecaiCombatHUDWidget::BuildDefaultLayout` (`WBP_CombatHUD` has no widget tree, so the C++ default layout is what ships).

## References
- `Specs/agent_playtest_plan_v1.md` — N17
- Sweep `Saved/Playtests/20261022-184702-sweep/summary.md`; run `Saved/Playtests/20261022-184803-us054`: `enemy_indicators` FAIL NEW ("not met within 3s: 0 EnemyIndicator widget(s) on screen (expected >= 3, one per enemy in aggro range)"); `enemy_aggro` PASS (all three not Idle)
- Done stories: US-064 AC-064.2 (registration and 120 cm offset), AC-064.4 (visibility range), AC-064.10 (one per enemy); US-054 AC-054.9 / US-052 AC-052.10 are migrated by this story from the retired `WBP_EnemyHealthBar` to the indicator — check `enemy_health_bars`
- Baseline code at the start of the recovery:
  - `Source/LyraGame/Mordecai/UI/MordecaiEnemyIndicatorComponent.cpp:65-71` (`NotifyAggroActivated`, no caller in `Source/LyraGame/Mordecai/`), `:95-108` (`bIndicatorRegistered = true` is set before the manager null-check, so a registration attempted before the PC has a manager is lost forever), `:193-208` (`FindIndicatorManager` → `ULyraIndicatorManagerComponent::GetComponent(PC)`)
  - `Source/LyraGame/Mordecai/Enemy/MordecaiEnemyCharacter.cpp:47,186-188,233-235` (damage/death notifications only)
  - `Source/LyraGame/UI/IndicatorSystem/LyraIndicatorManagerComponent.*`, `IndicatorLayer.*` (Lyra)
  - `Source/LyraGame/Mordecai/UI/MordecaiCombatHUDWidget.cpp:34-127` (no indicator layer)

## Acceptance Criteria
- [ ] AC-092.1: Every living enemy requests and registers its indicator from `BeginPlay`, without waiting for aggro or damage — verified by `Mordecai.Recovery.EnemyIndicator.RegistersOnSpawn`
- [x] AC-092.2: A registration attempted while no indicator manager is available is retried (the flag is set only after `AddIndicator` succeeds) — verified by `Mordecai.Recovery.EnemyIndicator.RetriesWhenManagerMissing`
- [x] AC-092.3: `AMordecaiPlayerController` has a `ULyraIndicatorManagerComponent` and the default combat HUD contains an `UIndicatorLayer`, without depending on the ShooterCore experience — verified by `Mordecai.Recovery.EnemyIndicator.ArenaHasManagerAndLayer`
- [ ] AC-092.4: In the packaged build, the player sees one indicator per living enemy from spawn within 3 s, including the Idle CombatTestMap TestDummy — verified by Tier 3 screenshots and check `enemy_indicators` of `Scripts/playtests/us054_arena.py`

## Technical Notes
- Jeff's 2026-11-05 decision supersedes aggro gating: `BeginPlay` latches the indicator request and tries registration immediately; the timer retries if the local manager is not ready. Damage and aggro notifications remain idempotent retries.
- `ResetForArena` replaces the descriptor after restoring attributes, so a survivor's widget rebinds current health and a killed enemy's indicator returns. This fixes N34's stale-fill path in code.
- The indicator widget class (`IndicatorWidgetClass`, soft class) must resolve in the packaged build; if it defaults to the native `UMordecaiEnemyIndicatorWidget`, make sure that widget builds a default tree (`BuildDefaultContent` exists at `MordecaiEnemyIndicatorWidget.cpp:46`).
- Jeff's 2026-11-05 final decision: remove `WBP_EnemyHealthBar` entirely; use one always-red, 150 px-wide EnemyIndicator health bar and migrate `enemy_health_bars` to verify the replacement.
- Script bookkeeping: `enemy_indicators` was NEW (no `known_issue`).
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.EnemyIndicator.RegistersOnSpawn` — AC-092.1
- `Mordecai.Recovery.EnemyIndicator.RetriesWhenManagerMissing` — AC-092.2
- `Mordecai.Recovery.EnemyIndicator.RefreshesOnArenaReset` — always-on reset lifecycle and N34
- `Mordecai.Recovery.EnemyIndicator.ArenaHasManagerAndLayer` — AC-092.3
- `Mordecai.Recovery.EnemyIndicator.ReplacesLegacyHealthBar` — retired asset/property absent; replacement bar is red and 150 x 20
- `Mordecai.UI.EnemyIndicator.HealthBarIsAlwaysRed` — full, mid and empty health all use the red enemy color
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_hud.py`: the PC has an indicator manager component; the three living enemies register always-on indicators from spawn
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us054_arena.py` check `enemy_indicators`

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py` (DevTestMap → B_MordecaiArenaExperience); Tier 4: `py Scripts/playtest_story.py US-054` on the same package
- **Player steps:** stand at the spawn
- **Visible on screen:** one red, 150 px-wide indicator health bar (120 cm offset) over each of the three enemies, with no legacy `WBP_EnemyHealthBar`, in the us054 `start` screenshot and Tier 3 `engine_NN.png`
- **Proof:** Tier 4 checks `enemy_health_bars` and `enemy_indicators` prove one visible indicator per living enemy and no legacy actor widget. Red fill and the fixed 150×20 size are Tier 1 plus screenshot evidence; bridge state does not expose progress-bar colour.
- **Tier 4 script:** `Scripts/playtests/us054_arena.py` — run: `py Scripts/playtest_story.py US-054`
- **Steps:**

| # | Check | Player step (bridge calls) → expected | Proves |
|---|---|---|---|
| 1 | `hud_visible` | state widgets -> HealthBar, StaminaBar, PostureBar, SpellPointsBar and KillCounter widgets on screen, a text 'Kills: 0' | AC-054.7, AC-054.8, US-063:AC-063.3, US-063:AC-063.4 |
| 2 | `hud_layout` | state widgets rects -> HealthBar top-left (x<300, y<60), StaminaBar below it, PostureBar below that | AC-054.7 |
| 3 | `enemies_present` | state actors (radius 3000) -> 3 kind=='enemy', health {200, 200}, pairwise > 300 cm apart | AC-054.11, AC-054.2, US-050:AC-050.3 |
| 4 | `enemies_visible` | state actors -> every enemy has_visible_mesh==true | AC-054.13 |
| 5 | `enemy_health_bars` | state actors + widgets -> exactly one visible EnemyIndicator per enemy and no actor-attached EnemyHealthBar | AC-054.9, US-052:AC-052.10 |
| 6 | `enemy_indicators` | state widgets -> one always-on EnemyIndicator widget per living enemy from spawn within 3s | US-064:AC-064.2, US-064:AC-064.10, US-092:AC-092.4 |
| 7 | `enemy_aggro` | stand at the spawn -> every enemy ai_state != 'Idle' within 3s | US-051:AC-051.3 |
| 8 | `enemy_approaches` | stand -> some enemy > 100 cm from every spawn within 6s | US-051:AC-051.4 |
| 9 | `enemy_damages_player` | stand at the spawn -> wait_until player.attributes.health < player.attributes.max_health within 20s | AC-054.13, US-051:AC-051.5 |

- **Playable alone:** YES

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: none; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py` exits 0 — never `--allow-stale`
- [ ] On that same fresh package `py Scripts/playtest_story.py US-054`: `enemy_indicators` goes from FAIL to PASS; no check that PASSed in the sweep now FAILs; `--lint` exits 0 — never `--allow-stale`
- [x] Known issue N17 marked FIXED in `Specs/agent_playtest_plan_v1.md`
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen vs. "Visible on screen"
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-092]` closing commit
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
### Prior dual-bar evidence (superseded)
Implementation commit `318be74`; playtest bookkeeping commit `dbe771f`. These runs preserved the then-current
dual-bar hold and provided the screenshots Jeff used to make the final remove-and-restyle decision.

**Red → green.** Before implementation, all three `Mordecai.Recovery.EnemyIndicator.*` tests failed on the
intended gaps (manager, widget class, HUD layer, aggro registration, retry membership). After implementation:
the project compiled; Tier 1 passed 775/775; Python tooling passed 588/588 plus the final 45-test US-054 subset;
US-092 lint passed; and Tier 2 passed 16/16 scripts. `verify_pie_hud.py` specifically saw the controller manager,
three enemy indicator components and all three registered indicators.

**Fresh package.** `py Scripts/package_playtest.py` packaged clean commit
`dbe771fb65074f75b3ad227e34a9bb5cab55631e` (dirty=false, no source changed during build; UAT log
`Saved/Playtests/package-20261105-015903.log`). Standalone Tier 3
`Saved/Playtests/20261105-021039-boot` passed every gameplay check: fresh build, Mordecai experience, no crash,
content cooked, Mordecai loading screen and 3/3 engine frames. Its desktop `shot_01..03.png` are black because
the workstation was covered/locked; the corresponding engine frames are valid.

**Tier 3 + Tier 4.** `Saved/Playtests/20261105-021148-story-us054` used that same fresh package.
Both Tier 3 boots passed: DevTestMap `Saved/Playtests/20261105-021148-boot` and CombatTestMap
`Saved/Playtests/20261105-021209-boot`. Tier 4 arena
`Saved/Playtests/20261105-021229-us054` changed `enemy_indicators` from 0/3 FAIL in
`Saved/Playtests/20261104-233032-us054` to **3/3 PASS** within 3 s. `enemy_health_bars` also remained PASS.
Every previously passing arena check remained passing. The run still exits 1 only on the pre-existing,
unchanged `enemies_present` spacing clause (229–264 cm after the Frontliners have already swarmed the player,
versus 230–265 cm in the prior run); this is tracked by in-progress US-085 and is outside US-092. Because the
reused Tier 4 script is not an acceptance candidate, AC-092.4 remains unchecked and this story remains in
`in-progress/` pending Jeff's close-at-call decision. Combat Tier 4
`Saved/Playtests/20261105-021241-us054combat` passed (32 PASS, 1 expected Burning WARN, 0 FAIL).

**Screenshot review.**
- Standalone boot `engine_loading.png`: black Mordecai loading screen with `PROJECT MORDECAI`; `engine_01.png`:
  arena/HUD and three Frontliners, each with a green named `EnemyIndicator` visibly above the retained red
  `WBP_EnemyHealthBar`; `engine_02.png` and `engine_03.png`: dimmed death screen/countdown with the old red bars
  still behind it.
- DevTestMap story boot `engine_loading.png`: Mordecai loading screen; `engine_01.png` and `engine_02.png`: the
  three red cylinder Frontliners with both bars—green named indicators over red legacy bars—with substantial
  duplicate-label overlap; `engine_03.png`: `You Died` countdown over the dimmed arena and legacy red bars.
- CombatTestMap story boot `engine_loading.png`: Mordecai loading screen; `engine_01.png`,
  `engine_02.png`, `engine_03.png`: fixed-camera grey grid, player and idle TestDummy with its legacy red bar,
  full combat HUD and loadout plate.
- Arena Tier 4 `bridge_01.png`: fixed-camera arena, three Frontliners crowded around the player, and both bars
  clearly visible on all three (green named `EnemyIndicator` plus red `WBP_EnemyHealthBar`); `bridge_02.png`:
  the same dual-bar view after a hit, with player health reduced.
- Combat Tier 4 `bridge_01.png`: grid start/HUD and distant dummy; `bridge_02.png`: player beside the live
  TestDummy and its red bar; `bridge_03.png`: dead dummy body gone, empty bar and `Kills: 1`;
  `bridge_04.png`: player beside the orange attacker after a hit, with the green named indicator over the
  retained legacy bar; `bridge_05.png`: dimmed `You Died`, countdown and `Kills: 1`; `bridge_06.png`: respawned
  player at full health, `Kills: 0`, loadout restored and dummy reset.

Those packaged screenshots showed both systems rendering simultaneously with heavy overlap. Jeff superseded the
interim hold on 2026-11-05: remove `WBP_EnemyHealthBar`, make the EnemyIndicator bar red, and retain the old 150 px
width. New-package evidence for that decision is recorded below after the replacement run.

### Single-bar replacement evidence (lifecycle superseded)

Implementation commit `45c834382852b80f865b64340c1b5704e19d1778` removes the legacy C++ widget,
`WBP_EnemyHealthBar`, its enemy property/component creation path, and the three enemy Blueprint references.
`UMordecaiEnemyIndicatorWidget` now wraps its health progress bar in a fixed 150 x 20 size box and uses the old
bar's exact `FLinearColor::Red` at every health percentage. `enemy_health_bars` now requires exactly one visible
EnemyIndicator per enemy and no actor-attached legacy widget.

**Red → green.** Before implementation,
`Mordecai.Recovery.EnemyIndicator.ReplacesLegacyHealthBar` failed because the legacy property and asset existed,
the size box did not, and the indicator started green; the migrated tooling test also failed because it still
accepted a legacy actor widget. After implementation: LyraEditor compiled; Tier 1 passed 776/776; Python tooling
passed 589/589; Tier 2 passed 16/16; US-092 lint passed; and the content guard passed with the deleted asset and
three resaved enemy Blueprints committed.

**Fresh package.** `py Scripts/package_playtest.py` packaged clean commit `45c8343828` (dirty=false; source
unchanged during build) with UAT log `Saved/Playtests/package-20261105-095609.log`. Standalone Tier 3
`C:\Users\jeffd\Documents\Gamedev\ProjectMordecai\Saved\Playtests\20261105-101208-boot` passed freshness,
experience, cooked-content, loading-screen, crash and 3/3 engine-frame checks with zero log errors.

**Tier 3 + Tier 4.** Story summary
`C:\Users\jeffd\Documents\Gamedev\ProjectMordecai\Saved\Playtests\20261105-101306-story-us054` used that same
package. DevTestMap Tier 3 `20261105-101306-boot` and CombatTestMap Tier 3 `20261105-101329-boot` passed.
Arena Tier 4 `20261105-101349-us054` reported `enemy_health_bars` **PASS** (3 visible EnemyIndicators for 3
enemies; legacy actor widgets none) and `enemy_indicators` **PASS** (3/3). All previously passing arena checks
still pass. The script exits 1 only on the unchanged US-085/#85 `enemies_present` live-spacing failure
(229–264 cm after the swarm reaches the player), matching the prior 229–264 cm failure. Combat Tier 4
`20261105-101400-us054combat` passed 32 checks, with the expected Burning WARN and no FAIL. Because the shared
arena run does not exit 0, AC-092.4 remains unchecked and the story remains in progress as directed.

**Screenshot review.**
- Standalone `20261105-101208-boot`: `engine_loading.png` is the black PROJECT MORDECAI loading screen;
  `engine_01.png` shows the arena/HUD and three Frontliners, each with one red named indicator bar and no second
  bar; `engine_02.png` and `engine_03.png` show the dimmed death countdown. `shot_01..03.png` are the desktop
  wallpaper because another window covered the game; the engine frames are valid.
- DevTestMap `20261105-101306-boot`: `engine_loading.png` is the Mordecai loading screen; `engine_01.png` and
  `engine_02.png` show one uniformly sized red bar per Frontliner, with no legacy bar; `engine_03.png` is the
  dimmed death countdown. `shot_01..03.png` again show only the covering desktop wallpaper.
- CombatTestMap `20261105-101329-boot`: `engine_loading.png` is the Mordecai loading screen;
  `engine_01..03.png` show the fixed-camera measuring grid, player and Idle TestDummy without an indicator because
  that package still used aggro/damage gating, plus the HUD/loadout. `shot_01..03.png` show the covering desktop
  wallpaper. Jeff subsequently rejected that gating in favor of always-on from spawn.
- Arena `20261105-101349-us054`: `bridge_01.png` and `bridge_02.png` show the three red Frontliner cylinders
  around the player with exactly one vivid-red indicator bar each. There is no lower legacy bar. Compared with
  prior dual-bar `20261105-021229-us054/bridge_01.png`, each replacement bar matches the old red bar's horizontal
  span; Tier 1 fixes that span at 150 px. The second frame shows reduced player health after a hit.
- Combat `20261105-101400-us054combat`: `bridge_01.png` is the grid start; `bridge_02.png` is the player beside
  the live TestDummy; `bridge_03.png` shows the dead dummy gone and no orphan legacy bar, with Kills: 1;
  `bridge_04.png` shows the orange attacker with one red indicator bar; `bridge_05.png` is the dimmed You Died
  screen; `bridge_06.png` shows the respawned player, restored targets, Kills: 0 and the loadout plate.

No new failure was filed: the sole failing check is the existing US-085/#85 spacing issue.

This package predates Jeff's always-on-from-spawn decision. Fresh evidence from the new committed lifecycle
implementation is required below before AC-092.1 or AC-092.4 can be accepted.
