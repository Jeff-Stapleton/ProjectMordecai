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

### Epic 2.5: Playable Vertical Slice ⚡ HIGH PRIORITY
Wire up all completed combat systems into a playable prototype. The goal: Jeff can launch the game, run around a test map, fight an enemy, and exercise melee/dodge/block/parry/stamina/posture.
- US-050: Enemy Character & Damage Reception (HEADLESS — enemy class, ASC, attributes, death, posture break) ⚡ **in progress**
- US-051: Basic Enemy AI Combat Loop (HEADLESS — state machine AI: idle/approach/attack/recover/leash) ← **scoped**
- US-052: Combat HUD C++ Framework (HEADLESS — health/stamina/posture widget classes, percent/tier calculations) ⚡ **in progress**
- US-053: Player Death & Arena Game Flow (HEADLESS — player death/respawn, arena reset, kill tracking) ← **scoped**
- US-054: Playable Arena Integration (EDITOR — enemy BP, attack DataAssets, HUD widget BP, level setup, full wiring) ← **scoped**

### Epic 3: Attributes & Progression ✅ (complete)
Wire up the attribute system and skill progression.
- US-010: Effective Mod Formula & Core Attribute Scaling ✅
- US-011: Skill Framework & Rank Progression ✅
- US-012: Feat System ✅

### Epic 4: Status Effects (in progress — 2/6 done, US-015 blocked, pausing for Epic 2.5)
Implement the launch status effect set via GAS. Split into 6 stories by mechanical grouping.
- US-013: Status Effect Framework (GAS infrastructure, tags, stacking, immunity, cleanse) ✅
- US-014: DoT & Resource Denial Statuses (Burning, Bleeding, Poisoned) ✅
- US-015: Speed & Timing Impairment Statuses (Frostbitten, Shocked) 🔴 **BLOCKED** — partial code exists, needs manual investigation
- US-016: Combat Modifier Debuffs (Weakened, Brittle, Exposed, Corroded) ← **scoped** (after Epic 2.5)
- US-017: Control & Mental Debuffs (Silenced, Rooted, Blinded, Fear, Cursed) ← **scoped** (after Epic 2.5)
- US-018: Drenched & Focused Buff (environmental interaction + perfect action streak) ← **scoped** (after Epic 2.5)

### Epic 5: Magic System
Implement spells with rank progression.
- US-019: Spell Framework (SP cost, casting, upcasting, rank system)
- US-020: Damage Spells (Fireball, Frost Nova, Magic Missile, Stone Skin)
- US-021: Utility Spells (Blink, Sleep/Wind, Fire Ward)
- US-022: Support Spells (Bless, Restoration)
- US-023: Tactical Spells (Illusion, Blur, Snare, Enchant Weapon, Enfeeble)

### Epic 6: Weapons & Equipment
Implement weapon classes and armor systems.
- US-024: Weapon Class Framework (data-driven profiles)
- US-025: Melee Weapons (Longsword, Greatsword, Shortsword, Dagger, Axe, Mace, Spear, Quarterstaff, Unarmed)
- US-026: Ranged Weapons (Longbow, Shortbow, Crossbow, Throwables, Wands)
- US-027: Two-Weapon Fighting
- US-028: Armor System (Leather, Chain, Plate + skills)
- US-029: Shield System

### Epic 7: Inventory
Implement unlimited-carry inventory with town gating (per agent_rules_v2).
- US-030: Flat Inventory & Auto-Store System
- US-031: Item Categories & Stack Rules
- US-032: Unidentified Item & Town Gating Flow

### Epic 8: World & Exploration
Movement skills, exploration systems, Metroidvania gating.
- US-033: Movement Skills (Climbing, Swimming, Sneaking)
- US-034: Exploration Skills (Perception, Tracking, Cartography, Lockpicking, Traps)
- US-035: Metroidvania World Gates

### Epic 9: Town Management
Delegated town systems.
- US-036: Town Building Framework
- US-037: NPC Job Assignment
- US-038: Initiatives & Upgrades
- US-039: Town Events

### Epic 10: UI & HUD
- US-040: Combat HUD (Health%, Stamina Tiers, SP)
- US-041: Status Effect VFX
- US-042: Inventory UI

---

## Priority Order
1. **Epic 1** — Foundation ✅
2. **Epic 2** — Core Combat ✅
3. **Epic 3** — Attributes & Progression ✅
4. **Epic 4 (partial)** — Status Effects: US-013 ✅, US-014 ✅, US-015 🔴 BLOCKED
5. **Epic 2.5** — Playable Vertical Slice ⚡ **IMMEDIATE PRIORITY** (Playability-First Rule: 3 system epics done, US-015 blocked — proceed to integration)
6. **Epic 4 (remaining)** — US-016, US-017, US-018 (resume after vertical slice is playable)
7. **Epic 5** — Magic System
8. **Epic 6** — Weapons & Equipment
9. **Epic 7** — Inventory
10. **Epic 10** — UI & HUD
11. **Epic 8** — World & Exploration
12. **Epic 9** — Town Management

## Notes
- This plan is a living document. Stories will be broken down further as we approach each epic.
- DLC reserved: Hex (Dark Magic), Venom (Nature/Poison), Conjure (Summoning)
- Story numbers are placeholders — will be refined when each epic is scoped in detail
