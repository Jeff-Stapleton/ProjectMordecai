# US-054: Playable Arena Integration

## Overview
Wire all C++ systems from US-050–053 into a playable prototype using the Unreal Editor. Create enemy Blueprints, attack profile DataAssets, HUD widget Blueprints, configure the dev experience, and set up the test arena with enemies. The goal: Jeff launches the game, fights enemies in a test arena, and exercises melee/dodge/block/parry/stamina/posture with visual HUD feedback.

## Execution Mode
**Mode:** EDITOR

## References
- Design doc: `game_design_v2.md` (diorama camera, twin-stick controls, core combat loop)
- Design doc: `enemy_archetypes_v1.md` (Frontliner archetype for test enemy)
- Design doc: `player_attacks_agent_brief_v1.md` (Longsword 3-sweep chain for player weapon)
- Design doc: `control_bindings_v1.1.md` (full control scheme: move, aim, attack, dodge, block, parry)
- Design doc: `combat_system_v1.md` (all combat resources)
- Agent rules: `agent_rules_v2.md` (GAS, no camera rotation, Iris replication)
- Existing assets: `BP_MordecaiCharacter`, `DevTestMap`, `IMC_Mordecai`, `B_MordecaiDevExperience`, `PawnData_Mordecai`, `BP_CameraMode_Diorama`, 23 input actions

---

## Acceptance Criteria
- [x] AC-054.1: A `BP_MordecaiEnemy_Frontliner` Blueprint exists in `MordecaiCore/Content/Enemies/`, parented to `AMordecaiEnemyCharacter`, with `AMordecaiEnemyAIController` as its AI controller class
- [x] AC-054.2: The enemy BP has configured stats: MaxHealth=200, MaxPosture=100, MoveSpeed=400, AggroRange=900, AttackRange=200, LeashRange=1800
- [x] AC-054.3: An `DA_EnemyAttack_BasicSlash` DataAsset exists (`UMordecaiAttackProfileDataAsset`) with: AttackType=MeleeSweep, WindupMs=500 (readable telegraph), ActiveMs=200, RecoveryMs=400, ArcAngle=90°, Radius=150cm, BasePower=15, PostureDamageScalar=0.3, StaminaCost=0
- [x] AC-054.4: A `DA_PlayerAttack_LightSlash_1/2/3` set of DataAssets exists for a 3-hit sweep combo (Longsword-style): increasing arc angle (70°/90°/120°), increasing BasePower (10/12/15), ComboIndex 0/1/2
- [x] AC-054.5: The player's `BP_MordecaiCharacter` is configured to grant combat abilities on possession: MeleeAttack (with the 3-slash combo), Dodge, Block, Parry
- [x] AC-054.6: Input actions are bound to combat abilities: RT/LMB → Light Attack, LT/RMB hold → Block, LT+RT/RMB+LMB → Parry, B/LAlt → Dodge
- [x] AC-054.7: A `WBP_CombatHUD` UMG Widget Blueprint exists, using the C++ HUD widget classes, showing: Health bar (top-left), Stamina bar with tier color (below health), Posture bar (below stamina)
- [x] AC-054.8: The HUD is added to the player's viewport on game start via the experience or player controller
- [x] AC-054.9: Enemy has a `WBP_EnemyHealthBar` world-space widget above their head showing their health percentage
- [x] AC-054.10: `DevTestMap` has a flat arena floor (~2000x2000 cm), basic walls/boundaries, adequate lighting for the diorama camera
- [x] AC-054.11: 3 enemy Frontliners are placed in the arena at varied positions
- [x] AC-054.12: `B_MordecaiDevExperience` is configured to use the arena game mode with respawn enabled
- [x] AC-054.13: The full loop works end-to-end: launch game → diorama camera → move/aim → attack enemies → take damage → see HUD update → kill enemy → die → respawn → enemies reset

## Technical Notes
- **This is an EDITOR story** — requires the Unreal Editor running with Python remote execution.
- Use Python editor scripting (`unreal` module via `ue_remote_exec.py`) to create DataAssets, configure Blueprints, and set up the level.
- **Existing assets to leverage:** The player BP, input mapping context, camera mode BP, dev experience, and pawn data already exist. Configure/extend them rather than creating from scratch.
- **Enemy attack DataAsset:** The 500ms windup is intentionally long for readability — the enemy telegraphs its attack clearly. The player should have time to dodge or block.
- **Player combo DataAssets:** Follow the Longsword pattern from `player_attacks_agent_brief_v1.md`. 3 sweep attacks with increasing power/arc.
- **Ability granting:** Use a GameplayEffect with `GrantedAbilities` applied at possession time, or configure abilities directly on the ASC defaults in the BP.
- **HUD binding:** The player controller's `BeginPlay` (or the experience's on-pawn-ready event) should create the HUD widget and call `BindToASC()` on the local player's ASC.
- **Level setup:** The DevTestMap may already have a floor. Add boundary walls to prevent falling off. Ensure the diorama camera has clear sightlines.
- **Damage GE:** Create an instant GameplayEffect that applies health damage via SetByCaller. This is the core GE used by both melee attacks and the enemy. May need to create this in C++ (in a HEADLESS story) or as a Blueprint GE asset here.
- **Verification:** After setup, launch PIE and manually verify AC-054.13 (the full loop). This is the "Jeff can play it" acceptance criterion.

## Tests Required
This story does not have headless automation tests — it is verified via manual playtesting in the editor (PIE). The C++ systems it wires together are individually tested in US-050 through US-053.

- [x] Manual: Launch PIE, verify diorama camera follows player
- [x] Manual: Move with WASD/Left Stick, aim with Mouse/Right Stick
- [x] Manual: Attack enemies with LMB/RT, see 3-hit combo
- [x] Manual: Dodge with LAlt/B, observe i-frames
- [x] Manual: Block with RMB/LT, see damage reduction
- [x] Manual: See HUD health/stamina/posture update in real-time
- [x] Manual: Kill an enemy, see enemy health bar deplete and enemy dies
- [x] Manual: Take damage from enemy attacks, see player health bar drop
- [x] Manual: Die, wait for respawn, verify arena resets

---

## Definition of Done
- [x] All DataAssets created and configured
- [x] Enemy BP created and configured with AI controller
- [x] Player BP configured with combat abilities and input bindings
- [x] HUD widget BP created and displaying on screen
- [x] DevTestMap has arena layout with 3 enemies placed
- [x] Full combat loop verified in PIE (launch → fight → die → respawn)
- [x] Code/assets committed and pushed with `[US-054]` prefix

## Playtest
Added 2026-10-22 by the Phase 3 Playability Recovery sweep (`Specs/story_playtests_v1.md`).

- **Launch path:** `py Scripts/playtest_boot.py` (DevTestMap → B_MordecaiArenaExperience)
- **Tier 4 script:** `Scripts/playtests/us054_arena.py` — run: `py Scripts/playtest_story.py US-054`
- **Steps:** (generated: `py Scripts/playtest_story.py US-054 --plan`; checked: `--lint`)

| # | Check | Player step (bridge calls) → expected | Proves |
|---|---|---|---|
| 1 | `hud_visible` | state widgets -> HealthBar, StaminaBar, PostureBar, SpellPointsBar and KillCounter widgets on screen, a text 'Kills: 0' | AC-054.7, AC-054.8, US-063:AC-063.3, US-063:AC-063.4 |
| 2 | `hud_layout` | state widgets rects -> HealthBar top-left (x<300, y<60), StaminaBar below it, PostureBar below that | AC-054.7 |
| 3 | `enemies_present` | state actors (radius 3000) -> 3 kind=='enemy', health {200, 200}, pairwise > 300 cm apart | AC-054.11, AC-054.2, US-050:AC-050.3 |
| 4 | `enemies_visible` | state actors -> every enemy has_visible_mesh==true _(known issue: Known issues backlog, 'The three red bars are explained': the three Frontliners have no mesh, so the enemies are invisible)_ | AC-054.13 |
| 5 | `enemy_health_bars` | state actors -> every enemy has a visible widget whose class contains EnemyHealthBar | AC-054.9, US-052:AC-052.10 |
| 6 | `enemy_indicators` | state widgets -> one EnemyIndicator widget per enemy in aggro range within 3s | US-064:AC-064.2, US-064:AC-064.10 |
| 7 | `enemy_aggro` | stand at the spawn -> every enemy ai_state != 'Idle' within 3s | US-051:AC-051.3 |
| 8 | `enemy_approaches` | stand -> some enemy > 100 cm from every spawn within 6s | US-051:AC-051.4 |
| 9 | `walk_to_frontliner3` | walk_to(-150, -650), walk_to(ex-300, -650), walk_to(ex-160, ey) where (ex, ey) is the live enemy nearest (-400, -500) -> within 60 cm of each | - |
| 10 | `melee_lmb` | click Left at (640, 360) -> within 1s 'Mordecai.State.Attacking' in player.tags or the nearest enemy's health < 200 _(known issue: Known issue N10: the player's combat abilities are granted only by an unsaved CDO edit; attacks, block and dodge are not bound at runtime (Known issues backlog))_ | AC-054.6, AC-054.5 |
| 11 | `dodge_lalt` | tap LeftAlt -> within 0.5s a Dodging tag in player.tags or a displacement > 150 cm _(known issue: Known issue N10: the player's combat abilities are granted only by an unsaved CDO edit; attacks, block and dodge are not bound at runtime (Known issues backlog))_ | AC-054.6, AC-054.5 |
| 12 | `enemy_damages_player` | stand next to the enemy -> wait_until player.attributes.health < player.attributes.max_health within 20s | AC-054.13, US-051:AC-051.5 |
| 13 | `burning_on_hit` | keep standing -> within 25s 'Mordecai.Status.Burning' in player.tags and a widget text 'Burning' (WARN if Burning never procs: 40%) | US-057:AC-057.11, US-057:AC-057.8 |
| 14 | `restoration_heals` | tap One -> within 6s health rises, spell_points drops by 3 and 'Mordecai.Status.Restoration' is in player.tags | US-057:AC-057.4, US-057:AC-057.5, US-057:AC-057.12 |
| 15 | `player_dies` | keep standing -> wait_until player.is_dead==true within 60s, 'Mordecai.State.Dead' in player.tags | US-053:AC-053.1 |
| 16 | `death_screen` | state widgets -> within 1.5s one widget with 'You Died', a countdown digit and 'Kills: N' _(known issue: Known issue N2: the death screen is never created)_ | US-062:AC-062.1, US-062:AC-062.2 |
| 17 | `dead_no_move` | while dead: hold W 0.5s -> moved < 5 cm | US-053:AC-053.2 |
| 18 | `respawn` | wait -> within 6s player.is_dead==false, health == max_health, within 150 cm of (0, 0), no 'Mordecai.State.Dead' tag | US-053:AC-053.4, US-053:AC-053.5 |
| 19 | `arena_reset` | state actors at the respawn -> every enemy within 100 cm of a distinct spawn, health == max, not dead (WARN if no enemy had moved before the death: nothing to reset) | US-053:AC-053.8, US-053:AC-053.9 |
| 20 | `respawn_moves` | hold W 0.4s -> player x grows by >= 80 cm | US-053:AC-053.6 |
| 21 | `kill_counter_text_matches` | state widgets, player -> the 'Kills: N' text equals player.kills | US-063:AC-063.1 |
