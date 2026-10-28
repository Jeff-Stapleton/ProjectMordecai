# US-084: Recovery — Enemy Attacks Reach the Player (code) + Enemy Content Guards

**Epic:** Epic R: Playability Recovery
**Priority:** P0
**Depends on:** none
**Paired story:** US-085 (EDITOR) — Frontliner content: mesh, 200 HP, attack profiles

## Overview
In the packaged arena the three Frontliners swarm the player, cycle `ai_state` Attack/Recover, and never hurt the player (N7 confirmed: 100/100 health for 20 s). The death, death-screen, respawn and arena-reset loop can therefore never run. This story makes the enemy attack path correct in code and makes missing enemy content impossible to miss: an enemy that begins play without attack profiles or without a visible mesh logs a `LogMordecai` error, which every playtest's `log_errors` reports. Feature: enemies, enemy_ai, death_respawn. The content half is US-085.

## Execution Mode
**HEADLESS**
C++ only: the enemy melee activation/hit path and BeginPlay content validation; every AC is verifiable with compile + headless tests. The Blueprint values live in US-085.

## References
- `Specs/agent_playtest_plan_v1.md` — N7 (suspected, then "N7 confirmed" by the first Phase 3 sweep), N13, N14
- Sweep `Saved/Playtests/20261022-184702-sweep/summary.md`; run `Saved/Playtests/20261022-184803-us054` (check `enemy_damages_player` FAIL NEW: "player.attributes.health<player.attributes.max_health not true within 20s"; `enemy_aggro` and `enemy_approaches` PASS — the AI runs, the swing never lands)
- `Specs/enemy_ai_framework_v1.md`, `Specs/enemy_archetypes_v1.md` (Frontliner)
- Done stories: US-050 (damage reception), US-051 AC-051.5 (enemy attacks damage the player), US-054 AC-054.13
- Existing code:
  - `Source/LyraGame/Mordecai/Enemy/MordecaiEnemyCharacter.cpp:376-387` — melee ability instance gets `EnemyAttackProfiles` ("set in Blueprint; otherwise no profiles")
  - `Source/LyraGame/Mordecai/Combat/MordecaiGA_MeleeAttack.cpp:292-296` — empty profiles → the attack does nothing (suspected root of N7)
  - `Source/LyraGame/Mordecai/Enemy/MordecaiEnemyCharacter.h:42` — `DefaultMaxHealth = 100.f` (C++ default the cooked enemies report, N13)
  - `Scripts/us057_03_create_fire_slash.py:103` sets `enemy_attack_profiles` and `Scripts/us054_02_create_enemy_bp.py:40` sets `default_max_health = 200` on the BP CDO; the cooked enemies show neither (unsaved, like N10 — see US-085). _Corrected by issue #18: the edits were saved. The cooked Frontliners have their profiles (N7 had another cause), and the 100 HP came from DevTestMap overriding `DefaultMaxHealth` with 100 on the placed actors; see N13 in `Specs/agent_playtest_plan_v1.md`._

## Acceptance Criteria
- [x] AC-084.1: An enemy with one attack profile, in its Attack state within AttackRange of a player pawn whose ASC lives on the PlayerState (Lyra pattern), applies health damage to that player — verified by `Mordecai.Recovery.Enemy.AttackDamagesPlayerWithPlayerStateASC`
- [x] AC-084.2: The enemy melee hit query targets the player (hostile filter, airborne rules, friendly fire off) and does not hit other enemies — verified by `Mordecai.Recovery.Enemy.AttackTargetsPlayerNotAllies`
- [x] AC-084.3: An enemy that begins play with an empty `EnemyAttackProfiles` logs `LogMordecai` Error "enemy <name> has no attack profiles"; an enemy with no visible mesh (no skeletal/static mesh asset, or hidden in game) logs `LogMordecai` Error "enemy <name> has no visible mesh" — verified by `Mordecai.Recovery.Enemy.LogsMissingContent`
- [x] AC-084.4: The enemy AI does not enter Attack → Recover cycles when its melee ability cannot activate (no profiles): it logs once and stays in Chase/Idle — verified by `Mordecai.Recovery.Enemy.NoSilentAttackLoop`

## Technical Notes
- Start by reproducing N7 in a Tier 1 test with a stub profile: spawn a player pawn + PlayerState ASC and an enemy with `EnemyAttackProfiles = {stub}`, drive the AI to Attack, tick, assert the player's Health dropped. If it passes with a profile, N7's root is content only (US-085) and AC-084.1/084.2 lock the behaviour in; if not, fix the hit/filter path here.
- Content guards use a new log category `LogMordecaiEnemy` (or `LogMordecai`) at Error level so `game.log` error counts and `log_errors` in every playtest surface missing content. Do NOT change `DefaultMaxHealth`'s C++ default to paper over N13: the Frontliner's 200 belongs in its Blueprint (US-085).
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.
- Script bookkeeping: none here (the checks are proven after US-085; `enemy_damages_player` carries no `known_issue`).

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.Enemy.AttackDamagesPlayerWithPlayerStateASC` — AC-084.1
- `Mordecai.Recovery.Enemy.AttackTargetsPlayerNotAllies` — AC-084.2
- `Mordecai.Recovery.Enemy.LogsMissingContent` — AC-084.3
- `Mordecai.Recovery.Enemy.NoSilentAttackLoop` — AC-084.4
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_spawn.py` and the existing enemy smoke checks still pass; note any new `LogMordecai` enemy-content errors in PIE (expected until US-085 lands)
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- none of its own (no input-driven AC). The us054 checks it unblocks are proven by US-085

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py` (DevTestMap → B_MordecaiArenaExperience)
- **Player steps:** none (boot only)
- **Visible on screen:** the arena, HUD and the three enemy health bars (enemies still invisible until US-085); the report's top errors now list the new enemy-content errors for the Frontliners if their BP lacks profiles/mesh — that is the expected, honest state until US-085
- **Proof:** Tier 3 `playtest_boot.py` exits 0 (the new content errors are expected and named in Completion Notes, not a failure of this story). Optional diagnostic: `py Scripts/playtest_story.py US-054` — record `enemy_damages_player`'s result
- **Tier 4 script:** none (no input-driven AC); the checks are owned by US-085 via `py Scripts/playtest_story.py US-054`
- **Steps:** none
- **Playable alone:** NO — playable after US-085 (EDITOR) lands

## Definition of Done
- [x] All ACs checked off
- [x] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [x] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [x] Content: no asset changes; `py Scripts/check_content_committed.py` exits 0
- [x] Work committed FIRST, then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [x] `py Scripts/playtest_boot.py` exits 0 — never `--allow-stale` (not playable alone; it proves the build still boots to B_MordecaiArenaExperience)
- [x] Every screenshot (`engine_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen and list the enemy-content errors the log now shows
- [x] Evidence cited: Tier 3 run dir in Completion Notes AND in the body of the `[US-084]` closing commit; planner log tracks the feature as `PENDING PAIR (waiting on US-085)`
- [x] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
Worked as GitHub issue #12 (N7) on branch `fix/issue-12-enemy-damage`. Delivered commit `c7e4b8d` (code commits `6c858f5`, `1fca56e`, `48f60b2`, `c7e4b8d`).

**Root cause of N7 (not the attack profiles: the cooked Frontliners have them).**
1. `UMordecaiHitDetectionSubsystem::AreActorsFriendly` compared Lyra team ids. The Mordecai experiences create no Lyra teams, so the player and every enemy are `FGenericTeamId::NoTeam`: "friends". Every enemy hit query filtered out the player, and every player hit query filtered out the enemies. Fix: the `Mordecai.Team.Enemy` tag decides whenever an enemy is involved; two untagged actors (co-op players) still compare ids, so friendly fire stays off (AC-003.9).
2. Found on the `48f60b2` package: the Frontliners' slash reaches 150 cm (`DA_EnemyAttack_BasicSlash`/`FireSlash` arc radius) but the AI rooted into Attack at `AttackRange` 200, so an enemy that stopped between 150 and 200 cm swung at air forever (exploratory `Saved/Playtests/20261027-080939-mcp`: 60 s at 100/100 with all three in Attack at ~195 cm; hit within 1.6 s after walking to 80 cm). Fix: `AMordecaiEnemyAIController::GetEffectiveAttackRange` = `min(AttackRange, UMordecaiGA_MeleeAttack::GetAttackReach())`. Whether the content should align AC-054.2/054.3 is a TODO(DECISION) in US-085.

**Also fixed here (filed as N31):** once hits landed, the player sat at 0 Health and never died: `AMordecaiCharacter` bound Health→death in `BeginPlay`/`PossessedBy`, before Lyra attaches the PlayerState ASC to the local pawn. It now binds from `OnAbilitySystemInitialized`. Tests `Mordecai.Recovery.Player.*` (3).

**Tests.** Tier 1: `Mordecai.Recovery.Enemy.*` (8: the four AC tests plus `ClosesToWeaponReachBeforeAttacking`, `AttackReachFollowsHitShape`, `PlayerSweepsSpareOtherPlayers`) and `Mordecai.Recovery.Player.*` (3). The AC-084.1/084.2 tests, the reach test and the death tests were run red before their fixes. All 702 `Mordecai.*` tests pass (`py Scripts/run_tests.py`). Tier 2: `py Scripts/run_pie_verify.py` 11/11 PASS (PIE logged "no visible mesh" for the three Frontliners, and no "no attack profiles"). Content: none; `check_content_committed.py` PASS. A read-only adversarial review (3 lenses, each finding verified) found two real issues in the first commit, both fixed in `1fca56e`: the "NoTeam is never friendly" clause would have turned co-op friendly fire on, and `LogsMissingContent` did not run BeginPlay.

**Tier 3 (fresh package of `c7e4b8d`, clean tree): `Saved/Playtests/20261027-085120-boot` — PASS (exit 0).** build_fresh, launch, experience_identified/loaded (B_MordecaiArenaExperience, 7.2 s), no_crash, content_cooked, game_window, engine_screenshots 3/3 PASS; screenshots WARN (desktop captures covered); log_errors INFO: 3 errors, all `LogMordecaiEnemy: Error: enemy BP_MordecaiEnemy_Frontliner_C_0/1/2 has no visible mesh` (AC-084.3 working; expected until US-085).
- `engine_01.png`: the diorama arena with the Mordecai HUD (bars top-left, "Kills: 0"), the player capsule at the centre, three red enemy health bars about 1.5 m around it (no enemy bodies, US-085), and the player's health bar a thin green sliver: the standing player is being hit.
- `engine_02.png`, `engine_03.png`: the same frame with the health bar fully grey: the enemies killed the player during the boot. No death screen (N2, US-086).
- `shot_01..03.png`: the Windows lock screen (the workstation was locked); not evidence.

**Diagnostic: the issue's named check (US-054's us054_arena.py, same package).** `Saved/Playtests/20261027-085212-us054` (sweep `Saved/Playtests/20261027-085120-sweep`): `enemy_damages_player` PASS (US-051 AC-051.5; FAIL in the 8ea853b sweep), `player_dies` PASS (AC-053.1), `respawn` PASS (AC-053.4/053.5). The script still exits 1 on known content and UI issues (enemies_present N13, enemies_visible, enemy_indicators N17, death_screen N2) and on the checks that now run with a hostile arena. `bridge_03.png`/`bridge_04.png`: the player in the south-west of the arena with three red bars beside it, health bar empty, no "You Died" (N2). `bridge_05.png`: the player back at the centre with a full health bar, the three red bars already around it again (they reset to their spawns and re-aggroed: the spawns are inside AggroRange of PlayerStart, US-085's placement TODO(DECISION)).

**Effect on the other playtests (full sweep on `c7e4b8d`).** Every DevTestMap script now runs in an arena that hits and kills a player standing near the spawn. A read-only triage of each NEW failure against the `01abb8b` sweep (`Saved/Playtests/20261023-192402-sweep`) found no branch regression. 14 checks fail because the player was being hit or was dead at the time (us001 move_s, walk_speed_baseline, walk_lane_reset, walk_lane_reset2, jump_space; us054 walk_to_frontliner3, melee_lmb, dodge_lalt, arena_reset, respawn_moves; us057 walk_blink_lane; us078 light_attack_lmb; us083 block_over_held_rt_no_parry, light_attack_rt_tap). 13 fail exactly as in the baseline but carry no `known_issue` (N13, N15, N16, N17, N18, the CommonUI inventory pool bug / US-094). Recorded in `Specs/agent_playtest_plan_v1.md`; the remedy is the enemy-free test maps (T1).

Playable status: enemies — NOT PLAYABLE alone, `PENDING PAIR (waiting on US-085)`: enemies now hit and kill the player, but they are invisible (no mesh) and have 100 instead of 200 HP.
