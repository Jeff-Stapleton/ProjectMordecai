# US-026: Blunt & Polearm Weapon Profiles

## Overview
Define complete attack profiles for the five non-blade melee weapons: Axe, Mace, Spear, Quarterstaff, and Unarmed. Each weapon gets a full light combo chain and heavy attack with weapon-specific shapes, timings, damage values, and combat feel. Extends `UMordecaiWeaponProfileFactory` (from US-025) with five new `Create*()` methods.

This builds directly on the weapon framework from US-024 and follows the identical factory pattern established in US-025.

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `attack_taxonomy_v1.md` (Section 4 — weapon class profiles; Section 1.1 — sweep/thrust/slam shapes and counterplay rules)
- Design doc: `player_attacks_agent_brief_v1.md` (Section 4 — Rapier/Spear 5-thrust chain example; Section 1.1 — shape definitions)
- Design doc: `damage_types_v1.md` (Section 2 — Slash/Pierce/Blunt damage types)
- Design doc: `combat_system_v1.md` (damage formula, stamina cost, posture damage)
- Existing code: `UMordecaiWeaponProfileFactory` (US-025), `UMordecaiWeaponDataAsset` (US-024), `UMordecaiAttackProfileDataAsset` (US-002), `UMordecaiEquipmentComponent` (US-024)
- Agent rules: `agent_rules_v2.md` — GAS, data-driven, `TODO(DECISION)` for open items

---

## Acceptance Criteria

### Factory Extension
- [x] AC-026.1: `UMordecaiWeaponProfileFactory` gains five new static methods: `CreateAxe()`, `CreateMace()`, `CreateSpear()`, `CreateQuarterstaff()`, `CreateUnarmed()`. Each returns a `UMordecaiWeaponDataAsset*` with all attack profiles populated. Same pattern as US-025's blade factory methods.

### Axe — Aggressive slash weapon, frontloaded burst (MainHand)
- [x] AC-026.2: Axe has 2 light attack profiles (wide sweeps) and 1 heavy attack profile (overhead slam). Lights use `EMordecaiDamageType::Slash`, heavy uses `Slash` with `MeleeSlam` attack type. EquipSlot: `MainHand`. GrantedTag: `Mordecai.Weapon.Type.Axe`.

Axe weapon stats: `BaseDamage=6, AttackSpeedMultiplier=0.85, Range=180, PostureDamageBonus=3`

Axe light combo chain:

| Step | AttackType | HitShapeType | Radius | Angle | Windup(ms) | Active(ms) | Recovery(ms) | BasePower | StaminaCost | PostureScalar | Airborne |
|------|-----------|-------------|--------|-------|------------|------------|-------------|-----------|-------------|---------------|----------|
| L1 | MeleeSweep | ArcSector | 180 | 140 | 280 | 180 | 350 | 16 | 12 | 0.7 | JA=true |
| L2 | MeleeSweep | ArcSector | 190 | 160 | 320 | 200 | 400 | 22 | 16 | 0.9 | JA=true |

Axe heavy: MeleeSlam, Circle(R=160), Windup=500, Active=200, Recovery=550, BasePower=30, StaminaCost=24, PostureScalar=1.8, HitsAirborne=true, RootedDuring=Active.

### Mace — Blunt posture-breaker (MainHand)
- [x] AC-026.3: Mace has 2 light attack profiles (side sweep + overhead slam) and 1 heavy attack profile (ground pound slam). All use `EMordecaiDamageType::Blunt`. L1 is `MeleeSweep`/`ArcSector`, L2 is `MeleeSlam`/`Circle`. Heavy is `MeleeSlam`/`Circle`. EquipSlot: `MainHand`. GrantedTag: `Mordecai.Weapon.Type.Mace`.

Mace weapon stats: `BaseDamage=5, AttackSpeedMultiplier=0.8, Range=160, PostureDamageBonus=5`

Mace light combo chain:

| Step | AttackType | HitShapeType | Param1 | Param2 | Windup(ms) | Active(ms) | Recovery(ms) | BasePower | StaminaCost | PostureScalar | Airborne |
|------|-----------|-------------|--------|--------|------------|------------|-------------|-----------|-------------|---------------|----------|
| L1 | MeleeSweep | ArcSector | R=160 | A=150 | 300 | 180 | 400 | 14 | 12 | 1.0 | JA=true |
| L2 | MeleeSlam | Circle | R=140 | — | 320 | 200 | 450 | 18 | 14 | 1.2 | HA=true |

Mace heavy: MeleeSlam, Circle(R=180), Windup=600, Active=200, Recovery=600, BasePower=28, StaminaCost=28, PostureScalar=2.5, HitsAirborne=true, RootedDuring=Active.

### Spear — Pierce polearm, longest melee reach (TwoHand)
- [x] AC-026.4: Spear has 3 light attack profiles (2 thrusts + sweep finisher) and 1 heavy attack profile (charging lunge thrust). L1-L2 are `MeleeThrust`/`Capsule`, L3 is `MeleeSweep`/`ArcSector` (polearm sweep). Heavy is `MeleeThrust`/`Capsule`. All use `EMordecaiDamageType::Pierce`. EquipSlot: `TwoHand`. GrantedTag: `Mordecai.Weapon.Type.Spear`.

Spear weapon stats: `BaseDamage=5, AttackSpeedMultiplier=0.9, Range=280, PostureDamageBonus=2`

Spear light combo chain:

| Step | AttackType | HitShapeType | Param1 | Param2 | Windup(ms) | Active(ms) | Recovery(ms) | BasePower | StaminaCost | PostureScalar | Airborne |
|------|-----------|-------------|--------|--------|------------|------------|-------------|-----------|-------------|---------------|----------|
| L1 | MeleeThrust | Capsule | L=280 | W=50 | 200 | 150 | 280 | 12 | 8 | 0.5 | HA=true |
| L2 | MeleeThrust | Capsule | L=290 | W=55 | 220 | 160 | 280 | 14 | 10 | 0.6 | HA=true |
| L3 | MeleeSweep | ArcSector | R=250 | A=160 | 280 | 200 | 350 | 18 | 14 | 0.8 | JA=true |

Spear heavy: MeleeThrust, Capsule(L=320, W=60), Windup=400, Active=180, Recovery=450, BasePower=26, StaminaCost=22, PostureScalar=1.3, HitsAirborne=true, RootedDuring=Active.

### Quarterstaff — Versatile two-handed blunt (TwoHand)
- [x] AC-026.5: Quarterstaff has 3 light attack profiles (sweep + thrust + wide sweep finisher) and 1 heavy attack profile (360-degree spinning sweep). L1 and L3 are `MeleeSweep`/`ArcSector`, L2 is `MeleeThrust`/`Capsule`. Heavy is `MeleeSweep`/`ArcSector` (360 degrees). All use `EMordecaiDamageType::Blunt`. EquipSlot: `TwoHand`. GrantedTag: `Mordecai.Weapon.Type.Staff`.

Quarterstaff weapon stats: `BaseDamage=3, AttackSpeedMultiplier=1.0, Range=240, PostureDamageBonus=2`

Quarterstaff light combo chain:

| Step | AttackType | HitShapeType | Param1 | Param2 | Windup(ms) | Active(ms) | Recovery(ms) | BasePower | StaminaCost | PostureScalar | Airborne |
|------|-----------|-------------|--------|--------|------------|------------|-------------|-----------|-------------|---------------|----------|
| L1 | MeleeSweep | ArcSector | R=240 | A=130 | 200 | 160 | 250 | 10 | 7 | 0.5 | JA=true |
| L2 | MeleeThrust | Capsule | L=250 | W=50 | 180 | 140 | 240 | 11 | 8 | 0.5 | HA=true |
| L3 | MeleeSweep | ArcSector | R=250 | A=200 | 250 | 180 | 320 | 15 | 12 | 0.7 | JA=true |

Quarterstaff heavy: MeleeSweep, ArcSector(R=260, A=360), Windup=400, Active=220, Recovery=450, BasePower=22, StaminaCost=20, PostureScalar=1.2, JumpAvoidable=true, RootedDuring=Active.

### Unarmed — Weakest but always available (MainHand fallback)
- [x] AC-026.6: Unarmed has 3 light attack profiles (quick punch thrusts) and 1 heavy attack profile (overhead slam). All lights are `MeleeThrust`/`Capsule`. Heavy is `MeleeSlam`/`Circle`. All use `EMordecaiDamageType::Blunt`. EquipSlot: `MainHand`. GrantedTag: `Mordecai.Weapon.Type.Unarmed`.

Unarmed weapon stats: `BaseDamage=0, AttackSpeedMultiplier=1.3, Range=100, PostureDamageBonus=0`

Unarmed light combo chain:

| Step | AttackType | HitShapeType | Length | Width | Windup(ms) | Active(ms) | Recovery(ms) | BasePower | StaminaCost | PostureScalar | Airborne |
|------|-----------|-------------|--------|-------|------------|------------|-------------|-----------|-------------|---------------|----------|
| L1 | MeleeThrust | Capsule | 100 | 40 | 80 | 60 | 100 | 3 | 2 | 0.1 | HA=true |
| L2 | MeleeThrust | Capsule | 100 | 40 | 80 | 60 | 100 | 3 | 2 | 0.1 | HA=true |
| L3 | MeleeThrust | Capsule | 110 | 45 | 100 | 80 | 140 | 5 | 4 | 0.2 | HA=true |

Unarmed heavy: MeleeSlam, Circle(R=80), Windup=200, Active=100, Recovery=250, BasePower=8, StaminaCost=8, PostureScalar=0.5, HitsAirborne=true, RootedDuring=Active.

### Cross-Cutting Rules
- [x] AC-026.7: All `MeleeSweep` attacks across these weapons have `JumpAvoidable=true` and `HitsAirborne=false`. All `MeleeThrust` and `MeleeSlam` attacks have `HitsAirborne=true` and `JumpAvoidable=false`. Consistent with US-025 blade family rules.
- [x] AC-026.8: Relative weapon stats enforce correct feel ordering across ALL melee weapons (blade + blunt/polearm combined):
  - **Speed**: Dagger (1.4) > Unarmed (1.3) > Shortsword (1.2) > Longsword (1.0) = Quarterstaff (1.0) > Spear (0.9) > Axe (0.85) > Mace (0.8) > Greatsword (0.75)
  - **BaseDamage**: Greatsword (8) > Axe (6) > Mace (5) = Spear (5) > Longsword (4) > Quarterstaff (3) > Shortsword (2) > Dagger (1) > Unarmed (0)
  - **Range**: Spear (280) > Greatsword (250) > Quarterstaff (240) > Longsword (200) > Axe (180) > Mace (160) > Shortsword (150) > Dagger (120) > Unarmed (100)
  - **PostureDamageBonus**: Mace (5) > Greatsword (4) > Axe (3) > Spear (2) = Quarterstaff (2) > Longsword (1) > Shortsword (0) = Dagger (0) = Unarmed (0)
- [x] AC-026.9: Equipping any blunt/polearm weapon via EquipmentComponent makes its light combo chain available to the melee attack ability. Unequipping reverts to Unarmed fallback (which now uses the factory-created Unarmed profiles).
- [x] AC-026.10: All light attacks have `CancelableIntoDodge=true` during Recovery. All heavy attacks have `CancelableIntoDodge=false` (committed attacks per design doc). Consistent with US-025 rules.
- [x] AC-026.11: All attacks across all five weapons have `CanCrit=true` and `AppliesPostureDamage=true` on their DamageProfile.

## Technical Notes
- **File location:** Extend `Source/LyraGame/Mordecai/Weapons/MordecaiWeaponProfileFactory.h/.cpp` with 5 new static methods. Same file as US-025.
- **Factory pattern:** Identical to US-025. Each `Create*()` method uses `NewObject<>()` to build configured weapons programmatically.
- **Unarmed integration:** The factory-created Unarmed profiles should be compatible with `UMordecaiEquipmentComponent::UnarmedAttackProfiles`. Consider having the EquipmentComponent use `CreateUnarmed()` to set its defaults, or have the test verify the factory output matches the expected fallback behavior.
- **All numeric values are initial tuning placeholders.** The relative ordering (faster/slower, more/less damage, more/less posture) is the design intent from the attack taxonomy. Some values (Axe, Mace, Quarterstaff, Unarmed) are planner-scoped since the design docs describe their identity but not specific numbers. Exact values may be adjusted in later tuning passes.
- **Do NOT create DataAssets.** This story is HEADLESS — all profiles are created in code via the factory. EDITOR story US-077 will create the actual DataAssets.
- **Mixed attack type chains:** Spear (thrust+sweep) and Quarterstaff (sweep+thrust+sweep) have mixed attack types within a single combo chain. This is intentional — polearm weapons are more versatile than pure-type weapons.

## Tests Required
- [x] `Mordecai.Weapon.Axe.ProfilesMatchSpec` — Axe factory output: 2 light ArcSector sweeps + slam heavy, MainHand, all Slash, correct BasePower/timing/stamina values (AC-026.2)
- [x] `Mordecai.Weapon.Mace.ProfilesMatchSpec` — Mace factory output: sweep L1 + slam L2 + slam heavy, MainHand, all Blunt, correct values (AC-026.3)
- [x] `Mordecai.Weapon.Spear.ProfilesMatchSpec` — Spear factory output: 2 thrust + sweep finisher light chain + lunge thrust heavy, TwoHand, all Pierce, correct values (AC-026.4)
- [x] `Mordecai.Weapon.Quarterstaff.ProfilesMatchSpec` — Quarterstaff factory output: sweep + thrust + sweep light chain + 360 sweep heavy, TwoHand, all Blunt, correct values (AC-026.5)
- [x] `Mordecai.Weapon.Unarmed.ProfilesMatchSpec` — Unarmed factory output: 3 thrust lights + slam heavy, MainHand, all Blunt, zero BaseDamage, correct values (AC-026.6)
- [x] `Mordecai.Weapon.BluntPolearm.SweepAttacksJumpAvoidable` — Every MeleeSweep profile across Axe/Mace/Spear/Quarterstaff/Unarmed has JumpAvoidable=true, HitsAirborne=false (AC-026.7)
- [x] `Mordecai.Weapon.BluntPolearm.ThrustSlamAttacksHitAirborne` — Every MeleeThrust/MeleeSlam profile has HitsAirborne=true, JumpAvoidable=false (AC-026.7)
- [x] `Mordecai.Weapon.AllMelee.RelativeSpeedOrdering` — Full 9-weapon speed ordering matches AC-026.8 (AC-026.8)
- [x] `Mordecai.Weapon.AllMelee.RelativeDamageOrdering` — Full 9-weapon damage ordering matches AC-026.8 (AC-026.8)
- [x] `Mordecai.Weapon.AllMelee.RelativeRangeOrdering` — Full 9-weapon range ordering matches AC-026.8 (AC-026.8)
- [x] `Mordecai.Weapon.AllMelee.RelativePostureOrdering` — Full 9-weapon posture bonus ordering matches AC-026.8 (AC-026.8)
- [x] `Mordecai.Weapon.BluntPolearm.EquipSetsActiveProfiles` — Equip each blunt/polearm weapon via EquipmentComponent -> GetActiveLightAttackProfiles() returns correct chain (AC-026.9)
- [x] `Mordecai.Weapon.BluntPolearm.HeavyAttacksNotCancelable` — All heavy profiles have CancelableIntoDodge=false (AC-026.10)
- [x] `Mordecai.Weapon.BluntPolearm.LightAttacksCancelIntoDodge` — All light profiles have CancelableIntoDodge=true (AC-026.10)

---

## Definition of Done
- [x] All tests written and failing (red phase confirmed)
- [x] All implementation complete
- [x] All tests passing (green)
- [x] Project compiles with zero errors
- [x] Code committed and pushed with `[US-026]` prefix
