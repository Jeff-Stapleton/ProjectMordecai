# Implementation Plan — Project Mordecai

## How This Works
1. Arc (coordinator) scopes features from design docs into user stories
2. Stories go in `backlog/` with acceptance criteria
3. When work starts, story moves to `in-progress/`
4. Claude Code picks up the story, writes tests, implements, verifies, commits
5. Completed stories move to `done/`

## Epics (High Level)

### Epic 1: Foundation ✅ (in progress)
Get a running project with a controllable character.
- US-001: Project Foundation & Initial Player Character

### Epic 2: Core Combat
Implement the combat system: attacks, defense, posture, stamina.
- US-002: Attack Profile Data Model & Damage Types (data foundation) ← **scoped**
- US-003: Melee Hit Detection System (overlap shapes, filtering, airborne rules) ← **scoped**
- US-004: Melee Attack GAS Ability & Combo System (phases, damage, combos) ← **scoped**
- US-005: Defense Mechanics (Dodge/Block/Parry)
- US-006: Posture & Stagger System
- US-007: Stamina Tier System
- US-008: Projectile System & Aim Assist

### Epic 3: Attributes & Progression
Wire up the attribute system and skill progression.
- US-009: Attribute Scaling (STR/DEX/END/CON/RES/DIS/INT/WIS/CHA)
- US-010: Skill Leveling & Milestone Unlocks
- US-011: Feat System (Achievement-Based Unlocks)

### Epic 4: Status Effects
Implement the launch status effect set via GAS.
- US-012: Status Effect Framework (GAS-based)
- US-013: Physical Statuses (Burning, Bleeding, Poisoned, Frostbitten, Shocked)
- US-014: Debuff Statuses (Weakened, Brittle, Silenced, Rooted, Blinded, Fear, Cursed, Exposed, Corroded, Drenched)
- US-015: Focused Buff (Perfect Action Streak)

### Epic 5: Magic System
Implement spells with rank progression.
- US-016: Spell Framework (SP cost, casting, upcasting, rank system)
- US-017: Damage Spells (Fireball, Frost Nova, Magic Missile, Stone Skin)
- US-018: Utility Spells (Blink, Sleep/Wind, Fire Ward)
- US-019: Support Spells (Bless, Restoration)
- US-020: Tactical Spells (Illusion, Blur, Snare, Enchant Weapon, Enfeeble)

### Epic 6: Weapons & Equipment
Implement weapon classes and armor systems.
- US-021: Weapon Class Framework (data-driven profiles)
- US-022: Melee Weapons (Longsword, Greatsword, Shortsword, Dagger, Axe, Mace, Spear, Quarterstaff, Unarmed)
- US-023: Ranged Weapons (Longbow, Shortbow, Crossbow, Throwables, Wands)
- US-024: Two-Weapon Fighting
- US-025: Armor System (Leather, Chain, Plate + skills)
- US-026: Shield System

### Epic 7: Inventory
Implement unlimited-carry inventory with town gating (per agent_rules_v2).
- US-027: Flat Inventory & Auto-Store System
- US-028: Item Categories & Stack Rules
- US-029: Unidentified Item & Town Gating Flow

### Epic 8: World & Exploration
Movement skills, exploration systems, Metroidvania gating.
- US-030: Movement Skills (Climbing, Swimming, Sneaking)
- US-031: Exploration Skills (Perception, Tracking, Cartography, Lockpicking, Traps)
- US-032: Metroidvania World Gates

### Epic 9: Town Management
Delegated town systems.
- US-033: Town Building Framework
- US-034: NPC Job Assignment
- US-035: Initiatives & Upgrades
- US-036: Town Events

### Epic 10: UI & HUD
- US-037: Combat HUD (Health%, Stamina Tiers, SP)
- US-038: Status Effect VFX
- US-039: Inventory UI

---

## Priority Order
1. **Epic 1** — Foundation (in progress)
2. **Epic 2** — Core Combat (next)
3. **Epic 4** — Status Effects (needed by combat)
4. **Epic 3** — Attributes & Progression
5. **Epic 5** — Magic System
6. **Epic 6** — Weapons & Equipment
7. **Epic 7** — Inventory
8. **Epic 10** — UI & HUD
9. **Epic 8** — World & Exploration
10. **Epic 9** — Town Management

## Notes
- This plan is a living document. Stories will be broken down further as we approach each epic.
- DLC reserved: Hex (Dark Magic), Venom (Nature/Poison), Conjure (Summoning)
- Story numbers are placeholders — will be refined when each epic is scoped in detail
