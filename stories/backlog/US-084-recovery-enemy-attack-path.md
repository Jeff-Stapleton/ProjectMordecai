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
  - `Source/LyraGame/Mordecai/Combat/MordecaiGA_MeleeAttack.cpp:280-284` — empty profiles → the attack does nothing (suspected root of N7)
  - `Source/LyraGame/Mordecai/Enemy/MordecaiEnemyCharacter.h:42` — `DefaultMaxHealth = 100.f` (C++ default the cooked enemies report, N13)
  - `Scripts/us057_03_create_fire_slash.py:103` sets `enemy_attack_profiles` and `Scripts/us054_02_create_enemy_bp.py:40` sets `default_max_health = 200` on the BP CDO; the cooked enemies show neither (unsaved, like N10 — see US-085)

## Acceptance Criteria
- [ ] AC-084.1: An enemy with one attack profile, in its Attack state within AttackRange of a player pawn whose ASC lives on the PlayerState (Lyra pattern), applies health damage to that player — verified by `Mordecai.Recovery.Enemy.AttackDamagesPlayerWithPlayerStateASC`
- [ ] AC-084.2: The enemy melee hit query targets the player (hostile filter, airborne rules, friendly fire off) and does not hit other enemies — verified by `Mordecai.Recovery.Enemy.AttackTargetsPlayerNotAllies`
- [ ] AC-084.3: An enemy that begins play with an empty `EnemyAttackProfiles` logs `LogMordecai` Error "enemy <name> has no attack profiles"; an enemy with no visible mesh (no skeletal/static mesh asset, or hidden in game) logs `LogMordecai` Error "enemy <name> has no visible mesh" — verified by `Mordecai.Recovery.Enemy.LogsMissingContent`
- [ ] AC-084.4: The enemy AI does not enter Attack → Recover cycles when its melee ability cannot activate (no profiles): it logs once and stays in Chase/Idle — verified by `Mordecai.Recovery.Enemy.NoSilentAttackLoop`

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
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: no asset changes; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py` exits 0 — never `--allow-stale` (not playable alone; it proves the build still boots to B_MordecaiArenaExperience)
- [ ] Every screenshot (`engine_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen and list the enemy-content errors the log now shows
- [ ] Evidence cited: Tier 3 run dir in Completion Notes AND in the body of the `[US-084]` closing commit; planner log tracks the feature as `PENDING PAIR (waiting on US-085)`
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
<Filled in by the coding agent.>
