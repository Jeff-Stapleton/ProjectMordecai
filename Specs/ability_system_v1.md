# Ability System Spec v1

## Ability Types

1.  Weapon Techniques
2.  Spells
3.  Mobility abilities
4.  Passive feats

------------------------------------------------------------------------

## Implementation Mapping (UE5 GAS)

Abilities → GameplayAbilities Buffs/Debuffs → GameplayEffects Tags →
GameplayTags

------------------------------------------------------------------------

## Ability Properties

Each ability defines:

-   cooldown
-   resource cost
-   cast time
-   targeting type
-   damage/effect magnitude

Example:

Fireball Cooldown: 5s Cost: 3 SP Radius: 4m Damage: FireDamage ×
SpellPower

------------------------------------------------------------------------

## Ability Targeting Types

-   Self
-   Cone
-   Circle
-   Line
-   Projectile
-   Persistent Area

------------------------------------------------------------------------

## Ability Scaling

AbilityPower = BaseAbilityPower + AttributeScaling + SkillScaling
