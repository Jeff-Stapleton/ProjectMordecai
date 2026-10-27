# US-085: Recovery — Frontliner Content: Visible Mesh, 200 HP, Attack Profiles, Placement

**Epic:** Epic R: Playability Recovery
**Priority:** P0
**Depends on:** US-084
**Paired story:** US-084 (HEADLESS) — enemy attack path + content guards

## Overview
The packaged arena's three `BP_MordecaiEnemy_Frontliner` enemies are invisible (no mesh: only their floating red health bars show), have 100 MaxHealth instead of 200 (N13), never damage the player (N7) and, being invisible, body-block the player at the spawn (N14). This story fixes the Frontliner Blueprint and its placement as saved, committed content, so the player sees three enemies, fights 200-HP Frontliners, takes hits, and can die, respawn and see the arena reset. Features: enemies, enemy_ai, death_respawn, movement (N14).

## Execution Mode
**EDITOR**
Blueprint defaults (mesh, stats, `EnemyAttackProfiles`), attack-profile DataAssets and actor placement in DevTestMap are editor assets created and saved via Python remote execution.

## References
- `Specs/agent_playtest_plan_v1.md` — "The three red bars are explained" (no mesh), N7 confirmed, N13, N14
- Sweep `Saved/Playtests/20261022-184702-sweep/summary.md`; runs `Saved/Playtests/20261022-184803-us054` (`enemies_present` FAIL NEW: "health 100.0/100.0 (expected 200/200) ... mesh=False"; `enemies_visible` FAIL known; `enemy_damages_player` FAIL NEW) and `Saved/Playtests/20261022-184725-us001` (`move_a` FAIL NEW: "93 cm along -Y (need > 100)"; `walk_lane_wp1` FAIL NEW: "walk_to timed out after 6.8s at (-78, -45)")
- Exploratory run `Saved/Playtests/20261022-185207-explore/report.md` re-found N7 and N14 independently
- `stories/done/US-054-playable-arena-integration.md` AC-054.2: MaxHealth=200, MaxPosture=100, MoveSpeed=400, AggroRange=900, AttackRange=200, LeashRange=1800
- `stories/done/US-057-playable-magic-arena.md` AC-057.7/AC-057.8: `DA_EnemyAttack_FireSlash` (Burning 40%) on one of the three Frontliners, `DA_EnemyAttack_BasicSlash` on the others
- `Specs/enemy_archetypes_v1.md` (Frontliner), `Specs/combat_system_v1.md`
- Existing assets: `/MordecaiCore/Enemies/BP_MordecaiEnemy_Frontliner`, `/MordecaiCore/Combat/Enemies/DA_EnemyAttack_BasicSlash`, `/MordecaiCore/Combat/Enemies/DA_EnemyAttack_FireSlash`, `/MordecaiCore/Game/Maps/DevTestMap`
- Existing scripts: `Scripts/us054_02_create_enemy_bp.py:40` (sets `default_max_health` 200), `Scripts/us057_03_create_fire_slash.py:103` (sets `enemy_attack_profiles`), `Scripts/us054_10_configure_all.py:20-52` (sets `default_abilities`, saves) — the cooked BP shows neither the 200 HP nor working profiles, so those edits were not saved (or the level instances override them)

## Acceptance Criteria
- [ ] AC-085.1: In the packaged build, the player sees three Frontliner bodies (a mesh under each red health bar) — verified by check `enemies_visible` of `Scripts/playtests/us054_arena.py` (Tier 4) and the us054 `start` screenshot
- [ ] AC-085.2: The three Frontliners report 200/200 health (AC-054.2 stats: MaxHealth 200, MaxPosture 100, MoveSpeed 400, AggroRange 900, AttackRange 200, LeashRange 1800) — verified by check `enemies_present` of `Scripts/playtests/us054_arena.py` (Tier 4)
- [ ] AC-085.3: Standing next to a Frontliner, the player takes damage within 20 s — verified by check `enemy_damages_player` of `Scripts/playtests/us054_arena.py` (Tier 4); the chain that was SKIPped behind it now runs: `player_dies`, `dead_no_move`, `respawn`, `arena_reset`, `respawn_moves` (US-053 ACs) and `burning_on_hit` (US-057 AC-057.8/057.11). Each must PASS, or its FAIL is filed as a new Epic R story / Known issues entry before this story closes (`death_screen` is US-086)
- [ ] AC-085.4: The player is no longer stopped by invisible bodies at the spawn: checks `move_a` and `walk_lane_wp1` of `Scripts/playtests/us001_movement.py` (Tier 4) PASS (see the placement TODO(DECISION))
- [ ] AC-085.5: No `LogMordecai` enemy-content error (US-084 AC-084.3) in the packaged `game.log`
- [ ] AC-085.6: every asset is saved and committed; `py Scripts/check_content_committed.py` passes

## Technical Notes
- One script, `Scripts/us085_configure_frontliner.py`, that sets AND saves: the BP's mesh component (a skeletal mesh already in the project — e.g. the Lyra mannequin the player uses; look up its real path, do not assume one — or an engine basic shape — anything cooked and visible from the diorama camera), `default_max_health`/posture/speed/ranges per AC-054.2, `enemy_attack_profiles = [DA_EnemyAttack_BasicSlash]`; then the DevTestMap instance overrides (one Frontliner → `DA_EnemyAttack_FireSlash` per AC-057.8); `EditorAssetLibrary.save_asset` on the BP and `save_current_level()`.
- Check that the level instances do not override the BP defaults with old values (N13 may be an instance override rather than the CDO).
- TODO(DECISION) (Jeff): AC-054.2's `AttackRange` 200 is longer than the slash's 150 cm reach (AC-054.3, `DA_EnemyAttack_BasicSlash` / `FireSlash` arc radius). Since issue #12 the AI closes to `min(AttackRange, reach)`, so enemies attack from ≤ 150 cm and hit; whether the content should align the two numbers (raise the reach or lower AttackRange) is a design call, not decided here.
- Since issue #12 (branch `fix/issue-12-enemy-damage`), enemies reliably hit and kill a player standing at the spawn, and the player dies and respawns (N31 fixed). The `arena_reset` check also requires the AC-054.2 200 max health (N13): on `48f60b2` the first read after the respawn had all three enemies back at their spawns at 100/100, and they re-aggroed onto the respawned player within ~1.5 s because every spawn is inside AggroRange of PlayerStart (the placement TODO(DECISION) above).
- Collision: the capsule stays Pawn-blocking (enemies should be solid once visible). N14 "invisible wall" is resolved by visibility; the remaining question is the swarm at the spawn.
- TODO(DECISION) (Jeff): the Frontliners are placed within AggroRange (900 cm) of PlayerStart (e.g. (-400, -500) is ~640 cm away), so they swarm the spawn on frame one and `us001_movement.py`'s movement lanes run inside a melee. Options: (a) place them > 900 cm from PlayerStart so the player starts out of aggro; (b) keep the layout and move us001's movement checks to a lane away from the enemies. Do not choose; until decided, record `move_a` / `walk_lane_wp1` results honestly. Option (a) changes waypoints that `us054_arena.py` (`walk_to_frontliner3` targets (-400, -500)) and `us057_magic.py` (`walk_shot_line`) use — update them in the same commit.
- Enemies that now hit the player affect other scripts (us057, us069, us079 run on DevTestMap with the enemies awake): rerun all six scripts (`py Scripts/playtest_story.py --all`) and report any new FAIL caused by being attacked (e.g. dying mid-run).
- Script bookkeeping (commit before packaging): remove the `known_issue` from `enemies_visible` in `us054_arena.py`; `enemies_present`, `enemy_damages_player`, `move_a`, `walk_lane_wp1` were NEW (no `known_issue` to remove).
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- No new logic; ALL `Mordecai.*` tests still pass (US-084's tests cover the code)
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_enemies.py` (new or extended): three Frontliners in DevTestMap, each with a visible mesh component, MaxHealth 200, non-empty `EnemyAttackProfiles`, one with FireSlash
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- Existing checks: `enemies_present`, `enemies_visible`, `enemy_damages_player` and the death/respawn chain in `Scripts/playtests/us054_arena.py`; `move_a`, `walk_lane_wp1` in `Scripts/playtests/us001_movement.py`

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py` (DevTestMap → B_MordecaiArenaExperience); Tier 4: `py Scripts/playtest_story.py US-054` and `py Scripts/playtest_story.py US-001` on the same package
- **Player steps:** stand at the spawn (enemies aggro and approach); walk next to Frontliner_3 (WASD); stand until hit, keep standing until dead; wait for respawn; hold W
- **Visible on screen:** Tier 3 `engine_NN.png`: three enemy bodies under the three red health bars in the diorama arena. Tier 4: `bridge_01.png` (us054 start) three visible Frontliners; the hit / dead / respawned screenshots of us054
- **Proof:** Tier 3 boot screenshots for the visible bodies; Tier 4 checks `enemies_present`, `enemies_visible`, `enemy_damages_player`, `player_dies`, `dead_no_move`, `respawn`, `arena_reset`, `respawn_moves`, `burning_on_hit` (us054), `move_a`, `walk_lane_wp1` (us001)
- **Tier 4 script:** `Scripts/playtests/us054_arena.py` — run: `py Scripts/playtest_story.py US-054`; `Scripts/playtests/us001_movement.py` — run: `py Scripts/playtest_story.py US-001`
- **Steps:** (owning stories' rows; `py Scripts/playtest_story.py US-054 --plan`)

| Script | Check | Player step (bridge calls) → expected | Sweep result (8ea853b) |
|---|---|---|---|
| us054 | `enemies_present` | state actors (radius 3000) -> 3 kind=='enemy', health {200, 200}, pairwise > 300 cm apart | FAIL (NEW): 100/100 |
| us054 | `enemies_visible` | state actors -> every enemy has_visible_mesh==true | FAIL (known): 0/3 |
| us054 | `enemy_damages_player` | stand next to the enemy -> wait_until player.attributes.health < player.attributes.max_health within 20s | FAIL (NEW) |
| us054 | `player_dies` … `respawn_moves`, `burning_on_hit` | see US-054 Steps rows 13-20 | SKIP (never hit) |
| us001 | `move_a` | hold A 0.4s -> player.location moves > 100 cm along -Y, < 30 cm across | FAIL (NEW): 93 cm |
| us001 | `walk_lane_wp1` | walk_to(0, -650) -> within 60 cm | FAIL (NEW): stuck at (-78, -45) |

- **Playable alone:** YES (with US-084 landed first)

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: the BP, any new DataAsset and DevTestMap are saved and committed in this branch; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST (content + script edits), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py` exits 0 — never `--allow-stale`
- [ ] On that same fresh package: `py Scripts/playtest_story.py US-054` and `py Scripts/playtest_story.py US-001`: `enemies_present`, `enemies_visible`, `enemy_damages_player`, `move_a`, `walk_lane_wp1` go from FAIL (sweep `Saved/Playtests/20261022-184702-sweep`) to PASS; the death/respawn chain runs and each result is recorded (new FAILs filed); no check that PASSed in the sweep now FAILs; `--lint` exits 0 for both — never `--allow-stale`
- [ ] Known issues "three red bars", N7, N13, N14 marked FIXED in `Specs/agent_playtest_plan_v1.md` with the run dirs
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen vs. "Visible on screen"
- [ ] Evidence cited: Tier 3 and Tier 4 run dirs in Completion Notes AND in the body of the `[US-085]` closing commit
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass. A failing playtest is reported honestly and the story stays in progress

## Completion Notes
<Filled in by the coding agent.>
