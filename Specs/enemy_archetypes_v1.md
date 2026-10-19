# Enemy Archetypes v1

Date: 2026-03-12  
Project: UE5 Diorama ARPG  
Purpose: Define the canonical enemy archetype schema and design categories so AI developer agents can create enemies with clear combat roles, readable behavior, and systemic consistency.

---

# 1. Enemy Philosophy

Enemies should challenge the player through:
- readable patterns
- positioning
- cadence
- area control
- synergy
- status pressure
- encounter composition

Enemies should not rely primarily on:
- oversized health pools
- cheap unreadable one-shots
- excessive permanent crowd control
- random behavior with no pattern language

Every enemy should have:
- a readable role
- a clear threat profile
- a weakness or counterplay angle
- an understandable place in encounter composition

---

# 2. Enemy Definition Schema

## Identity

```yaml
EnemyId: goblin_skirmisher
DisplayName: Goblin Skirmisher
Archetype: Skirmisher
Tier: Basic
Description: A fast melee harasser that pressures positioning.
```

### Required fields
- `EnemyId`
- `DisplayName`
- `Archetype`
- `Tier`

### Recommended tiers
- `Basic`
- `Elite`
- `Champion`
- `Boss`
- `Summon`
- `Miniboss`

---

## Classification

```yaml
Faction: Goblin
Family: Humanoid
BiomeTags:
  - Biome.Forest
  - Biome.Caves
DamageTags:
  - Damage.Physical
```

---

## Stats / Core Tuning

```yaml
CoreStats:
  HealthScalar: 1.0
  DamageScalar: 1.0
  MoveSpeed: 1.15
  Posture: 0.8
  Stability: 0.7
  AggroRange: 9.0
  LeashRange: 18.0
```

These should be tuned per archetype and per specific enemy.

---

## Resistances / Vulnerabilities

```yaml
Resistances:
  Fire: 0.0
  Frost: -0.15
  Poison: 0.25
  Lightning: 0.0

StatusRules:
  ImmuneTags: []
  ResistantTags:
    - Status.Fear
  VulnerableTags:
    - Status.Rooted
```

---

## Behavior Package

```yaml
Behavior:
  ControllerType: DefaultMeleeAI
  BehaviorTree: BT_GoblinSkirmisher
  Blackboard: BB_StandardHumanoid
  PreferredRange: Close
  AggressionStyle: FlankAndPressure
```

### Recommended aggression styles
- `DirectRush`
- `FlankAndPressure`
- `HoldLine`
- `KiteAndChip`
- `AreaDenial`
- `Ambush`
- `SupportFromRear`
- `SiegePressure`

---

## Ability Loadout

```yaml
Abilities:
  - goblin_quick_slash
  - goblin_leap_strike
  - goblin_evade
```

Abilities should use the same shared ability schema as player and boss abilities where practical.

---

## Telegraph Profile

```yaml
Telegraphs:
  WindupClarity: High
  FXClarity: Medium
  AudioCues: true
  GroundMarkers: false
```

### Rule
Telegraphs must remain readable from the fixed diorama camera.

---

## Encounter Role

```yaml
EncounterRole:
  Role: Harasser
  ThreatPriority: Medium
  IdealGroupSize: 2-4
  SynergyTags:
    - EnemyRole.Frontliner
    - EnemyRole.Support
```

### Recommended encounter roles
- `Frontliner`
- `Harasser`
- `Bruiser`
- `Artillery`
- `Controller`
- `Support`
- `Summoner`
- `Tank`
- `Assassin`
- `AreaDenial`
- `BossPhaseDriver`

---

## Loot Rules

```yaml
Loot:
  CommonTableId: loot_goblin_basic
  GuaranteedDrops: []
  SignatureDropId: null
```

---

## Spawn Rules

```yaml
SpawnRules:
  SpawnWeight: 1.0
  MinimumRegionTier: 1
  MaxSimultaneousRecommended: 6
```

---

# 3. Archetype Library

Below are recommended high-level archetypes for the project.

## Frontliner
Purpose:
- absorb attention
- define battle lines
- force spacing decisions

Typical traits:
- medium/high health
- stable posture
- moderate damage
- readable melee attacks

Counterplay:
- posture breaks
- flanking
- mobility

---

## Harasser / Skirmisher
Purpose:
- pressure movement
- interrupt comfort zones
- punish tunnel vision

Typical traits:
- high mobility
- low/medium health
- leap, dash, or disengage tools

Counterplay:
- roots
- spacing discipline
- burst punishment after commitment

---

## Bruiser
Purpose:
- high threat melee presence
- dangerous but committed attacks

Typical traits:
- heavy swings
- posture damage
- slower but punishing cadence

Counterplay:
- dodge/parry timing
- baiting commitments
- attack windows after misses

---

## Artillery
Purpose:
- force movement from afar
- punish stationary players

Typical traits:
- ranged attacks
- area markers
- low defenses

Counterplay:
- close distance
- line-of-sight abuse
- interruption

---

## Controller
Purpose:
- shape the battlefield
- apply roots, slows, silence, fear, or zone denial

Typical traits:
- lower direct damage
- high annoyance/threat through status effects

Counterplay:
- target priority
- cleanse
- interrupt
- resistance builds

---

## Support
Purpose:
- sustain or empower allies

Typical traits:
- buffs
- heals
- shields
- summons

Counterplay:
- target priority
- disruption
- burst damage

---

## Summoner
Purpose:
- generate pressure over time
- alter encounter composition

Typical traits:
- spawns adds
- low direct threat unless unchecked

Counterplay:
- pressure summoner
- AoE add clear
- anti-summon mechanics

---

## Tank / Sentinel
Purpose:
- anchor an encounter
- guard lanes or objectives

Typical traits:
- high health
- high stability
- taunt-like or protective behavior
- often paired with artillery/support

Counterplay:
- ignore and reposition
- exploit weak supports
- use armor-break/posture tools

---

## Assassin
Purpose:
- burst threat against backline or distracted players

Typical traits:
- stealth/opening burst
- mobility
- fragile if exposed

Counterplay:
- awareness
- anti-stealth
- quick punishment after reveal

---

## Area Denial
Purpose:
- make safe zones unsafe
- constrain movement

Typical traits:
- hazards
- puddles
- fire walls
- lightning zones
- corrupted terrain

Counterplay:
- mobility
- timing
- status resistance
- controlling fight space early

---

## Boss Phase Driver
Purpose:
- used in boss ecosystems
- changes cadence or rules of the encounter

Typical traits:
- scripted but readable phase transitions
- summons hazards or support units
- alters arena pressures

Counterplay:
- learning phase language
- preserving resources
- target priority

---

# 4. Example Enemy Definitions

## Basic Example

```yaml
EnemyId: skeletal_archer
DisplayName: Skeletal Archer
Archetype: Artillery
Tier: Basic
Faction: Undead
Family: Skeleton

CoreStats:
  HealthScalar: 0.8
  DamageScalar: 1.1
  MoveSpeed: 0.95
  Posture: 0.7
  Stability: 0.6
  AggroRange: 12.0
  LeashRange: 22.0

Behavior:
  ControllerType: DefaultRangedAI
  BehaviorTree: BT_SkeletalArcher
  Blackboard: BB_UndeadRanged
  PreferredRange: Far
  AggressionStyle: KiteAndChip

Abilities:
  - skeleton_arrow_shot
  - skeleton_rapid_volley
  - skeleton_retreat_step

EncounterRole:
  Role: Artillery
  ThreatPriority: High
  IdealGroupSize: 1-3
```

## Elite Example

```yaml
EnemyId: corrupted_knight
DisplayName: Corrupted Knight
Archetype: Bruiser
Tier: Elite
Faction: Corrupted
Family: Humanoid

CoreStats:
  HealthScalar: 2.4
  DamageScalar: 1.8
  MoveSpeed: 0.9
  Posture: 1.6
  Stability: 1.5
  AggroRange: 10.0
  LeashRange: 20.0

Behavior:
  ControllerType: EliteMeleeAI
  BehaviorTree: BT_CorruptedKnight
  Blackboard: BB_EliteHumanoid
  PreferredRange: Close
  AggressionStyle: DirectRush

Abilities:
  - corrupted_knight_cleave
  - corrupted_knight_guard_break
  - corrupted_knight_dark_wave

EncounterRole:
  Role: Bruiser
  ThreatPriority: High
  IdealGroupSize: 1
```

---

# 5. Validation Rules

Agents must validate:

- `EnemyId` is unique
- archetype and role are not contradictory without reason
- telegraph clarity is appropriate to threat level
- ranged enemies have real counterplay
- controller enemies do not over-stack frustration
- bosses are readable despite complexity
- loot tables resolve correctly
- all referenced ability IDs exist

---

# 6. Design Notes

Every enemy should answer:
- what role does it play?
- how does it pressure the player?
- what is its counterplay?
- how readable is it from the diorama camera?
- what allies does it synergize with?
- what makes it distinct from nearby archetypes?

If those answers are weak, the enemy is under-designed.
