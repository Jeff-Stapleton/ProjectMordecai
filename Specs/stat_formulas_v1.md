# Stat Formula Spec v1

## Primary Attributes

Strength Dexterity Intelligence Constitution Discipline

------------------------------------------------------------------------

## Derived Stats

Health = BaseHealth + (CON × 10)

Stamina = BaseStamina + (STR × 2) + (DEX × 1)

SpellPoints = BaseSP + (INT × 5)

SpellRegen = BaseSPRegen + (INT × 0.1)

Posture = BasePosture + (STR × 2) + (DIS × 1)

------------------------------------------------------------------------

## Resistance

DamageTaken = IncomingDamage × (1 − Resistance)

Example:

FireResistance 25% IncomingFireDamage 100 → FinalDamage 75
