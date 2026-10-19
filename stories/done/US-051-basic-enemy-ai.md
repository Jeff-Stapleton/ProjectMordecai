# US-051: Basic Enemy AI Combat Loop

## Overview
Implement a simple C++ state-machine AI controller for enemies that detects the player, approaches, attacks, and recovers. This gives the vertical slice an enemy that fights back — essential for exercising dodge/block/parry mechanics.

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `enemy_ai_framework_v1.md` (behavior loop: Idle → Detect → Engage → Attack → Recover → Repeat)
- Design doc: `enemy_archetypes_v1.md` (aggro range, leash range, aggression styles, telegraph clarity)
- Design doc: `combat_system_v1.md` (posture, readability)
- Design doc: `game_design_v2.md` (readable patterns, positioning, cadence)
- Design decisions: `design_decisions_log.md` (timing values)
- Agent rules: `agent_rules_v2.md` (server-authoritative, GAS for abilities)
- Existing code: `UMordecaiGA_MeleeAttack`, `UMordecaiAttackProfileDataAsset`, `AMordecaiEnemyCharacter` (US-050)

---

## Acceptance Criteria
- [x] AC-051.1: `AMordecaiEnemyAIController` class exists, extending `AAIController`
- [x] AC-051.2: AI uses a C++ state machine with states: `Idle`, `Approach`, `Attack`, `Recover`, `Staggered`, `Leash`, `Dead`
- [x] AC-051.3: In `Idle` state, enemy stands at spawn point. When player enters `AggroRange` (configurable, default 900cm), transitions to `Approach`
- [x] AC-051.4: In `Approach` state, enemy moves toward the player using `MoveToActor()`. When within `AttackRange` (configurable, default 200cm), transitions to `Attack`
- [x] AC-051.5: In `Attack` state, enemy activates its melee attack ability (via ASC `TryActivateAbilityByClass`). After the attack completes (Recovery phase ends), transitions to `Recover`
- [x] AC-051.6: In `Recover` state, enemy waits for `AttackCooldownMs` (configurable, default 1500ms) before deciding next action (approach again or attack if still in range)
- [x] AC-051.7: When enemy enters `Mordecai.State.PostureBroken`, AI transitions to `Staggered` state and pauses all actions until the tag is removed
- [x] AC-051.8: When enemy's `Mordecai.State.Dead` tag is applied, AI transitions to `Dead` state permanently (no further actions)
- [x] AC-051.9: If player distance exceeds `LeashRange` (configurable, default 1800cm), enemy transitions to `Leash` state and walks back toward spawn point. Upon reaching spawn, transitions to `Idle`
- [x] AC-051.10: Enemy faces the player during `Approach` and `Attack` states (uses `SetFocalPoint` or rotation toward target)
- [x] AC-051.11: All range/timing parameters are exposed as UPROPERTY for per-enemy tuning
- [x] AC-051.12: The state machine is tick-driven (`Tick()` on the controller) and respects server authority (only runs on server/authority)

## Technical Notes
- **Class location:** `Source/LyraGame/Mordecai/Enemy/MordecaiEnemyAIController.h/.cpp`
- **State machine:** Use a simple `EMordecaiEnemyAIState` enum + switch in `Tick()`. No behavior tree needed for the vertical slice — keep it simple and headless-testable.
- **State enum location:** `Source/LyraGame/Mordecai/Enemy/MordecaiEnemyAITypes.h`
- The AI controller should cache a reference to the possessed `AMordecaiEnemyCharacter` and its ASC.
- **Attack execution:** The AI calls `ASC->TryActivateAbilityByClass(MeleeAttackAbilityClass)` where `MeleeAttackAbilityClass` is a configurable UPROPERTY. The attack profile data asset on the ability determines the actual attack behavior.
- **Detection:** Use simple distance checks in Tick, not perception system. Keep it minimal for the vertical slice.
- **Leash:** Store `SpawnLocation` in `BeginPlay()`. In Leash state, `MoveToLocation(SpawnLocation)`.
- **Tag listening:** Register for `Mordecai.State.PostureBroken` and `Mordecai.State.Dead` tag changes via `ASC->RegisterGameplayTagEvent()` to trigger state transitions reactively.
- Do NOT implement behavior trees, blackboards, or UE AI perception. Those are future work for proper enemy archetypes.
- Do NOT implement multiple attack patterns or flanking. One melee attack is sufficient for the vertical slice.

## Tests Required
- [x] `Mordecai.EnemyAI.IdleStateAtSpawn` — enemy starts in Idle state (verifies AC-051.2)
- [x] `Mordecai.EnemyAI.TransitionsToApproachOnPlayerInRange` — detects player within aggro range (verifies AC-051.3)
- [x] `Mordecai.EnemyAI.StaysIdleWhenPlayerOutOfRange` — no transition when player beyond aggro range (verifies AC-051.3)
- [x] `Mordecai.EnemyAI.TransitionsToAttackAtAttackRange` — approaches then attacks (verifies AC-051.4, AC-051.5)
- [x] `Mordecai.EnemyAI.RecoverStateAfterAttack` — cooldown between attacks (verifies AC-051.6)
- [x] `Mordecai.EnemyAI.StaggeredOnPostureBroken` — pauses during posture break (verifies AC-051.7)
- [x] `Mordecai.EnemyAI.DeadStateOnDeath` — permanent stop on death (verifies AC-051.8)
- [x] `Mordecai.EnemyAI.LeashWhenPlayerTooFar` — returns to spawn (verifies AC-051.9)
- [x] `Mordecai.EnemyAI.FacesPlayerDuringCombat` — rotation toward target (verifies AC-051.10)
- [x] `Mordecai.EnemyAI.RangesAreConfigurable` — custom aggro/attack/leash ranges (verifies AC-051.11)
- [x] `Mordecai.EnemyAI.OnlyRunsOnServer` — state machine does not tick on clients (verifies AC-051.12)

---

## Definition of Done
- [x] All tests written and failing (red phase confirmed)
- [x] All implementation complete
- [x] All tests passing (green)
- [x] Project compiles with zero errors
- [x] Code committed and pushed with `[US-051]` prefix
