# US-050: Enemy Character & Damage Reception

## Overview
Create the foundational enemy character class that can receive damage, die, and have its posture broken. This is the first step toward a playable vertical slice — without a damageable enemy, no combat system can be exercised.

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `enemy_archetypes_v1.md` (enemy schema, core stats, tier system)
- Design doc: `combat_system_v1.md` (posture system, damage formula)
- Design doc: `game_design_v2.md` (skill-first combat, posture model)
- Design decisions: `design_decisions_log.md` (posture break duration 1000ms, stagger multiplier 1.5x, knock down 2.0x)
- Agent rules: `agent_rules_v2.md` (GAS for all attributes/abilities, server-authoritative, Iris replication)
- Existing code: `MordecaiAttributeSet`, `MordecaiPostureSystem`, `MordecaiStaminaSystem`, `MordecaiGameplayTags`

---

## Acceptance Criteria
- [ ] AC-050.1: `AMordecaiEnemyCharacter` class exists, extending `ALyraCharacter`
- [ ] AC-050.2: Enemy owns a `UMordecaiAbilitySystemComponent` initialized in `PossessedBy()` / `BeginPlay()`
- [ ] AC-050.3: Enemy has a `UMordecaiAttributeSet` with Health, MaxHealth, Posture, MaxPosture attributes initialized to configurable defaults
- [ ] AC-050.4: Enemy actor has the `Mordecai.Team.Enemy` gameplay tag applied at spawn
- [ ] AC-050.5: Applying a health-damage GameplayEffect (SetByCaller `Mordecai.SetByCaller.HealthDamage`) reduces the enemy's Health attribute by the specified amount
- [ ] AC-050.6: When Health reaches ≤ 0, the enemy enters a death state: `Mordecai.State.Dead` tag is applied, movement is disabled, collision profile changes to "Ragdoll"/"NoCollision", and a `Mordecai.Event.Death` gameplay event is broadcast
- [ ] AC-050.7: Applying a posture-damage GameplayEffect (SetByCaller `Mordecai.SetByCaller.PostureDamage`) reduces the enemy's Posture attribute
- [ ] AC-050.8: When Posture reaches ≤ 0, the enemy enters PostureBroken stagger state (`Mordecai.State.PostureBroken` tag applied) for `PostureBrokenDurationMs` (default 1000ms per design_decisions_log)
- [ ] AC-050.9: Posture regenerates at `PostureRegenRate` (default 10/sec) after `PostureRegenDelayMs` (default 3000ms) of not taking posture damage, matching existing `UMordecaiPostureSystem` behavior
- [ ] AC-050.10: Enemy base stats (MaxHealth, MaxPosture, MoveSpeed) are configurable via UPROPERTY defaults on the class
- [ ] AC-050.11: Death state and PostureBroken state are replicated to clients via gameplay tags on the ASC
- [ ] AC-050.12: A dead enemy does not take further damage (Health clamped at 0, damage GEs rejected or no-op)

## Technical Notes
- **Class location:** `Source/LyraGame/Mordecai/Enemy/MordecaiEnemyCharacter.h/.cpp`
- The enemy should initialize its own ASC (not on PlayerState like the player). The ASC lives directly on the enemy actor.
- Reuse `UMordecaiAttributeSet` — enemies use the same attribute set as players. Only initialize the attributes relevant to enemies (Health, MaxHealth, Posture, MaxPosture for now).
- Death handling: listen for `Health` attribute change via `GetGameplayAttributeValueChangeDelegate()`. When Health ≤ 0, apply death state.
- Posture break: integrate with existing `UMordecaiPostureSystem` if possible, or replicate the posture-break-on-zero logic directly since the enemy owns its own ASC.
- The `Mordecai.State.Dead` and `Mordecai.Event.Death` gameplay tags must be declared in `MordecaiGameplayTags.h/.cpp`.
- Do NOT implement AI behavior in this story — the enemy is a "training dummy" that can be damaged and killed. AI comes in US-051.
- Do NOT create Blueprint assets — this is pure C++. The EDITOR story (US-054) handles BP creation.

## Tests Required
- [ ] `Mordecai.Enemy.CharacterHasAbilitySystemComponent` — verifies AC-050.2
- [ ] `Mordecai.Enemy.CharacterHasAttributeSet` — verifies AC-050.3
- [ ] `Mordecai.Enemy.CharacterHasEnemyTeamTag` — verifies AC-050.4
- [ ] `Mordecai.Enemy.HealthDamageReducesHealth` — verifies AC-050.5
- [ ] `Mordecai.Enemy.DeathTriggeredAtZeroHealth` — verifies AC-050.6
- [ ] `Mordecai.Enemy.DeathDisablesMovement` — verifies AC-050.6
- [ ] `Mordecai.Enemy.DeathBroadcastsEvent` — verifies AC-050.6
- [ ] `Mordecai.Enemy.PostureDamageReducesPosture` — verifies AC-050.7
- [ ] `Mordecai.Enemy.PostureBreakAtZero` — verifies AC-050.8
- [ ] `Mordecai.Enemy.PostureRegeneratesAfterDelay` — verifies AC-050.9
- [ ] `Mordecai.Enemy.BaseStatsConfigurable` — verifies AC-050.10
- [ ] `Mordecai.Enemy.DeadEnemyRejectsDamage` — verifies AC-050.12

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-050]` prefix
