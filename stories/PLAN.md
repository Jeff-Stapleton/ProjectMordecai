# Implementation Plan — Project Mordecai

## How This Works
1. Arc (coordinator) scopes features from design docs into user stories
2. Stories go in `backlog/` with acceptance criteria
3. When work starts, story moves to `in-progress/`
4. Claude Code picks up the story, writes tests, implements, verifies, commits
5. Completed stories move to `done/`

## Epics (High Level)

### Epic 1: Foundation ✅ (complete)
Get a running project with a controllable character.
- US-001: Project Foundation & Initial Player Character ✅

### Epic 2: Core Combat ✅ (complete)
Implement the combat system: attacks, defense, posture, stamina, projectiles.
- US-002: Attack Profile Data Model & Damage Types ✅
- US-003: Melee Hit Detection System ✅
- US-004: Melee Attack GAS Ability & Combo System ✅
- US-005: Dodge System ✅
- US-006: Block & Parry System ✅
- US-007: Posture & Stagger System ✅
- US-008: Stamina Tier System ✅
- US-009: Projectile System & Aim Assist ✅

### Epic 2.5: Playable Vertical Slice ⚡ (nearly complete — US-053 + US-054 remaining)
Wire up all completed combat systems into a playable prototype. The goal: Jeff can launch the game, run around a test map, fight an enemy, and exercise melee/dodge/block/parry/stamina/posture.
- US-050: Enemy Character & Damage Reception ✅
- US-051: Basic Enemy AI Combat Loop ✅
- US-052: Combat HUD C++ Framework ✅
- US-053: Player Death & Arena Game Flow (HEADLESS — player death/respawn, arena reset, kill tracking) ⚡ **in progress** (no implementation yet)
- US-054: Playable Arena Integration (EDITOR — enemy BP, attack DataAssets, HUD widget BP, level setup, full wiring) ⚡ **in progress** (12/13 ACs done, PIE test pending US-053)

### Epic 3: Attributes & Progression ✅ (complete)
Wire up the attribute system and skill progression.
- US-010: Effective Mod Formula & Core Attribute Scaling ✅
- US-011: Skill Framework & Rank Progression ✅
- US-012: Feat System ✅

### Epic 4: Status Effects (2/8 done, US-015 blocked)
Implement the launch status effect set via GAS. Split into 8 stories by mechanical grouping.
- US-013: Status Effect Framework (GAS infrastructure, tags, stacking, immunity, cleanse) ✅
- US-014: DoT & Resource Denial Statuses (Burning, Bleeding, Poisoned) ✅
- US-015: Speed & Timing Impairment Statuses (Frostbitten, Shocked) 🔴 **BLOCKED** — partial code exists, needs manual investigation
- US-016: Combat Modifier Debuffs (Weakened, Brittle, Exposed, Corroded) ← **scoped**
- US-017: Action-Restricting Debuffs (Silenced, Rooted) ← **scoped** (trimmed from original 5 debuffs)
- US-059: Perception & Mental Debuffs (Blinded, Fear, Cursed) ← **scoped** (split from US-017)
- US-018: Drenched & Focused Buff (environmental interaction + perfect action streak) ← **scoped**

### Epic 5: Magic System ← **scoped** (7 stories: 5 HEADLESS + 2 split from US-023)
Implement spell framework and 14 launch spells at Rank 1. Higher rank milestones deferred.
- US-019: Spell Framework & Spell DataAsset (HEADLESS — base ability class, SP costs, cooldowns, casting phases, movement policies, interruption, scaling, upcasting) ← **scoped**
- US-020: Damage Spells (HEADLESS — Fireball, Cone of Cold, Magic Missile, Stone Skin) ← **scoped**
- US-021: Utility Spells (HEADLESS — Blink, Sleep, Fire Ward) ← **scoped**
- US-022: Support Spells (HEADLESS — Bless, Restoration) ← **scoped**
- US-023: Tactical Spells — Debuff Spells (HEADLESS — Snare, Enfeeble, Enchant Weapon) ← **scoped** (trimmed from original 5 spells)
- US-060: Tactical Spells — Summon & Evasion (HEADLESS — Illusion, Blur) ← **scoped** (split from US-023)

### Epic 5.5: Playable Magic Slice ← **NEW** (3 stories: 2 HEADLESS + 1 EDITOR)
Wire spells, status effects, and spell HUD into the playable arena. Integration milestone per Playability-First Rule (after 2 system epics: 4 + 5).
- US-055: Spell HUD & Status Effect Indicators (HEADLESS — SP bar, buff/debuff indicators, cooldown display) ← **scoped**
- US-056: Status-Applying Attack Profiles (HEADLESS — extend attack profiles with on-hit status application) ← **scoped**
- US-057: Playable Magic Arena Integration (EDITOR — spell DataAssets, spell inputs, enemy status attacks, HUD wiring, full magic loop verification) ← **scoped**

### Epic 6: Weapons & Equipment (partially scoped — US-024 detailed, rest placeholder)
Implement weapon classes and armor systems.
- US-024: Weapon Class Framework (HEADLESS — WeaponDataAsset, WeaponInstance, EquipmentComponent, equip/unequip via GAS, weapon slot model, melee attack integration) ← **scoped**
- US-025: Melee Weapons — Blade Family (Longsword, Greatsword, Shortsword, Dagger) ← placeholder, needs scoping
- US-026: Melee Weapons — Blunt & Polearm (Axe, Mace, Spear, Quarterstaff, Unarmed) ← placeholder, needs scoping
- US-027: Ranged Weapons (Longbow, Shortbow, Crossbow, Throwables, Wands) ← placeholder, needs scoping
- US-028: Two-Weapon Fighting ← placeholder, needs scoping
- US-029: Armor & Equipment System (Leather, Chain, Plate + skills) ← placeholder, needs scoping
- US-030: Shield System ← placeholder, needs scoping

### Epic 7: Inventory
Implement unlimited-carry inventory with town gating (per agent_rules_v2).
- US-031: Flat Inventory & Auto-Store System
- US-032: Item Categories & Stack Rules
- US-033: Unidentified Item & Town Gating Flow

### Epic 8: World & Exploration
Movement skills, exploration systems, Metroidvania gating.
- US-034: Movement Skills (Climbing, Swimming, Sneaking)
- US-035: Exploration Skills (Perception, Tracking, Cartography, Lockpicking, Traps)
- US-036: Metroidvania World Gates

### Epic 9: Town Management
Delegated town systems.
- US-037: Town Building Framework
- US-038: NPC Job Assignment
- US-039: Initiatives & Upgrades
- US-040: Town Events

### Epic 10: UI & HUD
- US-041: Combat HUD (Health%, Stamina Tiers, SP)
- US-042: Status Effect VFX
- US-043: Inventory UI

---

## Priority Order
1. **Epic 1** — Foundation ✅
2. **Epic 2** — Core Combat ✅
3. **Epic 3** — Attributes & Progression ✅
4. **Epic 4 (partial)** — Status Effects: US-013 ✅, US-014 ✅, US-015 🔴 BLOCKED
5. **Epic 2.5** — Playable Vertical Slice ⚡ **IMMEDIATE PRIORITY** (US-053 + US-054 remaining)
6. **Epic 4 (remaining)** — US-016 → US-017 → US-059 → US-018 (resume after vertical slice is playable)
7. **Epic 5** — Magic System: US-019 (gate) → US-020/021/022 (parallel) → US-023 → US-060
8. **Epic 5.5** — Playable Magic Slice: US-055 + US-056 (parallel, HEADLESS) → US-057 (EDITOR)
9. **Epic 6** — Weapons & Equipment: US-024 (gate) → US-025/026/027 → US-028/029/030
10. **Epic 7** — Inventory
11. **Epic 10** — UI & HUD
12. **Epic 8** — World & Exploration
13. **Epic 9** — Town Management

## Notes
- This plan is a living document. Stories will be broken down further as we approach each epic.
- DLC reserved: Hex (Dark Magic), Venom (Nature/Poison), Conjure (Summoning)
- Story numbers are placeholders for unscoped epics — will be refined when each epic is scoped in detail
- US-017 was split: Silenced + Rooted stay in US-017; Blinded + Fear + Cursed moved to US-059
- US-023 was split: Snare + Enfeeble + Enchant Weapon stay in US-023; Illusion + Blur moved to US-060
- Epic 6 US-025 (Melee Weapons) split into US-025 (Blades) and US-026 (Blunt/Polearm). Original US-026–029 renumbered to US-027–030
