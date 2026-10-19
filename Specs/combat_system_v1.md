# Combat System Spec v1

## Core Resources

Health (0--100%) Stamina (tiered resource) Spell Points (SP)

### Health

-   Represents survivability
-   Reaching 0 → death or downed state

### Stamina

-   Consumed by:
    -   attacks
    -   dodge
    -   block
    -   sprint
-   Low stamina reduces effectiveness but does not fully prevent
    actions.

### Spell Points

-   Used for abilities and spells.
-   Regenerates slower than stamina.
-   INT improves regeneration.

------------------------------------------------------------------------

## Posture System

Enemies have posture meters.

Posture damage comes from: - heavy attacks - perfect parries - certain
abilities

When posture breaks: - enemy enters **critical window** - player can
perform riposte.

------------------------------------------------------------------------

## Damage Formula (baseline)

Damage = BaseDamage × SkillModifier × AttributeScaling ×
CriticalModifier × StatusModifier

------------------------------------------------------------------------

## Critical Hits

CriticalChance = WeaponCrit + SkillBonus + Buffs

CriticalDamage = BaseDamage × CriticalMultiplier

Default multiplier: **1.5x**

------------------------------------------------------------------------

## Defense

FinalDamage = IncomingDamage × (1 − Mitigation)

Mitigation sources: - armor - buffs - resistances

------------------------------------------------------------------------

## Dodge / Parry / Block

### Dodge

-   i‑frames
-   perfect dodge bonus (stamina refund or slow‑motion window)

### Block

-   drains stamina
-   shield stability reduces stamina cost

### Parry

-   high risk / high reward
-   perfect parry causes posture damage and stagger
