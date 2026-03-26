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

---

## 2026-03-19 Nightly Planning Run

### Completed Since Last Run
- (none — US-001 still in progress)

### Currently In Progress
- **US-001: Project Foundation & Initial Player Character** — still in `stories/in-progress/`. **Critical finding:** MordecaiCore plugin has content assets (input actions, maps, blueprints, camera modes, experiences) and compiled binaries (DLL/PDB), but the C++ source files are MISSING from `Source/MordecaiCoreRuntime/`. Only the `Build.cs` file exists. UHT-generated intermediates reference classes (MordecaiCharacter, MordecaiAbilitySystemComponent, MordecaiAttributeSet, MordecaiCameraMode_Diorama, MordecaiDebugHUD, MordecaiGameMode, MordecaiGameState, MordecaiHeroComponent) that were compiled at some point but whose source is gone. The entire `Plugins/GameFeatures/MordecaiCore/` directory remains untracked in git. No Mordecai automation tests exist yet.

### New Stories Created
All **HEADLESS** execution mode. All placed in `stories/backlog/`.

- **US-005: Dodge System** (`US-005-dodge-system.md`)
  - GAS ability with i-frames via `Mordecai.State.Dodging` tag (not collision disable)
  - Directional dodge from movement input, backward default
  - Perfect Dodge timing window with stamina refund reward
  - Cancel-into-dodge from attack Recovery, cooldown, movement lock during dodge
  - 12 automation tests
  - Split from old "US-005: Defense Mechanics" — dodge is now its own story

- **US-006: Block & Parry System** (`US-006-block-parry-system.md`)
  - Block: held ability (LT/RMB), damage mitigation formula, stamina drain per hit, guard break at 0 stamina
  - Perfect Block: timing window, no stamina drain, posture damage to attacker
  - Parry: simultaneous LT+RT/RMB+LMB input, narrow window, large posture damage + riposte on success, vulnerability on failure
  - Cancel-into-block from attack Recovery, block allows reduced movement
  - 17 automation tests (7 block, 3 perfect block, 7 parry)
  - Split from old "US-005: Defense Mechanics" — block/parry together as they share input mechanics

- **US-007: Posture & Stagger System** (`US-007-posture-stagger-system.md`)
  - Posture damage via GE with SetByCaller, scaled by PostureDamageScalar from attack profiles
  - Posture break at zero → PostureBroken stagger state with riposte damage multiplier
  - Posture regen with delay after damage, no regen during broken state
  - Blocked hits still apply posture damage at reduced rate
  - Weakened/Brittle interfaces stubbed for Epic 4
  - 12 automation tests
  - Was US-006, renumbered to US-007

- **US-008: Stamina Tier System** (`US-008-stamina-tier-system.md`)
  - Three tiers (Green >66%, Yellow 33-66%, Red <33%) + Exhausted at ≤0
  - Tier gameplay tags: `Mordecai.Stamina.Tier.{Green,Yellow,Red,Exhausted}`
  - Effectiveness multipliers per tier (damage, dodge distance, block stability)
  - Actions NEVER blocked — even at 0/negative stamina
  - Regen with delay, sprint drain, standardized consumption GE
  - 14 automation tests
  - Was US-007, renumbered to US-008

- **US-009: Projectile System & Aim Assist** (`US-009-projectile-aim-assist.md`)
  - `AMordecaiProjectile` actor with UProjectileMovementComponent, gravity, lifetime
  - Pierce (pass through N targets), Ricochet (redirect to nearby target), On-hit AoE
  - Soft aim assist subsystem: candidate directions at ±2°/4°/6° offsets, raycast, pick best
  - No lock-on or snapping — subtle angular correction only
  - Reuses damage GE pipeline from US-004
  - 18 automation tests (12 projectile, 6 aim assist)
  - Was US-008, renumbered to US-009

### PLAN.md Updates
- Split old "US-005: Defense Mechanics (Dodge/Block/Parry)" into two stories: US-005 (Dodge) and US-006 (Block/Parry)
- This added one story to Epic 2, so all downstream story numbers shifted +1
- Epic 2 is now fully scoped: 8 stories (US-002 through US-009), all HEADLESS
- Renumbered Epics 3-10 stories: US-010 through US-040

### Blockers / Decisions Needed

**Carried from last run:**
- **stat_formulas_v1.md inconsistency** — still unresolved. Will need updating before Epic 3.
- **Gameplay tag taxonomy** — US-005 through US-009 introduce more tags (`Mordecai.State.*`, `Mordecai.Stamina.*`, `Mordecai.Data.*`, `Mordecai.Team.*`). Formal naming policy still undecided.
- **Damage formula stub** — still applies. US-007 and US-008 add posture and stamina tier multipliers but attribute scaling remains stubbed.

**New this run:**
- **CRITICAL: US-001 C++ source files missing** — The MordecaiCore plugin has compiled binaries and content assets from a previous session, but all C++ source files (*.h, *.cpp) are gone from `Source/MordecaiCoreRuntime/`. Only `Build.cs` remains. The coding agent must rewrite these classes from scratch. Content assets (input actions, maps, BPs) may still be usable.
- **TODO(DECISION) items across new stories** (these need design input before implementation):
  - Perfect Dodge timing window (`PerfectDodgeWindowMs`) and stamina refund amount
  - Perfect Block timing window (`PerfectBlockWindowMs`)
  - Parry timing window (`ParryWindowMs`) and failed parry consequences (`ParryWhiffDurationMs`, `ParryWhiffDamageMultiplier`)
  - Guard broken duration (`GuardBrokenDurationMs`)
  - Posture broken duration (`PostureBrokenDurationMs`) and riposte damage multiplier (`RiposteDamageMultiplier`)
  - Stamina tier effectiveness multipliers (Yellow, Red, Exhausted)
  - Whether pierce has damage falloff per target
- **Parry input detection**: The control bindings spec shows Parry as LT+RT simultaneous press. This requires custom input detection logic (simultaneous press within a tolerance window) that doesn't exist yet. The input system from US-001 will need to support this. May need a small input detection component.

### Dependency Graph for Epic 2
```
US-001 (Foundation) ──┬──> US-002 (Data Model) ──┬──> US-003 (Hit Detection) ──> US-004 (Melee Attack)
                      │                          │
                      │                          └──> US-009 (Projectile/Aim Assist)
                      │
                      ├──> US-005 (Dodge) ────────────> requires US-004 for cancel-into-dodge testing
                      │
                      ├──> US-006 (Block/Parry) ──────> requires US-004 for cancel-into-block testing
                      │                                  requires US-007 for posture damage on parry
                      │
                      ├──> US-007 (Posture) ──────────> requires US-004 for posture damage source
                      │
                      └──> US-008 (Stamina Tiers) ───> standalone, but US-005/006/004 read tier multipliers
```

### Next Session Recommendation
1. **Complete US-001** — CRITICAL BLOCKER. C++ source must be rewritten. Content assets exist and may be reusable. Foundation is prerequisite for everything.
2. **Then US-002** → **US-003** → **US-004** — core melee chain (data → detection → ability)
3. **Then US-008** (Stamina Tiers) — standalone, no combat dependencies. Can parallelize with US-002.
4. **Then US-007** (Posture) → **US-005** (Dodge) → **US-006** (Block/Parry) — defense systems
5. **Then US-009** (Projectile) — depends on US-002 + US-003, can start after those complete
6. All stories are HEADLESS — no editor needed for any Epic 2 work.

---

## 2026-03-19 Nightly Planning Run (2)

### Completed Since Last Run
- (none — US-001 still in progress)

### Currently In Progress
- **US-001: Project Foundation & Initial Player Character** — still in `stories/in-progress/`. C++ source now exists in `Source/LyraGame/Mordecai/` (MordecaiCharacter, GameMode, GameState, PlayerController, PlayerState, AbilitySystemComponent, AttributeSet, CameraMode_Diorama). Test files exist but are `.disabled` (Attributes, Camera, Character, Foundation, Input, Level). MordecaiCore plugin has `Build.cs` only for C++ source. Implementation partially complete, tests not yet running.

### New Stories Created
All **HEADLESS** execution mode. All placed in `stories/backlog/`. These scope **Epic 4: Status Effects** — expanding from 4 placeholder stories to 6 properly scoped stories.

- **US-013: Status Effect Framework** (`US-013-status-effect-framework.md`)
  - GAS infrastructure: base GE subclass, status effect component, 16 status tags + 5 category tags + 16 immunity tags
  - Application/removal API, duration management, periodic tick support
  - Immunity via `Mordecai.Immunity.*` tags blocking GE application
  - Cleanse by category (e.g., cleanse all DoT statuses)
  - Stacking behavior: `TODO(DECISION)`, defaulting to duration refresh
  - Status-to-category mapping function
  - 14 automation tests
  - Prerequisite for all other Epic 4 stories

- **US-014: DoT & Resource Denial Statuses** (`US-014-dot-resource-denial-statuses.md`)
  - Burning: periodic fire damage, cast interruption chance, duration refresh on re-apply
  - Bleeding: healing reduction (50%), hit-refresh timer, clot mechanic (auto-remove after no damage)
  - Poisoned: stamina regen suppression (60%), movement speed reduction (15%)
  - Introduces new attributes: `HealingReceivedMultiplier`, `StaminaRegenRate`, `MoveSpeedMultiplier`
  - 13 automation tests (5 Burning, 4 Bleeding, 3 Poisoned + immunity tests)
  - Depends on US-013

- **US-015: Speed & Timing Impairment Statuses** (`US-015-speed-timing-impairment-statuses.md`)
  - Frostbitten: movement/attack speed reduction, dodge recovery increase, stack-based freeze at max stacks
  - Shocked: micro-stun on hit (stacking chance), block stamina cost increase, cast interruption
  - Both use GAS stack counts for escalating pressure
  - Introduces: `AttackSpeedMultiplier`, `DodgeRecoveryMultiplier`, `BlockStaminaCostMultiplier` attributes; `Mordecai.Status.Frozen`, `Mordecai.Status.MicroStunned` tags
  - 13 automation tests (8 Frostbitten, 5 Shocked)
  - Depends on US-013

- **US-016: Combat Modifier Debuffs** (`US-016-combat-modifier-debuffs.md`)
  - Weakened: outgoing posture damage reduction (40%), heavy attack partial bypass
  - Brittle: incoming posture damage increase (35%), max posture reduction (20%)
  - Exposed: next hit deals +30% bonus damage, consumed on first hit or timer
  - Corroded: block stability reduction (30%), armor efficiency reduction (25%), disables repair buffs
  - Introduces: `OutgoingPostureDamageMultiplier`, `IncomingPostureDamageMultiplier`, `IncomingDamageMultiplier`, `BlockStabilityMultiplier`, `ArmorEfficiencyMultiplier` attributes
  - 14 automation tests (4 Weakened, 3 Brittle, 4 Exposed, 3 Corroded)
  - Depends on US-013; interfaces with US-007 (Posture), US-006 (Block)

- **US-017: Control & Mental Debuffs** (`US-017-control-mental-debuffs.md`)
  - Silenced: blocks spell casting (tag-based), pauses SP regen
  - Rooted: blocks movement only, allows all other actions, break-free stamina action
  - Blinded: reduces ranged accuracy (40%), aim assist (50%), stealth detection (60%)
  - Fear: amplifies stamina tier penalties (50%), reduces guard stability (25%), aim drift
  - Cursed: reduces SP regen (50%), healing (30%), disables some buffs (TODO(DECISION))
  - Introduces: `SpellPointRegenMultiplier`, `RangedAccuracyMultiplier`, `AimAssistMultiplier`, `StealthDetectionMultiplier`, `StaminaTierPenaltyMultiplier` attributes
  - 18 automation tests (3 Silenced, 6 Rooted, 4 Blinded, 4 Fear, 3 Cursed)
  - Depends on US-013

- **US-018: Drenched & Focused Buff** (`US-018-drenched-focused-buff.md`)
  - Drenched: fire damage reduced, Shocked/Frostbitten amplified, douses Burning (bidirectional fire interaction)
  - Focused: perfect action streak self-buff, infinite duration until hit, stamina tier forgiveness + posture damage bonus
  - `UMordecaiPerfectActionTracker` component for streak counting
  - Introduces: `FireDamageReceivedMultiplier` attribute, `Mordecai.Event.PerfectAction` tag
  - 14 automation tests (7 Drenched, 7 Focused)
  - Depends on US-013; interfaces with US-014 (Burning), US-015 (Frostbitten, Shocked)

### PLAN.md Updates
- Expanded Epic 4 from 4 placeholder stories (US-013–016) to 6 scoped stories (US-013–018)
- Split by mechanical grouping: framework → DoT → speed/timing → combat modifiers → control/mental → environmental/buff
- Original "Physical Statuses" (5 in one story) split into US-014 (3 DoT) + US-015 (2 speed/timing)
- Original "Debuff Statuses" (10 in one story) split into US-016 (4 combat modifiers) + US-017 (5 control/mental)
- Original "Focused Buff" combined with Drenched into US-018 (both small, complementary)
- All downstream story numbers shifted +2: Epic 5 now starts at US-019, Epic 6 at US-024, etc.
- Total stories: 42 (was 40)

### Dependency Graph for Epic 4
```
US-013 (Framework) ──┬──> US-014 (Burning, Bleeding, Poisoned)
                     ├──> US-015 (Frostbitten, Shocked)
                     ├──> US-016 (Weakened, Brittle, Exposed, Corroded)
                     ├──> US-017 (Silenced, Rooted, Blinded, Fear, Cursed)
                     └──> US-018 (Drenched, Focused)
                                ├── interfaces with US-014 (Burning fire/drenched interaction)
                                └── interfaces with US-015 (Frostbitten/Shocked amplification)
```
US-014 through US-017 are independent of each other (all depend only on US-013).
US-018 has cross-references to US-014 and US-015 for elemental synergies but can stub those interfaces.

### Attribute Set Expansion Tracker
Epic 4 stories collectively introduce many new multiplier attributes on `UMordecaiAttributeSet`. Full list:
- US-014: `HealingReceivedMultiplier`, `StaminaRegenRate`, `MoveSpeedMultiplier`
- US-015: `AttackSpeedMultiplier`, `DodgeRecoveryMultiplier`, `BlockStaminaCostMultiplier`
- US-016: `OutgoingPostureDamageMultiplier`, `IncomingPostureDamageMultiplier`, `IncomingDamageMultiplier`, `BlockStabilityMultiplier`, `ArmorEfficiencyMultiplier`
- US-017: `SpellPointRegenMultiplier`, `RangedAccuracyMultiplier`, `AimAssistMultiplier`, `StealthDetectionMultiplier`, `StaminaTierPenaltyMultiplier`
- US-018: `FireDamageReceivedMultiplier`

All default to 1.0 (neutral multiplier). Multiple statuses can modify the same attribute (e.g., both Corroded and Fear modify `BlockStabilityMultiplier`). GAS handles stacking.

### Blockers / Decisions Needed

**Carried from last run:**
- **stat_formulas_v1.md inconsistency** — still unresolved
- **Gameplay tag taxonomy** — Epic 4 adds ~40 new tags (`Mordecai.Status.*`, `Mordecai.Immunity.*`, `Mordecai.Status.Category.*`, `Mordecai.Event.*`). Formal naming policy still undecided.
- **Damage formula stub** — StatusModifier slot in damage formula now has concrete consumers (Exposed's `IncomingDamageMultiplier`, Weakened's posture modifier, etc.)
- **US-001 C++ source** — partially exists in `Source/LyraGame/Mordecai/` (not in MordecaiCore plugin). Tests are `.disabled`.

**New this run (TODO(DECISION) items for Epic 4):**
- **Stacking policy** (agent_rules_v2 Open Item #2): All stories default to duration refresh. Frostbitten and Shocked use stack counts. Final stacking rules need design input.
- **Bleeding clot timer** (`BleedingClotTimeSec`): How long without taking damage before bleed auto-removes? Defaulted to 4.0s.
- **Frostbitten stack-to-freeze threshold** (`FrostbittenMaxStacks`): Defaulted to 5. Perfect dodge stack removal: remove 1 or all stacks?
- **Shocked micro-stun parameters**: `ShockedMicroStunDurationMs` (300ms), `ShockedMicroStunChancePerStack` (15%). These directly affect combat feel.
- **Exposed consumption**: Does perfect block remove Exposed without triggering the bonus? (Currently yes — but this is a gameplay judgment call.)
- **Drenched↔Fire interaction**: When fire hits a Drenched target, does it remove Drenched and skip Burning? (Current: yes. Alternative: remove Drenched, still apply Burning.)
- **Cursed buff suppression**: Which buffs does Cursed disable? Currently TODO(DECISION). Needs a clear list.
- **Focused streak threshold** (`FocusedStreakThreshold`): How many consecutive perfect actions? Defaulted to 3.

### Next Session Recommendation
1. **Complete US-001** — still the critical blocker. C++ source exists but tests are disabled.
2. **Then US-002 → US-003 → US-004** — core melee chain
3. **Then US-008** (Stamina Tiers) — can parallelize with US-002
4. **Then US-007 → US-005 → US-006** — defense systems
5. **Then US-009** (Projectile) — after US-002 + US-003
6. **Then US-013** (Status Effect Framework) — gate for all Epic 4 work
7. **Then US-014 through US-018** — can be parallelized (all depend only on US-013)
8. All Epic 2 and Epic 4 stories are **HEADLESS** — no editor needed.

---

## 2026-03-21 Nightly Planning Run

### Completed Since Last Run
- **US-001: Project Foundation & Initial Player Character** — moved to `stories/done/`
- **US-002: Attack Profile Data Model & Damage Types** — moved to `stories/done/`
- **US-003: Melee Hit Detection System** — moved to `stories/done/`
- **US-004: Melee Attack GAS Ability & Combo System** — moved to `stories/done/`
- **US-005: Dodge System** — moved to `stories/done/`
- **US-006: Block & Parry System** — moved to `stories/done/`

### Currently In Progress
- (none — all 6 completed stories moved to done, nothing currently in-progress)

### Existing Backlog (unchanged)
- **US-007**: Posture & Stagger System (HEADLESS, 16 ACs)
- **US-008**: Stamina Tier System (HEADLESS, 13 ACs)
- **US-009**: Projectile System & Aim Assist (HEADLESS, 28 ACs)
- **US-013–018**: Status Effects (HEADLESS, 6 stories, 100 ACs total)

### New Stories Created
All **HEADLESS** execution mode. All placed in `stories/backlog/`. These scope **Epic 3: Attributes & Progression**.

- **US-010: Effective Mod Formula & Core Attribute Scaling** (`US-010-attribute-scaling.md`)
  - Diminishing returns formula: `EffMod = min(Mod,5) + max(min(Mod-5,5),0)*0.7 + max(min(Mod-10,5),0)*0.45 + max(Mod-15,0)*0.25`
  - 9 primary scaling effects (STR→PhysDmg 3.0%, DEX→AtkSpd 1.5%, END→Stamina 4.0%, CON→Health 6.0%, RES→AfflictionResist 3.0%, DIS→Posture 4.0%, INT→SpellPoints 4.0%, WIS→MagicDmg 3.0%, CHA→CastSpd 1.5%)
  - 9 secondary scaling effects (ArmorPen, PhysCrit, StaminaRegen, HealthRegen, AfflictionRecovery, PostureRecovery, SPRegen, ResPen, MagicCrit)
  - New derived attributes on `UMordecaiAttributeSet`, GAS recalculation via MMCs
  - 26 automation tests
  - Depends on: existing AttributeSet (US-001)

- **US-011: Skill Framework & Rank Progression** (`US-011-skill-framework.md`)
  - `UMordecaiSkillDataAsset` (PrimaryDataAsset) with category, rank descriptions, milestone ability mappings
  - `UMordecaiSkillComponent` (ActorComponent) for per-character skill rank storage (TMap<FName, int32>)
  - Skill point allocation API, rank clamping [0, 20], replication
  - Milestone system: gameplay events + ability grants + gameplay tags at Ranks 1/5/10/15/20
  - Sample Longswords skill validates full pipeline
  - 17 automation tests
  - Standalone — no combat system dependencies

- **US-012: Feat System (Achievement-Based Unlocks)** (`US-012-feat-system.md`)
  - `UMordecaiFeatDataAsset` with tiers (Common/Rare/Legendary), conditions, granted effects, drawback effects
  - `FMordecaiFeatCondition` supporting 4 condition types: StatThreshold, EventCount, TagPresent, SkillRank
  - `UMordecaiFeatComponent` for feat tracking, stat incrementing, auto-unlock evaluation
  - Tier behavior: Common (no drawback), Rare/Legendary (drawback effects applied)
  - Sample "Pyromaniac" feat validates pipeline (burn 6 enemies → unlock)
  - 21 automation tests
  - Soft dependency on US-011 (SkillRank condition type queries SkillComponent)

### PLAN.md Updates
- Marked Epic 3 as fully scoped (3 stories, all HEADLESS)
- Updated story descriptions with implementation detail
- No renumbering needed — US-010, US-011, US-012 were already reserved

### Dependency Graph for Epic 3
```
US-010 (Attribute Scaling) ——— standalone, depends only on existing AttributeSet
US-011 (Skill Framework) ———— standalone
US-012 (Feat System) ————————— soft dependency on US-011 (SkillRank condition type)
```
US-010 and US-011 are fully independent of each other and can be parallelized.
US-012 should follow US-011 (or at minimum stub the SkillRank condition).

### Blockers / Decisions Needed

**Carried from prior runs:**
- **Gameplay tag taxonomy** — US-010–012 add more tags (`Mordecai.Skill.*`, `Mordecai.Feat.*`, `Mordecai.Event.SkillMilestone`, `Mordecai.Event.FeatUnlocked`). Formal naming policy still undecided.
- **Damage formula stub** — US-010 provides the attribute multipliers (PhysicalDamageMultiplier, etc.) but they are not yet consumed by the damage pipeline. Full formula integration is needed when damage calculation is unified.

**New this run:**
- **TODO(DECISION): stat_formulas_v1.md vs character_attributes_v1.md reconciliation** — stat_formulas_v1.md defines linear base formulas (`Health = BaseHealth + CON × 10`) while character_attributes_v1.md defines percentage-based Effective Mod scaling (`CON → 6.0% × EffMod`). These potentially conflict. stat_formulas lists only 5 primary attributes; character_attributes lists 9. US-010 defaults to character_attributes_v1.md as authoritative. **Recommendation: deprecate stat_formulas_v1.md or explicitly reconcile the two docs before US-010 implementation.**
- **TODO(DECISION): Mod input value** — Does "Mod" in the Effective Mod formula equal the raw attribute value (STR 12 → Mod 12) or a derived modifier (STR 12 → Mod 2)? Examples in character_attributes_v1.md strongly suggest Mod = raw attribute value. Defaulted to this interpretation.
- **TODO(DECISION): Skill point economy** — How many skill points per level? 50 skills × 20 ranks = 1000 total ranks vs 60 max level. Distribution needs design input.
- **TODO(DECISION): Skill rank-down** — Can skill ranks decrease (curses, respec)? The system supports it but no trigger is defined.
- **TODO(DECISION): Full feat list** — Only one example feat exists ("Pyromaniac"). A complete feat list with conditions, effects, and drawback definitions needs design authoring.
- **TODO(DECISION): Feat stat reset behavior** — Should feat tracking stats (e.g., "enemies burned simultaneously") reset per encounter or be lifetime values? Defaulted to: thresholds use peak watermark, counts are cumulative.

### Attribute Set Expansion Tracker (Updated)
Epic 3 adds new derived attributes to `UMordecaiAttributeSet`:
- US-010: `PhysicalDamageMultiplier`, `AttackSpeedMultiplier`, `AfflictionResistMultiplier`, `MagicDamageMultiplier`, `CastSpeedMultiplier`, `ArmorPenetrationMultiplier`, `PhysicalCritChance`, `StaminaRegenMultiplier`, `HealthRegenMultiplier`, `AfflictionRecoveryMultiplier`, `PostureRecoveryMultiplier`, `SpellPointsRegenMultiplier`, `ResistancePenetrationMultiplier`, `MagicCritChance`

Combined with Epic 4 attributes (US-014–018), the attribute set will grow significantly. All multipliers default to 1.0; all additive chances default to 0.0.

### Next Session Recommendation
1. **US-007** (Posture & Stagger) — next in Epic 2 dependency chain. HEADLESS.
2. **US-008** (Stamina Tiers) — can parallelize with US-007. HEADLESS.
3. **US-009** (Projectile & Aim Assist) — after US-007/008. HEADLESS.
4. **US-013** (Status Effect Framework) — gate for all Epic 4 work. HEADLESS.
5. **US-010** and **US-011** can start anytime (no combat dependencies) — good candidates for parallel work while combat stories proceed.
6. All stories remain **HEADLESS** — no editor needed through Epic 4.

---

## 2026-03-22 Nightly Planning Run

### Completed Since Last Run
- **US-007: Posture & Stagger System** — moved to `stories/done/`
- **US-008: Stamina Tier System** — moved to `stories/done/`
- **US-009: Projectile System & Aim Assist** — moved to `stories/done/`
- **US-010: Effective Mod Formula & Core Attribute Scaling** — moved to `stories/done/`

**Epic 2 (Core Combat) is now COMPLETE** — all 8 stories (US-002 through US-009) done.
**Epic 3** — US-010 done (1 of 3 stories).

### Currently In Progress
- (none — backlog being replenished with Epic 2.5)

### Existing Backlog (unchanged)
- **US-011**: Skill Framework & Rank Progression (HEADLESS)
- **US-012**: Feat System (HEADLESS)
- **US-013–018**: Status Effects (HEADLESS, 6 stories)

### New Stories Created
**Epic 2.5: Playable Vertical Slice** — 5 stories, 4 HEADLESS + 1 EDITOR. All placed in `stories/backlog/`.

Per the Playability-First Rule (PLANNER.md §5.1): after completing 2 system-focused epics (Epic 2 Core Combat + partial Epic 3 Attributes), the next priority MUST be a playable integration milestone. Epic 2.5 is that milestone.

The original PLAN.md placeholder stories (US-050–054) were re-scoped because key infrastructure already exists: player BP, test map, input actions, camera mode, dev experience, and pawn data are all in the MordecaiCore plugin. The re-scoped stories focus on what's genuinely missing.

- **US-050: Enemy Character & Damage Reception** (`US-050-enemy-character-damage-reception.md`) — **HEADLESS**
  - `AMordecaiEnemyCharacter` extending `ALyraCharacter` with own ASC + `UMordecaiAttributeSet`
  - Health damage via SetByCaller GE, death at Health ≤ 0 (Dead tag, movement disabled, event broadcast)
  - Posture damage, posture break at zero, posture regen after delay
  - `Mordecai.Team.Enemy` tag, configurable base stats, death prevents further damage
  - 12 automation tests
  - Standalone — depends only on existing combat infrastructure

- **US-051: Basic Enemy AI Combat Loop** (`US-051-basic-enemy-ai.md`) — **HEADLESS**
  - `AMordecaiEnemyAIController` with C++ state machine (Idle/Approach/Attack/Recover/Staggered/Leash/Dead)
  - Distance-based detection (aggro range), approach via `MoveToActor`, attack via `TryActivateAbilityByClass`
  - Cooldown between attacks, leash to spawn point when player leaves range
  - Reacts to PostureBroken and Dead tags, server-authority only
  - 11 automation tests
  - Depends on US-050 (needs enemy character)

- **US-052: Combat HUD C++ Framework** (`US-052-combat-hud-framework.md`) — **HEADLESS**
  - `UMordecaiCombatHUDWidget` root container + `UMordecaiHealthBarWidget`, `UMordecaiStaminaBarWidget`, `UMordecaiPostureBarWidget`
  - Health as 0–100% of MaxHealth, stamina with tier color (Green/Yellow/Red/Exhausted), posture meter
  - `BindToASC()` for real-time attribute change delegation
  - `UMordecaiEnemyHealthBarWidget` for world-space enemy health display
  - All percent/tier computation in static testable functions
  - 12 automation tests
  - Standalone — reads existing attributes

- **US-053: Player Death & Arena Game Flow** (`US-053-death-respawn-arena-flow.md`) — **HEADLESS**
  - Player death at Health ≤ 0: Dead tag, movement/input disabled, gameplay event
  - `AMordecaiGameMode` respawn timer (default 3s), restores full health/stamina/posture
  - Arena reset on player respawn: dead enemies respawned at original locations
  - Kill count tracking via enemy death events
  - 11 automation tests
  - Depends on US-050 (enemy death events)

- **US-054: Playable Arena Integration** (`US-054-playable-arena-integration.md`) — **EDITOR**
  - Enemy BP (`BP_MordecaiEnemy_Frontliner`) with configured stats and AI controller
  - Attack DataAssets: 1 enemy basic slash, 3 player light combo (Longsword 3-sweep chain)
  - Player BP configured to grant combat abilities (melee, dodge, block, parry) via input
  - HUD widget BP showing health/stamina/posture, added to viewport
  - Enemy world-space health bar
  - Arena layout in DevTestMap with 3 enemy frontliners
  - Dev experience configured for arena game mode with respawn
  - Full end-to-end combat loop verified in PIE
  - Manual verification tests only (no headless automation)
  - Depends on ALL HEADLESS stories (US-050–053)

### PLAN.md Updates
- Marked Epic 2 as ✅ complete (all 8 stories done)
- Marked US-010 as ✅ done in Epic 3
- Re-scoped Epic 2.5 from placeholder descriptions to 5 properly scoped stories
- Updated priority order: Epic 1 ✅, Epic 2 ✅, Epic 2.5 IN PROGRESS, Epic 3 partially done
- Story numbers remain US-050–054 as originally planned

### Dependency Graph for Epic 2.5
```
US-050 (Enemy Character) ──┬──> US-051 (Enemy AI — needs enemy character to possess)
                           ├──> US-053 (Arena Flow — listens for enemy death events)
                           │
US-052 (Combat HUD) ───────┘    (standalone — just reads attribute values)
                           │
All (US-050–053) ──────────> US-054 (Integration — wires everything in editor)
```
US-050 and US-052 are fully independent — can be parallelized.
US-051 depends on US-050. US-053 depends on US-050.
US-054 depends on all four HEADLESS stories.

### Blockers / Decisions Needed

**Carried from prior runs:**
- **Gameplay tag taxonomy** — Epic 2.5 adds more tags (`Mordecai.State.Dead`, `Mordecai.Event.Death`, `Mordecai.Event.PlayerDeath`, `Mordecai.Event.EnemyKill`). Formal naming policy still undecided.
- **Damage formula stub** — The vertical slice will use BasePower directly without attribute/skill/status multipliers. Full formula integration deferred to post-slice.
- **stat_formulas_v1.md vs character_attributes_v1.md** — US-010 resolved this by using character_attributes_v1.md as authoritative. stat_formulas_v1.md should be deprecated.
- **TODO(DECISION) items from Epic 3 and 4** — still open (skill point economy, feat list, stacking policy, etc.)

**New this run:**
- **TODO(DECISION): Damage GE architecture** — The vertical slice needs a GameplayEffect that applies health/posture damage via SetByCaller. Should this be a C++ UGameplayEffect subclass (created programmatically) or a Blueprint GE asset created in the editor? C++ is more testable headless; BP is more designer-friendly. Recommendation: create a minimal C++ GE for the headless stories, then optionally replace with BP GE in US-054 if needed.
- **TODO(DECISION): Enemy attack feel** — US-054 sets enemy windup to 500ms for readability. Is this too generous? Too punishing? Needs playtesting to determine. Defaulted to 500ms as a starting point.
- **TODO(DECISION): Arena enemy count** — US-054 places 3 frontliners. Is this the right number for initial testing? Too many may overwhelm; too few may not stress the systems. Defaulted to 3.

### Next Session Recommendation
1. **US-050** (Enemy Character) — first priority, unblocks US-051 and US-053. HEADLESS.
2. **US-052** (Combat HUD) — can parallelize with US-050. HEADLESS.
3. **US-051** (Enemy AI) — after US-050. HEADLESS.
4. **US-053** (Death & Arena Flow) — after US-050. Can parallelize with US-051. HEADLESS.
5. **US-054** (Playable Arena Integration) — after ALL HEADLESS stories. EDITOR.
6. After Epic 2.5 is playable, resume with **US-011** (Skill Framework) and **US-013** (Status Effect Framework) from the backlog.

---

## 2026-03-23 Nightly Planning Run

### Completed Since Last Run
- **US-011: Skill Framework & Rank Progression** — moved to `stories/done/`
- **US-012: Feat System** — moved to `stories/done/`

**Epic 3 (Attributes & Progression) is now COMPLETE** — all 3 stories (US-010, US-011, US-012) done.

### Currently In Progress
- (none — backlog ready for Epic 2.5 work)

### Existing Backlog (unchanged, already well-scoped)
- **US-050**: Enemy Character & Damage Reception (HEADLESS, 12 ACs)
- **US-051**: Basic Enemy AI Combat Loop (HEADLESS, 12 ACs)
- **US-052**: Combat HUD C++ Framework (HEADLESS, 11 ACs)
- **US-053**: Player Death & Arena Game Flow (HEADLESS, 11 ACs)
- **US-054**: Playable Arena Integration (EDITOR, 13 ACs)
- **US-013–018**: Status Effects (HEADLESS, 6 stories)

### New Stories Created
- (none — Epic 2.5 stories were scoped last run and remain valid)

### PLAN.md Updates
- Marked Epic 3 as ✅ complete (US-011 and US-012 now done)
- Updated priority order to reflect Epic 3 completion

### Source Tree Snapshot
Mordecai C++ code now includes:
- `Mordecai/AbilitySystem/` — ASC, AttributeSet, AttributeScaling
- `Mordecai/Camera/` — Diorama camera mode
- `Mordecai/Combat/` — Full combat suite: attacks, hit detection, block, parry, dodge, posture, stamina, projectiles, aim assist
- `Mordecai/Skills/` — SkillComponent, SkillDataAsset, SkillTypes (NEW — from US-011)
- `Mordecai/Feats/` — FeatComponent, FeatDataAsset, FeatTypes (NEW — from US-012)
- `Tests/` — 15 test files across Attributes, Camera, Character, Combat (9 files), Feats, Foundation, Input, Level, Skills, Stamina

No `Enemy/` or `UI/` subdirectories yet — these will be created by US-050 and US-052 respectively.

### Backlog Validation
Reviewed all 11 backlog stories (US-050–054 and US-013–018):
- All have **Execution Mode** tags set correctly (10 HEADLESS, 1 EDITOR)
- Epic 2.5 stories correctly reference existing code (AttributeSet, PostureSystem, StaminaSystem, GameplayTags, GA_MeleeAttack, AttackProfileDataAsset)
- US-011/US-012 completion does not affect Epic 2.5 stories — skill/feat systems are orthogonal to the vertical slice
- Epic 4 stories (US-013–018) remain valid with no dependency on Epic 2.5

### Blockers / Decisions Needed

**Carried from prior runs:**
- **Gameplay tag taxonomy** — formal naming policy still undecided. Growing tag surface area across epics.
- **Damage formula stub** — vertical slice uses BasePower directly. Full formula integration (attribute scaling × skill modifier × status modifier) deferred to post-slice.
- **TODO(DECISION): Damage GE architecture** — C++ vs Blueprint GE for the vertical slice. Recommendation remains: C++ GE in headless stories, optionally BP GE in US-054.
- **TODO(DECISION) items from Epic 3** — skill point economy, feat list, stat reset behavior still open.
- **TODO(DECISION) items from Epic 4** — stacking policy, bleeding clot timer, frostbitten freeze threshold, etc. still open.

**Resolved this run:**
- **stat_formulas_v1.md vs character_attributes_v1.md** — US-010 resolved this by using character_attributes_v1.md as authoritative. Can consider stat_formulas_v1.md deprecated.
- **Epic 3 completion** — US-011 and US-012 shipped cleanly. Skill framework (18 tests) and feat system (21 tests) both passing.

### Dependency Graph for Epic 2.5 (unchanged)
```
US-050 (Enemy Character) ──┬──> US-051 (Enemy AI — needs enemy character to possess)
                           ├──> US-053 (Arena Flow — listens for enemy death events)
                           │
US-052 (Combat HUD) ───────┘    (standalone — just reads attribute values)
                           │
All (US-050–053) ──────────> US-054 (Integration — wires everything in editor)
```

### Next Session Recommendation
**Epic 2.5 is the top priority.** This is the playability milestone — Jeff needs to play the game.

1. **US-050** (Enemy Character & Damage Reception) — first priority, unblocks US-051 and US-053. HEADLESS.
2. **US-052** (Combat HUD C++ Framework) — can parallelize with US-050. HEADLESS.
3. **US-051** (Enemy AI) — after US-050 completes. HEADLESS.
4. **US-053** (Death & Arena Flow) — after US-050 completes. Can parallelize with US-051. HEADLESS.
5. **US-054** (Playable Arena Integration) — after ALL HEADLESS stories complete. EDITOR. This is the "Jeff can play it" story.
6. After Epic 2.5 is playable, resume with **US-013** (Status Effect Framework) — gate for all Epic 4 work. HEADLESS.

---

## 2026-03-24 Nightly Planning Run

### Completed Since Last Run
- **US-013: Status Effect Framework** — moved to `stories/done/`. GAS infrastructure: tags, types, base GE, component, 14 tests.
- **US-014: DoT & Resource Denial Statuses** — moved to `stories/done/`. Burning, Bleeding, Poisoned — 3 GameplayEffects, 12 tests.

**Epic 4 progress:** 2 of 6 stories complete. US-013 (framework) and US-014 (DoT/resource denial) shipped.

**Note:** The previous run recommended Epic 2.5 as top priority. The coding agent instead picked up Epic 4 status effect stories (US-013, US-014, US-015). This is a deviation from plan — the Playability-First Rule was not followed. Correcting priority now.

### Currently In Progress
- **US-015: Speed & Timing Impairment Statuses (Frostbitten, Shocked)** — in `stories/in-progress/`. HEADLESS. 15 ACs, 13 tests. Implements movement/attack speed reduction, freeze mechanic, micro-stun, block cost multipliers.

### New Stories Created
- (none — all next stories are already scoped)

### Backlog Review
All 8 backlog stories reviewed and validated:

**Epic 4 (remaining — deferred until after Epic 2.5):**
- **US-016**: Combat Modifier Debuffs — HEADLESS, 19 ACs, 14 tests. Well-scoped.
- **US-017**: Control & Mental Debuffs — HEADLESS, 28 ACs, 20 tests. Largest story at 5 debuffs. Monitor scope — may need splitting if it exceeds a single session.
- **US-018**: Drenched & Focused Buff — HEADLESS, 15 ACs, 14 tests. Has 3 TODO(DECISION) items for cross-interaction rules.

**Epic 2.5 (Playable Vertical Slice — TOP PRIORITY after US-015):**
- **US-050**: Enemy Character & Damage Reception — HEADLESS, 12 ACs, 12 tests.
- **US-051**: Basic Enemy AI Combat Loop — HEADLESS, 12 ACs, 12 tests. Depends on US-050.
- **US-052**: Combat HUD C++ Framework — HEADLESS, 11 ACs, 12 tests. Independent.
- **US-053**: Player Death & Arena Game Flow — HEADLESS, 11 ACs, 11 tests. Depends on US-050.
- **US-054**: Playable Arena Integration — EDITOR, 13 ACs, manual verification. Depends on US-050-053.

### PLAN.md Updates
- Marked US-013 ✅ and US-014 ✅ in Epic 4
- Marked US-015 as ⚡ in progress
- Annotated US-016/017/018 as "(after Epic 2.5)"
- Updated Priority Order to reflect actual completion sequence: Epics 1-3 done, Epic 4 partially done, Epic 2.5 is NEXT after US-015 finishes
- Added Playability-First Rule annotation

### Source Tree Snapshot
Mordecai C++ code now includes:
- `Mordecai/AbilitySystem/` — ASC, AttributeSet (33 attributes including HealingReceivedMultiplier, MoveSpeedMultiplier from US-014), AttributeScaling
- `Mordecai/Camera/` — Diorama camera mode
- `Mordecai/Combat/` — Full combat suite (attacks, hit detection, block, parry, dodge, posture, stamina, projectiles, aim assist)
- `Mordecai/Skills/` — SkillComponent, SkillDataAsset, SkillTypes
- `Mordecai/Feats/` — FeatComponent, FeatDataAsset, FeatTypes
- `Mordecai/StatusEffects/` — StatusEffectComponent, base GE, types (NEW — US-013)
- `Mordecai/StatusEffects/Effects/` — GE_Burning, GE_Bleeding, GE_Poisoned (NEW — US-014)
- `MordecaiGameplayTags.h` — 87 tags across 10 categories (state, stamina, status, immunity, event, data, team, damage)
- `Tests/` — 17 test files including StatusEffects/ (2 files from US-013/014)

No `Enemy/` or `UI/` subdirectories yet — these will be created by US-050 and US-052.

### Blockers / Decisions Needed

**Carried from prior runs:**
- **Gameplay tag taxonomy** — formal naming policy still undecided. Tag count now at 87 and growing. Not blocking but a governance risk.
- **Damage formula stub** — vertical slice will use BasePower directly. Full formula integration deferred.
- **TODO(DECISION): Damage GE architecture** — C++ vs Blueprint GE. Recommendation: C++ GE for headless stories, optionally BP in US-054.
- **TODO(DECISION) items from Epics 3 & 4** — skill point economy, feat list, stacking policy, etc.

**New this run:**
- **PRIORITY ENFORCEMENT:** Coding agents picked up Epic 4 instead of the recommended Epic 2.5. The Playability-First Rule requires integration after 3 system epics. Recommendation: **hard-gate** — after US-015, the ONLY stories that should be picked up are Epic 2.5 (US-050 first). Do NOT pick up US-016/017/018 until Epic 2.5 is complete.
- **US-017 scope risk:** 5 debuffs (Silenced, Rooted, Blinded, Fear, Cursed) with 28 ACs and 20 tests may exceed a single agent session. Monitor — if first attempt stalls, split into US-017a (Silenced, Rooted, Blinded) and US-017b (Fear, Cursed).

### Dependency Graph

```
Current:
  US-015 (in progress) ──> FINISH FIRST

Then Epic 2.5:
  US-050 (Enemy Character) ──┬──> US-051 (Enemy AI)
                              ├──> US-053 (Arena Flow)
  US-052 (Combat HUD) ───────┘    (independent)
                              │
  All (US-050–053) ───────────> US-054 (Integration — EDITOR)

Then Epic 4 remaining:
  US-016 (Combat Modifiers) ──> US-017 (Control/Mental) ──> US-018 (Drenched/Focused)
```

### Next Session Recommendation

**MANDATORY: Finish US-015, then Epic 2.5. Do NOT pick up US-016/017/018.**

1. **US-015** (Frostbitten, Shocked) — finish current in-progress work. HEADLESS.
2. **US-050** (Enemy Character & Damage Reception) — first Epic 2.5 story, unblocks US-051 and US-053. HEADLESS.
3. **US-052** (Combat HUD C++ Framework) — can parallelize with US-050. HEADLESS.
4. **US-051** (Basic Enemy AI) — after US-050. HEADLESS.
5. **US-053** (Player Death & Arena Flow) — after US-050, can parallelize with US-051. HEADLESS.
6. **US-054** (Playable Arena Integration) — after US-050-053 all complete. EDITOR. **This is the "Jeff can play it" milestone.**
7. After Epic 2.5 complete, resume Epic 4: **US-016** → **US-017** → **US-018**. All HEADLESS.

---

## 2026-03-25 Nightly Planning Run

### Completed Since Last Run
- (none — US-015 still in progress, no new completions)

### Currently In Progress
- **US-015: Speed & Timing Impairment Statuses (Frostbitten, Shocked)** — still in `stories/in-progress/`. HEADLESS. Moved to in-progress on 2026-03-24 but no implementation code committed yet. No Frostbitten/Shocked GE files exist in `StatusEffects/Effects/`, no test file created. The story needs to be picked up and completed.

### Existing Backlog (unchanged, all validated)

**Epic 2.5 — Playable Vertical Slice (TOP PRIORITY after US-015):**
| Story | Mode | ACs | Tests | Dependencies |
|-------|------|-----|-------|-------------|
| US-050: Enemy Character & Damage Reception | HEADLESS | 12 | 12 | Standalone |
| US-051: Basic Enemy AI Combat Loop | HEADLESS | 12 | 12 | US-050 |
| US-052: Combat HUD C++ Framework | HEADLESS | 11 | 12 | Standalone |
| US-053: Player Death & Arena Game Flow | HEADLESS | 11 | 11 | US-050 |
| US-054: Playable Arena Integration | EDITOR | 13 | Manual | US-050–053 |

**Epic 4 remaining (deferred until after Epic 2.5):**
| Story | Mode | ACs | Tests | Dependencies |
|-------|------|-----|-------|-------------|
| US-016: Combat Modifier Debuffs | HEADLESS | 19 | 14 | US-013 |
| US-017: Control & Mental Debuffs | HEADLESS | 28 | 20 | US-013 |
| US-018: Drenched & Focused Buff | HEADLESS | 15 | 14 | US-013, US-014, US-015 |

### New Stories Created
- (none — all next stories are already well-scoped with Execution Mode tags)

### PLAN.md Updates
- (no changes needed — PLAN.md accurately reflects current state)

### Source Tree Snapshot
No change from last run. Mordecai C++ code includes:
- `Mordecai/AbilitySystem/` — ASC, AttributeSet (33+ attributes), AttributeScaling
- `Mordecai/Camera/` — Diorama camera mode
- `Mordecai/Combat/` — Full combat suite
- `Mordecai/Skills/` — SkillComponent, SkillDataAsset, SkillTypes
- `Mordecai/Feats/` — FeatComponent, FeatDataAsset, FeatTypes
- `Mordecai/StatusEffects/` — Framework (US-013), Effects: Burning, Bleeding, Poisoned (US-014)
- `Tests/` — 17 test files, 2 StatusEffect test files
- No `Enemy/` or `UI/` directories yet (created by US-050 and US-052)

### Backlog Validation
All 8 backlog stories reviewed:
- ✅ All have Execution Mode set (7 HEADLESS, 1 EDITOR)
- ✅ All have testable ACs with named automation tests
- ✅ All reference correct design docs and existing code
- ✅ Dependency chains are correct
- ✅ No mixed HEADLESS/EDITOR stories — US-054 is the only EDITOR story, cleanly separated
- ⚠️ **US-017 scope risk** (carried): 5 debuffs with 28 ACs may exceed a single session. If implementation stalls, split into US-017a (Silenced, Rooted, Blinded — 17 ACs) and US-017b (Fear, Cursed — 11 ACs).

### Blockers / Decisions Needed

**Carried from prior runs (no changes):**
- **Gameplay tag taxonomy** — formal naming policy still undecided. Tag count at 87+ and growing. Not blocking work but governance risk increases with each epic.
- **Damage formula stub** — vertical slice uses BasePower directly. Full formula integration (attribute scaling × skill modifier × status modifier) deferred to post-slice.
- **TODO(DECISION): Damage GE architecture** — C++ vs Blueprint GE for vertical slice. Recommendation: C++ GE in headless stories, optionally BP GE in US-054.
- **TODO(DECISION) items from Epics 3 & 4** — skill point economy, feat list, stacking policy, bleeding clot timer, frostbitten thresholds, etc.

**New this run:**
- **US-015 stall risk** — Story was moved to in-progress 24 hours ago but has zero implementation. If the coding agent was blocked, investigate and unblock. The 15 ACs and 13 tests are well-defined — no story ambiguity should be causing delay.

### Dependency Graph

```
Current:
  US-015 (in progress — MUST FINISH) ──> DONE

Then Epic 2.5 (MANDATORY — Playability-First Rule):
  US-050 (Enemy Character) ──┬──> US-051 (Enemy AI)
                              ├──> US-053 (Arena Flow)
  US-052 (Combat HUD) ───────┘    (independent)
                              │
  All (US-050–053) ───────────> US-054 (Integration — EDITOR)

Then Epic 4 remaining:
  US-016 (Combat Modifiers) ──> US-017 (Control/Mental) ──> US-018 (Drenched/Focused)
```

### Next Session Recommendation

**MANDATORY: Finish US-015, then Epic 2.5. Do NOT pick up US-016/017/018 until Epic 2.5 is complete.**

1. **US-015** (Frostbitten, Shocked) — **IMMEDIATE PRIORITY**. No implementation exists yet. Complete this story first. HEADLESS.
2. **US-050** (Enemy Character & Damage Reception) + **US-052** (Combat HUD C++ Framework) — start these in parallel as soon as US-015 is done. Both are standalone HEADLESS stories.
3. **US-051** (Basic Enemy AI) + **US-053** (Player Death & Arena Flow) — after US-050 completes. Can be parallelized. Both HEADLESS.
4. **US-054** (Playable Arena Integration) — after ALL HEADLESS Epic 2.5 stories complete. EDITOR. **This is the "Jeff can play it" milestone.**
5. After Epic 2.5 is playable, resume Epic 4: **US-016** → **US-017** → **US-018**. All HEADLESS.
6. After Epic 4 completes, scope **Epic 5: Magic System** (US-019–023). Read `ability_schema_v1.md`, `ability_system_v1.md`, `skill_sheet_v1.1.md`, `new_spells_proposal.md`.
