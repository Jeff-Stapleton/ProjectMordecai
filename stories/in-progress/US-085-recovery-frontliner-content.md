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
  - Issue #18 (2026-10-27): it was the level instances. The saved BP has `DefaultMaxHealth` 200 and `EnemyAttackProfiles` [BasicSlash, FireSlash]; DevTestMap's three Frontliners overrode only `DefaultMaxHealth`, with 100. Fixed by `Scripts/us085_configure_frontliner.py` (see Completion Notes).

## Acceptance Criteria
- [x] AC-085.1: In the packaged build, the player sees three Frontliner bodies (a mesh under each red health bar) — verified by check `enemies_visible` of `Scripts/playtests/us054_arena.py` (Tier 4) and the us054 `start` screenshot
  - Issue #19, fresh package of `f21525f`. This AC needs no input, so it is accepted at Tier 3: boot `Saved/Playtests/20261028-085310-boot` exit 0, and its `engine_01..03.png` show three red bodies under the three bars.
  - `enemies_visible` PASS (3/3 `mesh=True`) and the us054 `start` screenshot (`bridge_01.png`) show the same. That run exits 1 on other checks (the spawn swarm, N2, N17), so the kit accepts none of its ACs.
- [ ] AC-085.2: The three Frontliners report 200/200 health (AC-054.2 stats: MaxHealth 200, MaxPosture 100, MoveSpeed 400, AggroRange 900, AttackRange 200, LeashRange 1800) — verified by check `enemies_present` of `Scripts/playtests/us054_arena.py` (Tier 4)
- [ ] AC-085.3: Standing next to a Frontliner, the player takes damage within 20 s — verified by check `enemy_damages_player` of `Scripts/playtests/us054_arena.py` (Tier 4); the chain that was SKIPped behind it now runs: `player_dies`, `dead_no_move`, `respawn`, `arena_reset`, `respawn_moves` (US-053 ACs) and `burning_on_hit` (US-057 AC-057.8/057.11). Each must PASS, or its FAIL is filed as a new Epic R story / Known issues entry before this story closes (`death_screen` is US-086)
- [ ] AC-085.4: The player is no longer stopped by invisible bodies at the spawn: checks `move_a` and `walk_lane_wp1` of `Scripts/playtests/us001_movement.py` (Tier 4) PASS (see the placement TODO(DECISION))
  - Issue #19: the bodies are no longer invisible (AC-085.1). The two checks still depend on the spawn swarm. Jeff decided (2026-10-28) that the swarm is T1's (issue #46), so this AC waits on T1.
  - Results on `f21525f`:
    - story run `Saved/Playtests/20261028-085852-us001`: `move_a` PASS; `walk_lane_wp1` FAIL, the player killed at (152, -109);
    - sweep run `20261028-090829-us001`: `walk_lane_wp1` PASS; `move_a` FAIL on 37 cm drift after an enemy shove.
- [x] AC-085.5: No `LogMordecai` enemy-content error (US-084 AC-084.3) in the packaged `game.log`
  - Issue #19, `f21525f`: 0 errors in the Tier 3 `game.log` (`Saved/Playtests/20261028-085310-boot`, exit 0) and in every Tier 4 run of the sweep `Saved/Playtests/20261028-090808-sweep`. Before: three "has no visible mesh" errors.
- [ ] AC-085.6: every asset is saved and committed; `py Scripts/check_content_committed.py` passes

## Technical Notes
- One script, `Scripts/us085_configure_frontliner.py`, that sets AND saves: the BP's mesh component (a skeletal mesh already in the project — e.g. the Lyra mannequin the player uses; look up its real path, do not assume one — or an engine basic shape — anything cooked and visible from the diorama camera), `default_max_health`/posture/speed/ranges per AC-054.2, `enemy_attack_profiles = [DA_EnemyAttack_BasicSlash]`; then the DevTestMap instance overrides (one Frontliner → `DA_EnemyAttack_FireSlash` per AC-057.8); `EditorAssetLibrary.save_asset` on the BP and `save_current_level()`.
  - DONE for the mesh (issue #19). The player is not the Lyra mannequin: its body is a C++ engine-shape placeholder (`/Engine/BasicShapes/Cylinder` + Sphere, `AMordecaiCharacter`), so the Frontliner got an engine shape too.
    - `AMordecaiEnemyCharacter` has an empty, non-colliding `BodyMesh` slot.
    - The script sets it on the BP to `/Engine/BasicShapes/Cylinder` with `MI_Enemy_Frontliner` (dark red), scaled to the capsule (0.8 x 0.8 x 1.8 for radius 40, half height 90).
    - DevTestMap is unchanged: the placed Frontliners take the Blueprint's body.
- Check that the level instances do not override the BP defaults with old values (N13 may be an instance override rather than the CDO).
  - DONE for N13 (issue #18): it was an instance override (`DefaultMaxHealth` 100 on each placed Frontliner vs the BP's 200). `Scripts/us085_configure_frontliner.py` now holds the stat part of this story's script: it clears instance overrides of `DefaultMaxHealth`/`DefaultMaxPosture`/`DefaultMoveSpeed` and saves DevTestMap. The BP stats themselves already match AC-054.2 (MaxHealth 200 in the BP; MaxPosture 100, MoveSpeed 400 and the AI's 900/200/1800 ranges are native defaults). The mesh, attack-profile and placement parts are still to be added.
  - `EnemyAttackProfiles` is `EditDefaultsOnly`, so "one Frontliner → `DA_EnemyAttack_FireSlash`" cannot be an instance override (Python refuses to write an `EditDefaultsOnly` property on a placed actor). Today every Frontliner has [BasicSlash, FireSlash]. TODO(DECISION) (Jeff): a Frontliner Blueprint child with FireSlash, or make `EnemyAttackProfiles` editable on instances.
- TODO(DECISION) (Jeff): AC-054.2's `AttackRange` 200 is longer than the slash's 150 cm reach (AC-054.3, `DA_EnemyAttack_BasicSlash` / `FireSlash` arc radius). Since issue #12 the AI closes to `min(AttackRange, reach)`, so enemies attack from ≤ 150 cm and hit; whether the content should align the two numbers (raise the reach or lower AttackRange) is a design call, not decided here.
- Since issue #12 (branch `fix/issue-12-enemy-damage`), enemies reliably hit and kill a player standing at the spawn, and the player dies and respawns (N31 fixed). The `arena_reset` check also requires the AC-054.2 200 max health (N13): on `48f60b2` the first read after the respawn had all three enemies back at their spawns at 100/100, and they re-aggroed onto the respawned player within ~1.5 s because every spawn is inside AggroRange of PlayerStart (the placement TODO(DECISION) above).
- Collision: the capsule stays Pawn-blocking (enemies should be solid once visible). N14 "invisible wall" is resolved by visibility; the remaining question is the swarm at the spawn.
  - Issue #19: `BodyMesh` is NoCollision, makes no overlaps and does not affect navigation, so blocking and hits stay the capsule's. A dead enemy's collision is off (AC-050.6), so its body is hidden too; the arena reset shows it again.
- TODO(DECISION) (Jeff): corpse visuals (death animation, ragdoll, fade-out). Until then a dead enemy shows only its empty bar, as before issue #19.
- DECIDED (Jeff, 2026-10-28, issue #19): the placement stays, and the swarm at the spawn is T1's (issue #46). The original question follows.
- ~~TODO(DECISION)~~ (Jeff): the Frontliners are placed within AggroRange (900 cm) of PlayerStart (e.g. (-400, -500) is ~640 cm away), so they swarm the spawn on frame one and `us001_movement.py`'s movement lanes run inside a melee. Options: (a) place them > 900 cm from PlayerStart so the player starts out of aggro; (b) keep the layout and move us001's movement checks to a lane away from the enemies. Do not choose; until decided, record `move_a` / `walk_lane_wp1` results honestly. Option (a) changes waypoints that `us054_arena.py` (`walk_to_frontliner3` targets (-400, -500)) and `us057_magic.py` (`walk_shot_line`) use — update them in the same commit.
- Enemies that now hit the player affect other scripts (us057, us069, us079 run on DevTestMap with the enemies awake): rerun all six scripts (`py Scripts/playtest_story.py --all`) and report any new FAIL caused by being attacked (e.g. dying mid-run).
- Script bookkeeping (commit before packaging): remove the `known_issue` from `enemies_visible` in `us054_arena.py`; `enemies_present`, `enemy_damages_player`, `move_a`, `walk_lane_wp1` were NEW (no `known_issue` to remove).
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- ALL `Mordecai.*` tests still pass (US-084's tests cover the code)
- Issue #18: `Mordecai.Recovery.Enemy.LogsInstanceStatOverride` — an enemy whose base stats differ from its class's logs a `LogMordecaiEnemy` error naming the stat (the guard that makes N13 visible in every `game.log`)
- Issue #19: `Mordecai.Recovery.Enemy.BodyMeshSlot` — every enemy has an empty `BodyMesh` on its capsule that never collides, generates no overlaps and does not affect navigation; with a mesh the enemy is visible; the capsule still blocks pawns
- Issue #19: `Mordecai.Recovery.Enemy.BodyHiddenWhileDead` — a dead enemy shows no body (its collision is off); the arena reset brings both back
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_enemies.py` (new or extended): three Frontliners in DevTestMap, each with a visible mesh component, MaxHealth 200, non-empty `EnemyAttackProfiles`, one with FireSlash
  - Issue #18 added it with the stat checks: three enemies, BP `DefaultMaxHealth` 200, no instance override of the base stats, each starts at 200/200.
  - Issue #19 added the mesh checks: on the BP and on each enemy, `BodyMesh` has a mesh, is visible in game and has NoCollision. The profile checks are still to be added.
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

### 2026-10-27 — N13 / issue #18 only (branch `fix/issue-18-frontliner-max-health`, delivered `39500e2`)
Only the 200-HP part of this story is done. The mesh (AC-085.1), attack profiles, placement and N14 (AC-085.4) are not, so the story stays in progress and every AC stays unchecked.

**Root cause.** The BP was fine: `BP_MordecaiEnemy_Frontliner` has saved `DefaultMaxHealth` 200. DevTestMap saved `DefaultMaxHealth` 100 on each placed Frontliner (an instance override, there since `f7fc80d`). Editor readback before the fix: CDO 200.0; Frontliner_1/2/3 100.0.

**Fix.**
- `870c1f0`: `Scripts/us085_configure_frontliner.py` writes the CDO to the stale value and back (notify NEVER); the placed actors follow it to 200; DevTestMap saved. Readback after: all three 200.0, no dirty packages; the saved `.umap` no longer contains the name `DefaultMaxHealth`. A second run: "nothing to save". The BP is unchanged.
- `ad84cff`: `ValidateContent` logs a `LogMordecaiEnemy` error when a placed enemy's base stats differ from its class's.
- `39500e2`: N13's real cause recorded (Specs N13, tribal knowledge, US-084's note).

**Tier 1.** Project compiles. `Mordecai.Recovery.Enemy.LogsInstanceStatOverride` was red before the guard: all three `TestFalse` failed (`Saved/TestLogs/tests-20261027-174015.log`). All 703 Mordecai tests pass (`Saved/TestLogs/tests-20261027-185711.log`).

**Tier 2.**
- `Scripts/verify_pie_enemies.py`: 4 passed / 6 failed before the map fix (100/100 and `default_max_health 100.0 (class 200.0)` on each enemy), 10/10 after.
- Full `py Scripts/run_pie_verify.py`: 12/12. The first attempt failed only because PIE was not ready within the runner's 30 s on a fresh editor launch; the `--no-launch` rerun passed. `verify_pie_enemies.py` has no `[LOGIC]` checks: it only reads state. The run also contains `verify_pie_inventory.py`'s untagged direct-call checks (`try_pickup_focused`, `toggle_pause_menu_to_tab`), which this story neither uses nor changed.
- The PIE log has no override error, only the three "no visible mesh" errors (AC-085.1).

**Offline Python tests.** `py Scripts/run_python_tests.py`: the first run (while the editor was launching) had 1 unnamed failure out of 446; the rerun passed 446/446. The only `Scripts/` changes the tests cover are comments.

**Content.** `py Scripts/check_content_committed.py` PASS (1 asset-writing script, 1 `.umap`).

**Tier 3** (fresh package of `39500e2`, clean tree, `Saved/Playtests/package-20261027-201937.log`): `Saved/Playtests/20261027-202616-boot` — PASS (exit 0).
- build_fresh, launch, experience_identified/loaded (B_MordecaiArenaExperience at 8.2 s), no_crash, content_cooked, game_window and engine_screenshots PASS.
- screenshots WARN: the desktop captures are black or covered.
- log_errors INFO: 3 errors, all "has no visible mesh" for Frontliner_C_0/1/2, and no "overrides Default…" error.
- `engine_01.png`: the diorama arena from the fixed overhead camera. The player capsule is at the centre with three red enemy health bars around it and no bodies under them (AC-085.1 not done). HUD top-left with the health bar at a thin green sliver; Kills: 0; the six pickups with their labels.
- `engine_02.png`, `engine_03.png`: the same frame with the health bar empty: the standing player was killed during the boot.
- `shot_01..03.png`: black, not evidence.
- Not visible on any screenshot: whether an enemy has 100 or 200 health. The bars are world-space widgets and the bridge does not read their fill (B2), so the 200 is proven by the Tier 4 state reads.

**Tier 4** (same package): `py Scripts/playtest_story.py US-054` → `Saved/Playtests/20261027-202640-us054` (summary `Saved/Playtests/20261027-202616-story-us054`). Exit 1, 0 ACs accepted.
- `enemies_present` FAIL (NEW), now **only on spacing**: "(145, 0) hp 200.0/200.0; (-91, 113) hp 200.0/200.0; (-91, -113) hp 200.0/200.0", but 226–261 cm apart. The enemies have already closed on the spawn by the first read. Baseline `Saved/Playtests/20261027-085212-us054` @ `c7e4b8d`: 100.0/100.0 and the same spacing failure. So AC-085.2's health clause holds on real state, but the check, and therefore AC-085.2, is not accepted. It needs the placement TODO(DECISION) or Jeff's call.
- `arena_reset` FAIL: 200.0/200.0 after the reset (the health clause holds; it was 100.0/100.0), positions 454–494 cm from the spawns (re-aggro).
- Every other check has the same status as the `c7e4b8d` baseline:
  - PASS: hud_visible, hud_layout, enemy_health_bars, enemy_aggro, enemy_approaches, enemy_damages_player, player_dies, respawn, kill_counter_text_matches.
  - FAIL: enemies_visible (known), enemy_indicators (N17), walk_to_frontliner3 / melee_lmb / dodge_lalt (the player was killed on the walk), death_screen (known, N2), respawn_moves (+66 cm).
  - WARN: burning_on_hit, restoration_heals, dead_no_move.
- `bridge_01.png` (start): the arena with the player at the centre at ~40% health, three red bars around them, no bodies.
- `bridge_02.png` (near_enemy): the player in the lower-left part of the arena near the inner wall, three red bars stacked on them, health bar empty: dead before reaching Frontliner_3.
- `bridge_03.png` (hit), `bridge_04.png` (dead): the same view. The player is dead with the health bar empty and no death screen (N2). The on-screen red debug line reads "enemy BP_MordecaiEnemy_Frontliner_C_0 has no visible mesh".
- `bridge_05.png` (respawned): the player back at the centre with a full health bar; one red bar just above the player and two below it on either side: the enemies are already back on the player, not at their spawns (the `arena_reset` position failure).
- `py Scripts/playtest_story.py US-054 --lint`: PASS.

**Sweep** (same package): `Saved/Playtests/20261027-202756-sweep`. us054 (`20261027-202849-us054`: 200.0/200.0 again), us057, us069, us078, us079 and us083 have the same check statuses as the `c7e4b8d` sweep (`Saved/Playtests/20261027-085120-sweep`). us001 varies with where the enemies stand (N14):
- `move_d` PASS→FAIL: 11 cm, with the player already at 55 health from adjacent enemies.
- `walk_lane_reset` FAIL→PASS and `sprint_shift` SKIP→PASS.
- Enemy max health cannot change how enemies move; us001 never attacks.

**Not done here** (open TODO(DECISION)s for Jeff above): the mesh, FireSlash on one Frontliner, the placement out of aggro (which `enemies_present`'s spacing clause and `arena_reset` need), AttackRange vs reach (#47).

**Playable Status:** enemies — NOT PLAYABLE. 200 HP now (N13 fixed), still invisible (AC-085.1), and they swarm the spawn (placement / T1).

### 2026-10-28 — The bodies / issue #19 (branch `fix/issue-19-invisible-enemies`, delivered `f21525f`)
The Frontliners are visible (AC-085.1) and the packaged `game.log` has no enemy-content error (AC-085.5). FireSlash, AttackRange vs reach (#47) and AC-085.4 (now T1's) remain, so the story stays in progress.

**Decision (Jeff, 2026-10-28).** Issue #19 is fixed by the mesh only. The Frontliners keep their placement, and the swarm at the spawn is T1's (issue #46).

**Fix.**
- `122f1c8` (C++): `AMordecaiEnemyCharacter::BodyMesh` is an empty static mesh slot on the capsule: NoCollision, no overlaps, no navigation. A native enemy stays "no visible mesh", so the US-084 guard still catches an enemy Blueprint without a body. The body is hidden with the collision when the enemy dies and shown again by `ResetForArena`. The review workflow found that a standing body you can walk through is misleading. TODO(DECISION): corpse visuals.
- `c9d3897` (content): `Scripts/us085_configure_frontliner.py` creates `MI_Enemy_Frontliner` (BasicShapeMaterial, Color 0.55/0.04/0.04). It sets the BP's `BodyMesh` to `/Engine/BasicShapes/Cylinder` with it, scaled from the capsule (radius 40, half height 90 → 0.8 x 0.8 x 1.8), then compiles, saves and reads back. A second run changes nothing. DevTestMap is not modified: the name `BodyMesh` is absent from the `.umap`, and the placed actors read the Cylinder. `Scripts/verify_pie_enemies.py` gains the body checks. Tribal knowledge: inherited-component edits from Python persist; `set_material_instance_vector_parameter_value` always returns False; remote exec needs absolute paths.
- `f21525f` (playtests): `enemies_visible` loses its known issue (an invisible enemy is NEW again). The us054/us057/us083 texts, the explorer charter and the offline fakes no longer call the enemies invisible.

**Tier 1.** The project compiles.
- `Mordecai.Recovery.Enemy.BodyMeshSlot` was red before the C++ change ("Expected 'the enemy has a BodyMesh static mesh component' to be not null", `Saved/TestLogs/tests-20261028-004339.log`).
- `Mordecai.Recovery.Enemy.BodyHiddenWhileDead` was red before the hide ("Expected 'a dead enemy shows no body' to be false", `tests-20261028-074024.log`).
- All 705 Mordecai tests pass (`tests-20261028-082436.log`).
- Offline Python tests: 447/447.

**Tier 2.** `py Scripts/run_pie_verify.py --no-launch` passed 12/12 twice, each time on an editor restarted after the content save (after the content, and after the dead-body change).
- `verify_pie_enemies.py` 14/14: "Cylinder, no collision" on the BP and on each of the three enemies.
- The editor log has no `LogMordecaiEnemy` error. Earlier sessions each logged three "no visible mesh".
- No `[LOGIC]` checks: it only reads state.

**Content.** `py Scripts/check_content_committed.py` PASS: 1 asset-writing script, 2 assets (`BP_MordecaiEnemy_Frontliner.uasset`, the new `Enemies/Materials/MI_Enemy_Frontliner.uasset`).

**Package.** Fresh package of `f21525f`, clean tree, 13.3 min (`Saved/Playtests/package-20261028-083847.log`).

**Tier 3** `Saved/Playtests/20261028-085310-boot`: PASS (exit 0).
- build_fresh, launch, experience_identified/loaded (B_MordecaiArenaExperience at 6.8 s), no_crash, content_cooked, game_window and engine_screenshots PASS.
- screenshots WARN: the desktop captures are covered or black.
- log_errors INFO: **0 errors** (before: the three "no visible mesh").
- `engine_01.png`: the diorama arena from the fixed overhead camera. The grey player cylinder is at the centre with **three red cylinder bodies against it**, one behind (+X) and two in front, each under its red health bar. HUD top-left with a sliver of health; Kills: 0; the labelled pickups. The red Health Potion pickup is a much smaller cylinder with no bar.
- `engine_02.png`, `engine_03.png`: the same frame with the health bar empty. The player was killed during the boot.
- `shot_01..03.png`: black or covered, not evidence.
- The US-001 run's boot `Saved/Playtests/20261028-085832-boot` is also PASS (0 errors). `engine_01..03.png` show the same three red bodies under their bars, around a player with an empty health bar.

**Tier 4 US-054** (same package): `py Scripts/playtest_story.py US-054` → `Saved/Playtests/20261028-085332-us054` (summary `20261028-085310-story-us054`). Exit 1, 0 ACs accepted. `--lint` PASS.
- Changed since the issue #18 run `20261027-202640-us054`:
  - **`enemies_visible` PASS**: "3/3 enemies have a visible mesh" (was FAIL known, 0/3).
  - `arena_reset` and `respawn_moves` PASS this time (the swarm's timing).
- The same as before:
  - `enemies_present` FAIL (NEW), spacing only: 200.0/200.0, 232–261 cm apart (the swarm).
  - `enemy_indicators` FAIL (N17).
  - `walk_to_frontliner3`, `melee_lmb`, `dodge_lalt` FAIL: the player was killed on the first leg.
  - `death_screen` FAIL (known, N2).
  - `burning_on_hit`, `restoration_heals` WARN: the player was dead.
  - log_errors: 0.
- Screenshots:
  - `bridge_01.png` (start): the arena with the player at the centre. **A red cylinder body under each of the three bars**, already against the player, whose health is a sliver.
  - `bridge_02.png` (near_enemy): the player dead (health empty) on the way to Frontliner_3, with the three red bodies just past it.
  - `bridge_03.png` (hit): not the first hit. The player had died and respawned before the shot: full health, and the three bodies motion-blurred as the arena reset moves them back to their spawns.
  - `bridge_04.png` (dead): the player dead at the spawn after the respawn, ringed by the three red bodies. No death screen (N2).
  - `bridge_05.png` (respawned): full health; the bodies are back at their spawns (top, lower left, lower right), blurred.

**Tier 4 US-001** (same package): `py Scripts/playtest_story.py US-001` → `Saved/Playtests/20261028-085852-us001` (summary `20261028-085832-story-us001`). Exit 1. `--lint` PASS.
- PASS: `move_a` (207 cm along -Y, 0 across), `move_w`, `move_d`, `camera_fixed`, `no_camera_rotation_input`.
- FAIL from the swarm:
  - `move_s`: 37 cm; an enemy stood behind the player.
  - `walk_lane_wp1`: "the player is dead at (152, -109)": killed, not walled.
  - `walk_lane_reset`, `walk_lane_reset2`, `jump_space`: the player was dead.
- Known: `facing_mouse`, `facing_stick`, `gamepad_move`.
- Screenshots:
  - `bridge_01.png` (start): the player at the centre at about 55% health, three red bodies against it under their bars.
  - `bridge_02.png` (after_moves): the camera followed without rotating. The player is dead with the three red bodies in front of it, not at the lane start.
  - `bridge_03.png` (facing_arrow): respawned with full health, the three bodies back at their spawns, no facing arrow (N12).

**Sweep** (same package): `Saved/Playtests/20261028-090808-sweep`, against `20261027-202756-sweep`. No regression from the bodies.
- us054 `enemies_visible` FAIL (known) → PASS. Every other us054 failure is unchanged.
- us057/us069/us078/us079: identical NEW and known lists.
- us001 and us083 changed only in checks that depend on the swarm:
  - us001 `walk_lane_wp1` PASS here, while `move_a` FAILs on 37 cm drift after a 385 cm enemy shove.
  - us083 `dodge_gamepad_b` FAILed because its B tap reached a dead player 0.2 s before the respawn (bridge.jsonl). `block_over_held_rt_no_parry` went FAIL → PASS.
- All 29 sweep screenshots (boot `engine_01..03`, `shot_01..03`; every `bridge_NN.png` of the 7 scripts) were opened by parallel reader agents against each report's expectations; I opened the US-054/US-001 runs above myself.
  - Every frame with game content shows a red body under each red bar, and no bar without a body.
  - The frames that do not match their description are all existing issues: us054 `hit` taken after the death; us057 no projectile or damage number (N15) and no Stone Skin text (N16); us079 empty inventory rows (US-094).
  - The boot `shot_NN.png` show the lock-screen wallpaper (covered window).

**Not done here:** FireSlash on one Frontliner (TODO(DECISION)), AttackRange vs reach (#47), AC-085.2/AC-085.3 (their us054 checks still fail on the swarm), AC-085.4 (T1), the DoD's exit-0 runs.

**Playable Status:** enemies — NOT PLAYABLE. Visible now (AC-085.1) at 200 HP, but they swarm the spawn (T1, #46), and `enemy_indicators` (N17) and `death_screen` (N2) fail.
