# US-100: Recovery — Playtest Test Maps (Movement and Combat)

**Epic:** Epic R: Playability Recovery
**Priority:** P0
**Depends on:** US-083, US-084, US-085
**Paired story:** none (the test enemies are data-only Blueprints; no C++ gameplay change)

## Overview
Until this story, every Tier 4 script played DevTestMap. Its three Frontliners stand inside AggroRange of the PlayerStart, so they swarm the spawn, body-block the player and kill it in about 10 s. Since issue #12, movement, melee, dodge, spell and attack checks that have nothing to do with enemies fail on a dead or shoved player (14 checks in the `c7e4b8d` sweep, issue #46). This story adds two small cooked test maps: MovementTestMap (no enemies) and CombatTestMap (a dummy that never moves or attacks, and an attacker that holds its spot and swings at a known cadence). Each Tier 4 script launches its own map. DevTestMap stays the enemy-AI arena. This is plan item T1 (Jeff, 2026-10-24; chosen over moving the Frontliners on 2026-10-28). It unblocks the movement, sprint, melee_attack, dodge, death_respawn, spells, weapons and pause_menu checks.

## Execution Mode
**EDITOR**
The story creates and saves two `.umap` levels and five enemy assets (two enemy Blueprints, one AI-controller Blueprint, two material instances) through Python remote execution in the editor. Its only C++ is Tier 1 test code; there is no gameplay change.

## References
- `Specs/agent_playtest_plan_v1.md`:
  - T1 under "Playtest infrastructure"
  - N14 and Jeff's decision there (2026-10-28: T1, the Frontliners keep their placement)
  - the issue #12 sweep entry (the 14 checks)
  - N27 (dodge distance)
  - N32 (enemy melee ignores block, parry and i-frames), new
  - Open decisions
- `Specs/design_decisions_log.md` 2026-10-29 "Test maps (T1, issue #46)":
  - crouch is out of scope
  - us069 moves to MovementTestMap
  - us079 stays on DevTestMap
- GitHub issue #46 (T1: separate movement and combat test maps for the playtest suites)
- Baselines:
  - sweep `Saved/Playtests/20261028-090808-sweep` @ `f21525f`, the newest
  - sweep `Saved/Playtests/20261023-192402-sweep` @ `01abb8b`, where the 14 checks PASSed; `arena_reset` and `respawn_moves` were SKIP there and PASSed in `Saved/Playtests/20261028-085332-us054` @ `f21525f`
  - us079 run `Saved/Playtests/20261028-232550-us079` @ `92e7575`, exit 0
  - sweep `Saved/Playtests/20261027-085120-sweep` @ `c7e4b8d`, where the 14 checks failed
- `Specs/agent_rules_v2.md`:
  - §3 NO PIE-ONLY BEHAVIOR: maps under `/MordecaiCore/Game/Maps`, and every Mordecai map resolves the Mordecai experience in a packaged build
  - §3 CONTENT IS COMMITTED
  - §8 Map & World Building: read; see the exemption in Technical Notes
- `Specs/map_style_technical_design_v1.md`, `Specs/agentic_map_pipeline_v1.md`: read per `CLAUDE.md` for any map story. They govern world cells, not test fixtures (see Technical Notes).
- `Specs/control_bindings_v1.1.md`:
  - WASD move, Space jump, Left Shift sprint (toggle), LMB light/heavy attack, Left Alt dodge, Q/E/R/One spells, Tab/BackSlash weapons, Escape/I pause menu
  - Gamepad LT/RT/A/B
  - "Sneak (Toggle)" (Ctrl / R3) is the design's crouch: out of scope, AC-100.6
- Movement numbers (`Source/LyraGame/Mordecai/MordecaiCharacter.cpp`): WalkSpeed 600, sprint ×1.5. JumpZVelocity 600 with GravityScale 1.75 gives an apex of about 105 cm. The engine default MaxStepHeight is 45 cm.
- Enemy code:
  - `AMordecaiEnemyCharacter`: `DefaultMaxHealth`, `DefaultMoveSpeed`, `DefaultAbilities`, `EnemyAttackProfiles`, `EnemyHealthBarWidgetClass`, `BodyMesh`, all EditDefaultsOnly
  - `AMordecaiEnemyAIController`: AggroRange 900, AttackRange 200, LeashRange 1800, AttackCooldownMs 1500
  - `/MordecaiCore/Combat/Enemies/DA_EnemyAttack_BasicSlash`: windup 500 / active 200 / recovery 400 ms, 150 cm / 90° arc, 15 Slash
- Existing assets and scripts:
  - `/MordecaiCore/Game/Maps/DevTestMap` (not modified)
  - `/MordecaiCore/Enemies/BP_MordecaiEnemy_Frontliner`, `MI_Enemy_Frontliner`
  - `Scripts/us085_configure_frontliner.py` (pattern for the BPs and MIs), `Scripts/us054_04_setup_arena.py` (lights), `Scripts/verify_gamemode_wiring.py` (World Settings game mode)
  - `Source/LyraGame/Tests/Level/MordecaiLevelTests.cpp` (`Mordecai.Level.DevTestMapExists` pattern)

## Acceptance Criteria
- [ ] AC-100.1: `/MordecaiCore/Game/Maps/MovementTestMap` exists as one non-World-Partition `.umap`. It has:
  - World Settings GameMode Override = `MordecaiGameMode` and no World Settings experience
  - exactly one PlayerStart, at (0, 0, 100), yaw 0
  - a floor over x, y ∈ [-2000, 2000] with its top at z = 0 (WorldGridMaterial), and 100 cm boundary walls on its edges
  - no `AMordecaiEnemyCharacter` and no pickups
  - lane L1 (y = -800, x -1000..+1600) clear
  - two steps at +Y: `T1_Step_Walk30` (x 600..1000, y 800..1200, top z 30, walkable) and `T1_Step_Jump60` (x 1200..1600, y 800..1200, top z 60, reachable only by jumping)
  - movable lights only
  - Verified by:
    - Tier 1 `Mordecai.Level.TestMaps.*`: the World Settings game mode and no experience, not World Partition, the PlayerStart (count, location, yaw), no enemies or pickups, the floor and wall boxes with WorldGridMaterial, the steps, lane L1, Movable lights.
    - Tier 2 `Scripts/verify_pie_test_maps.py`: the PIE spawn near (0, 0), the game mode, no enemies or pickups.
    - Tier 3 `engine_NN.png`: the grid floor on screen.
- [ ] AC-100.2: `/MordecaiCore/Game/Maps/CombatTestMap` exists as one non-World-Partition `.umap`. It has:
  - the same World Settings rule and one PlayerStart at (0, 0, 100), yaw 0
  - a floor over x ∈ [-1500, 2700], y ∈ [-1600, 1600], with walls
  - `TestDummy` (`BP_MordecaiEnemy_TestDummy`) at (600, 600, 100): 200 HP, `DefaultMoveSpeed` 0, AI `BP_MordecaiEnemyAI_TestDummy` with AggroRange 0. It stays Idle and never attacks.
  - `TestAttacker` (`BP_MordecaiEnemy_TestAttacker`) at (2100, 0, 100): 200 HP, `DefaultMoveSpeed` 0 (holds its spot), the native AI (AggroRange 900, AttackRange 200, LeashRange 1800, AttackCooldownMs 1500), `EnemyAttackProfiles` [`DA_EnemyAttack_BasicSlash`] only. That gives the known cadence: one 15-damage swing about every 2.6 s while the player is within 150 cm.
  - a built, saved navmesh: a valid, non-partial path from (0, 0) to (2000, 0)
  - Placement rule: the attacker is more than 900 cm from the PlayerStart, the dummy, the dummy's melee spot (440, 600) and shot spot (300, 600), and every point of the Blink lane (from (0, -600) along +X to x 1100). Only the fight spot (2000, 0) is inside its AggroRange.
  - Base stats live on the Blueprint CDOs only, never as per-instance overrides. Each enemy has its own body colour (dummy blue-grey, attacker orange).
  - Verified by:
    - Tier 1 `Mordecai.Level.TestMaps.*`: the World Settings game mode and no experience, not World Partition, the PlayerStart (count, location, yaw), the floor and wall boxes with WorldGridMaterial, Movable lights, the enemy classes at their spots, the CDO stats and AI classes (the dummy AI's AggroRange 0), no instance overrides, the placement rule, the navmesh presence and bounds, the body materials and their colours.
    - The read-back of `Scripts/us100_02_create_test_maps.py` (its PASS lines and RESULT line, recorded in the Completion Notes): the valid, non-partial navmesh path (0, 0) → (2000, 0), checked in the editor world (a path query during PIE trips a handled ensure).
    - Tier 2 `Scripts/verify_pie_test_maps.py`: the PIE spawn, both enemies present, Idle, 200/200 and with MaxWalkSpeed 0, the attacker beyond its AggroRange from the pawn, a RecastNavMesh in the PIE world.
    - Tier 3 `engine_NN.png`: the grid floor and the dummy's blue-grey body on screen.
- [ ] AC-100.3: Both maps are in the default package cook (`Scripts/package_playtest.py` `DEFAULT_MAPS` and `+MapsToCook` in `Config/DefaultGame.ini`). On a fresh package, `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap` and `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` each exit 0:
  - `B_MordecaiArenaExperience` identified
  - `content_cooked` PASS
  - no `LogMordecai*` error
  - `engine_NN.png` show the grid floor, the player and the Mordecai combat HUD, plus the TestDummy body under its health bar on CombatTestMap
  - `py Scripts/playtest_boot.py` (DevTestMap) still exits 0.
- [ ] AC-100.4: Every Tier 4 script declares its `MAP`. The kit's `map` check compares the loaded map (`ping.map`, any `.Object` suffix stripped, case-insensitive) with the script's `MAP`. A mismatch FAILs (gameplay) and blocks the plan, as a wrong experience does; a front-end run records INFO. Verified offline by `py Scripts/run_python_tests.py` (kit tests) and by `map` PASS in every Tier 4 report on the fresh package.
- [ ] AC-100.5: Each script runs on its map:
  - MovementTestMap: `us001_movement.py`, `us083_combat_input.py`, `us069_pause_menu.py`
  - CombatTestMap: `us054_combat.py` (new; US-054's walk/melee/dodge/damage/death/respawn/reset chain), `us057_magic.py`, `us078_weapons.py`
  - DevTestMap: `us054_arena.py` (enemy-AI checks), `us079_inventory.py`
  - On a fresh package, `py Scripts/playtest_story.py --all` shows the issue's 14 checks PASS on their new maps:
    - us001: `move_s`, `walk_speed_baseline`, `walk_lane_reset`, `walk_lane_reset2`, `jump_space`
    - us054: `walk_to_dummy` (was `walk_to_frontliner3` on DevTestMap), `melee_lmb`, `dodge_lalt`, `arena_reset`, `respawn_moves`
    - us057: `walk_blink_lane`
    - us078: `light_attack_lmb`
    - us083: `block_over_held_rt_no_parry`, `light_attack_rt_tap`
  - No check that PASSed in a baseline (the `f21525f` sweep `20261028-090808-sweep`, the `01abb8b` sweep `20261023-192402-sweep`, or the `92e7575` us079 run `20261028-232550-us079`) FAILs.
- [ ] AC-100.6: Crouch is recorded out of scope (Jeff, 2026-10-29) in `Specs/design_decisions_log.md` and the Open decisions of `Specs/agent_playtest_plan_v1.md`. It is the spec'd "Sneak (Toggle)" (Ctrl / R3), owned by blocked US-034. No crouch geometry or check is built.
- [ ] AC-100.7: Every asset is saved and committed (both `.umap`, the three Blueprints, the two material instances); `py Scripts/check_content_committed.py` exits 0.

## Technical Notes
- **Assets** (paths as in the spec; agents must not rename them):
  - Maps, single `.umap` each (Git LFS), with no `__ExternalActors__` folder and no `_BuiltData`:
    - `/MordecaiCore/Game/Maps/MovementTestMap` (`Plugins/GameFeatures/MordecaiCore/Content/Game/Maps/MovementTestMap.umap`)
    - `/MordecaiCore/Game/Maps/CombatTestMap` (`.../Content/Game/Maps/CombatTestMap.umap`)
  - `/MordecaiCore/Enemies/BP_MordecaiEnemyAI_TestDummy`: Blueprint, parent `/Script/LyraGame.MordecaiEnemyAIController`. Its CDO sets AggroRange = 0, so it never leaves Idle and never targets decoys; the other AI values stay default.
  - `/MordecaiCore/Enemies/BP_MordecaiEnemy_TestDummy`: Blueprint, parent `/Script/LyraGame.MordecaiEnemyCharacter`, like the Frontliner, not a child of it. CDO:
    - `AIControllerClass` = `BP_MordecaiEnemyAI_TestDummy_C`, `DefaultMoveSpeed` 0, `DefaultMaxHealth` 200
    - `DefaultAbilities` [`MordecaiGA_MeleeAttack`], `EnemyAttackProfiles` [`DA_EnemyAttack_BasicSlash`]. These are kept so `ValidateContent` and `TryEnterAttackState` log nothing; the dummy never reaches Attack.
    - `EnemyHealthBarWidgetClass` `WBP_EnemyHealthBar`
    - `BodyMesh` `/Engine/BasicShapes/Cylinder`, scale (0.8, 0.8, 1.8), with `MI_Enemy_TestDummy`
  - `/MordecaiCore/Enemies/BP_MordecaiEnemy_TestAttacker`: Blueprint, parent `MordecaiEnemyCharacter`. CDO:
    - `AIControllerClass`: the native `MordecaiEnemyAIController` (C++ default)
    - `DefaultMoveSpeed` 0, `DefaultMaxHealth` 200
    - `DefaultAbilities` [`MordecaiGA_MeleeAttack`], `EnemyAttackProfiles` [`DA_EnemyAttack_BasicSlash`] only. The AI swings only index 0 (N24), so FireSlash would never be swung.
    - health bar as above; `BodyMesh` Cylinder (0.8, 0.8, 1.8) with `MI_Enemy_TestAttacker`
  - `MI_Enemy_TestDummy` (blue-grey, about (0.25, 0.35, 0.75)) and `MI_Enemy_TestAttacker` (orange, about (1.0, 0.45, 0.05)): MaterialInstanceConstants made like `MI_Enemy_Frontliner`.
- **Editor scripts.** Each is idempotent and prints PASS/FAIL lines and a RESULT line. Each uses `#` comments instead of a module docstring, does all its work in `main()`, and holds no module-level UObjects. Run them with `py Scripts/ue_remote_exec.py "exec(open(r'<absolute path>').read())"`.
  - `Scripts/us100_01_create_test_enemies.py`: the MIs, the AI BP and the two enemy BPs. It compiles twice, saves and reads back (the `us085_configure_frontliner.py` pattern). Base stats go on the BP CDOs only, never as instance overrides on placed actors: N13, and `ValidateContent` logs an error for an override.
  - `Scripts/us100_02_create_test_maps.py`: both maps.
    - If a map is absent it uses `LevelEditorSubsystem.new_level(path, False)`. That call fails on an existing asset and discards the open world without a prompt, so nothing may be dirty before it. If the map exists, the script loads it and deletes the actors it owns: labels `T1_*`, the PlayerStart, `TestDummy` and `TestAttacker`.
    - It sets World Settings `default_game_mode` = `unreal.MordecaiGameMode` and reads it back. It sets no experience.
    - It saves with `save_current_level()` (raise on False).
    - It ends by reloading DevTestMap, because `run_pie_verify.py` PIEs whatever map is open.
- **Common layout (both maps).**
  - `T1_Floor`: one `/Engine/BasicShapes/Cube` scaled to the floor rectangle, top at z = 0 (centre z -50), `/Engine/EngineMaterials/WorldGridMaterial` (a measuring grid; always cooked).
  - `T1_Wall_N/S/E/W`: Cubes 50 cm thick and 100 cm high (z 0..100). Their inner faces sit on the floor edges, and they extend 50 cm past the corners.
  - One plain `PlayerStart` at (0, 0, 100), yaw 0, as in DevTestMap.
  - Movable lights only: `T1_Sun` DirectionalLight (pitch -50, yaw 30) at (0, 0, 1000), `T1_SkyLight` (real-time capture), `T1_SkyAtmosphere`, `T1_HeightFog`. No static lighting, no BuiltData and no reflection captures (the project uses Lumen).
  - Camera reading: the diorama camera looks along +X, so screen up = +X and screen right = +Y.
- **MovementTestMap:**
  - Wall boxes (x0, x1, y0, y1):
    - N (-2050, 2050, 2000, 2050)
    - S (-2050, 2050, -2050, -2000)
    - E (2000, 2050, -2050, 2050)
    - W (-2050, -2000, -2050, 2050)
  - Lane L1 at y = -800 from x -1000 to +1600 is plain floor.
  - `T1_Step_Walk30` (x 600..1000, y 800..1200, top z 30) is under MaxStepHeight 45. `T1_Step_Jump60` (x 1200..1600, y 800..1200, top z 60) needs a jump (apex about 105 cm). No current check uses the steps, and they are off every scripted route.
  - No NavMeshBoundsVolume.
  - The us001 and us083 waypoints and check names are unchanged, because DevTestMap's frame is kept: PlayerStart at (0, 0) and lane L1 at y = -800. AC-085.4 names `move_a` and `walk_lane_wp1`.
- **CombatTestMap:**
  - Wall boxes:
    - N (-1550, 2750, 1600, 1650)
    - S (-1550, 2750, -1650, -1600)
    - E (2700, 2750, -1650, 1650)
    - W (-1550, -1500, -1650, 1650)
  - `TestDummy` at (600, 600, 100), yaw 180 (cosmetic). `TestAttacker` at (2100, 0, 100), yaw 180.
  - `T1_NavBounds`: a NavMeshBoundsVolume over the whole floor (z -100..400). Navigation is engine-default `RuntimeGeneration=Static`, so the script builds the navmesh blocking before the save:
    - `RebuildNavigation` does nothing until the world has navigation data: `FNavigationSystemExec` runs `Build()` only when `NavDataSet.Num() > 0`. A NavMeshBoundsVolume spawned in the same Python call only queues a bounds update, which a nav tick or a map load processes.
    - So, after spawning and scaling the volume: save the map, reload it (the load spawns `RecastNavMesh-Default` from the saved volume), fetch the editor world again, then `unreal.SystemLibrary.execute_console_command(world, 'RebuildNavigation')`. Do this on every run, not only on a new map: a rerun replaces the volume, and the reload drops the old one's bounds.
    - then asserts `not NavigationSystemV1.is_navigation_being_built(world)`, a RecastNavMesh actor, and a valid non-partial `find_path_to_location_synchronously(world, (0, 0, 92), (2000, 0, 92))`, raising on any failure, and only then saves
    - The attacker never moves, but its AI calls `MoveToActor` in Approach. The navmesh keeps that quiet, and a future moving attacker would work.
  - Scripted spots, all on open floor:
    - dummy melee spot (440, 600), 160 cm -X of the dummy
    - Fireball shot spot (300, 600), 300 cm -X
    - attacker fight spot (2000, 0), 100 cm -X of the attacker. It was (1980, 0), 120 cm off; it moved so that a walk_to stopping anywhere within its 25 cm tolerance leaves the player at most 125 cm (centre to centre) from the attacker, well inside the 150 cm swing (it was up to 145).
    - Blink lane from (0, -600) along +X, clear to x 1100 (±100 cm in Y)
  - Routes: the spawn → melee spot is a straight line clear of the dummy. The melee spot → fight spot goes via (440, 0) and then along y = 0; the straight line passes within 58 cm of the dummy's centre.
  - Attacker cadence ("known cadence"):
    - BasicSlash windup 500 ms, then the hit: 15 Slash, a 150 cm / 90° arc from the enemy's forward; the enemy faces its focus. Then active 200 ms, recovery 400 ms, then AttackCooldownMs 1500.
    - That is one swing about every 2.6 s while the player stays within 150 cm (centre to centre). The first hit lands 0.5 s after the attacker enters Attack.
    - A 100 HP player dies after 7 hits, about 16-18 s.
    - The attacker tracks the player through its windup, so only leaving 150 cm avoids a hit. A jump does not: nothing grants `Mordecai.State.Airborne`, and the arc test ignores Z (N32).
- **DevTestMap is not modified.** `us054_arena.py` (the 3-Frontliner AI checks) and `us079_inventory.py` stay on it. Jeff decided on 2026-10-29 that us079 and its 6 pickups stay there (US-079's "do not create a new level" stands), so the inventory slice still races the swarm.
- **Script placement:**
  - us083 runs whole on MovementTestMap. None of its checks needs an enemy: block, parry and attacks are judged by player tags.
  - us054 is split:
    - `us054_arena.py` (DevTestMap, `RUN_SUFFIX` "us054"): hud_visible, hud_layout, enemies_present, enemies_visible, enemy_health_bars, enemy_indicators, enemy_aggro, enemy_approaches, enemy_damages_player.
    - `us054_combat.py` (CombatTestMap, `STORY` "US-054", `RUN_SUFFIX` "us054combat"): combat_targets_present, walk_to_dummy, melee_lmb, dodge_lalt, walk_to_attacker, attacker_damages_player, restoration_heals (before burning_on_hit, so the heal runs on a live player), burning_on_hit (WARN when no Burning: the attacker swings BasicSlash), player_dies, death_screen (known N2), dead_no_move, respawn (within 150 cm of (0, 0)), arena_reset, respawn_moves, kill_counter_text_matches.
  - `arena_reset` on CombatTestMap PASSes only when every enemy is alive, at full health, within 100 cm of its spawn and in ai_state Idle, AND the reset was observed: the dummy was below max health before the death and full after, or the attacker had left Idle before the death and is Idle after. A reset with no observable evidence stays WARN.
  - us057 targets `TestDummy` (by label, else the enemy nearest the dummy's spot). It shoots from (300, 600) and Blinks from (0, -600).
  - us078 and us083 FAIL with an explicit "player dead" detail instead of treating a dead player as idle. The scripts' preconditions must never hide a death.
  - us083 records each dodge's displacement as a measurement (N27; no FAIL).
- **Tooling** (in this branch):
  - `Scripts/playtest_boot.py` defines `MOVEMENT_TEST_MAP`, `COMBAT_TEST_MAP` and `TEST_MAPS`, re-exported by `Scripts/playtest_kit.py`; scripts write `MAP = kit.MOVEMENT_TEST_MAP` / `kit.COMBAT_TEST_MAP`.
  - `package_playtest.py` `DEFAULT_MAPS` and `Config/DefaultGame.ini` `+MapsToCook` gain both maps.
  - The kit gains the `map` check (AC-100.4).
  - `playtest_story.py` supports a story with several scripts:
    - one Steps table per script, keyed by the `<!-- Scripts/playtests/x.py -->` marker that `--plan` emits
    - `--lint` of a story fails when one of its scripts shares its `RUN_SUFFIX` (case-insensitive) with another discovered script; `test_the_real_scripts_have_unique_run_suffixes` checks every real script
    - in the AC roll-up, a FAIL of any check that proves an AC, in any of the story's runs, beats a PASS elsewhere
  - `agent_bridge.py` and `mordecai_play_mcp.py` name the test maps in their help (default stays DevTestMap; no protocol change).
- **Exemption from `agent_rules_v2.md` §8:** the test maps are test fixtures, not world cells. §8's cell framework (`DA_Cell_*` metadata first, `PM_*` markup, height tiers, `/Content/World/Biomes/...`) does not exist yet (backlog US-080..082), and these maps are never part of the room graph. So they carry no `DA_Cell_` or `PM_` markup and live under `/MordecaiCore/Game/Maps`, where the AssetManager Map scan is AlwaysCook. The pipeline's spirit still holds: simple deliberate collision, a clear purpose per space, and nothing tall on the camera side of a scripted spot.
- **No playlist:** test fixtures are launched by `--map` (`playtest_boot.py`, the kit's `MAP`, `agent_bridge.py launch --map`). A player never reaches them, by design.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state. The maps get the Mordecai experience the way DevTestMap does in the package: World Settings GameMode Override = `MordecaiGameMode`, whose `InitGame` appends `?Experience=B_MordecaiArenaExperience`. PIE's `ExperienceOverride` would hide a missing override, so only Tier 1 (the World Settings test) and Tier 3 prove it.
- **Source audit (2026-10-29, new Known issue N32, not fixed here):** block, parry, perfect block, dodge i-frames and Blink invulnerability do not reduce enemy melee damage.
  - `UMordecaiGA_MeleeAttack::ApplyDamageToTarget` applies instant health and posture GEs to the target ASC without reading `State_Blocking`, `State_Dodging`, `State_Invulnerable` or `State_Parrying`.
  - `UMordecaiGA_Block::ReceiveBlockedHit` (mitigation, stamina drain, perfect-block posture damage) has no caller outside a test.
  - `UMordecaiAttributeSet` only clamps.
  - A jump does not avoid it either: the `JumpAvoidable` filter (`UMordecaiHitDetectionSubsystem::IsTargetAirborne`) reads `Mordecai.State.Airborne`, which nothing in Source/, Config/ or content grants (only a test adds it), and `IsPointInArcSector` ignores Z.
  - The us083 block and parry checks are tag-only and stay valid. A future CombatTestMap check that asserts mitigation would FAIL today, as a real bug.
- TODO(DECISION) (Jeff): none open in this story. Crouch was decided out of scope (2026-10-29). FireSlash on an attacker (so `burning_on_hit` can PASS) stays with US-085's FireSlash TODO(DECISION).

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Level.TestMaps.*` (`Source/LyraGame/Tests/Level/MordecaiTestMapTests.cpp`) covers AC-100.1 and AC-100.2. Each test loads the map's `UWorld` package, like `Mordecai.Level.DevTestMapExists`. Names as of 2026-10-29; the Completion Notes record the final list:
  - `MovementTestMapExists`, `CombatTestMapExists`
  - `MapsUseMordecaiGameModeWithoutExperience`: World Settings `DefaultGameMode` is `AMordecaiGameMode`, with no `DefaultGameplayExperience`
  - `MapsHaveOnePlayerStartAtOrigin`: exactly one `APlayerStart`, at (0, 0, 100)
  - `MovementTestMapHasNoEnemiesOrPickups`, `MovementTestMapHasStepsAndClearLane`
  - `CombatTestMapHasDummyAndAttackerAtTheirSpots`: exactly the TestDummy and the TestAttacker, of their BP classes, at their spots
  - `CombatTestMapAttackerOutsideAggroRangeOfScriptedSpots`: the placement rule
  - `CombatTestMapHasNavMesh`
  - `TestDummyNeverMovesOrAggroes`, `TestAttackerHoldsItsSpotAndSwings`: the enemy BP CDOs (`DefaultMoveSpeed` 0, `DefaultMaxHealth` 200, profiles, AI controller classes, the dummy AI's AggroRange 0)
  - the layout clauses of AC-100.1/AC-100.2 (review of 2026-10-29): `MapsAreOneUmapNotWorldPartition`; `MapsHaveGridFloorAndBoundaryWalls` (`T1_Floor` and `T1_Wall_N/S/E/W` at their boxes on both maps, with WorldGridMaterial); `MapsHaveOnlyMovableLights`; the PlayerStart yaw 0 (in `MapsHaveOnePlayerStartAtOrigin`); and each enemy's `BodyMesh` material (`MI_Enemy_TestDummy` / `MI_Enemy_TestAttacker`) and its colour (in the two BP tests)
  - These are content tests, like `Mordecai.Level.DevTestMapExists`: red until the content lands. The cook list itself (AC-100.3) is checked offline by `Scripts/tests/test_package_playtest.py` and at Tier 3.
- ALL `Mordecai.*` tests pass (`py Scripts/run_tests.py` exits 0).
### Tier 2 — PIE smoke (editor integration)
- `py Scripts/run_pie_verify.py --map /MordecaiCore/Game/Maps/MovementTestMap --tests spawn,hud,test_maps` and the same with `/MordecaiCore/Game/Maps/CombatTestMap`. `Scripts/verify_pie_test_maps.py` reads state only, with no `[LOGIC]` calls:
  - the game mode is `AMordecaiGameMode`
  - the pawn spawned near (0, 0)
  - MovementTestMap: no enemies
  - CombatTestMap: the dummy and the attacker are present, Idle and at 200/200; their movement max walk speed is 0; the navmesh exists
- `py Scripts/run_pie_verify.py` on DevTestMap (the unchanged suite): the map script leaves DevTestMap open.
- PIE applies the PIE-only `ExperienceOverride` to every map, so Tier 2 cannot prove AC-100.3.
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest (player-facing stories with input-driven ACs; otherwise `none`)
- US-100 has no script of its own. AC-100.5 is proven by the owning stories' scripts, each on its own map:
  - MovementTestMap: `Scripts/playtests/us001_movement.py`, `us083_combat_input.py`, `us069_pause_menu.py`
  - CombatTestMap: `us054_combat.py` (new), `us057_magic.py`, `us078_weapons.py`
  - DevTestMap: `us054_arena.py`, `us079_inventory.py`
  - Run them all with `py Scripts/playtest_story.py --all` on the fresh package.
- `py Scripts/playtest_story.py US-0XX --lint` exits 0 for US-001, US-054 (two scripts, two marker-keyed Steps tables), US-057, US-069, US-078 and US-083.
- Offline: `py Scripts/run_python_tests.py`:
  - the kit's `map` check
  - multi-script lint, unique `RUN_SUFFIX` and the FAIL-wins AC roll-up
  - each script against `FakeWorld(**MAP_PRESETS[map])` from `Scripts/tests/fake_bridge.py`
- Real input only (key/axis/mouse/click through the bridge); no `debug_*` or direct calls.

## Playtest
- **Launch path:**
  - Tier 3: three boots, each expecting `B_MordecaiArenaExperience` (the default `--expect-experience`):
    - `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap` (MovementTestMap → B_MordecaiArenaExperience)
    - `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` (CombatTestMap → B_MordecaiArenaExperience)
    - `py Scripts/playtest_boot.py` (DevTestMap, unchanged)
  - Tier 4: `py Scripts/playtest_story.py --all` on the same package. It runs one Tier 3 boot per launch path, then every script on its own `MAP`.
- **Player steps:**
  - MovementTestMap:
    - us001: hold A, W, D, S (0.4 s each); walk (WASD) to lane L1 and its start (-700, -800); hold W 1 s; Left Shift then W (sprint); Space (jump).
    - us083: the same lane walk and a Left Shift sprint; then the gamepad: LT block, RT parry while blocking, RT tap/hold, B dodge, A jump. Then LMB tap/hold, Left Alt, Space.
    - us069: Escape, tab clicks, gamepad Back/Start, I.
  - CombatTestMap:
    - us054_combat: walk to (440, 600) beside the dummy; click LMB; tap Left Alt. Walk via (440, 0) to (2000, 0) beside the attacker and stand until hit; tap One (Restoration); stand until dead; wait for the respawn; hold W.
    - us057: walk to (300, 600) and tap Q at the dummy; walk to (0, -600) and tap E (Blink); tap R; tap One.
    - us078: Tab ×4, Tab, BackSlash, Left Shift+Tab, gamepad Y; LMB tap and hold at the spawn.
  - DevTestMap: unchanged (us054_arena stands at the spawn; us079 walks the six pickups).
- **Visible on screen:**
  - Tier 3 `engine_NN.png`, MovementTestMap: the grey measuring-grid floor from the fixed overhead camera, the player cylinder at the centre, and the combat HUD (bars top-left, 'Kills: 0' top-right, '[I] Inventory' bottom-centre). No enemy health bar, no pickup. The steps, if in frame, are up and to the right of the player.
  - CombatTestMap: the same floor, player and HUD, plus the blue-grey TestDummy cylinder under its red health bar, up and to the right of the player at (600, 600). The orange TestAttacker at (2100, 0) may be beyond the top of the frame at the spawn; the us054_combat screenshots at the fight spot show it.
  - DevTestMap: unchanged.
  - Tier 4: every `bridge_NN.png` of the scripts above, against what each report says it must show.
- **Proof:**
  - AC-100.3: the three Tier 3 boots.
  - AC-100.4: the kit check `map` PASS in every Tier 4 report.
  - AC-100.5: the 14 checks below in the sweep's Tier 4 runs, and no baseline PASS → FAIL.
  - AC-100.1, AC-100.2, per clause as each AC's "Verified by" says: Tier 1 `Mordecai.Level.TestMaps.*` (World Settings, partition, PlayerStart and its yaw, floor and wall boxes, WorldGridMaterial, Movable lights, steps, lane, enemies, CDO stats, placement rule, navmesh presence and bounds, body materials and colours); the `Scripts/us100_02_create_test_maps.py` read-back for the navmesh path (0, 0) → (2000, 0); Tier 2 `Scripts/verify_pie_test_maps.py`; Tier 3 `engine_NN.png` for the grid floor and the body colours on screen.
  - AC-100.6: the doc entries.
  - AC-100.7: `check_content_committed.py`.
- **Tier 4 script:** none of its own (no input-driven AC owned by this story). The owning stories' scripts run with `py Scripts/playtest_story.py --all`.
- **Steps:** the issue's 14 checks, in the owning stories' scripts. The rows come from those stories' `--plan` tables, which are authoritative.

| Script | Check | Map | Player step (bridge calls) → expected | Sweep `f21525f` (`20261028-090808-sweep`) | Sweep `01abb8b` (`20261023-192402-sweep`) |
|---|---|---|---|---|---|
| us001 | `move_s` | MovementTestMap | hold S 0.4s -> > 100 cm along -X, < 30 cm across | FAIL (NEW): 394 cm along, 33 across (shoved) | PASS |
| us001 | `walk_speed_baseline` | MovementTestMap | from (-700, -800): hold W 1s -> dx in [400, 700] cm, stopped within 0.75s | PASS | PASS |
| us001 | `walk_lane_reset` | MovementTestMap | walk_to(-700, -800) -> within 60 cm | PASS | PASS |
| us001 | `walk_lane_reset2` | MovementTestMap | walk_to(-700, -800) -> within 60 cm | FAIL (NEW): the player dead at (-523, -807) | PASS |
| us001 | `jump_space` | MovementTestMap | tap SpaceBar -> movement_mode=="Falling" within 1s | FAIL (NEW): movement_mode NULL (dead) | PASS |
| us054_combat | `walk_to_dummy` (was `walk_to_frontliner3`) | CombatTestMap | walk_to(440, 600) -> within 25 cm (the melee spot, 160 cm -X of the TestDummy) | `walk_to_frontliner3` FAIL (NEW): dead on leg 3 at (-301, -497) | `walk_to_frontliner3` PASS |
| us054_combat | `melee_lmb` | CombatTestMap | click Left at (640, 360) -> within 1s 'Mordecai.State.Attacking' or the dummy's health falls | FAIL (NEW): tags ['Mordecai.State.Dead'] | PASS |
| us054_combat | `dodge_lalt` | CombatTestMap | tap LeftAlt -> within 0.5s a Dodging tag or > 150 cm displacement | FAIL (NEW): tags ['Mordecai.State.Dead'] | PASS |
| us054_combat | `arena_reset` | CombatTestMap | at the respawn: both enemies alive, full health, within 100 cm of their spawns, Idle, and the reset observed | FAIL (NEW): re-aggro, 450-497 cm from the spawns | SKIP (no death); PASS in `20261028-085332-us054` |
| us054_combat | `respawn_moves` | CombatTestMap | hold W 0.4s (simulated time, B3) -> x grows >= 80 cm | FAIL (NEW): +70 cm | SKIP (no death); PASS (+175 cm) in `20261028-085332-us054` |
| us057 | `walk_blink_lane` | CombatTestMap | walk_to(0, -600) -> within 60 cm (Blink lane start) | FAIL (NEW): the player dead at (-599, -551) | PASS |
| us078 | `light_attack_lmb` | CombatTestMap | idle and alive -> click Left at (640, 360) -> Attacking without Heavy within 1s | FAIL (NEW): the click reached a dead player | PASS |
| us083 | `block_over_held_rt_no_parry` | MovementTestMap | RT=1, 0.15s later LT=1 -> Blocking within 1s, no parry tag for 0.6s | PASS | PASS |
| us083 | `light_attack_rt_tap` | MovementTestMap | idle -> RT=1 held 0.1s -> Attacking without Heavy within 1s | FAIL (NEW): tags ['Mordecai.State.Dead'] | PASS |

  The f21525f sweep also lost these to the swarm, beyond the 14: us001 `move_a`, and us083 `heavy_attack_rt_hold` and `dodge_gamepad_b`. They PASSed in the `01abb8b` sweep, so the no-PASS→FAIL rule covers them too.
- **Playable alone:** YES (no pair). The features it unblocks become PLAYABLE only through their owning stories' exit-0 runs.

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0), including `Mordecai.Level.TestMaps.*`
- [ ] Tier 2: PIE smoke tests pass: `py Scripts/run_pie_verify.py --map <map> --tests spawn,hud,test_maps` for both test maps, and `py Scripts/run_pie_verify.py` on DevTestMap. Direct-call checks are labeled LOGIC.
- [ ] Content: both `.umap`, the three Blueprints and the two material instances are saved and committed in this branch; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST (content, scripts, tooling and tests), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0 and `build_info.json` `maps` lists both test maps
- [ ] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap`, `--map /MordecaiCore/Game/Maps/CombatTestMap` and the default DevTestMap boot each exit 0 — never `--allow-stale`
- [ ] On that same fresh package `py Scripts/playtest_story.py --all`: the 14 checks of AC-100.5 PASS; the kit check `map` PASSes in every Tier 4 run; no baseline PASS → FAIL. Each owning story's `--lint` exits 0. Never `--allow-stale`.
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen vs. "Visible on screen"
- [ ] T1 marked delivered in `Specs/agent_playtest_plan_v1.md` with the run dirs; N14 and the issue #12 sweep entry updated with the results
- [ ] Evidence cited: run dirs (Tier 3 `Saved/Playtests/<run>-boot/report.md` ×3, the sweep `Saved/Playtests/<run>-sweep/summary.md` and its Tier 4 runs) in Completion Notes AND in the body of the `[US-100]` commit that closes the story
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass. A failing playtest is reported honestly and the story stays in progress

## Completion Notes
Pending. Nothing here is evidence yet: no package of this work exists and no playtest has run.

- 2026-10-29: story written; issue #46 is in progress on branch `fix/issue-46-test-maps`. Decisions recorded: crouch out of scope; us069 moves to MovementTestMap; us079 stays on DevTestMap (`Specs/design_decisions_log.md`). N32 filed from the source audit.
- To fill in:
  - the exact `Mordecai.Level.TestMaps.*` test names and the Tier 1 red/green logs
  - the RESULT lines of `Scripts/us100_01_create_test_enemies.py` and `Scripts/us100_02_create_test_maps.py` (the us100_02 read-back is the evidence for the navmesh path (0, 0) → (2000, 0))
  - the Tier 2 runs
  - `check_content_committed.py`
  - the package log and `build_info.json` `maps`
  - the three Tier 3 boot run dirs with every `engine_NN.png` described
  - the sweep dir with every check of AC-100.5 and every `bridge_NN.png` described
  - the PASS → FAIL comparison against the baselines
  - the dodge displacements us083 measured on MovementTestMap (N27)
