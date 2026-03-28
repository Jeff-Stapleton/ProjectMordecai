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
- [ ] AC-054.13: The full loop works end-to-end: launch game → diorama camera → move/aim → attack enemies → take damage → see HUD update → kill enemy → die → respawn → enemies reset

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

- [ ] Manual: Launch PIE, verify diorama camera follows player
- [ ] Manual: Move with WASD/Left Stick, aim with Mouse/Right Stick
- [ ] Manual: Attack enemies with LMB/RT, see 3-hit combo
- [ ] Manual: Dodge with LAlt/B, observe i-frames
- [ ] Manual: Block with RMB/LT, see damage reduction
- [ ] Manual: See HUD health/stamina/posture update in real-time
- [ ] Manual: Kill an enemy, see enemy health bar deplete and enemy dies
- [ ] Manual: Take damage from enemy attacks, see player health bar drop
- [ ] Manual: Die, wait for respawn, verify arena resets

---

## Definition of Done
- [ ] All DataAssets created and configured
- [ ] Enemy BP created and configured with AI controller
- [ ] Player BP configured with combat abilities and input bindings
- [ ] HUD widget BP created and displaying on screen
- [ ] DevTestMap has arena layout with 3 enemies placed
- [ ] Full combat loop verified in PIE (launch → fight → die → respawn)
- [ ] Code/assets committed and pushed with `[US-054]` prefix
