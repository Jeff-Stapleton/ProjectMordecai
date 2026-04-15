# Damage Types v1

Date: 2026-03-12  
Project: UE5 Diorama ARPG  
Purpose: Define the canonical damage taxonomy, status relationships, and resistance hooks so AI developer agents can implement damage consistently across abilities, items, enemies, and status systems.

---

# 1. Design Goals

Damage types should:
- create meaningful build identity
- support clear enemy strengths and weaknesses
- interact cleanly with statuses
- be readable in gameplay and content authoring
- avoid redundant “just recolored damage” categories

Every damage type should have:
- a clear fantasy
- a typical delivery pattern
- common status interactions
- resistance hooks
- item/build support opportunities

---

# 2. Core Damage Type Taxonomy

## Physical
Represents direct bodily harm from force, cuts, punctures, and impacts.

### Common subtypes
- Slash
- Pierce
- Blunt

### Typical delivery
- melee hits
- arrows / bolts
- thrown weapons
- weapon techniques

### Common interactions
- posture damage
- bleed buildup
- armor mitigation
- crit synergy

### Common tags
- `Damage.Physical`
- `Damage.Slash`
- `Damage.Pierce`
- `Damage.Blunt`

---

## Fire
Represents heat, burning, combustion, and searing force.

### Typical delivery
- fire spells
- ignited weapons
- burning zones
- explosive attacks

### Common interactions
- Burning
- ignition of hazards
- extra pressure against certain materials or frozen targets

### Common tags
- `Damage.Fire`

---

## Frost
Represents cold, freezing force, chill, and brittle pressure.

### Typical delivery
- ice spells
- frost weapon procs
- freezing zones
- cold projectiles

### Common interactions
- Frostbitten
- slows
- Brittle
- synergy versus Drenched targets

### Common tags
- `Damage.Frost`

---

## Lightning
Represents electricity, shock, and rapid burst energy.

### Typical delivery
- chain attacks
- strikes from above
- conductive bursts
- energized weapons

### Common interactions
- Shocked
- chaining
- bonus against Drenched or metal-heavy enemies

### Common tags
- `Damage.Lightning`

---

## Poison
Represents toxins, venoms, rot, or corrosive biological damage.

### Typical delivery
- poison clouds
- coated weapons
- darts
- lingering pools

### Common interactions
- Poisoned
- attrition pressure
- anti-healing or stamina attrition depending on design

### Common tags
- `Damage.Poison`

---

## Corrosive / Acid
Represents material erosion and armor-eating damage.

### Typical delivery
- acid spit
- alchemical bombs
- corruption pools
- elite monster attacks

### Common interactions
- Corroded
- armor reduction
- durability pressure if that system is used

### Common tags
- `Damage.Corrosive`

---

## Arcane
Represents raw magical force, refined spell energy, or non-elemental mystical damage.

### Typical delivery
- force bolts
- magical blades
- beams
- area pulses

### Common interactions
- less environmental logic than elemental damage
- often good for pure caster builds
- may interact with Silence, Focused, warding systems, or anti-magic targets

### Common tags
- `Damage.Arcane`

---

## Shadow / Curse
Represents corruption, affliction, entropy, hexes, and forbidden magic.

### Typical delivery
- curses
- corrupted melee
- boss attacks
- lingering affliction zones

### Common interactions
- Cursed
- Weakened
- Fear
- max-health pressure or anti-buff logic depending on design

### Common tags
- `Damage.Shadow`
- `Damage.Curse`

---

## Holy / Radiant
Represents sanctified, purifying, or light-based force.

### Typical delivery
- consecrated strikes
- radiant spells
- anti-undead abilities
- healing-adjacent offensive effects

### Common interactions
- extra effectiveness versus undead/corrupted targets
- cleansing
- anti-curse / anti-shadow hooks

### Common tags
- `Damage.Radiant`

---

# 3. Recommended Status Relationships

These are guidance mappings, not absolute hard locks.

```yaml
DamageToStatusSuggestions:
  Damage.Fire:
    CommonStatus: Status.Burning
  Damage.Frost:
    CommonStatus: Status.Frostbitten
  Damage.Lightning:
    CommonStatus: Status.Shocked
  Damage.Poison:
    CommonStatus: Status.Poisoned
  Damage.Corrosive:
    CommonStatus: Status.Corroded
  Damage.Shadow:
    CommonStatus: Status.Cursed
  Damage.Physical.Slash:
    CommonStatus: Status.Bleeding
```

### Rule
Damage type and status type should be related, but not inseparable.
Not every fire effect must burn. Not every slash must bleed.

---

# 4. Resistance Schema Pattern

```yaml
Resistances:
  Physical: 0.10
  Fire: -0.20
  Frost: 0.25
  Lightning: 0.00
  Poison: 0.50
  Corrosive: 0.00
  Arcane: 0.15
  Shadow: 0.10
  Radiant: -0.10
```

### Interpretation
- positive values reduce damage taken
- negative values indicate vulnerability
- `0.25` means 25% mitigation
- `-0.20` means 20% extra damage taken

---

# 5. Damage Delivery Metadata

Abilities, items, and enemy attacks should specify not just damage type, but also how the damage behaves.

```yaml
DamageProfile:
  Type: Fire
  Delivery: Projectile
  Shape: Explosion
  CanCrit: true
  AppliesPostureDamage: false
  BuildsStatus:
    Status: Burning
    Magnitude: 12
```

### Recommended delivery values
- `MeleeHit`
- `Projectile`
- `Beam`
- `Explosion`
- `PersistentArea`
- `GroundHazard`
- `Chain`
- `Pulse`
- `Aura`
- `ScriptedEvent`

---

# 6. Counterplay Guidelines

Each damage type should have common counterplay levers.

## Physical
Counterplay:
- armor
- block
- parry
- posture management
- mobility

## Fire
Counterplay:
- fire resistance
- movement out of hazards
- anti-burn tools
- water / drenched interactions if supported

## Frost
Counterplay:
- frost resistance
- mobility timing
- cleanse
- anti-slow tools

## Lightning
Counterplay:
- spacing
- anti-chain behavior
- lightning resistance
- avoiding conductive setups

## Poison
Counterplay:
- cleanse
- poison resistance
- attrition mitigation
- pressure the source before long fights develop

## Corrosive
Counterplay:
- resistance
- fast kill
- anti-armor-break tools
- avoid standing in pools

## Arcane
Counterplay:
- wards
- interrupts
- anti-caster play
- magic resistance

## Shadow / Curse
Counterplay:
- cleanse
- radiant tools
- curse resistance
- priority target elimination

## Radiant
Counterplay:
- resistance
- avoiding line/channel attacks
- disrupting casters
- specialized enemy immunities where appropriate

---

# 7. Example Damage Definitions

## Example 1: Fireball

```yaml
AbilityId: fireball
DamageProfile:
  Type: Fire
  Delivery: Projectile
  Shape: Explosion
  BasePower: 40
  CanCrit: true
  AppliesPostureDamage: false
  BuildsStatus:
    Status: Burning
    Magnitude: 18
```

## Example 2: Heavy Slash

```yaml
AbilityId: heavy_slash
DamageProfile:
  Type: Slash
  ParentType: Physical
  Delivery: MeleeHit
  Shape: Arc
  BasePower: 32
  CanCrit: true
  AppliesPostureDamage: true
  BuildsStatus:
    Status: Bleeding
    Magnitude: 8
```

## Example 3: Acid Spit

```yaml
AbilityId: acid_spit
DamageProfile:
  Type: Corrosive
  Delivery: Projectile
  Shape: PersistentArea
  BasePower: 18
  CanCrit: false
  AppliesPostureDamage: false
  BuildsStatus:
    Status: Corroded
    Magnitude: 14
```

---

# 8. Validation Rules

Agents must validate:

- every damage type referenced exists in the taxonomy
- statuses linked to damage types are intentional
- resistances use the same canonical naming
- subtypes like Slash/Pierce/Blunt remain children of Physical
- damage descriptions match actual type and delivery
- build-defining items and abilities clearly declare their damage types

---

# 9. Design Notes

A damage type should not exist unless it creates:
- different resist profiles
- different status hooks
- different build opportunities
- or different encounter logic

If a damage type adds no meaningful differentiation, it should likely be merged into another.
