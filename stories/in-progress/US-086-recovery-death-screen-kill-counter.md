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
- [x] AC-086.1: When the player dies, a `UMordecaiDeathScreenWidget` is pushed to the Lyra UI layer stack showing "You Died", a respawn countdown (ceiling integer) and "Kills: N"; it is removed on respawn — verified by `Mordecai.Recovery.DeathScreen.ShownOnDeathRemovedOnRespawn`
- [x] AC-086.2: The death screen builds a default widget tree when its Blueprint/class has none (no blank widget) — verified by `Mordecai.Recovery.DeathScreen.BuildsDefaultContent`
- [ ] AC-086.3: When an enemy killed by the player dies, the kill counter widget increments from the game mode's kill event (one source of truth: the counter shows the same number as `AMordecaiGameMode`'s count / `player.kills`), and resets on `Mordecai.Event.PlayerDeath` per AC-063.2 — verified by `Mordecai.Recovery.KillCounter.IncrementsOnPlayerKill` and `Mordecai.Recovery.KillCounter.ResetsOnPlayerDeath`
- [x] AC-086.4: In the packaged build, after the enemies have killed the player, the player sees the death screen within 1.5 s — verified by check `death_screen` of `Scripts/playtests/us054_combat.py` (Tier 4, CombatTestMap: the TestAttacker kills the player)
- [ ] AC-086.5: In the packaged build, the player kills the TestDummy (200 HP, never attacks) with light attacks (LMB) and the HUD reads "Kills: 1" — verified by NEW checks `kill_enemy_melee` and `kill_counter_increments` added to `Scripts/playtests/us054_combat.py` (Tier 4, CombatTestMap), and `kill_counter_text_matches` still PASSes

## Technical Notes
- Death screen: create it from the player controller / HUD owner on the PlayerDeath event (the same signal US-053 uses), via Lyra's `UCommonUIExtensions::PushContentToLayer_ForPlayer` on the `UI.Layer.Game` (or `GameMenu`) layer; pass `RespawnDelay` from the game mode and the session kill count.
- Kill counter: either grant/broadcast `Mordecai.Event.EnemyKill` to the killer's ASC (gameplay event + loose tag count), or bind the widget to a game-mode/game-state delegate. Pick one path and delete the dead one; `player.kills` (bridge) and the widget must agree.
- Since US-100 (issue #46) US-054's walk/melee/dodge/death/respawn/reset chain is `Scripts/playtests/us054_combat.py` on CombatTestMap (`RUN_SUFFIX` "us054combat"; `py Scripts/playtest_story.py US-054` runs it). `us054_arena.py` (DevTestMap) keeps only the Frontliner AI checks and has no death chain, so every check below goes in `us054_combat.py`.
- New us054_combat checks (script edit, committed with the code before packaging), inserted after `melee_lmb`/`dodge_lalt` (at the dummy melee spot (440, 600)) and before `walk_to_attacker`:
  - `kill_enemy_melee`: repeat `click Left (640, 360)` (≤ 0.4 s apart) at the dummy melee spot, walking back to it if the dodge moved the player, until the TestDummy's (found by label, like the script's other dummy checks) `dead==true` or 45 s → FAIL with the dummy's last health. Proves AC-086.5 and US-054 AC-054.13. If the 200-HP TestDummy cannot be killed in 45 s, record the damage per hit and file it — do not lower the HP.
  - `kill_counter_increments`: wait_until a widget text equals "Kills: 1" and `player.kills==1` within 2 s. Proves AC-086.5, US-063:AC-063.1.
  - The kill counter resets on death: the existing `kill_counter_text_matches` (end of run) then compares the reset value.
  - Add both to the script's `PLAN` with `proves` ids; add US-086 to us054_combat's `COVERS` with qualified ids (`US-086:AC-086.4`, `US-086:AC-086.5`) so `py Scripts/playtest_story.py US-086` resolves; offline test against `Scripts/tests/fake_bridge.py`.
- `arena_reset` in us054_combat.py expects every enemy alive at full health after the respawn. A killed TestDummy must therefore come back alive at 200/200 (`ResetForArena` removes `Mordecai.State.Dead` and restores health); a dummy that stays dead FAILs `arena_reset`, which is a real reset bug, not a script problem.
- Remove the `known_issue` (N2) from `death_screen` in `us054_combat.py`, and its KNOWN_ISSUES lines for N2 and "kills no enemy, so kill_counter_text_matches can only compare 'Kills: 0'".
- The death itself comes from the TestAttacker at the fight spot (2000, 0), as today (`walk_to_attacker` .. `player_dies`). The TestDummy never attacks, so the kill step cannot kill the player.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.
- **Delivered in two halves (2026-10-30).** The death screen (N2, issue #7: AC-086.1, AC-086.2, AC-086.4) is fixed first, on branch `fix/issue-7-death-screen`. The kill counter (N1, issue #6: AC-086.3, AC-086.5) follows. The story stays in progress until both are done.
- **Death screen as built (issue #7):**
  - `UMordecaiDeathScreenComponent` on `AMordecaiPlayerController`, bound in `OnPossess` for a local controller, next to the combat HUD. It listens for `Mordecai.State.Dead` on the PlayerState ASC (`RegisterGameplayTagEvent`, NewOrRemoved). That tag is the one signal for both directions: there is no respawn event, and `Event.PlayerDeath` changes no tag count.
  - On the tag it pushes `UMordecaiDeathScreenWidget::StaticClass()` (no content) onto `UI.Layer.Game` through `UPrimaryGameLayout::PushWidgetToLayerStack`, with `InitializeDeathScreen(RespawnDelaySeconds, GetEnemyKillCount())` in the init function, so a pooled instance is set up again on every death. When the tag is removed, it calls `FindAndRemoveWidgetFromLayer`.
  - Layer: `UI.Layer.Game`, which is empty in Mordecai and sits below the pause menu (`Menu`) and Lyra's dialogs (`Modal`). US-062's "UI.Layer.Modal" note is superseded.
  - The widget builds its own tree in `NativeOnInitialized` when it has none: a full-screen dim with "You Died", the countdown and "Kills: N" as `UTextBlock`s in its own tree (what the bridge reads). The root is HitTestInvisible.
  - Tier 1 pushes through a real `UCommonActivatableWidgetStack` in a test layout (`Tests/Recovery/MordecaiRecoveryTestLayout.h`). The component's `SetLayoutOverride` and `SetGameModeOverride` exist for that test, because the headless world has no UI policy and no auth game mode.
  - The kill count on the death screen is `AMordecaiGameMode::GetEnemyKillCount()`, the number the bridge reports as `player.kills`. The HUD counter is N1's. Once the kill path lands, `death_screen` can also compare the screen's N with `player.kills`. Today both are 0.
  - Found by the review of the fix:
    - The pause menu can open while the player is dead, because Escape is bound on the controller and only the pawn's input is disabled. It pauses the world and the respawn timer. The countdown therefore holds while the world is paused (`TickCountdown`).
    - The screen's input config stays on the viewport after it goes, because CommonUI restores none. It therefore uses the project's capture mode, `CapturePermanently_IncludingInitialMouseDown`.
  - Like the combat HUD, the death screen serves the local player of a standalone or listen-server game. `OnPossess` runs only on the server, and `Mordecai.State.Dead` is a non-replicated loose tag (US-053). A remote client sees no death screen.
- New Tier 4 check `death_screen_removed` (after `respawn`): no widget text "You Died" within 1.5 s of the respawn. `kill_counter_text_matches` cannot tell a left-over screen, because it shows the same N. The `dead` screenshot is now taken after `death_screen`.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.DeathScreen.ShownOnDeathRemovedOnRespawn` — AC-086.1
- `Mordecai.Recovery.DeathScreen.BuildsDefaultContent` — AC-086.2
- `Mordecai.Recovery.DeathScreen.CountdownHoldsWhilePaused` — AC-062.1 (found by the issue #7 review)
- `Mordecai.Recovery.DeathScreen.KeepsTheViewportCaptureMode` — AC-062.3 (found by the issue #7 review)
- `Mordecai.Recovery.KillCounter.IncrementsOnPlayerKill` — AC-086.3
- `Mordecai.Recovery.KillCounter.ResetsOnPlayerDeath` — AC-086.3
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_death_screen.py`: the controller's DeathScreenComponent exists and is bound to the PlayerState ASC; no death screen while alive, an activated and visible one while dead (read-only)
- `Scripts/verify_pie_hud.py` extended: KillCounter bound; `[LOGIC]` check that kills an enemy by applying damage directly and sees "Kills: 1" (diagnostic only)
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us054_combat.py`: `death_screen`, `death_screen_removed`, `kill_counter_text_matches`; new with issue #6: `kill_enemy_melee`, `kill_counter_increments` (offline-tested with `py Scripts/run_python_tests.py`)

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` (CombatTestMap → B_MordecaiArenaExperience); Tier 4: `py Scripts/playtest_story.py US-054` on the same package (it boots each of US-054's maps and runs `us054_combat.py` on CombatTestMap)
- **Player steps:** walk (WASD) to the dummy melee spot (440, 600) beside the TestDummy (600, 600), click LMB repeatedly until it dies; walk via (440, 0) to the fight spot (2000, 0) beside the TestAttacker and stand until it kills the player; wait for respawn
- **Visible on screen:** "Kills: 1" top-right after the kill; a death screen with "You Died", a countdown digit and "Kills: 1" in the `dead` screenshot; HUD back without the death screen in the `respawned` screenshot
- **Proof:** Tier 4 checks `kill_enemy_melee`, `kill_counter_increments`, `death_screen`, `death_screen_removed`, `kill_counter_text_matches` of `Scripts/playtests/us054_combat.py`
- **Tier 4 script:** `Scripts/playtests/us054_combat.py` — run: `py Scripts/playtest_story.py US-086` (or `US-054`)
- **Steps:** (generated: `py Scripts/playtest_story.py US-086 --plan`; checked: `--lint`). US-086's checks are `death_screen` (AC-086.1, AC-086.4) and `death_screen_removed` (AC-086.1), added with the issue #7 fix, and `kill_counter_text_matches`. The kill checks `kill_enemy_melee` and `kill_counter_increments` (AC-086.5) come with the issue #6 (N1) half, between `dodge_lalt` and `walk_to_attacker`. Before the fix, the 8ea853b sweep (us054 on DevTestMap, before US-100 split it) had `death_screen` SKIP (never died) and `kill_counter_text_matches` PASS (only 'Kills: 0').

<!-- Scripts/playtests/us054_combat.py -->

| # | Check | Player step (bridge calls) → expected | Proves |
|---|---|---|---|
| 1 | `combat_targets_present` | state player, actors (radius 3000) -> the TestDummy and the TestAttacker (by label, else class name) alive, health 200/200, within 100 cm of (600, 600) and (2100, 0), ai_state 'Idle'; the TestAttacker >= 900 cm from the player | - |
| 2 | `walk_to_dummy` | walk_to(440, 600) -> within 25 cm (the melee spot, 160 cm -X of the TestDummy) | - |
| 3 | `melee_lmb` | click Left at (640, 360) -> within 1s 'Mordecai.State.Attacking' in player.tags or the TestDummy's health below its value before the click | AC-054.6, AC-054.5 |
| 4 | `dodge_lalt` | tap LeftAlt -> within 0.5s a Dodging tag in player.tags or a displacement > 150 cm | AC-054.6, AC-054.5 |
| 5 | `walk_to_attacker` | walk_to(440, 0), walk_to(2000, 0) -> within 60 cm, then within 25 cm (the fight spot, 100 cm -X of the TestAttacker) | - |
| 6 | `attacker_damages_player` | stand at the fight spot -> player.attributes.health < player.attributes.max_health within 20s | - |
| 7 | `restoration_heals` | tap One -> within 6s health rises, spell_points drops by 3 and 'Mordecai.Status.Restoration' is in player.tags | US-057:AC-057.4, US-057:AC-057.5, US-057:AC-057.12 |
| 8 | `burning_on_hit` | keep standing -> within 25s 'Mordecai.Status.Burning' in player.tags and a widget text 'Burning' (WARN if Burning never procs: the TestAttacker swings BasicSlash; ends at once, WARN, when the player dies) | US-057:AC-057.11, US-057:AC-057.8 |
| 9 | `player_dies` | keep standing -> player.is_dead==true within 60s (or already seen dead by an earlier step), 'Mordecai.State.Dead' in player.tags | US-053:AC-053.1 |
| 10 | `death_screen` | state widgets -> within 1.5s one widget with 'You Died', a countdown digit and 'Kills: N' | US-062:AC-062.1, US-062:AC-062.2, US-086:AC-086.1, US-086:AC-086.4 |
| 11 | `dead_no_move` | while dead: hold W 0.5s -> moved < 5 cm | US-053:AC-053.2 |
| 12 | `respawn` | wait -> within 6s player.is_dead==false, health == max_health, within 150 cm of (0, 0), no 'Mordecai.State.Dead' tag | US-053:AC-053.4, US-053:AC-053.5 |
| 13 | `death_screen_removed` | state widgets -> within 1.5s of the respawn no widget text 'You Died' (needs the death screen seen) | US-086:AC-086.1 |
| 14 | `arena_reset` | the first read that shows the respawned player (state actors) -> the TestDummy and the TestAttacker alive, health == max, within 100 cm of their spawns, ai_state 'Idle' in that read; PASS only when that read shows the reset (the TestDummy below max before the death and full at the respawn, or the TestAttacker out of Idle before the death and Idle at the respawn), else WARN | US-053:AC-053.8, US-053:AC-053.9 |
| 15 | `respawn_moves` | hold W 0.4s -> player x grows by >= 80 cm | US-053:AC-053.6 |
| 16 | `kill_counter_text_matches` | state widgets, player -> the 'Kills: N' text equals player.kills | US-063:AC-063.1 |

- **Playable alone:** YES (after US-083, US-084, US-085)

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: none expected; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST (code + us054_combat script edits), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` exits 0 — never `--allow-stale`
- [ ] `py Scripts/playtest_story.py US-086` (Tier 3 + Tier 4) on that same fresh package: `death_screen`, `death_screen_removed`, `kill_enemy_melee`, `kill_counter_increments`, `kill_counter_text_matches` PASS and the summary's Acceptance criteria table shows AC-086.1, AC-086.4 and AC-086.5 accepted; no check that PASSed in the sweep now FAILs; `py Scripts/playtest_story.py US-086 --lint` and `US-054 --lint` exit 0 — never `--allow-stale`
- [ ] Known issues N1 and N2 marked FIXED in `Specs/agent_playtest_plan_v1.md` with the run dir
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen vs. "Visible on screen"
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-086]` closing commit
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes

### Issue #7 half: the death screen (N2), 2026-10-30
**Status: AC-086.1, AC-086.2 and AC-086.4 done.** AC-086.3 and AC-086.5 (the kill counter, N1, issue #6) are still open, so the story stays in progress. As built: see "Death screen as built" in the Technical Notes. Delivered commit `8486aa1` (code `892cb1d`) on `fix/issue-7-death-screen`.

**Tier 1.** `py Scripts/run_tests.py`: 739/739 (`Saved/TestLogs/tests-20261030-112526.log`).
- `Mordecai.Recovery.DeathScreen.ShownOnDeathRemovedOnRespawn`, `BuildsDefaultContent`, `CountdownHoldsWhilePaused` and `KeepsTheViewportCaptureMode` were each red before their fix.
- The `LyraGame` game target builds too, so the tests compile there.

**Tier 2.** `py Scripts/run_pie_verify.py`: 14/14. The new `verify_pie_death_screen.py` passes: the component is on the controller and bound in `OnPossess`; the player was alive, and no death screen was showing.

**Content.** None. `check_content_committed.py` exits 0. Python tooling: 515/515. `--lint` passes for US-086 and US-054.

**Package.** `py Scripts/package_playtest.py` exit 0 for `8486aa1`: dirty=False, no source change during the build (`Saved/Playtests/package-20261030-120418.log`).

**Tier 3 + Tier 4.** `py Scripts/playtest_story.py US-086` exit 0 (`Saved/Playtests/20261030-122547-story-us086/summary.md`).
- **Tier 3**, CombatTestMap: `Saved/Playtests/20261030-122547-boot`, exit 0, every check PASS. B_MordecaiArenaExperience identified and loaded, no crash, `content_cooked` PASS, 3/3 engine screenshots, 0 errors.
  - `engine_01`–`03` (and `shot_01`–`03`): the overhead view of the grey grid floor. The player is at the spawn, with the pale-blue TestDummy under its red health bar up and to the right. The Mordecai HUD is top-left with SP 10/10, "Kills: 0" is top-right, and the "[I] Inventory" prompt is at the bottom. No death screen, since the player is alive. The yellow Steam config messages are issue #29.
- **Tier 4**, `us054_combat.py`: `Saved/Playtests/20261030-122609-us054combat`, exit 0 (the first exit 0 of this script). 27 PASS, 1 WARN, 0 FAIL.
  - `death_screen` PASS: one `MordecaiDeathScreenWidget` with 'You Died', '3' and 'Kills: 0' on the first read after the death.
  - `death_screen_removed` PASS: no 'You Died' after the respawn.
  - `dead_no_move`, `respawn`, `arena_reset`, `respawn_moves` and `kill_counter_text_matches` PASS.
  - `burning_on_hit` WARN: BasicSlash applies no Burning, as known.
  - Accepted: US-086 AC-086.1 and AC-086.4, and US-062 AC-062.1 and AC-062.2, plus the US-053, US-054, US-057 and US-063 ACs listed in the report.
  - `bridge_01` (start): the player at the spawn, the TestDummy up and to the right, the HUD.
  - `bridge_02` (near_dummy): the player just -X of the TestDummy.
  - `bridge_03` (hit): the player against the yellow TestAttacker, health bar at about 85%.
  - `bridge_04` (dead): **the death screen**. The arena is dimmed, with a red "You Died", a white "3" and "Kills: 0" in the middle over the dead player and the TestAttacker. The health bar is empty and the HUD is still visible.
  - `bridge_05` (respawned): a motion-blurred frame as the camera returns to the spawn. Health is full, the TestAttacker's bar is at the top, and there is **no death screen and no dimming**.
  - Against "Visible on screen": the `dead` and `respawned` frames match. The "Kills: 1" parts wait for issue #6.

**Regression sweep.** `py Scripts/playtest_story.py --all` on the same package: `Saved/Playtests/20261030-122726-sweep`.
- us054_combat: 27/0, exit 0 (`20261030-122858-us054combat`). Its `bridge_04` shows the same death screen, and its `bridge_05` none.
- Every other script has the same PASS/FAIL counts and the same failing checks as the 2026-10-29 sweep (`20261029-175504-sweep`): us001 27/4, us054_arena 16/2, us057 21/6, us069 26/5, us078 19/2, us079 56/0, us083 28/0. No check went PASS → FAIL. Only the two us054_combat frames above were opened from this sweep.
- Seen in passing, not this issue's: in the sweep run's `bridge_05` the TestDummy's floating health bar still shows about 92% (its 184/200 before the death), while the state read at the respawn reports 200/200. The enemy health bar may not redraw after `ResetForArena`.
