# US-086: Recovery — Death Screen Shown on Death + Kill Counter Counts Kills

**Epic:** Epic R: Playability Recovery
**Priority:** P1
**Depends on:** US-083, US-084, US-085
**Paired story:** none

## Overview
Two "done" HUD features cannot work in the packaged game: the death screen is never created (N2: nothing outside its own files references `UMordecaiDeathScreenWidget`), and the kill counter can never go up (N1: it waits for `Mordecai.Event.EnemyKill` tag-count changes on the player ASC that nothing ever grants, while `AMordecaiGameMode` counts kills elsewhere). After US-083 (melee bound) and US-085 (enemies that fight back), the player can kill and die; this story makes both visible. Features: death_respawn, kill_counter.

## Execution Mode
**HEADLESS**
Native widgets and C++ event wiring; no asset is required (the HUD and death screen use C++-built default widget trees, like `UMordecaiCombatHUDWidget::BuildDefaultLayout`).

## References
- `Specs/agent_playtest_plan_v1.md` — N1, N2
- Sweep `Saved/Playtests/20261022-184702-sweep/summary.md`; run `Saved/Playtests/20261022-184803-us054` (`death_screen` SKIP: the player never died; `kill_counter_text_matches` PASS only compares the starting 'Kills: 0' — "no kill path exists while attacks are unbound")
- Done stories: US-062 AC-062.1 ("You Died" + ceiling-integer countdown), AC-062.2 (session kill count); US-063 AC-063.1 (counter increments on EnemyKill), AC-063.2 (resets on `Mordecai.Event.PlayerDeath`); US-053 (death/respawn flow); US-054 AC-054.13
- `Specs/control_bindings_v1.1.md` — LMB light attack, WASD
- Existing code:
  - `Source/LyraGame/Mordecai/UI/MordecaiDeathScreenWidget.*` (`InitializeDeathScreen(RespawnDelay, KillCount)`, `FormatRespawnCountdown`, `FormatKillCount`; `ULyraActivatableWidget`)
  - `Source/LyraGame/Mordecai/UI/MordecaiKillCounterWidget.cpp:68` (listens for EnemyKill tag count)
  - `Source/LyraGame/Mordecai/MordecaiGameMode.cpp:115-118` (where kills are counted)
  - `Source/LyraGame/Mordecai/MordecaiGameplayTags.cpp:168` (`Mordecai.Event.EnemyKill`)
  - `Source/LyraGame/Mordecai/Enemy/MordecaiEnemyCharacter.cpp:233-235` (death path)

## Acceptance Criteria
- [ ] AC-086.1: When the player dies, a `UMordecaiDeathScreenWidget` is pushed to the Lyra UI layer stack showing "You Died", a respawn countdown (ceiling integer) and "Kills: N"; it is removed on respawn — verified by `Mordecai.Recovery.DeathScreen.ShownOnDeathRemovedOnRespawn`
- [ ] AC-086.2: The death screen builds a default widget tree when its Blueprint/class has none (no blank widget) — verified by `Mordecai.Recovery.DeathScreen.BuildsDefaultContent`
- [ ] AC-086.3: When an enemy killed by the player dies, the kill counter widget increments from the game mode's kill event (one source of truth: the counter shows the same number as `AMordecaiGameMode`'s count / `player.kills`), and resets on `Mordecai.Event.PlayerDeath` per AC-063.2 — verified by `Mordecai.Recovery.KillCounter.IncrementsOnPlayerKill` and `Mordecai.Recovery.KillCounter.ResetsOnPlayerDeath`
- [ ] AC-086.4: In the packaged build, after the enemies have killed the player, the player sees the death screen within 1.5 s — verified by check `death_screen` of `Scripts/playtests/us054_arena.py` (Tier 4)
- [ ] AC-086.5: In the packaged build, the player kills a Frontliner with light attacks (LMB) and the HUD reads "Kills: 1" — verified by NEW checks `kill_enemy_melee` and `kill_counter_increments` added to `Scripts/playtests/us054_arena.py` (Tier 4), and `kill_counter_text_matches` still PASSes

## Technical Notes
- Death screen: create it from the player controller / HUD owner on the PlayerDeath event (the same signal US-053 uses), via Lyra's `UCommonUIExtensions::PushContentToLayer_ForPlayer` on the `UI.Layer.Game` (or `GameMenu`) layer; pass `RespawnDelay` from the game mode and the session kill count.
- Kill counter: either grant/broadcast `Mordecai.Event.EnemyKill` to the killer's ASC (gameplay event + loose tag count), or bind the widget to a game-mode/game-state delegate. Pick one path and delete the dead one; `player.kills` (bridge) and the widget must agree.
- New us054 checks (script edit, committed with the code before packaging), inserted after `walk_to_frontliner3`/`melee_lmb` and before the player stands still to die:
  - `kill_enemy_melee`: repeat `click Left (640, 360)` (≤ 0.4 s apart) while walking to the live nearest enemy, until that enemy's `dead==true` or 45 s → FAIL with the enemy's last health. Proves AC-086.5 and US-054 AC-054.13. If the 200-HP Frontliner cannot be killed in 45 s, record the damage per hit and file it — do not lower the HP.
  - `kill_counter_increments`: wait_until a widget text equals "Kills: 1" and `player.kills==1` within 2 s. Proves AC-086.5, US-063:AC-063.1.
  - The kill counter resets on death: the existing `kill_counter_text_matches` (end of run) then compares the reset value.
  - Add both to the script's `PLAN` with `proves` ids; add US-086 to `COVERS` with qualified ids (`US-086:AC-086.4`, `US-086:AC-086.5`) so `py Scripts/playtest_story.py US-086` resolves; offline test against `Scripts/tests/fake_bridge.py`.
- Remove the `known_issue` (N2) from `death_screen` in `us054_arena.py`.
- Risk: the player may die before killing an enemy (3 Frontliners). If so, the kill step should target one enemy first after the walk and use Dodge (US-083); report honestly.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.DeathScreen.ShownOnDeathRemovedOnRespawn` — AC-086.1
- `Mordecai.Recovery.DeathScreen.BuildsDefaultContent` — AC-086.2
- `Mordecai.Recovery.KillCounter.IncrementsOnPlayerKill` — AC-086.3
- `Mordecai.Recovery.KillCounter.ResetsOnPlayerDeath` — AC-086.3
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_hud.py` extended: KillCounter bound; `[LOGIC]` check that kills an enemy by applying damage directly and sees "Kills: 1" (diagnostic only)
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us054_arena.py`: existing `death_screen`, `kill_counter_text_matches`; new `kill_enemy_melee`, `kill_counter_increments` (offline-tested with `py Scripts/run_python_tests.py`)

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py` (DevTestMap → B_MordecaiArenaExperience); Tier 4: `py Scripts/playtest_story.py US-054` on the same package
- **Player steps:** walk (WASD) to Frontliner_3, click LMB repeatedly until it dies; then stand among the enemies until the player dies; wait for respawn
- **Visible on screen:** "Kills: 1" top-right after the kill; a death screen with "You Died", a countdown digit and "Kills: 1" in the `dead` screenshot; HUD back without the death screen in the `respawned` screenshot
- **Proof:** Tier 4 checks `kill_enemy_melee`, `kill_counter_increments`, `death_screen`, `kill_counter_text_matches` of `Scripts/playtests/us054_arena.py`
- **Tier 4 script:** `Scripts/playtests/us054_arena.py` — run: `py Scripts/playtest_story.py US-054` (and `py Scripts/playtest_story.py US-086` once US-086 is in `COVERS`)
- **Steps:** after implementation paste the new rows from `py Scripts/playtest_story.py US-086 --plan`; today's relevant row:

| Script | Check | Player step (bridge calls) → expected | Sweep result (8ea853b) |
|---|---|---|---|
| us054 | `death_screen` | state widgets -> within 1.5s one widget with 'You Died', a countdown digit and 'Kills: N' | SKIP (never died) |
| us054 | `kill_counter_text_matches` | state widgets, player -> the 'Kills: N' text equals player.kills | PASS (only 'Kills: 0') |
| us054 | `kill_enemy_melee` (new) | click Left repeatedly next to the nearest enemy -> its dead==true within 45 s | — |
| us054 | `kill_counter_increments` (new) | wait_until a widget text 'Kills: 1' and player.kills==1 within 2 s | — |

- **Playable alone:** YES (after US-083, US-084, US-085)

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: none expected; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST (code + us054 script edits), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py` exits 0 — never `--allow-stale`
- [ ] `py Scripts/playtest_story.py US-086` (Tier 3 + Tier 4) on that same fresh package: `death_screen`, `kill_enemy_melee`, `kill_counter_increments`, `kill_counter_text_matches` PASS and the summary's Acceptance criteria table shows AC-086.4 and AC-086.5 accepted; no check that PASSed in the sweep now FAILs; `py Scripts/playtest_story.py US-086 --lint` and `US-054 --lint` exit 0 — never `--allow-stale`
- [ ] Known issues N1 and N2 marked FIXED in `Specs/agent_playtest_plan_v1.md` with the run dir
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen vs. "Visible on screen"
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-086]` closing commit
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
<Filled in by the coding agent.>
