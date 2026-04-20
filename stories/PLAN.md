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

### Epic 2.5: Playable Vertical Slice ✅ (complete)
Wire up all completed combat systems into a playable prototype. The goal: Jeff can launch the game, run around a test map, fight an enemy, and exercise melee/dodge/block/parry/stamina/posture.
- US-050: Enemy Character & Damage Reception ✅
- US-051: Basic Enemy AI Combat Loop ✅
- US-052: Combat HUD C++ Framework ✅
- US-053: Player Death & Arena Game Flow ✅
- US-054: Playable Arena Integration ✅

### Epic 3: Attributes & Progression ✅ (complete)
Wire up the attribute system and skill progression.
- US-010: Effective Mod Formula & Core Attribute Scaling ✅
- US-011: Skill Framework & Rank Progression ✅
- US-012: Feat System ✅

### Epic 4: Status Effects ✅ (complete)
Implement the launch status effect set via GAS. Split into 8 stories by mechanical grouping.
- US-013: Status Effect Framework (GAS infrastructure, tags, stacking, immunity, cleanse) ✅
- US-014: DoT & Resource Denial Statuses (Burning, Bleeding, Poisoned) ✅
- US-015: Speed & Timing Impairment Statuses (Frostbitten, Shocked) ✅
- US-016: Combat Modifier Debuffs (Weakened, Brittle, Exposed, Corroded) ✅
- US-017: Action-Restricting Debuffs (Silenced, Rooted) ✅
- US-059: Perception & Mental Debuffs (Blinded, Fear, Cursed) ✅
- US-018: Drenched & Focused Buff (environmental interaction + perfect action streak) ✅

### Epic 5: Magic System ✅ (complete)
Implement spell framework and 14 launch spells at Rank 1. Higher rank milestones deferred.
- US-019: Spell Framework & Spell DataAsset ✅
- US-020: Damage Spells (Fireball, Cone of Cold, Magic Missile, Stone Skin) ✅
- US-021: Utility Spells (Blink, Sleep, Fire Ward) ✅
- US-022: Support Spells (Bless, Restoration) ✅
- US-023: Tactical Spells — Debuff Spells (Snare, Enfeeble, Enchant Weapon) ✅
- US-060: Tactical Spells — Summon & Evasion (Illusion, Blur) ✅

### Epic 5.5: Playable Magic Slice ✅ (complete)
Wire spells, status effects, and spell HUD into the playable arena. Integration milestone per Playability-First Rule.
- US-055: Spell HUD & Status Effect Indicators (HEADLESS — SP bar, buff/debuff indicators, cooldown display) ✅
- US-056: Status-Applying Attack Profiles (HEADLESS — extend attack profiles with on-hit status application) ✅
- US-057: Playable Magic Arena Integration (EDITOR — spell DataAssets, spell inputs, enemy status attacks, HUD wiring) ✅

### Epic 6: Weapons & Equipment (3/7 scoped, US-025 next)
Implement weapon classes and armor systems.
- US-024: Weapon Class Framework (HEADLESS — WeaponDataAsset, WeaponInstance, EquipmentComponent, equip/unequip via GAS, weapon slot model, melee attack integration) ✅
- US-025: Melee Weapons — Blade Family (HEADLESS — Longsword, Greatsword, Shortsword, Dagger factory profiles) ✅
- US-026: Melee Weapons — Blunt & Polearm (HEADLESS — Axe, Mace, Spear, Quarterstaff, Unarmed factory profiles) ✅
- US-027: Ranged Weapons (Longbow, Shortbow, Crossbow, Throwables, Wands) ← placeholder, needs design input
- US-028: Two-Weapon Fighting ← placeholder, needs design input
- US-029: Armor & Equipment System (Leather, Chain, Plate + skills) ← placeholder, needs design input
- US-030: Shield System ← placeholder, needs design input

### Epic 6.5: Playable Weapon Arena (integration milestone)
Wire melee weapons into the playable arena. Per Playability-First Rule (after Epics 4+5+10 Phase 2-3 system work).
- US-077: Weapon Cycling & Equipped Weapon Display (HEADLESS — cycling logic, weapon HUD widget, OnWeaponChanged delegate) ✅
- US-078: Playable Weapon Arena Integration (EDITOR — weapon DataAssets, cycling input, HUD wiring, arena setup, 5-weapon play-test) ✅

### Epic 7: Inventory (3/3 scoped, ready for implementation)
Implement unlimited-carry inventory with town gating (per agent_rules_v2).
- US-032: Item Definition & Categories (HEADLESS — foundation: UMordecaiItemDefinition, enums, sort/stack helpers, item tags) ✅
- US-031: Flat Inventory & Auto-Store System (HEADLESS — UMordecaiInventoryComponent, UMordecaiResourceLedger, pickup routing) ✅
- US-033: Unidentified Items & Identification Service (HEADLESS — per-instance ID state, partial-info API, equip gating, service subsystem) — after US-031

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

### Epic 10: UI & HUD (Phase 1 complete, Phase 2-4 scoped)
**Phase 1 — Combat HUD Completion ✅**
- US-055: Spell HUD & Status Effect Indicators (SP bar, status icons, cooldowns) ✅
- US-061: Combat Feedback — Combo Counter & Action Indicators ✅
- US-062: Death Screen & Respawn Countdown ✅
- US-063: Kill Counter Display ✅

**Phase 2 — World-Space Combat Indicators ✅**
- US-064: Enemy Indicator System Migration (Lyra IndicatorSystem) ✅
- US-065: Floating Damage Numbers (world-space, color-coded by damage type) ✅

**Phase 3 — Menus & Progression UI ✅**
- US-069: Pause Menu Framework (tabbed, CommonUI) ✅
- US-066: Character Sheet — Attributes & Derived Stats (HEADLESS — ASC-bound widget, 9 primary attrs + derived stats + core resources) ✅
- US-067: Skill Tree Display (HEADLESS — SkillComponent-bound widget, category tabs, rank/milestone display, point allocation) ✅
- US-068: Feat Display (HEADLESS — FeatComponent-bound widget, unlocked/locked feats, tier colors, condition progress) ✅

**Phase 4 — Blueprint & Visual Polish (EDITOR)**
- US-070: Combat HUD Blueprint Polish ← after Phase 1
- US-076: Pause Menu Blueprint & Tab Wiring ← after Phase 3
- US-075: Status Effect VFX (Niagara) ← after Epic 4 complete

**Future (blocked on later epics)**
- US-071: Inventory UI — Flat List & Filtering ← after Epic 7
- US-072: Equipment Panel — Weapon Sets & Armor ← after Epic 6+7
- US-073: Map — Metroidvania World Map ← after Epic 8
- US-074: Town Management UI ← after Epic 9

---

## Priority Order
1. **Epic 1** — Foundation ✅
2. **Epic 2** — Core Combat ✅
3. **Epic 3** — Attributes & Progression ✅
4. **Epic 4** — Status Effects ✅
5. **Epic 2.5** — Playable Vertical Slice ✅
6. **Epic 5** — Magic System ✅
7. **Epic 10 Phase 1** — Combat HUD Completion ✅
8. **Epic 4+5 remaining + Epic 5.5** — Status/magic completion + magic slice ✅
9. **Epic 10 Phase 2-3** — Enemy indicators, damage numbers, pause menu, progression UI ✅
10. **Epic 6 (melee weapons) + Epic 6.5 (playable weapon slice)** ✅
11. **Epic 7 — Inventory** ⚡ **CURRENT PRIORITY**
    - Batch 1: US-032 (Item Definition & Categories, HEADLESS) — foundation, go first
    - Batch 2: US-031 (Flat Inventory & Auto-Store, HEADLESS) — after US-032
    - Batch 3: US-033 (Unidentified Items & Identification Service, HEADLESS) — after US-031
12. **Epic 6 (ranged weapons)** — US-027/028/029/030 — needs design input for crossbow/throwables/wands/two-weapon/armor/shields
13. **Epic 10 Phase 4** — Blueprint polish, VFX (US-070, US-076, US-075)
14. **Epic 10 Phase 5+** — Inventory/town/map UI (US-071 Inventory UI unblocks after Epic 7)
15. **Epic 8** — World & Exploration
16. **Epic 9** — Town Management

## Notes
- This plan is a living document. Stories will be broken down further as we approach each epic.
- DLC reserved: Hex (Dark Magic), Venom (Nature/Poison), Conjure (Summoning)
- Story numbers are placeholders for unscoped epics — will be refined when each epic is scoped in detail
- US-017 was split: Silenced + Rooted stay in US-017; Blinded + Fear + Cursed moved to US-059
- US-023 was split: Snare + Enfeeble + Enchant Weapon stay in US-023; Illusion + Blur moved to US-060
- Epic 6 US-025 (Melee Weapons) split into US-025 (Blades) and US-026 (Blunt/Polearm). Original US-026–029 renumbered to US-027–030
- Epic 6.5 added as integration milestone: US-077 (weapon cycling C++) + US-078 (playable weapon arena EDITOR)
- US-027–030 marked as needing design input: crossbow, throwables, wands, two-weapon fighting, armor, and shields have insufficient design detail in current docs
- Epic 7 (Inventory) implementation order: US-032 → US-031 → US-033. US-032 delivers item definition foundation; US-031 and US-033 depend on it.
- Per Playability-First Rule: Epic 6.5 was the last integration milestone. Epic 7 is pure system work (HEADLESS). After Epic 7, next integration candidate is UI-focused (US-071 Inventory UI — EDITOR — after Epic 7 complete).
