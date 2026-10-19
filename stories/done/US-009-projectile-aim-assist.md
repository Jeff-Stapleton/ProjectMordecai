# US-009: Projectile System & Aim Assist

## Execution Mode
**Mode:** HEADLESS

## Overview
Implement projectile actors and a soft aim assist system for ranged attacks. Projectiles use the `FMordecaiProjectileSpec` from US-002 to define flight behavior (speed, gravity, lifetime, pierce, ricochet, on-hit AoE). Aim assist provides subtle angular correction when firing — no lock-on or snapping. The aim assist algorithm builds candidate directions in small angular offsets, raycasts each, and picks the best match to slightly rotate the firing direction toward a valid target.

## References
- Design doc: `attack_taxonomy_v1.md` — Projectile system details, ProjectileSpec fields
- Design doc: `player_attacks_agent_brief_v1.md` — Aim assist algorithm (candidate directions), projectile agent tasks
- Design doc: `combat_system_v1.md` — Ranged attack damage, stamina costs
- Design doc: `control_bindings_v1.1.md` — Right stick / mouse = aim direction (no lock-on)
- Design doc: `game_design_v2.md` — Subtle aim assist, no auto-aim
- Agent rules: `agent_rules_v2.md` — GAS for effects; server-authoritative with Iris; UE 5.7

---

## Acceptance Criteria

### Projectile Actor
- [x] AC-009.1: `AMordecaiProjectile` actor exists with a `UProjectileMovementComponent` (or custom movement) configurable via `FMordecaiProjectileSpec`
- [x] AC-009.2: Projectile moves forward at `Speed` units/second from spawn
- [x] AC-009.3: If `GravityDrop > 0`, projectile trajectory curves downward over time (applies gravity scale to ProjectileMovementComponent)
- [x] AC-009.4: Projectile auto-destroys after `Lifetime` seconds OR after traveling `MaxRange` units (whichever comes first). All projectiles and attacks should have a max range — nothing should fly much further than the edge of the screen. `MaxRange` is configurable per ProjectileSpec (default placeholder: 2000 units, tunable per weapon type in Epic 6)
- [x] AC-009.5: On overlap with a valid target (enemy/destructible), projectile triggers hit logic: applies `DamageProfile` from its associated attack profile via Gameplay Effect
- [x] AC-009.6: Projectile ignores the shooter (instigator excluded from hit detection)
- [x] AC-009.7: Projectile uses a dedicated collision channel for projectile traces

### Pierce
- [x] AC-009.8: If `PierceCount > 0`, projectile passes through hit targets and continues flying. Each pierce decrements a counter. When counter reaches 0, projectile stops on next hit.
- [x] AC-009.9: If `PierceCount == 0`, projectile destroys on first hit (default behavior)
- [x] AC-009.10: Each pierced target receives full 100% damage — no falloff per pierce. Damage stays constant for all targets hit. (DECISION: confirmed no falloff)

### Ricochet
- [x] AC-009.11: If `RicochetCount > 0`, on hit, projectile searches for the nearest valid target within `RicochetRange` (radius around hit point) that is within `RicochetAngleLimit` degrees of the projectile's current forward direction
- [x] AC-009.12: If a ricochet target is found, projectile redirects toward it at the same speed. Ricochet counter decrements.
- [x] AC-009.13: If no valid ricochet target is found, projectile destroys (does not ricochet into empty space)
- [x] AC-009.14: Ricochet does not re-hit already-hit targets (maintain a hit list per projectile)

### On-Hit AoE
- [x] AC-009.15: If `OnHitAoERadius > 0`, on hit (or final hit if piercing), spawn a circle overlap at the hit location with radius `OnHitAoERadius`
- [x] AC-009.16: AoE damage = `BasePower × OnHitAoEDamageScalar` from the projectile spec, applied to all targets in the AoE radius
- [x] AC-009.17: AoE uses the same damage type as the projectile's `DamageProfile`
- [x] AC-009.18: The original hit target is included in AoE damage (hit + AoE stack)

### Aim Assist
- [x] AC-009.19: `UMordecaiAimAssistSubsystem` (UWorldSubsystem) provides a function `ComputeAssistedAimDirection(Origin, AimForward, AssistRange, CandidateAngles)` that returns a corrected aim direction
- [x] AC-009.20: Algorithm builds candidate directions around `AimForward` at angular offsets: 0°, ±2°, ±4°, ±6° (configurable `CandidateAngleOffsets` array, default: {0, 2, 4, 6})
- [x] AC-009.21: Each candidate direction is raycasted to `AssistRange` (configurable, default placeholder: 3000 units)
- [x] AC-009.22: Best candidate selected by: (1) smallest angular offset from AimForward, (2) closest target distance as tiebreaker
- [x] AC-009.23: If a valid candidate is found, the returned direction is rotated toward the best candidate. Correction is subtle — maximum correction angle equals the largest CandidateAngleOffset (default 6°)
- [x] AC-009.24: If no valid candidates hit a target, the original `AimForward` is returned unchanged (fire straight)
- [x] AC-009.25: Aim assist does NOT produce lock-on, target highlighting, or snapping behavior — only direction adjustment before projectile spawn
- [x] AC-009.26: Aim assist only considers actors with `Mordecai.Team.Enemy` tag (or configurable filter) within line of sight

### Integration
- [x] AC-009.27: A ranged attack ability (stub/test ability) can spawn `AMordecaiProjectile` using aim-assisted direction and a `FMordecaiProjectileSpec` from an attack profile
- [x] AC-009.28: Projectile damage application uses the same standardized damage GE pipeline as melee (SetByCaller magnitude, damage type tags)

## Technical Notes
- Place in `Plugins/GameFeatures/MordecaiCore/Source/MordecaiCoreRuntime/Combat/`
- **Projectile actor**: Use `UProjectileMovementComponent` (UE built-in) for flight. Set `InitialSpeed`, `MaxSpeed`, `ProjectileGravityScale` from spec. Use `OnComponentHit` or `OnComponentBeginOverlap` for hit detection.
- **Pierce**: On hit, check pierce counter. If > 0, decrement and ignore the hit (let projectile continue). Add hit actor to a `TSet<AActor*>` to avoid re-hits.
- **Ricochet**: On hit, if ricochet counter > 0, do a sphere overlap at hit location with `RicochetRange`. Filter by angle limit and already-hit list. If found, `SetVelocity` toward the new target.
- **On-hit AoE**: On final hit (or when pierce ends), call the hit detection subsystem's `CircleOverlapQuery` (from US-003) at the hit location with `OnHitAoERadius`. Apply AoE damage GE to all results.
- **Aim assist subsystem**: Pure function — no state. Called before projectile spawn. Uses `UWorld::LineTraceSingleByChannel` for each candidate ray. Return original direction if nothing found.
- **Candidate directions**: Generate in a cone around `AimForward`. For 2D (horizontal plane for fixed camera), generate horizontal offsets only. For 3D, generate a grid. Since the camera is fixed (diorama), horizontal-plane offsets are likely sufficient.
- **Damage application**: Reuse the damage GE pipeline from US-004. Apply `SetByCaller` magnitude for health and posture damage.
- **Spawning**: The ranged attack ability constructs the projectile via `GetWorld()->SpawnActor<AMordecaiProjectile>()`, passing the spec and instigator.
- This story does NOT implement specific ranged weapons (Longbow, Shortbow, etc.) — those are Epic 6 (Weapons & Equipment). This is the generic projectile + aim assist infrastructure.

## Tests Required

### Projectile Tests
- [x] `Mordecai.Projectile.SpawnsAndMovesForward` — spawn projectile, tick, verify position advances along forward vector (AC-009.1, AC-009.2)
- [x] `Mordecai.Projectile.GravityDropCurvesTrajectory` — spawn with GravityDrop > 0, verify downward deflection after tick (AC-009.3)
- [x] `Mordecai.Projectile.DestroysAfterLifetime` — spawn with Lifetime=1.0, tick past 1s, verify actor destroyed (AC-009.4)
- [x] `Mordecai.Projectile.AppliesDamageOnHit` — place target in path, tick until hit, verify target health reduced by BasePower (AC-009.5)
- [x] `Mordecai.Projectile.IgnoresInstigator` — place instigator in path, verify no self-hit (AC-009.6)
- [x] `Mordecai.Projectile.PiercesThroughTargets` — set PierceCount=2, place 3 targets in line, verify first two hit and pierced, third hit and projectile stops (AC-009.8, AC-009.9)
- [x] `Mordecai.Projectile.NoPierceDestroysOnFirstHit` — PierceCount=0, place target, verify projectile destroys on hit (AC-009.9)
- [x] `Mordecai.Projectile.RicochetFindsNextTarget` — set RicochetCount=1, place two targets, hit first, verify projectile redirects to second (AC-009.11, AC-009.12)
- [x] `Mordecai.Projectile.RicochetDestroysIfNoTarget` — set RicochetCount=1, hit target with no other targets in range, verify projectile destroys (AC-009.13)
- [x] `Mordecai.Projectile.RicochetSkipsAlreadyHit` — set RicochetCount=2, place 2 targets, verify no re-hit of first target (AC-009.14)
- [x] `Mordecai.Projectile.OnHitAoEDamagesNearbyTargets` — set OnHitAoERadius=200, place 2 targets (one direct hit, one nearby), verify both take damage (AC-009.15, AC-009.16)
- [x] `Mordecai.Projectile.OnHitAoEUsesCorrectDamageType` — verify AoE damage carries same damage type tag as projectile (AC-009.17)

### Aim Assist Tests
- [x] `Mordecai.AimAssist.FindsBestCandidate` — place target at 3° offset, call ComputeAssistedAimDirection, verify returned direction points toward target (AC-009.19, AC-009.22)
- [x] `Mordecai.AimAssist.PrefersSmallestAngle` — place two targets at 2° and 5° offset, verify aim corrects toward 2° target (AC-009.22)
- [x] `Mordecai.AimAssist.ReturnsOriginalIfNoTargets` — no targets in range, verify returned direction equals AimForward (AC-009.24)
- [x] `Mordecai.AimAssist.DoesNotExceedMaxCorrection` — place target at 10° offset (beyond max 6°), verify no correction applied (AC-009.23)
- [x] `Mordecai.AimAssist.IgnoresFriendlyTargets` — place friendly actor in aim path, verify no correction toward it (AC-009.26)
- [x] `Mordecai.AimAssist.SubtleCorrectionOnly` — verify max correction angle never exceeds largest CandidateAngleOffset (AC-009.25)

---

## Definition of Done
- [x] All tests written and failing (red phase confirmed)
- [x] All implementation complete
- [x] All tests passing (green)
- [x] Project compiles with zero errors
- [x] Code committed and pushed with `[US-009]` prefix
