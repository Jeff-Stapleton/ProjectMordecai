# US-003: Melee Hit Detection System

## Overview
Implement the spatial query system for the three melee attack shapes (sweep/thrust/slam) plus target filtering and airborne interaction rules. This is the geometry layer that all melee attacks use to determine what they hit. Depends on the data types from US-002.

## References
- Design doc: `attack_taxonomy_v1.md` — Sections 3.1-3.3 (Sweep, Thrust, Slam implementation details)
- Design doc: `player_attacks_agent_brief_v1.md` — Sections 3.1-3.3 (shape specs, agent tasks)
- Design doc: `combat_system_v1.md` — Defense section (dodge i-frames, block)
- Agent rules: `agent_rules_v2.md` — GAS for effects; UE 5.7; server-authoritative with Iris

---

## Acceptance Criteria
- [ ] AC-003.1: `UMordecaiHitDetectionSubsystem` (or static utility class) exists with three overlap query functions:
  - `ArcSectorOverlapQuery(Origin, Forward, Radius, AngleDegrees, StartAngleOffset)` → returns array of hit actors
  - `CapsuleOverlapQuery(Origin, Forward, Length, Width)` → returns array of hit actors
  - `CircleOverlapQuery(ImpactPoint, Radius)` → returns array of hit actors
- [ ] AC-003.2: A unified `PerformMeleeHitDetection(AttackProfile, Origin, Forward)` function dispatches to the correct shape query based on `HitShapeType`
- [ ] AC-003.3: Arc sector query correctly detects actors within the defined arc and ignores actors outside it
- [ ] AC-003.4: Capsule query correctly detects actors within the forward lane and ignores actors to the side
- [ ] AC-003.5: Circle query correctly detects actors within the radius and ignores actors outside it
- [ ] AC-003.6: Target filtering supports an `EMordecaiTargetFilter` (Enemies, Destructibles, All) and excludes the attacker
- [ ] AC-003.7: If `JumpAvoidable=true` on the attack AND target has gameplay tag `Mordecai.State.Airborne`, the target is excluded from hit results
- [ ] AC-003.8: If `HitsAirborne=true` on the attack, airborne targets are included normally (thrusts/slams)
- [ ] AC-003.9: Friendly fire is off by default; a bool parameter allows overriding
- [ ] AC-003.10: All queries use UE collision channels (not manual distance checks) for performance
- [ ] AC-003.11: Hit results return `FMordecaiHitResult` structs containing: `HitActor`, `HitLocation`, `HitNormal`, `bWasAirborne`

## Technical Notes
- Place in `Plugins/GameFeatures/MordecaiCore/Source/MordecaiCoreRuntime/Combat/`
- Use `UWorldSubsystem` for `UMordecaiHitDetectionSubsystem` — gives world access without requiring an actor
- Arc sector: use `UWorld::OverlapMultiByChannel` with sphere sweep at `Radius`, then filter by angle between `Forward` and direction-to-target. The angle check is: `FMath::Acos(Forward.Dot(DirToTarget)) <= HalfAngle`
- Capsule/Box: use `UWorld::SweepMultiByChannel` with appropriate collision shape
- Circle: use `UWorld::OverlapMultiByChannel` with sphere at `ImpactPoint`
- Use a dedicated collision channel or object type for hit detection (e.g., `ECC_GameTraceChannel1`) — document the channel assignment
- Airborne check: look for `Mordecai.State.Airborne` gameplay tag on the target's ASC
- All queries should be server-authoritative (no client-side hit detection for now)
- This story does NOT apply damage or effects — it only returns what was hit. US-004 handles the GAS integration.

## Tests Required
- [ ] `Mordecai.HitDetection.ArcSectorHitsActorInArc` — places actor at 45° in a 90° arc, verifies hit (AC-003.3)
- [ ] `Mordecai.HitDetection.ArcSectorMissesActorOutsideArc` — places actor at 120° in a 90° arc, verifies miss (AC-003.3)
- [ ] `Mordecai.HitDetection.ArcSectorMissesActorBeyondRadius` — actor in angle but past radius, verifies miss (AC-003.3)
- [ ] `Mordecai.HitDetection.CapsuleHitsActorInLane` — actor directly ahead within lane, verifies hit (AC-003.4)
- [ ] `Mordecai.HitDetection.CapsuleMissesActorBesideLane` — actor perpendicular to lane, verifies miss (AC-003.4)
- [ ] `Mordecai.HitDetection.CircleHitsActorInRadius` — actor within circle, verifies hit (AC-003.5)
- [ ] `Mordecai.HitDetection.CircleMissesActorOutsideRadius` — actor outside circle, verifies miss (AC-003.5)
- [ ] `Mordecai.HitDetection.JumpAvoidableSkipsAirborneTarget` — sweep with JumpAvoidable=true vs airborne target, verifies no hit (AC-003.7)
- [ ] `Mordecai.HitDetection.JumpAvoidableHitsGroundedTarget` — sweep with JumpAvoidable=true vs grounded target, verifies hit (AC-003.7)
- [ ] `Mordecai.HitDetection.HitsAirborneIncludesJumpingTarget` — thrust with HitsAirborne=true vs airborne target, verifies hit (AC-003.8)
- [ ] `Mordecai.HitDetection.AttackerExcludedFromResults` — attacker standing in own sweep area, verifies not hit (AC-003.6)
- [ ] `Mordecai.HitDetection.FriendlyFireOffExcludesAllies` — friendly actor in range with FF off, verifies no hit (AC-003.9)
- [ ] `Mordecai.HitDetection.UnifiedDispatchUsesCorrectShape` — calls PerformMeleeHitDetection with each AttackType, verifies correct query used (AC-003.2)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] `Scripts/Verify.bat` passes (tests + build)
- [ ] Code committed and pushed with `[US-003]` prefix
