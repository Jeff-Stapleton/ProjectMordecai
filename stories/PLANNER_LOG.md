# Planner Log

## 2026-03-18 Nightly Planning Run

### Completed Since Last Run
- (none — this is the first planning run)

### Currently In Progress
- **US-001: Project Foundation & Initial Player Character** — in `stories/in-progress/`. No code exists yet in MordecaiCore plugin (empty source directory). Lyra base code is present.

### Changes to Existing Stories
- **US-001 updated**: Fixed AC-2.2.2 — changed "D&D attributes (STR, DEX, CON, INT, WIS, CHA)" to the canonical 9 primary attributes from character_attributes_v1.md: **STR, DEX, END, CON, RES, DIS, INT, WIS, CHA**. Updated corresponding test name.

### New Stories Created
- **US-002: Attack Profile Data Model & Damage Types** (`stories/backlog/US-002-attack-profile-data-model.md`)
  - Data-driven foundation: enums, structs, `UMordecaiAttackProfileDataAsset`, damage type gameplay tags
  - 10 automation tests covering all data types and serialization
  - Prerequisite for all other combat stories

- **US-003: Melee Hit Detection System** (`stories/backlog/US-003-melee-hit-detection.md`)
  - Spatial queries: arc sector (sweep), capsule (thrust), circle (slam)
  - Target filtering, JumpAvoidable/HitsAirborne rules, friendly fire toggle
  - 13 automation tests covering each shape, filtering, and airborne interactions
  - Depends on US-002 data types

- **US-004: Melee Attack GAS Ability & Combo System** (`stories/backlog/US-004-melee-attack-ability.md`)
  - GAS ability with Windup→Active→Recovery phases
  - Hit detection integration, damage/posture via GameplayEffects, stamina cost
  - Light combo chains with timeout/reset rules, cancel-into-dodge/block
  - 14 automation tests covering phases, damage, combos, cancels
  - Depends on US-002 and US-003

### PLAN.md Updates
- Refined Epic 2 from 5 placeholder stories to 7 properly scoped stories (US-002 through US-008)
- Renumbered downstream epics (3-10) to account for Epic 2 expansion
- Fixed Epic 7 (Inventory): changed "bag-based inventory" to "unlimited-carry with town gating" per agent_rules_v2.md — the old description directly contradicted the locked agent rules

### Blockers / Decisions Needed
- **stat_formulas_v1.md vs character_attributes_v1.md inconsistency**: stat_formulas lists only 5 primary attributes (STR, DEX, INT, CON, DIS) while character_attributes lists 9 (adds END, RES, WIS, CHA). The character_attributes doc appears authoritative (more recent, more detailed). stat_formulas_v1.md may need updating before Epic 3 work begins.
- **Damage formula stub**: US-004 implements only BasePower. The full formula (`BaseDamage x SkillModifier x AttributeScaling x CriticalModifier x StatusModifier`) requires Epics 3 and 4. Multipliers are stubbed at 1.0 for now.
- **Gameplay tag taxonomy (Open Item #1 from agent_rules_v2)**: US-002 introduces the first batch of native tags (`Mordecai.Damage.*`). A formal tag naming/ownership policy hasn't been decided yet.

### Next Session Recommendation
1. **Complete US-001** — the foundation must be in place before combat work starts. No MordecaiCore C++ files exist yet.
2. **Then US-002** — pure data model, no runtime dependencies. Quick win that unblocks US-003 and US-004.
3. **Then US-003 → US-004** — these form a clean chain: data → detection → ability.
