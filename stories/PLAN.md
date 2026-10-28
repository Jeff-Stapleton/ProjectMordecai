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

### Epic 7: Inventory ✅ (complete)
Implement unlimited-carry inventory with town gating (per agent_rules_v2).
- US-032: Item Definition & Categories (HEADLESS — foundation: UMordecaiItemDefinition, enums, sort/stack helpers, item tags) ✅
- US-031: Flat Inventory & Auto-Store System (HEADLESS — UMordecaiInventoryComponent, UMordecaiResourceLedger, pickup routing) ✅
- US-033: Unidentified Items & Identification Service (HEADLESS — per-instance ID state, partial-info API, equip gating, service subsystem) ✅

### Epic 7.5: Playable Inventory Slice (integration milestone — 2/2 scoped)
Wire inventory, auto-store, and identification into the playable arena per Playability-First Rule.
- US-071: Inventory UI — Flat List, Filter & Ledger Widget (HEADLESS — UMordecaiInventoryWidget, category filter, rarity colors, identify action, ledger panel)
- US-079: Playable Inventory Arena Integration (EDITOR — pickup actor, 6 test DataAssets, arena placement, pause-menu tab wiring, PIE smoke test)

### Epic R: Playability Recovery ⚡ CURRENT PRIORITY (scoped 2026-10-22)
The first Phase 3 sweep (`py Scripts/playtest_story.py --all` on a fresh package of `8ea853b`, `Saved/Playtests/20261022-184702-sweep/summary.md`) and the first exploratory-agent run (`Saved/Playtests/20261022-185207-explore/report.md`) show that every player-facing feature marked done is NOT PLAYABLE in the packaged build (6/6 Tier 4 scripts FAIL; Tier 3 boot PASS). Per `stories/PLANNER.md` §5.1 this epic outranks every system epic. One recovery story per broken feature; each AC names the failing Tier 4 check of the owning story's existing script, and is done only when that check goes FAIL → PASS on a fresh package. Dependency order (combat abilities and enemies first: they unblock the death/respawn/kill checks):
- US-083: Player combat abilities granted at runtime + attack/dodge/block/sprint/jump bound (HEADLESS — N10, unbound input) — P0
- US-084: Enemy attacks reach the player (code) + enemy content guards (HEADLESS — N7) — P0, pair of US-085
- US-085: Frontliner content: visible mesh, 200 HP, attack profiles, placement (EDITOR — invisible enemies, N13, N7, N14) — P0, pair of US-084. In progress: 200 HP (N13) done, issue #18 (`39500e2`); visible bodies (N14's invisible part) done, issue #19 (`f21525f`); the spawn swarm is T1's (#46, Jeff 2026-10-28)
- US-094: Inventory tab current on every menu open (HEADLESS — CommonUI pool) — P0, unblocks in-progress US-071 / US-079
- US-086: Death screen shown on death + kill counter counts kills (HEADLESS — N1, N2; adds kill checks to us054) — after US-083/084/085
- US-087: Fireball projectile flies and hits; Blink moves and ends its invulnerability (HEADLESS — N15, N11, N19, N20)
- US-088: Spell cooldown tags + four bound HUD cooldown widgets (HEADLESS — cooldowns, N8) — pair of US-089
- US-089: DA_Spell_* cooldown tags and durations (EDITOR) — pair of US-088
- US-091: HUD readouts: status names, weapon name, no debug text (HEADLESS — N16, N3, on-screen debug/Steam text)
- US-092: Enemy indicators for enemies in aggro range (HEADLESS — N17)
- US-093: Pause menu Character / Skills / Feats tab contents (HEADLESS — N4, N5)
- US-095: Pause menu input: I closes, LB/RB switch tabs (HEADLESS — I-close, N18 ← TODO(DECISION) for LB/RB)
- US-097: Gamepad move / look / sprint key names (HEADLESS — invalid FKeys)
- US-096: Previous-weapon key + weapon effects reach the ASC (HEADLESS — N9 ← TODO(DECISION) Shift+Tab vs BackSlash)
- US-098: Facing and aim follow mouse / right stick (HEADLESS — BLOCKED on TODO(DECISION) fixed facing)
- US-099: Player visible behind near walls (HEADLESS, maybe + EDITOR pair — N21, BLOCKED on TODO(DECISION) cutaway approach)
- US-090: Spell point budget (HEADLESS — N6, BLOCKED on TODO(DECISION))

Note: US-079 and US-071 remain in `stories/in-progress/`. US-079's Tier 4 now FAILS on the second-open CommonUI pool bug (`Saved/Playtests/20261022-184957-us079`: `menu_rows` etc.); they resume after US-094.

### Epic 8: World & Exploration
Authored-cell world framework, movement/exploration skills, Metroidvania gating.

**Phase 1 — Cell Framework Foundation (Milestone 1, HEADLESS — scoped)**
Implements the *already-locked* data model + markup framework from `map_style_technical_design_v1.md` §12. Does NOT introduce gate types/biomes/tier rules (those need human approval).
- US-080: Cell Framework — Metadata Asset & Core World Types (HEADLESS — `UMordecaiCellMetadata`, biome/cell-type/height-tier/gate/exit enums, naming helpers)
- US-081: Cell Framework — Gameplay Markup Actors & Cell Registry (HEADLESS — 19 `PM_*` markup types, `AMordecaiGameplayMarker` + Gate/SpawnGroup subclasses, queryable `UMordecaiCellMarkupSubsystem`)
- US-082: Cell Framework — Validation Library & Report (HEADLESS — deterministic Critical/High/Medium checks, per-cell + cross-cell reports)

**Phase 2 — Skills & Content (blocked — see notes)**
- US-034: Movement Skills (Climbing, Swimming, Sneaking) ← BLOCKED: skill rank values are `{x}*rank` placeholders in skill_sheet (need decided constants); traversal interacts with US-081 markup
- US-035: Exploration Skills (Perception, Tracking, Cartography, Lockpicking, Traps) ← BLOCKED: same value-placeholder issue
- US-036: Metroidvania World Gates ← BLOCKED: gate-type introduction + biome graph + progression sequence require HUMAN APPROVAL per agent_rules_v2 / agentic_map_pipeline_v1 §8
- (future EDITOR) One-Biome Vertical Slice (Milestone 2): blockout template cell, `DA_Cell_*` content, cell-from-template + validation editor utilities — blocked on World Intent approval (Stage A)

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
- US-071: Inventory UI — Flat List, Filter & Ledger Widget (HEADLESS) ← scoped under Epic 7.5
- US-079: Playable Inventory Arena Integration (EDITOR) ← scoped under Epic 7.5
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
11. **Epic 7 — Inventory** ✅
12. **Epic R — Playability Recovery** ⚡ **CURRENT PRIORITY** (ahead of everything below, incl. Epic 7.5; `stories/PLANNER.md` §5.1)
    - Batch 1 (combat + enemies, unblock death/respawn/kill): US-083 (HEADLESS) → US-084 (HEADLESS) → US-085 (EDITOR, pair of US-084)
    - Batch 2: US-094 (HEADLESS, unblocks US-079/US-071) → US-086 (HEADLESS, needs US-083/084/085)
    - Batch 3 (spells): US-087 (HEADLESS) → US-088 (HEADLESS) → US-089 (EDITOR, pair of US-088)
    - Batch 4 (HUD + menus): US-091 → US-092 → US-093 → US-095 (all HEADLESS; US-095's LB/RB ACs wait on N18)
    - Batch 5 (input): US-097 (HEADLESS, after US-083) → US-096 (HEADLESS, N9 decision)
    - Blocked on Jeff's decisions: US-098 (facing), US-099 (cutaway), US-090 (SP budget)
    - Re-run `py Scripts/playtest_story.py --all` on a fresh package after each batch; every NEW failure becomes a recovery story or a Known issues entry
13. **Epic 7.5 — Playable Inventory Slice** (in progress; resumes after US-094)
    - Batch 1: US-071 (Inventory Widget, HEADLESS) — foundation, go first
    - Batch 2: US-079 (Playable Inventory Arena, EDITOR) — after US-071
14. **Epic 6 (ranged weapons)** — US-027/028/029/030 — needs design input for crossbow/throwables/wands/two-weapon/armor/shields
15. **Epic 8 Phase 1 — Cell Framework Foundation** (US-080 → US-081 → US-082) — HEADLESS, no blockers, good pipeline filler after Epic 7.5. Strict order: US-080 (types) → US-081 (markup, depends on US-080 enums) → US-082 (validation, depends on both).
16. **Epic 10 Phase 4** — Blueprint polish, VFX (US-070, US-076, US-075)
17. **Epic 10 Phase 5+** — Equipment/map/town UI (US-072/073/074 after their parent epics)
18. **Epic 8 Phase 2** — Movement/exploration skills + world gates — BLOCKED (skill values undecided; gates need human approval)
19. **Epic 9** — Town Management

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
- 2026-10-22: Epic R (Playability Recovery, US-083…US-099) scoped from the first Phase 3 sweep and exploratory run. "✅ complete" on the epics above records that their stories were closed under the old (PIE-only) rules; it does not mean the features are playable — see the Playable Status lines in `stories/PLANNER_LOG.md`.
- Per Playability-First Rule: Epic 6.5 was the last integration milestone. Epic 7 was pure system work. Epic 7.5 (US-071 + US-079) is the integration milestone that turns the inventory plumbing into something Jeff can pick up, filter, and identify in PIE.
