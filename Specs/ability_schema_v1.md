# Ability Schema v1

Date: 2026-03-12  
Project: UE5 Diorama ARPG  
Purpose: Define the canonical schema for active and passive abilities so AI developer agents can create abilities consistently within the GAS-centered architecture.

---

# 1. Ability Philosophy

An **ability** is a gameplay action, persistent passive, triggered response, or system-granted power.

Abilities are typically implemented as:
- `UGameplayAbility`
- supporting `UGameplayEffect`
- `GameplayTags`
- optionally projectiles, actors, targeting helpers, or execution calculations

Abilities should be:
- readable
- data-driven
- tuneable in editor
- compatible with the fixed diorama camera
- explicit about cost, cooldown, targeting, and scaling

---

# 2. Recommended Runtime Mapping

- Ability definition asset → Data Asset / Blueprint / config row
- Runtime execution → `UGameplayAbility`
- Applied buff/debuff/state → `UGameplayEffect`
- Restrictions / identity / interactions → `GameplayTags`

---

# 3. Canonical Ability Definition Schema

## Identity

```yaml
AbilityId: fireball
DisplayName: Fireball
AbilityType: Spell
Description: Hurls a fiery projectile that explodes on impact.
Icon: T_UI_Ability_Fireball
```

### Required fields
- `AbilityId`
- `DisplayName`
- `AbilityType`
- `Description`

### Recommended optional fields
- `ShortDescription`
- `Icon`
- `SortOrder`
- `LoreText`

---

## Classification

```yaml
AbilityType: Spell
ActivationType: Active
School: Pyromancy
Tags:
  - Ability.Spell
  - Ability.Fire
  - Damage.Fire
```

### Recommended ability types
- `WeaponTechnique`
- `Spell`
- `Mobility`
- `Defensive`
- `Utility`
- `Summon`
- `Passive`
- `Triggered`
- `ItemGranted`
- `EnemyAbility`

### Recommended activation types
- `Active`
- `Passive`
- `Triggered`
- `Channeled`
- `Toggled`

---

## Costs and Cooldowns

```yaml
Costs:
  Stamina: 0
  SpellPoints: 3
  Health: 0

Cooldown:
  Duration: 5.0
  CooldownTag: Cooldown.Fireball
  Charges: 1
```

---

## Cast and Timing

```yaml
Cast:
  CastTime: 0.35
  WindupTime: 0.2
  RecoveryTime: 0.3
  Interruptible: true
  MovementPolicy: SlowWhileCasting
```

### Recommended movement policies
- `FreeMove`
- `SlowWhileCasting`
- `RootWhileCasting`
- `CommittedLunge`
- `AnimationDriven`

---

## Targeting

```yaml
Targeting:
  TargetingType: Projectile
  Range: 18.0
  Radius: 3.5
  Width: 0.0
  ArcDegrees: 0.0
  RequiresTargetActor: false
  GroundTargeted: false
```

### Recommended targeting types
- `Self`
- `MeleeArc`
- `Cone`
- `Circle`
- `Line`
- `Projectile`
- `Dash`
- `PersistentArea`
- `SummonLocation`
- `GlobalScripted`

### Rule
Targeting must remain readable from the fixed diorama perspective.

---

## Delivery / Spawn

```yaml
Delivery:
  DeliveryType: SpawnProjectile
  ProjectileId: proj_fireball
  SpawnSocket: Hand_R
  SpawnOffset:
    X: 0
    Y: 25
    Z: 40
```

### Recommended delivery types
- `InstantApply`
- `SpawnProjectile`
- `SpawnPersistentArea`
- `TraceHit`
- `MeleeWindow`
- `DashMovement`
- `SummonActor`
- `ScriptedSequence`

---

## Scaling

```yaml
Scaling:
  BasePower: 40
  ScalingStats:
    - Stat: Intelligence
      Coefficient: 0.8
  ScalingSkills:
    - SkillId: pyromancy
      Coefficient: 0.4
  WeaponContribution:
    Enabled: false
```

### Rule
All scaling must be explicit and inspectable.

---

## Effects

```yaml
Effects:
  OnActivate:
    - EffectId: ge_fireball_cast_lock
  OnHit:
    - EffectId: ge_fireball_damage
    - EffectId: ge_burning_buildup_small
  OnExpire: []
```

### Common effect phases
- `OnActivate`
- `OnCommit`
- `OnHit`
- `OnBlock`
- `OnParry`
- `OnCrit`
- `OnExpire`
- `OnTick`
- `OnEnd`

---

## Status / Synergy Hooks

```yaml
Synergy:
  AppliesTags:
    - Attack.Projectile
  ConsumesTags: []
  RequiresTags: []
  BonusVsTags:
    - Tag: Status.Drenched
      Effect: ConvertPartDamageToSteamBurst
```

---

## AI / Enemy Use Hints

```yaml
AIHints:
  PreferredRange: Far
  UseWhenTargetClustered: true
  MinimumTargetsForValue: 2
  AvoidIfTargetHasTag: Immune.Fire
```

This is optional but extremely helpful for enemy ability authoring.

---

## Presentation

```yaml
Presentation:
  Animation: GA_Fireball_Cast
  VFX: NS_Fireball
  SFX: SFX_Fireball_Cast
  CameraShake: Light
  TelegraphType: CastGlow
```

Presentation metadata should not override gameplay authority.

---

# 4. Passive Ability Schema Pattern

Passive abilities use the same base schema but emphasize always-on behavior.

```yaml
AbilityId: flame_retention
DisplayName: Flame Retention
AbilityType: Passive
ActivationType: Passive
Description: Increases the duration of Burning applied by your fire abilities.

Scaling:
  BasePower: 0.15

Effects:
  OnActivate:
    - EffectId: ge_passive_flame_retention
```

---

# 5. Validation Rules

Agents must validate:

- `AbilityId` is unique
- `AbilityType` is valid
- cooldown values are non-negative
- cost values are non-negative
- targeting fields match the `TargetingType`
- all referenced effects, projectiles, animations, VFX, and tags exist
- passive abilities should not require nonsensical active-targeting fields
- `Scaling` should never be ambiguous
- ability descriptions should match actual mechanics

---

# 6. Example Full Ability Definition

```yaml
AbilityId: whirlwind
DisplayName: Whirlwind
AbilityType: WeaponTechnique
ActivationType: Active
School: None
Description: Spin rapidly, striking all nearby enemies.
Icon: T_UI_Ability_Whirlwind
Tags:
  - Ability.WeaponTechnique
  - Weapon.Sword
  - Damage.Physical
  - Shape.Circle

Costs:
  Stamina: 25
  SpellPoints: 0
  Health: 0

Cooldown:
  Duration: 8.0
  CooldownTag: Cooldown.Whirlwind
  Charges: 1

Cast:
  CastTime: 0.0
  WindupTime: 0.25
  RecoveryTime: 0.45
  Interruptible: false
  MovementPolicy: AnimationDriven

Targeting:
  TargetingType: Circle
  Range: 0.0
  Radius: 3.25
  Width: 0.0
  ArcDegrees: 360
  RequiresTargetActor: false
  GroundTargeted: false

Delivery:
  DeliveryType: MeleeWindow

Scaling:
  BasePower: 32
  ScalingStats:
    - Stat: Strength
      Coefficient: 0.7
    - Stat: Dexterity
      Coefficient: 0.2
  ScalingSkills:
    - SkillId: sword_mastery
      Coefficient: 0.5
  WeaponContribution:
    Enabled: true
    WeaponDamageScalar: 1.1

Effects:
  OnHit:
    - EffectId: ge_whirlwind_damage
    - EffectId: ge_minor_posture_damage

Presentation:
  Animation: GA_Whirlwind
  VFX: NS_Whirlwind_Slash
  SFX: SFX_Whirlwind
  CameraShake: Medium
  TelegraphType: WindupPose
```

---

# 7. Design Notes

Every ability should answer:
- what does it cost?
- how often can it be used?
- how is it aimed?
- how does it scale?
- what effects does it apply?
- how is it read by the player?
- does it fit the camera and combat philosophy?

If those answers are fuzzy, the ability is under-specified.
