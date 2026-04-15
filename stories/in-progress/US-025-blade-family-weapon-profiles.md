# US-025: Blade Family Weapon Profiles

## Overview
Define complete attack profiles for the four blade-type weapons: Longsword, Greatsword, Shortsword, and Dagger. Each weapon gets a full light combo chain and heavy attack with weapon-specific shapes, timings, damage values, and combat feel. A `UMordecaiWeaponProfileFactory` static class creates fully-configured weapon setups programmatically for tests and later DataAsset generation.

This builds directly on the weapon framework from US-024: `WeaponDataAsset`, `AttackProfileDataAsset`, and `EquipmentComponent` already exist.

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `attack_taxonomy_v1.md` (Section 4 — Longsword sweep chain example, weapon class profiles)
- Design doc: `player_attacks_agent_brief_v1.md` (Section 4 — Longsword 3 sweep chain + charged spin; Section 1.1 — sweep/thrust/slam shapes)
- Design doc: `damage_types_v1.md` (Section 2 — Physical/Slash/Pierce damage types, common tags)
- Design doc: `combat_system_v1.md` (damage formula, stamina cost, posture damage)
- Existing code: `UMordecaiWeaponDataAsset` (US-024), `UMordecaiAttackProfileDataAsset` (US-002), `UMordecaiEquipmentComponent` (US-024), `EMordecaiWeaponType` (US-024)
- Agent rules: `agent_rules_v2.md` — GAS, data-driven, `TODO(DECISION)` for open items

---

## Acceptance Criteria

### Factory Class
- [ ] AC-025.1: `UMordecaiWeaponProfileFactory` static class exists in `Mordecai/Weapons/`. Provides static methods to create fully-configured weapon setups (WeaponDataAsset + all AttackProfiles) for each weapon type. Methods: `CreateLongsword()`, `CreateGreatsword()`, `CreateShortsword()`, `CreateDagger()`. Each returns a `UMordecaiWeaponDataAsset*` with all attack profiles populated.

### Longsword — Balanced sweep sword (MainHand)
- [ ] AC-025.2: Longsword has 3 light attack profiles (sweep combo chain) and 1 heavy attack profile (360° spin sweep). All attacks use `EMordecaiDamageType::Slash`. EquipSlot: `MainHand`. GrantedTag: `Mordecai.Weapon.Type.Sword`.

Longsword weapon stats: `BaseDamage=4, AttackSpeedMultiplier=1.0, Range=200, PostureDamageBonus=1`

Longsword light combo chain:

| Step | AttackType | HitShapeType | Radius | Angle | Windup(ms) | Active(ms) | Recovery(ms) | BasePower | StaminaCost | PostureScalar | Airborne |
|------|-----------|-------------|--------|-------|------------|------------|-------------|-----------|-------------|---------------|----------|
| L1 | MeleeSweep | ArcSector | 200 | 120 | 200 | 150 | 250 | 12 | 8 | 0.5 | JA=true |
| L2 | MeleeSweep | ArcSector | 200 | 150 | 180 | 170 | 250 | 14 | 10 | 0.5 | JA=true |
| L3 | MeleeSweep | ArcSector | 220 | 180 | 250 | 200 | 350 | 18 | 14 | 0.8 | JA=true |

Longsword heavy: MeleeSweep, ArcSector(220, **360°**), Windup=500, Active=250, Recovery=500, BasePower=28, StaminaCost=25, PostureScalar=1.5, JA=true, RootedDuring=Active.

### Greatsword — Slow, powerful two-hander (TwoHand)
- [ ] AC-025.3: Greatsword has 2 light attack profiles (wide sweeps) and 1 heavy attack profile (overhead slam). Lights use `Slash` damage, heavy uses `Slash` damage with `MeleeSlam` type. EquipSlot: `TwoHand`. GrantedTag: `Mordecai.Weapon.Type.Sword`.

Greatsword weapon stats: `BaseDamage=8, AttackSpeedMultiplier=0.75, Range=250, PostureDamageBonus=4`

| Step | AttackType | HitShapeType | Radius | Angle | Windup(ms) | Active(ms) | Recovery(ms) | BasePower | StaminaCost | PostureScalar | Airborne |
|------|-----------|-------------|--------|-------|------------|------------|-------------|-----------|-------------|---------------|----------|
| L1 | MeleeSweep | ArcSector | 250 | 180 | 350 | 200 | 400 | 20 | 14 | 0.8 | JA=true |
| L2 | MeleeSweep | ArcSector | 260 | 200 | 400 | 250 | 500 | 26 | 18 | 1.0 | JA=true |

Greatsword heavy: **MeleeSlam**, Circle(180), Windup=600, Active=200, Recovery=600, BasePower=35, StaminaCost=30, PostureScalar=2.0, HitsAirborne=true, RootedDuring=Active.

### Shortsword — Fast, low commitment (MainHand)
- [ ] AC-025.4: Shortsword has 4 light attack profiles (3 quick sweeps + thrust finisher) and 1 heavy attack profile (forward thrust). First 3 lights are `MeleeSweep`/`ArcSector`, 4th light is `MeleeThrust`/`Capsule`. Heavy is `MeleeThrust`/`Capsule`. EquipSlot: `MainHand`. GrantedTag: `Mordecai.Weapon.Type.Sword`.

Shortsword weapon stats: `BaseDamage=2, AttackSpeedMultiplier=1.2, Range=150, PostureDamageBonus=0`

| Step | AttackType | HitShapeType | Param1 | Param2 | Windup(ms) | Active(ms) | Recovery(ms) | BasePower | StaminaCost | PostureScalar | Airborne |
|------|-----------|-------------|--------|--------|------------|------------|-------------|-----------|-------------|---------------|----------|
| L1 | MeleeSweep | ArcSector | R=150 | A=90 | 140 | 120 | 180 | 8 | 5 | 0.3 | JA=true |
| L2 | MeleeSweep | ArcSector | R=150 | A=100 | 130 | 120 | 170 | 9 | 6 | 0.3 | JA=true |
| L3 | MeleeSweep | ArcSector | R=160 | A=110 | 140 | 130 | 190 | 10 | 7 | 0.4 | JA=true |
| L4 | MeleeThrust | Capsule | L=170 | W=60 | 180 | 150 | 250 | 14 | 10 | 0.6 | HA=true |

Shortsword heavy: MeleeThrust, Capsule(L=180, W=70), Windup=300, Active=150, Recovery=350, BasePower=20, StaminaCost=18, PostureScalar=1.0, HitsAirborne=true.

### Dagger — Fastest, combo-oriented (MainHand)
- [ ] AC-025.5: Dagger has 5 light attack profiles (all thrusts) and 1 heavy attack profile (lunge thrust). All use `MeleeThrust`/`Capsule` shape. Damage type: `Pierce`. EquipSlot: `MainHand`. GrantedTag: `Mordecai.Weapon.Type.Dagger`.

Dagger weapon stats: `BaseDamage=1, AttackSpeedMultiplier=1.4, Range=120, PostureDamageBonus=0`

| Step | AttackType | HitShapeType | Length | Width | Windup(ms) | Active(ms) | Recovery(ms) | BasePower | StaminaCost | PostureScalar | Airborne |
|------|-----------|-------------|--------|-------|------------|------------|-------------|-----------|-------------|---------------|----------|
| L1 | MeleeThrust | Capsule | 120 | 40 | 100 | 80 | 120 | 5 | 3 | 0.2 | HA=true |
| L2 | MeleeThrust | Capsule | 120 | 40 | 90 | 80 | 120 | 5 | 3 | 0.2 | HA=true |
| L3 | MeleeThrust | Capsule | 120 | 45 | 80 | 80 | 110 | 6 | 4 | 0.2 | HA=true |
| L4 | MeleeThrust | Capsule | 125 | 45 | 80 | 80 | 110 | 6 | 4 | 0.3 | HA=true |
| L5 | MeleeThrust | Capsule | 150 | 50 | 120 | 100 | 200 | 10 | 8 | 0.5 | HA=true |

Dagger heavy: MeleeThrust, Capsule(L=160, W=50), Windup=250, Active=120, Recovery=300, BasePower=22, StaminaCost=15, PostureScalar=1.2, HitsAirborne=true.

### Cross-Cutting Rules
- [ ] AC-025.6: All `MeleeSweep` attacks across blade weapons have `JumpAvoidable=true` and `HitsAirborne=false`. All `MeleeThrust` and `MeleeSlam` attacks have `HitsAirborne=true` and `JumpAvoidable=false`. This follows the attack taxonomy rule: sweeps are low → jumpable; thrusts/slams hit airborne.
- [ ] AC-025.7: Relative weapon stats enforce correct feel ordering:
  - **Speed**: Dagger (1.4) > Shortsword (1.2) > Longsword (1.0) > Greatsword (0.75)
  - **BaseDamage**: Greatsword (8) > Longsword (4) > Shortsword (2) > Dagger (1)
  - **Range**: Greatsword (250) > Longsword (200) > Shortsword (150) > Dagger (120)
  - **PostureDamageBonus**: Greatsword (4) > Longsword (1) > Shortsword (0) = Dagger (0)
- [ ] AC-025.8: Equipping any blade weapon via EquipmentComponent makes its light combo chain available to the melee attack ability. Unequipping reverts to Unarmed fallback.
- [ ] AC-025.9: All light attacks have `CancelableIntoDodge=true` during Recovery. All heavy attacks have `CancelableIntoDodge=false` (committed attacks per design doc).

## Technical Notes
- **File location:** New file `Source/LyraGame/Mordecai/Weapons/MordecaiWeaponProfileFactory.h/.cpp`. Static class, no instances.
- **Factory pattern:** Each `Create*()` method uses `NewObject<UMordecaiWeaponDataAsset>()` and `NewObject<UMordecaiAttackProfileDataAsset>()` to build configured weapons. The outer pointer must be passed as the `Outer` to keep objects alive.
- **Profile DamageType:** Set `DamageProfile.DamageType` on each AttackProfile. For Slash weapons, all profiles use `EMordecaiDamageType::Slash`. For Dagger (Pierce), all profiles use `EMordecaiDamageType::Pierce`.
- **All numeric values are initial tuning placeholders.** The relative ordering (faster/slower, more/less damage) is the design intent. Exact numbers may be adjusted in later tuning passes.
- **Do NOT create DataAssets.** This story is HEADLESS — all profiles are created in code via the factory. EDITOR story US-077 will create the actual DataAssets.
- **CanCrit:** All light and heavy attacks should have `CanCrit=true` (per combat_system_v1 — crit chance comes from weapon + skill bonuses).
- **AppliesPostureDamage:** Set `true` on all attacks. PostureDamageScalar controls magnitude.

## Tests Required
- [ ] `Mordecai.Weapon.Longsword.ProfilesMatchSpec` — Longsword factory output: 3 light ArcSector sweeps + 360° sweep heavy, all Slash, correct BasePower/timing/stamina values (AC-025.2)
- [ ] `Mordecai.Weapon.Greatsword.ProfilesMatchSpec` — Greatsword factory output: 2 light wide sweeps + slam heavy, TwoHand, all Slash, correct values (AC-025.3)
- [ ] `Mordecai.Weapon.Shortsword.ProfilesMatchSpec` — Shortsword factory output: 3 sweeps + thrust finisher light chain + thrust heavy, correct values (AC-025.4)
- [ ] `Mordecai.Weapon.Dagger.ProfilesMatchSpec` — Dagger factory output: 5 thrust lights + lunge heavy, all Pierce, correct values (AC-025.5)
- [ ] `Mordecai.Weapon.Blades.SweepAttacksJumpAvoidable` — Every MeleeSweep profile across all blade weapons has JumpAvoidable=true, HitsAirborne=false (AC-025.6)
- [ ] `Mordecai.Weapon.Blades.ThrustSlamAttacksHitAirborne` — Every MeleeThrust/MeleeSlam profile across all blade weapons has HitsAirborne=true, JumpAvoidable=false (AC-025.6)
- [ ] `Mordecai.Weapon.Blades.RelativeSpeedOrdering` — AttackSpeedMultiplier: Dagger > Shortsword > Longsword > Greatsword (AC-025.7)
- [ ] `Mordecai.Weapon.Blades.RelativeDamageOrdering` — BaseDamage: Greatsword > Longsword > Shortsword > Dagger (AC-025.7)
- [ ] `Mordecai.Weapon.Blades.RelativeRangeOrdering` — Range: Greatsword > Longsword > Shortsword > Dagger (AC-025.7)
- [ ] `Mordecai.Weapon.Blades.EquipSetsActiveProfiles` — Equip each blade via EquipmentComponent → GetActiveLightAttackProfiles() returns correct weapon's chain (AC-025.8)
- [ ] `Mordecai.Weapon.Blades.HeavyAttacksNotCancelable` — All heavy profiles have CancelableIntoDodge=false (AC-025.9)
- [ ] `Mordecai.Weapon.Blades.LightAttacksCancelIntoDodge` — All light profiles have CancelableIntoDodge=true (AC-025.9)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-025]` prefix
