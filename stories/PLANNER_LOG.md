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

---

## 2026-03-26 Nightly Planning Run

### Completed Since Last Run
- (none — US-015 was marked BLOCKED, no new completions)

### Currently In Progress
- (none — no stories are in `stories/in-progress/`)

### Blocked
- **US-015: Speed & Timing Impairment Statuses (Frostbitten, Shocked)** — marked BLOCKED after multiple failed agent attempts. Partial implementation recovered from failed agent run (GE_Frostbitten 43 lines, GE_Shocked 96 lines, GE_Frozen, GE_MicroStunned, test file 589 lines). Story moved back to `stories/backlog/` with BLOCKED header. **Workers should SKIP this story.** Needs manual investigation by Jeff.

### Existing Backlog

**Epic 2.5 — Playable Vertical Slice (IMMEDIATE PRIORITY):**
| Story | Mode | ACs | Tests | Dependencies | Status |
|-------|------|-----|-------|-------------|--------|
| US-050: Enemy Character & Damage Reception | HEADLESS | 12 | 12 | Standalone | Ready |
| US-051: Basic Enemy AI Combat Loop | HEADLESS | 12 | 12 | US-050 | Ready |
| US-052: Combat HUD C++ Framework | HEADLESS | 11 | 12 | Standalone | Ready |
| US-053: Player Death & Arena Game Flow | HEADLESS | 11 | 11 | US-050 | Ready |
| US-054: Playable Arena Integration | EDITOR | 13 | Manual | US-050–053 | Ready |

**Epic 4 remaining (deferred until after Epic 2.5):**
| Story | Mode | ACs | Tests | Dependencies | Status |
|-------|------|-----|-------|-------------|--------|
| US-015: Speed & Timing Impairments | HEADLESS | 15 | 13 | US-013 | 🔴 BLOCKED |
| US-016: Combat Modifier Debuffs | HEADLESS | 19 | 14 | US-013 | Deferred |
| US-017: Control & Mental Debuffs | HEADLESS | 28 | 20 | US-013 | Deferred |
| US-018: Drenched & Focused Buff | HEADLESS | 15 | 14 | US-013, US-014, US-015 | Deferred |

### New Stories Created
- (none — all stories for the next 2 epics are already well-scoped with Execution Mode tags)

### PLAN.md Updates
- Updated US-015 from ⚡ "in progress" to 🔴 "BLOCKED"
- Updated priority order: Epic 2.5 is now IMMEDIATE PRIORITY (no longer gated on US-015 completion)

### Source Tree Snapshot
Mordecai C++ code:
- `Mordecai/AbilitySystem/` — ASC, AttributeSet (33+ attributes), AttributeScaling
- `Mordecai/Camera/` — Diorama camera mode
- `Mordecai/Combat/` — Full combat suite (attacks, hit detection, block, parry, dodge, posture, stamina, projectiles, aim assist)
- `Mordecai/Skills/` — SkillComponent, SkillDataAsset, SkillTypes
- `Mordecai/Feats/` — FeatComponent, FeatDataAsset, FeatTypes
- `Mordecai/StatusEffects/` — Framework (US-013), Effects: Burning, Bleeding, Poisoned (US-014), Frostbitten/Shocked/Frozen/MicroStunned (US-015 partial — BLOCKED)
- `Tests/` — 17 test files including StatusEffects/ (3 files — framework, DoT, speed/timing partial)
- No `Enemy/` or `UI/` directories yet (created by US-050 and US-052)

### Backlog Validation
All 9 backlog stories reviewed:
- ✅ All have Execution Mode set (8 HEADLESS, 1 EDITOR)
- ✅ All have testable ACs with named automation tests
- ✅ All reference correct design docs and existing code
- ✅ Dependency chains are correct
- ✅ No mixed HEADLESS/EDITOR stories — US-054 is the only EDITOR story, cleanly separated
- ⚠️ **US-015 BLOCKED** — partial code in tree may cause compile issues. Coding agents starting US-050 should verify full project compiles first.
- ⚠️ **US-017 scope risk** (carried): 5 debuffs with 28 ACs may exceed a single session. Split plan ready if needed.
- ⚠️ **US-018 depends on US-015** — Drenched amplifies Frostbitten/Shocked. If US-015 remains blocked, US-018 will need to stub those interactions.

### Blockers / Decisions Needed

**Carried from prior runs (no changes):**
- **Gameplay tag taxonomy** — formal naming policy still undecided. 87+ tags across 10 categories.
- **Damage formula stub** — vertical slice uses BasePower directly. Full integration deferred.
- **TODO(DECISION): Damage GE architecture** — C++ vs Blueprint GE. Recommendation: C++ GE in headless stories, optionally BP in US-054.
- **TODO(DECISION) items from Epics 3 & 4** — skill point economy, feat list, stacking policy, etc.

**New this run:**
- **US-015 BLOCKED resolution** — Jeff needs to investigate what went wrong. The partial code (Frostbitten, Shocked, Frozen, MicroStunned GEs + test file) was recovered from a failed agent run. Options: (a) Jeff manually reviews and fixes, (b) a fresh agent attempt with more context, (c) descope US-015 by simplifying ACs (the story has 15 ACs with complex stacking/freeze/micro-stun mechanics — consider splitting into simpler pieces).
- **US-015 partial code compile risk** — The recovered code may or may not compile. First coding agent picking up Epic 2.5 should run a full build to verify the tree is clean before starting new work.

### Dependency Graph

```
BLOCKED:
  US-015 (Frostbitten/Shocked) ──> BLOCKED — skip, investigate later

IMMEDIATE — Epic 2.5 (Playability-First Rule):
  US-050 (Enemy Character) ──┬──> US-051 (Enemy AI)
                              ├──> US-053 (Arena Flow)
  US-052 (Combat HUD) ───────┘    (independent)
                              │
  All (US-050–053) ───────────> US-054 (Integration — EDITOR)

AFTER Epic 2.5 — Epic 4 remaining:
  US-015 (unblock first) ──> US-016 ──> US-017 ──> US-018
  (US-016/017 can start without US-015; US-018 needs US-015 for Drenched synergies)
```

### Next Session Recommendation

**MANDATORY: Start Epic 2.5 immediately. US-015 is BLOCKED — skip it.**

1. **US-050** (Enemy Character & Damage Reception) + **US-052** (Combat HUD C++ Framework) — start in parallel. Both standalone HEADLESS. **Verify full project compiles first** (US-015 partial code may affect build).
2. **US-051** (Basic Enemy AI) + **US-053** (Player Death & Arena Flow) — after US-050 completes. Can be parallelized. Both HEADLESS.
3. **US-054** (Playable Arena Integration) — after ALL HEADLESS Epic 2.5 stories complete. EDITOR. **This is the "Jeff can play it" milestone.**
4. After Epic 2.5 is playable, investigate and unblock **US-015**. Then resume Epic 4: **US-016** → **US-017** → **US-018**. All HEADLESS.
5. After Epic 4 completes, scope **Epic 5: Magic System** (US-019–023).

---

## 2026-03-27 Nightly Planning Run

### Completed Since Last Run
- (none — US-050 and US-052 moved to in-progress but have zero implementation)

### Currently In Progress
- **US-050: Enemy Character & Damage Reception** — in `stories/in-progress/`. HEADLESS. Moved to in-progress since last run. **No implementation code exists yet** — `Source/LyraGame/Mordecai/Enemy/` directory does not exist, no test files created. All 12 ACs unchecked.
- **US-052: Combat HUD C++ Framework** — in `stories/in-progress/`. HEADLESS. Moved to in-progress since last run. **No implementation code exists yet** — `Source/LyraGame/Mordecai/UI/` directory does not exist, no test files created. All 11 ACs unchecked.

### Blocked
- **US-015: Speed & Timing Impairment Statuses (Frostbitten, Shocked)** — still BLOCKED in `stories/backlog/`. Partial code was fully reverted per commits `c979300` and `19a8e8c` (StatusEffectComponent restored to pre-US-015 state). Build tree should be clean.

### Existing Backlog (unchanged, all validated)

**Epic 2.5 — Playable Vertical Slice (IMMEDIATE PRIORITY):**
| Story | Mode | ACs | Tests | Dependencies | Status |
|-------|------|-----|-------|-------------|--------|
| US-050: Enemy Character & Damage Reception | HEADLESS | 12 | 12 | Standalone | ⚡ In Progress (no code yet) |
| US-051: Basic Enemy AI Combat Loop | HEADLESS | 12 | 12 | US-050 | Ready |
| US-052: Combat HUD C++ Framework | HEADLESS | 11 | 12 | Standalone | ⚡ In Progress (no code yet) |
| US-053: Player Death & Arena Game Flow | HEADLESS | 11 | 11 | US-050 | Ready |
| US-054: Playable Arena Integration | EDITOR | 13 | Manual | US-050–053 | Ready |

**Epic 4 remaining (deferred until after Epic 2.5):**
| Story | Mode | ACs | Tests | Dependencies | Status |
|-------|------|-----|-------|-------------|--------|
| US-015: Speed & Timing Impairments | HEADLESS | 15 | 13 | US-013 | 🔴 BLOCKED |
| US-016: Combat Modifier Debuffs | HEADLESS | 19 | 14 | US-013 | Deferred |
| US-017: Control & Mental Debuffs | HEADLESS | 28 | 20 | US-013 | Deferred |
| US-018: Drenched & Focused Buff | HEADLESS | 15 | 14 | US-013, US-014, US-015 | Deferred |

### New Stories Created
- (none — all Epic 2.5 and Epic 4 stories remain well-scoped with Execution Mode tags)

### PLAN.md Updates
- Marked US-050 and US-052 as ⚡ in progress (moved from "scoped" to "in progress")

### Source Tree Snapshot
No change from last run. Mordecai C++ code includes:
- `Mordecai/AbilitySystem/` — ASC, AttributeSet (33+ attributes), AttributeScaling
- `Mordecai/Camera/` — Diorama camera mode
- `Mordecai/Combat/` — Full combat suite (attacks, hit detection, block, parry, dodge, posture, stamina, projectiles, aim assist)
- `Mordecai/Skills/` — SkillComponent, SkillDataAsset, SkillTypes
- `Mordecai/Feats/` — FeatComponent, FeatDataAsset, FeatTypes
- `Mordecai/StatusEffects/` — Framework (US-013), Effects: Burning, Bleeding, Poisoned (US-014). US-015 partial code fully reverted.
- `Tests/` — 17 test files across 11 subdirectories (Attributes, Camera, Character, Combat×9, Feats, Foundation, Input, Level, Skills, Stamina, StatusEffects×2)
- **No `Enemy/` or `UI/` directories yet** — these will be created by US-050 and US-052 respectively.

### Backlog Validation
All 9 stories (2 in-progress + 7 backlog) reviewed:
- ✅ All have Execution Mode set (8 HEADLESS, 1 EDITOR)
- ✅ All have testable ACs with named automation tests (except US-054 which is manual)
- ✅ All reference correct design docs and existing code
- ✅ Dependency chains are correct
- ✅ No mixed HEADLESS/EDITOR stories — US-054 is the only EDITOR story
- ✅ US-015 partial code fully reverted — build tree should be clean
- ⚠️ **US-050 and US-052 stall risk** — both were moved to in-progress but have zero implementation. Coding agents need to pick these up immediately.
- ⚠️ **US-017 scope risk** (carried): 5 debuffs with 28 ACs may exceed a single session. Split plan ready if needed.
- ⚠️ **US-018 depends on US-015** — Drenched amplifies Frostbitten/Shocked. If US-015 remains blocked, US-018 will need to stub those interactions.

### Blockers / Decisions Needed

**Carried from prior runs (no changes):**
- **Gameplay tag taxonomy** — formal naming policy still undecided. 87+ tags across 10 categories.
- **Damage formula stub** — vertical slice uses BasePower directly. Full formula integration deferred.
- **TODO(DECISION): Damage GE architecture** — C++ vs Blueprint GE. Recommendation: C++ GE in headless stories, optionally BP in US-054.
- **TODO(DECISION) items from Epics 3 & 4** — skill point economy, feat list, stacking policy, etc.
- **US-015 BLOCKED** — needs Jeff's manual investigation. Partial code was reverted. Options: (a) Jeff manually fixes, (b) fresh agent attempt with simplified scope, (c) split into simpler sub-stories.

### Dependency Graph

```
BLOCKED:
  US-015 (Frostbitten/Shocked) ──> BLOCKED — skip, investigate later

IN PROGRESS — Epic 2.5 (Playability-First Rule):
  US-050 (Enemy Character) ⚡──┬──> US-051 (Enemy AI)
                                ├──> US-053 (Arena Flow)
  US-052 (Combat HUD) ⚡────────┘    (independent)
                                │
  All (US-050–053) ─────────────> US-054 (Integration — EDITOR)

AFTER Epic 2.5 — Epic 4 remaining:
  US-015 (unblock first) ──> US-016 ──> US-017 ──> US-018
  (US-016/017 can start without US-015; US-018 needs US-015 for Drenched synergies)
```

### Next Session Recommendation

**MANDATORY: Complete US-050 and US-052. Both are in-progress with zero code — coding agents must implement them NOW.**

1. **US-050** (Enemy Character & Damage Reception) + **US-052** (Combat HUD C++ Framework) — **IMMEDIATE**. Both are in-progress, standalone, HEADLESS, and have zero implementation. Can be parallelized. Coding agents should verify the project compiles cleanly first (US-015 partial code was reverted).
2. **US-051** (Basic Enemy AI) + **US-053** (Player Death & Arena Flow) — after US-050 completes. Can be parallelized. Both HEADLESS.
3. **US-054** (Playable Arena Integration) — after ALL HEADLESS Epic 2.5 stories complete. EDITOR. **This is the "Jeff can play it" milestone.**
4. After Epic 2.5 is playable, investigate and unblock **US-015**. Then resume Epic 4: **US-016** → **US-017** → **US-018**. All HEADLESS.
5. After Epic 4 completes, scope **Epic 5: Magic System** (US-019–023). Read `ability_schema_v1.md`, `ability_system_v1.md`, `skill_sheet_v1.1.md`, `new_spells_proposal.md`.

---

## 2026-03-28 Nightly Planning Run

### Completed Since Last Run
- **US-050: Enemy Character & Damage Reception** — moved to `stories/done/`. AMordecaiEnemyCharacter with ASC, attributes, death, posture break. 12 tests.
- **US-051: Basic Enemy AI Combat Loop** — moved to `stories/done/`. AMordecaiEnemyAIController with state machine (Idle/Approach/Attack/Recover/Staggered/Leash/Dead). 12 tests.
- **US-052: Combat HUD C++ Framework** — moved to `stories/done/`. Health/Stamina/Posture bar widgets, enemy health bar, BindToASC. 12 tests.

**Epic 2.5 progress:** 3 of 5 stories complete. 2 remaining (US-053, US-054).

### Currently In Progress
- **US-053: Player Death & Arena Game Flow** — in `stories/in-progress/`. HEADLESS. **Zero implementation exists.** No Arena test file, no death/respawn code in MordecaiGameMode, all 11 ACs unchecked. This is the critical gap blocking the vertical slice.
- **US-054: Playable Arena Integration** — in `stories/in-progress/`. EDITOR. **12 of 13 ACs done.** All editor assets created: enemy BP, attack DataAssets, player combo chain, HUD widgets, arena layout, 3 enemies placed, experience configured. Only AC-054.13 (full end-to-end PIE loop test including death/respawn) remains — blocked on US-053 completion.

### Blocked
- **US-015: Speed & Timing Impairment Statuses (Frostbitten, Shocked)** — still BLOCKED in `stories/backlog/`. No change. Partial code was fully reverted.

### New Stories Created
**Epic 5: Magic System** — 5 stories, all HEADLESS. Scoped ahead of schedule since Epic 4 remaining stories (US-016–018) are already queued.

Per the dependency chain: Epic 2.5 (almost done) → Epic 4 remaining → Epic 5. Scoping Epic 5 now ensures stories are ready when coding agents reach it.

- **US-019: Spell Framework & Spell DataAsset** (`US-019-spell-framework.md`) — **HEADLESS**
  - `UMordecaiSpellDataAsset` (all spell configuration: cost, cooldown, cast timing, targeting, scaling, delivery)
  - `UMordecaiGA_SpellBase` (base ability class: SP cost check/deduction, cooldown GE, casting phases, movement policies, interrupt on damage, attribute+rank scaling, upcasting, Silenced immunity check)
  - 13 ACs, 13 tests
  - Depends on US-010 (attribute scaling), US-011 (skill ranks). No combat dependencies.

- **US-020: Damage Spells — Fireball, Cone of Cold, Magic Missile, Stone Skin** (`US-020-damage-spells.md`) — **HEADLESS**
  - Fireball (projectile, fire damage), Cone of Cold (AoE cone, cold damage), Magic Missile (channeled homing projectiles, force damage), Stone Skin (self-buff, damage reduction)
  - Reuses existing projectile system (US-009) and hit detection (US-003)
  - 12 ACs, 12 tests
  - Depends on US-019

- **US-021: Utility Spells — Blink, Sleep, Fire Ward** (`US-021-utility-spells.md`) — **HEADLESS**
  - Blink (teleport + i-frames), Sleep (single-target CC, wake on damage), Fire Ward (fire damage absorb shield)
  - Fire Ward introduces a new attribute/mechanic (absorb shield HP)
  - 11 ACs, 11 tests
  - Depends on US-019

- **US-022: Support Spells — Bless, Restoration** (`US-022-support-spells.md`) — **HEADLESS**
  - Bless (damage multiplier self-buff, non-stacking), Restoration (HoT with periodic ticks)
  - Smaller story — 2 simple self-buff/heal spells
  - 8 ACs, 8 tests
  - Depends on US-019

- **US-023: Tactical Spells — Snare, Enfeeble, Enchant Weapon, Illusion, Blur** (`US-023-tactical-spells.md`) — **HEADLESS**
  - Snare (single-target root), Enfeeble (Weakened debuff), Enchant Weapon (elemental melee buff), Illusion (summon aggro decoy actor), Blur (ranged evasion buff)
  - ⚠️ **Scope risk:** 5 spells, 15 ACs, 15 tests. If too big, split into US-023a (Snare, Enfeeble, Enchant Weapon) and US-023b (Illusion, Blur)
  - Illusion requires new `AMordecaiIllusionActor` + AI aggro modification. Blur requires projectile miss-chance pipeline change.
  - Depends on US-019, US-051 (enemy AI for Illusion aggro), US-009 (projectile for Blur miss)

### PLAN.md Updates
- Marked US-050, US-051, US-052 as ✅ complete in Epic 2.5
- Marked US-053 as ⚡ in progress (no implementation)
- Marked US-054 as ⚡ in progress (12/13 ACs)
- Expanded Epic 5 from placeholder descriptions to 5 properly scoped stories with Execution Mode tags
- Updated priority order to reflect Epic 5 scoping

### Source Tree Snapshot
Mordecai C++ code now includes:
- `Mordecai/AbilitySystem/` — ASC, AttributeSet (33+ attributes), AttributeScaling
- `Mordecai/Camera/` — Diorama camera mode
- `Mordecai/Combat/` — Full combat suite (attacks, hit detection, block, parry, dodge, posture, stamina, projectiles, aim assist)
- `Mordecai/Enemy/` — **NEW** — MordecaiEnemyCharacter, MordecaiEnemyAIController, MordecaiEnemyAITypes (US-050, US-051)
- `Mordecai/Skills/` — SkillComponent, SkillDataAsset, SkillTypes
- `Mordecai/Feats/` — FeatComponent, FeatDataAsset, FeatTypes
- `Mordecai/StatusEffects/` — Framework (US-013), Effects: Burning, Bleeding, Poisoned (US-014), MicroStunned
- `Mordecai/UI/` — **NEW** — CombatHUDWidget, HealthBarWidget, StaminaBarWidget, PostureBarWidget, EnemyHealthBarWidget (US-052)
- `Tests/` — 20+ test files across 12 subdirectories (added Enemy/×2, HUD/×1)
- MordecaiCore plugin assets: BP_MordecaiEnemy_Frontliner, DA_EnemyAttack_BasicSlash, DA_PlayerAttack_LightSlash_1/2/3, WBP_CombatHUD, WBP_EnemyHealthBar, WBP_HealthBar/PostureBar/StaminaBar, AS_MordecaiCombat, updated DevTestMap with arena + 3 enemies

### Backlog Validation
All 12 backlog stories reviewed (5 Epic 4 + 5 Epic 5 + 2 in-progress):

**Epic 2.5 (in progress):**
- ✅ US-053: HEADLESS, 11 ACs, 11 tests. Well-scoped. **Zero implementation — CRITICAL.**
- ✅ US-054: EDITOR, 13 ACs, manual verification. 12/13 done. Blocked on US-053 for AC-054.13.

**Epic 4 remaining (deferred):**
- ✅ US-015: BLOCKED. No change.
- ✅ US-016: HEADLESS, 19 ACs, 14 tests. Well-scoped.
- ✅ US-017: HEADLESS, 28 ACs, 20 tests. ⚠️ Scope risk carried.
- ✅ US-018: HEADLESS, 15 ACs, 14 tests. Depends on US-015.

**Epic 5 (newly scoped):**
- ✅ US-019: HEADLESS, 13 ACs, 13 tests. Foundation — gate for US-020–023.
- ✅ US-020: HEADLESS, 12 ACs, 12 tests. Depends on US-019.
- ✅ US-021: HEADLESS, 11 ACs, 11 tests. Depends on US-019.
- ✅ US-022: HEADLESS, 8 ACs, 8 tests. Small story. Depends on US-019.
- ✅ US-023: HEADLESS, 15 ACs, 15 tests. ⚠️ Scope risk. Depends on US-019.

All have Execution Mode tags. No mixed HEADLESS/EDITOR stories. Dependency chains correct.

### TODO(DECISION) Items

**Carried from prior runs (no changes):**
- **Gameplay tag taxonomy** — 87+ tags. No formal naming policy.
- **Damage formula stub** — vertical slice uses BasePower. Full integration deferred.
- **Damage GE architecture** — C++ vs Blueprint GE.
- **Epic 3 & 4 items** — skill point economy, feat list, stacking policy, etc.
- **US-015 BLOCKED** — needs Jeff's investigation.

**New this run:**
- **TODO(DECISION): Spell skill mapping** — Which skills from skill_sheet_v1.1 map to which spell? Fireball→Fireball skill, Cone of Cold→Cone of Cold skill, etc. are obvious. But Bless has no skill entry in the sheet — does it get its own skill, or is it mapped to a general "Support Magic" skill? Defaulted to: each spell is its own skill in the SkillComponent, matching skill_sheet names.
- **TODO(DECISION): Missing spells** — skill_sheet_v1.1 lists Frost Ward, Float, and Counter Spell, which are not in the current Epic 5 scope (US-019–023). These spells are not in the PLAN.md placeholder. Should they be added? Recommendation: defer to a US-023b or a future Epic 5 extension. They're lower priority than the 14 spells already scoped.
- **TODO(DECISION): Enchant Weapon element selection** — At Rank 1, Enchant Weapon adds bonus elemental damage "matching your highest element." There's no attribute for "highest element" yet. Defaulted to: Fire element at Rank 1. Adaptive element selection (Rank 15 milestone) is deferred.
- **TODO(DECISION): Blur miss chance implementation** — How does projectile miss chance work mechanically? Options: (a) roll on hit, skip damage application; (b) deflect projectile collision. Recommend (a) — simple roll in the damage pipeline.

### Dependency Graph

```
CRITICAL PATH — Epic 2.5 (finish now):
  US-053 (Death & Arena Flow — ZERO CODE) ──> US-054 AC-054.13 (PIE verification)

BLOCKED:
  US-015 (Frostbitten/Shocked) ──> BLOCKED — skip

AFTER Epic 2.5 — Epic 4 remaining:
  US-016 (Combat Modifiers) ──> US-017 (Control/Mental) ──> US-018 (Drenched/Focused)
  (US-016/017 can start without US-015; US-018 needs US-015 for Drenched synergies)

AFTER Epic 4 — Epic 5:
  US-019 (Spell Framework) ──┬──> US-020 (Damage Spells)
                              ├──> US-021 (Utility Spells)
                              ├──> US-022 (Support Spells)
                              └──> US-023 (Tactical Spells — also depends on US-051 for Illusion)
```

### Next Session Recommendation

**MANDATORY: Complete US-053. This is the ONLY blocker to finishing the vertical slice.**

1. **US-053** (Player Death & Arena Game Flow) — **IMMEDIATE CRITICAL PRIORITY**. Zero implementation exists. This is the single remaining HEADLESS story blocking the playable prototype. HEADLESS. Complete this first.
2. **US-054 AC-054.13** (Full PIE loop verification) — After US-053 is done, launch the editor and verify the full combat loop: move → attack → take damage → see HUD → kill enemy → die → respawn → enemies reset. EDITOR.
3. After Epic 2.5 is verified playable, start **Epic 4 remaining**: **US-016** (Combat Modifier Debuffs) → **US-017** (Control/Mental Debuffs) → **US-018** (Drenched & Focused). All HEADLESS.
4. After Epic 4, start **Epic 5**: **US-019** (Spell Framework) first — it gates all other spell stories. Then US-020–022 can be parallelized. US-023 last (scope risk).
5. After Epic 5, consider a **Playable Slice 2** milestone (Playability-First Rule: Epics 4+5 are 2 system epics) to wire spells into the arena.
6. Investigate and unblock **US-015** when time allows — US-018 (Drenched) depends on it.

---

## 2026-03-29 Nightly Planning Run

### Completed Since Last Run
- (none — no new stories moved to done since last run)

### Currently In Progress
- **US-053: Player Death & Arena Game Flow** — HEADLESS. **STILL ZERO IMPLEMENTATION.** This is the single remaining blocker to completing the playable vertical slice. No commits since last planning run (877cb2e).
- **US-054: Playable Arena Integration** — EDITOR. 12/13 ACs done. AC-054.13 (full PIE loop verification) blocked on US-053 completion.

### Stories Modified
- **US-017 SPLIT**: Original had 28 ACs across 5 debuffs (Silenced, Rooted, Blinded, Fear, Cursed). Now:
  - **US-017**: Action-Restricting Debuffs — Silenced & Rooted only (11 ACs, 9 tests). HEADLESS.
  - **US-059**: Perception & Mental Debuffs — Blinded, Fear, Cursed (17 ACs, 11 tests). HEADLESS.
  - Rationale: 28 ACs exceeds 2-4 hour session scope. Silenced/Rooted are "hard restriction" debuffs; Blinded/Fear/Cursed are "soft degradation" debuffs — natural split point.

- **US-023 SPLIT**: Original had 15 ACs across 5 spells (Snare, Enfeeble, Enchant Weapon, Illusion, Blur). Scope risk warning was already carried. Now:
  - **US-023**: Tactical Spells — Debuff Spells: Snare, Enfeeble, Enchant Weapon (9 ACs, 7 tests). HEADLESS.
  - **US-060**: Tactical Spells — Summon & Evasion: Illusion, Blur (8 ACs, 8 tests). HEADLESS.
  - Rationale: Illusion requires new actor class + AI aggro modification. Blur requires projectile pipeline modification. Both are mechanically complex. Snare/Enfeeble/Enchant Weapon are simple "apply GE" spells that follow established patterns.

### New Stories Created

**Epic 5.5: Playable Magic Slice** (NEW EPIC — Playability-First Rule integration milestone)

- **US-055: Spell HUD & Status Effect Indicators** (`stories/backlog/US-055-spell-hud-status-indicators.md`) — HEADLESS
  - SP bar widget (mirrors Health/Stamina/Posture pattern from US-052)
  - Active buff/debuff indicator bar (dynamically adds/removes indicators on tag change)
  - Spell cooldown display widget (radial fill + remaining seconds)
  - Extends CombatHUDWidget with UPROPERTY slots for new widgets
  - 10 ACs, 8 tests. Depends on US-052 (existing HUD framework).

- **US-056: Status-Applying Attack Profiles** (`stories/backlog/US-056-status-applying-attacks.md`) — HEADLESS
  - `FMordecaiStatusOnHitEntry` struct: StatusEffectTag, ApplicationChance, StatusEffectGEClass
  - Extends `UMordecaiAttackProfileDataAsset` with `StatusEffectsOnHit` array (backwards compatible)
  - Melee attack and projectile hit processing apply status effects from profile on hit
  - 7 ACs, 6 tests. Depends on US-002 (attack profiles), US-013 (status framework).

- **US-057: Playable Magic Arena Integration** (`stories/backlog/US-057-playable-magic-arena.md`) — EDITOR
  - Create SpellDataAssets for 4 spells: Fireball, Blink, Stone Skin, Restoration
  - Spell input bindings: LB/Q, RB/E, LB+RB/R, DPadUp/1
  - Grant spells to player BP, map to inputs
  - One enemy gets fire attack with 40% Burning on-hit
  - HUD updated with SP bar, status indicators, cooldown displays
  - Full magic + status loop verified in PIE
  - 12 ACs, manual verification. Depends on US-019–022, US-055, US-056.

**Epic 6: Weapons & Equipment (gate story scoped)**

- **US-024: Weapon Class Framework** (`stories/backlog/US-024-weapon-class-framework.md`) — HEADLESS
  - `UMordecaiWeaponDataAsset`: weapon identity, stats, attack profile references, stat modifiers, granted abilities/tags
  - `EMordecaiWeaponType` enum with 14 weapon types
  - `FMordecaiWeaponInstance` runtime struct for player-owned weapon state
  - `UMordecaiEquipmentComponent`: equip/unequip via GAS (applies stat GE, grants abilities, grants tags)
  - Weapon slot model: MainHand, OffHand, TwoHand (TwoHand clears both slots)
  - Melee attack integration: reads combo chain from equipped weapon, adds BaseDamage, applies AttackSpeedMultiplier
  - 13 ACs, 11 tests. Foundation for all weapon type stories.

### PLAN.md Updates
- Added **Epic 5.5** (Playable Magic Slice) with US-055, US-056, US-057
- Updated **Epic 4** story list to reflect US-017 split (US-017 trimmed, US-059 added)
- Updated **Epic 5** story list to reflect US-023 split (US-023 trimmed, US-060 added)
- Updated **Epic 6** with detailed US-024 and renumbered placeholders:
  - US-025: Melee Weapons — Blade Family (Longsword, Greatsword, Shortsword, Dagger)
  - US-026: Melee Weapons — Blunt & Polearm (Axe, Mace, Spear, Quarterstaff, Unarmed)
  - US-027: Ranged Weapons (Longbow, Shortbow, Crossbow, Throwables, Wands)
  - US-028: Two-Weapon Fighting
  - US-029: Armor & Equipment System
  - US-030: Shield System
- Renumbered Epic 7 (Inventory: US-031–033), Epic 8 (World: US-034–036), Epic 9 (Town: US-037–040), Epic 10 (UI: US-041–043)
- Updated priority order to include Epic 5.5 between Epic 5 and Epic 6

### Backlog Validation

**All 19 backlog stories reviewed:**

**Epic 4 remaining (6 stories):**
- ✅ US-015: BLOCKED. No change.
- ✅ US-016: HEADLESS, 19 ACs, 14 tests. Well-scoped.
- ✅ US-017: HEADLESS, 11 ACs, 9 tests. **Trimmed** (was 28 ACs). Now manageable.
- ✅ US-059: HEADLESS, 17 ACs, 11 tests. **NEW** (split from US-017). Formulaic attribute-modifier debuffs.
- ✅ US-018: HEADLESS, 15 ACs, 14 tests. Depends on US-015 for Drenched synergies.

**Epic 5 (7 stories):**
- ✅ US-019: HEADLESS, 13 ACs, 13 tests. Gate for all spells.
- ✅ US-020: HEADLESS, 12 ACs, 12 tests.
- ✅ US-021: HEADLESS, 11 ACs, 11 tests.
- ✅ US-022: HEADLESS, 8 ACs, 8 tests. Small.
- ✅ US-023: HEADLESS, 9 ACs, 7 tests. **Trimmed** (was 15 ACs). Now manageable.
- ✅ US-060: HEADLESS, 8 ACs, 8 tests. **NEW** (split from US-023). Complex mechanics isolated.

**Epic 5.5 (3 stories — NEW):**
- ✅ US-055: HEADLESS, 10 ACs, 8 tests.
- ✅ US-056: HEADLESS, 7 ACs, 6 tests.
- ✅ US-057: EDITOR, 12 ACs, manual verification.

**Epic 6 (1 story scoped):**
- ✅ US-024: HEADLESS, 13 ACs, 11 tests. Gate for all weapon stories.

All have Execution Mode tags. No mixed HEADLESS/EDITOR stories. Dependency chains correct.

### TODO(DECISION) Items

**Carried from prior runs (no changes):**
- **Gameplay tag taxonomy** — 90+ tags. No formal naming policy.
- **Damage formula stub** — vertical slice uses BasePower. Full integration deferred.
- **Damage GE architecture** — C++ vs Blueprint GE.
- **Epic 3 & 4 items** — skill point economy, feat list, stacking policy, etc.
- **US-015 BLOCKED** — needs Jeff's investigation.
- **Spell skill mapping** — each spell = its own skill (default).
- **Missing spells** — Frost Ward, Float, Counter Spell deferred.
- **Enchant Weapon element** — Fire at Rank 1 (default).
- **Blur miss chance** — simple roll in damage pipeline (default).

**No new TODO(DECISION) items this run.**

### Dependency Graph

```
CRITICAL PATH — Epic 2.5 (finish now):
  US-053 (Death & Arena Flow — ZERO CODE) ──> US-054 AC-054.13 (PIE verification)

BLOCKED:
  US-015 (Frostbitten/Shocked) ──> BLOCKED — skip

AFTER Epic 2.5 — Epic 4 remaining:
  US-016 (Combat Modifiers) ──> US-017 (Silenced/Rooted) ──> US-059 (Blinded/Fear/Cursed) ──> US-018 (Drenched/Focused)
  (US-016/017/059 can start without US-015; US-018 needs US-015 for Drenched synergies)

AFTER Epic 4 — Epic 5:
  US-019 (Spell Framework) ──┬──> US-020 (Damage Spells)
                              ├──> US-021 (Utility Spells)
                              ├──> US-022 (Support Spells)
                              ├──> US-023 (Snare/Enfeeble/EnchantWeapon)
                              └──> US-060 (Illusion/Blur — also depends on US-051 for Illusion)

AFTER Epic 5 — Epic 5.5 (Playable Magic Slice):
  US-055 (Spell HUD) ──┐
  US-056 (Status Attacks) ──┤──> US-057 (Playable Magic Arena — EDITOR)
  US-019–023/060 (all spells) ──┘

AFTER Epic 5.5 — Epic 6:
  US-024 (Weapon Framework) ──> US-025/026/027 (weapon types) ──> US-028/029/030 (dual-wield, armor, shields)
```

### Next Session Recommendation

**MANDATORY: Complete US-053. This remains the ONLY blocker to finishing the vertical slice. No progress since last planning run.**

1. **US-053** (Player Death & Arena Game Flow) — **IMMEDIATE CRITICAL PRIORITY**. ZERO implementation. HEADLESS. This must be completed before anything else can move.
2. **US-054 AC-054.13** (Full PIE loop verification) — After US-053, launch editor and verify the full combat loop. EDITOR. This completes Epic 2.5.
3. After Epic 2.5 is playable: **US-016** (Combat Modifier Debuffs) → **US-017** (Silenced/Rooted) → **US-059** (Blinded/Fear/Cursed) → **US-018** (Drenched/Focused). All HEADLESS. Note: US-018 depends on US-015 which is BLOCKED — US-018 can be partially implemented but Drenched synergies with Frostbitten/Shocked must wait.
4. After Epic 4: **US-019** (Spell Framework) first — gates all spell stories. Then **US-020/021/022** can be parallelized. Then **US-023** → **US-060**.
5. After Epic 5: **US-055** + **US-056** (parallel HEADLESS) → **US-057** (EDITOR — Playable Magic Slice).
6. After Epic 5.5: **US-024** (Weapon Framework) — gates Epic 6.
7. Investigate and unblock **US-015** when time allows — US-018 depends on it.

---

## 2026-03-31 Nightly Planning Run

### Completed Since Last Run
- **US-053: Player Death & Arena Game Flow** — implemented player death, respawn, and arena reset flow. Commit: `092800e`, `0e29035`.
- **US-054: Playable Arena Integration** — configured enemy attack profiles, wired arena. Commit: `2067b81`, `3f4d1b5`.
- **Epic 2.5: Playable Vertical Slice is COMPLETE.** All 5 stories (US-050 through US-054) are done. Jeff can now launch the game, fight enemies in the test arena, and exercise the full melee/dodge/block/parry/stamina/posture loop with HUD feedback.

### Currently In Progress
- (none — all in-progress stories cleared)

### Corrective Action
- **US-019 moved back to backlog.** It was found in `stories/in-progress/` but Epic 4 remaining (US-016 → US-017 → US-059 → US-018) must be completed before Epic 5 per priority order. US-019 also has an integration point with US-017 (Silenced tag blocking spells — AC-019.13). No implementation code existed for US-019 so nothing was lost.

### New Stories Created
- (none — all stories through Epic 5.5 are already scoped and in backlog)

### Backlog Review — Epic 4 Remaining (All HEADLESS)
All four stories reviewed and confirmed well-scoped with execution modes, ACs, tests, and design doc references:

| Story | Status Effects | ACs | Tests | Dependencies |
|-------|---------------|-----|-------|-------------|
| US-016 | Weakened, Brittle, Exposed, Corroded | 19 | 14 | US-013 ✅ |
| US-017 | Silenced, Rooted | 11 | 9 | US-013 ✅ |
| US-059 | Blinded, Fear, Cursed | 17 | 11 | US-013 ✅, US-017 (shared attrs) |
| US-018 | Drenched, Focused | 15 | 14 | US-013 ✅, US-015 🔴 (partial) |

**New attributes introduced across Epic 4 remaining:**
- US-016: `OutgoingPostureDamageMultiplier`, `IncomingPostureDamageMultiplier`, `IncomingDamageMultiplier`, `BlockStabilityMultiplier`, `ArmorEfficiencyMultiplier`
- US-017: `SpellPointRegenMultiplier`
- US-059: `RangedAccuracyMultiplier`, `AimAssistMultiplier`, `StealthDetectionMultiplier`, `StaminaTierPenaltyMultiplier`
- US-018: `FireDamageReceivedMultiplier`

These all follow the established multiplier pattern (default 1.0) from US-014.

### Existing Source Code State
- `Mordecai/StatusEffects/Effects/` contains: Bleeding, Burning, Poisoned, MicroStunned (from US-014 + partial US-015)
- No `Mordecai/Magic/` directory exists yet (US-019 not started)
- Test directories exist for: Arena, Attributes, Camera, Character, Combat, Enemy, Feats, Foundation, HUD, Input, Level, Skills, Stamina, StatusEffects

### Blockers / Decisions Needed
- **US-015 remains BLOCKED.** Partial code exists (MicroStunned GE). Needs manual investigation by Jeff. This blocks US-018's Drenched synergies with Frostbitten/Shocked.
- **US-018 partial implementation:** Drenched ACs 3/4 (Shocked/Frostbitten amplification) depend on US-015. Focused and Drenched fire/burning interactions can be implemented without US-015. Recommend implementing what's possible and marking the remaining ACs as `BLOCKED_BY_US015`.

### Dependency Graph (Updated)

```
CURRENT PRIORITY — Epic 4 remaining:
  US-016 (Weakened/Brittle/Exposed/Corroded) ──┐
  US-017 (Silenced/Rooted) ──────────────────────┤──> US-059 (Blinded/Fear/Cursed) ──> US-018 (Drenched/Focused)
                                                  │
  Note: US-016 and US-017 can be parallelized     │
  US-059 depends on US-017 (shared attribute defs) │
  US-018 partially blocked by US-015 🔴           │

BLOCKED:
  US-015 (Frostbitten/Shocked) ──> US-018 ACs 3/4 (Drenched synergies)

AFTER Epic 4 — Epic 5:
  US-019 (Spell Framework) ──┬──> US-020 (Damage Spells)
                              ├──> US-021 (Utility Spells)
                              ├──> US-022 (Support Spells)
                              ├──> US-023 (Snare/Enfeeble/EnchantWeapon)
                              └──> US-060 (Illusion/Blur)

AFTER Epic 5 — Epic 5.5 (Playable Magic Slice):
  US-055 (Spell HUD) ──┐
  US-056 (Status Attacks) ──┤──> US-057 (Playable Magic Arena — EDITOR)
  US-019–023/060 (all spells) ──┘

AFTER Epic 5.5 — Epic 6:
  US-024 (Weapon Framework) ──> US-025/026/027 ──> US-028/029/030
```

### Next Session Recommendation

1. **US-016** (Combat Modifier Debuffs: Weakened, Brittle, Exposed, Corroded) — **START HERE**. HEADLESS. 19 ACs, 14 tests. All dependencies met (US-013 done). Follows established GE + attribute modifier pattern from US-014.

2. **US-017** (Silenced, Rooted) — **CAN PARALLELIZE with US-016**. HEADLESS. 11 ACs, 9 tests. Independent of US-016. Both share US-013 as their only dependency.

3. **US-059** (Blinded, Fear, Cursed) — After US-017 completes (shares `SpellPointRegenMultiplier` attr). HEADLESS.

4. **US-018** (Drenched, Focused) — After US-059. HEADLESS. Partially blocked by US-015 — implement all non-Frostbitten/Shocked-dependent ACs. Mark remaining as blocked.

5. After Epic 4: **US-019** (Spell Framework) — gates all Epic 5 spell stories.

6. Investigate and unblock **US-015** when Jeff has time — this remains the only long-standing blocker.

---

## 2026-03-31 Nightly Planning Run (2)

### Completed Since Last Run
- (none — no coding work since the earlier run today)

### Currently In Progress
- (none — queue is clear)

### New Stories Created
- (none — all stories through Epic 5.5 are scoped and in backlog)

### Backlog Audit
All 15 backlog stories verified with correct Execution Mode tags:

| Mode | Count | Stories |
|------|-------|---------|
| HEADLESS | 14 | US-015 (blocked), US-016, US-017, US-018, US-019, US-020, US-021, US-022, US-023, US-024, US-055, US-056, US-059, US-060 |
| EDITOR | 1 | US-057 (Playable Magic Arena) |

No mixed stories found. All stories comply with PLANNER.md classification rules.

### Blockers / Decisions Needed
- **US-015 (Frostbitten/Shocked) remains BLOCKED.** Partial code exists (MicroStunned GE in `StatusEffects/Effects/`). Multiple agent attempts failed. Needs manual investigation by Jeff. This blocks US-018 ACs 3/4 (Drenched synergies with Frostbitten/Shocked). All other Epic 4 stories are unblocked.

### Next Session Recommendation

Priority order unchanged — agents should start here:

1. **US-016** (Weakened, Brittle, Exposed, Corroded) — **START HERE.** HEADLESS. 19 ACs, 14 tests. All dependencies met. Follows established GE + attribute modifier pattern from US-014.

2. **US-017** (Silenced, Rooted) — **CAN PARALLELIZE with US-016.** HEADLESS. 11 ACs, 9 tests. Independent of US-016.

3. **US-059** (Blinded, Fear, Cursed) — After US-017. HEADLESS. Depends on `SpellPointRegenMultiplier` attr from US-017.

4. **US-018** (Drenched, Focused) — After US-059. HEADLESS. Implement all non-Frostbitten/Shocked ACs. Mark ACs 3/4 as `BLOCKED_BY_US015`.

5. **US-019** (Spell Framework) — After Epic 4. Gates all Epic 5 spell stories.

6. **US-015** — Still needs Jeff's manual investigation. Only long-standing blocker.

---

## 2026-04-02 Nightly Planning Run

### Completed Since Last Run (3/31)
- **US-019: Spell Framework & Spell DataAsset** — moved to done. Base spell ability class, SP costs, cooldowns, casting phases, movement policies, interruption, scaling infrastructure all implemented.
- **US-020: Damage Spells** — moved to done. Fireball, Cone of Cold, Magic Missile, Stone Skin all implemented with 12 ACs passing.
- **US-021: Utility Spells** — moved to done. Blink, Sleep, Fire Ward all implemented with 11 ACs passing.

Epic 5 jumped ahead of Epic 4 remaining (agents worked on spell system instead of status effects). This is acceptable — Epic 5 stories had no hard dependencies on remaining Epic 4 status effects. US-023 and US-060 do have soft dependencies on US-016/US-017 for reusing Weakened/Rooted GEs.

### Currently In Progress
- **US-022: Support Spells (Bless, Restoration)** — WIP recovery from previous agent run. HEADLESS. Small story (2 spells, 8 ACs). Git log shows `[WIP] Recovering US-022 move to in-progress from previous agent run`.

### New Stories Created
- (none — all stories through Epic 5.5 + US-024 are already well-scoped in backlog)

### PLAN.md Updates
- Updated Epic 5 header from "← **scoped**" to "(4/6 done, US-022 in progress)" with individual story status markers
- Updated Priority Order to reflect that Epic 5 partially completed ahead of Epic 4 remaining
- New priority 7 merges remaining Epic 4 + Epic 5 stories into an interleaved dependency chain:
  `US-022 (finish) → US-016 + US-017 (parallel) → US-059 + US-060 (parallel) → US-023 → US-018`

### Backlog Audit (11 stories)

| Story | Epic | Mode | ACs | Tests | Dependencies Met? |
|-------|------|------|-----|-------|-------------------|
| US-015 | 4 | HEADLESS | 15 | 13 | 🔴 BLOCKED |
| US-016 | 4 | HEADLESS | 19 | 14 | ✅ Ready |
| US-017 | 4 | HEADLESS | 11 | 9 | ✅ Ready |
| US-059 | 4 | HEADLESS | 17 | 11 | ⏳ After US-017 |
| US-018 | 4 | HEADLESS | 15 | 14 | ⏳ After US-059; partially blocked by US-015 |
| US-023 | 5 | HEADLESS | 9 | 7 | ⏳ After US-016 + US-017 |
| US-060 | 5 | HEADLESS | 8 | 8 | ✅ Ready |
| US-055 | 5.5 | HEADLESS | 10 | 8 | ✅ Ready (but logically after spells) |
| US-056 | 5.5 | HEADLESS | 7 | 6 | ✅ Ready |
| US-057 | 5.5 | EDITOR | 12 | Manual | ⏳ After US-055 + US-056 + all spells |
| US-024 | 6 | HEADLESS | 13 | 11 | ✅ Ready (but after Epic 5.5) |

All 10 HEADLESS, 1 EDITOR. No mixed stories. All comply with PLANNER.md classification rules.

### Source Code State
- `Mordecai/StatusEffects/Effects/`: Burning, Bleeding, Poisoned, MicroStunned (from US-014 + partial US-015)
- `Mordecai/Magic/`: SpellBase, SpellDataAsset, SpellTypes + 8 spell implementations (Fireball, ConeOfCold, MagicMissile, StoneSkin, Blink, Sleep, FireWard + types)
- No Bless or Restoration code yet (US-022 WIP)
- Test directories: Arena, Attributes, Camera, Character, Combat, Enemy, Feats, Foundation, HUD, Input, Level, Magic, Skills, Stamina, StatusEffects

### Dependency Graph (Current)

```
IN PROGRESS:
  US-022 (Bless/Restoration) ──> done

NEXT — Parallelizable pair:
  US-016 (Weakened/Brittle/Exposed/Corroded) ─┬──> US-023 (Snare/Enfeeble/EnchantWeapon)
  US-017 (Silenced/Rooted) ───────────────────┤
                                                └──> US-059 (Blinded/Fear/Cursed) ──> US-018 (Drenched/Focused)

INDEPENDENT (can run anytime after US-019):
  US-060 (Illusion/Blur)

INTEGRATION MILESTONE (Epic 5.5):
  US-055 (Spell HUD) ──────┐
  US-056 (Status Attacks) ──┤──> US-057 (Playable Magic Arena — EDITOR)
  All spells complete ──────┘

BLOCKED:
  US-015 (Frostbitten/Shocked) ──> US-018 ACs 3/4 (Drenched synergies)
```

### Blockers / Decisions Needed
1. **US-015 (Frostbitten/Shocked) remains BLOCKED.** Partial code exists (MicroStunned GE). Multiple agent attempts failed. Needs manual investigation by Jeff. Blocks US-018 ACs 3/4 (Drenched synergies with Frostbitten/Shocked). All other stories are unblocked.
2. **US-022 recovery:** Previous agent run left US-022 in a WIP state. Next coding agent should check the current code state and either continue or restart cleanly.

### Next Session Recommendation

1. **US-022** (Support Spells: Bless, Restoration) — **FINISH FIRST.** HEADLESS. In-progress WIP recovery. Small story, 8 ACs. Check code state, continue or restart.

2. **US-016 + US-017** — **PARALLELIZE.** Both HEADLESS. Both dependencies met. US-016 has 19 ACs (larger), US-017 has 11 ACs. Together they unblock US-023, US-059, and US-060.

3. **US-060** (Illusion/Blur) — **CAN START IMMEDIATELY** after US-022 finishes. No dependency on Epic 4 stories. HEADLESS.

4. **US-059** (Blinded/Fear/Cursed) — After US-017. HEADLESS. Shares `SpellPointRegenMultiplier` attribute from US-017.

5. **US-023** (Snare/Enfeeble/EnchantWeapon) — After US-016 + US-017. HEADLESS. Reuses Weakened (US-016) and Rooted (US-017) GEs.

6. **US-018** (Drenched/Focused) — After US-059. HEADLESS. Implement non-Frostbitten/Shocked ACs. Mark ACs 3/4 as `BLOCKED_BY_US015`.

7. **US-055 + US-056** — After all spells + Epic 4 remaining. HEADLESS. Can parallelize.

8. **US-057** — EDITOR. After US-055 + US-056. Playable Magic Arena integration milestone.

9. **US-015** — Still needs Jeff's manual investigation. Only long-standing blocker.

---

## 2026-04-02 Nightly Planning Run (2)

### Completed Since Last Run
- (none — US-022 still in WIP recovery, no new completions)

### Currently In Progress
- **US-022: Support Spells (Bless, Restoration)** — HEADLESS. WIP recovery from previous agent run. Small story (2 spells, 8 ACs). No Bless or Restoration code exists yet in `Mordecai/Magic/`. Last commit: `3e8129a [WIP] Recovering US-022 move to in-progress from previous agent run`.

### Changes to Existing Stories
- **US-017 AC-017.2 updated:** Original text said "Since spell abilities don't exist yet (Epic 5)..." — this is now stale. Spells DO exist (US-019/020/021 done, US-022 in progress). Updated to instruct the coding agent to add `Mordecai.Status.Silenced` to `ActivationBlockedTags` on all 9 existing spell abilities (Fireball, ConeOfCold, MagicMissile, StoneSkin, Blink, Sleep, FireWard, Bless, Restoration), not just register the tag for future use.
- **PLAN.md Epic 4 count fixed:** "2/8 done" → "2/7 done" (actual count: US-013, 014, 015, 016, 017, 059, 018 = 7 stories).

### New Stories Created
- (none — backlog is healthy at 11 stories covering the full dependency chain through Epic 5.5 + US-024)

### Backlog Audit (11 stories)

| Story | Epic | Mode | Status | Dependencies Met? |
|-------|------|------|--------|-------------------|
| US-015 | 4 | HEADLESS | 🔴 BLOCKED | Needs Jeff investigation |
| US-016 | 4 | HEADLESS | ✅ Ready | All deps met (US-013 done) |
| US-017 | 4 | HEADLESS | ✅ Ready (updated) | All deps met (US-013 done) |
| US-059 | 4 | HEADLESS | ⏳ After US-017 | Shares SpellPointRegenMultiplier attr |
| US-018 | 4 | HEADLESS | ⏳ After US-059 | Partially blocked by US-015 (ACs 3/4) |
| US-023 | 5 | HEADLESS | ⏳ After US-016 + US-017 | Reuses Weakened/Rooted GEs |
| US-060 | 5 | HEADLESS | ✅ Ready | Independent (only needs US-019 done) |
| US-055 | 5.5 | HEADLESS | ✅ Ready | Logically after spells complete |
| US-056 | 5.5 | HEADLESS | ✅ Ready | No hard deps beyond US-002/US-013 |
| US-057 | 5.5 | EDITOR | ⏳ After US-055 + US-056 + all spells | Integration milestone |
| US-024 | 6 | HEADLESS | ✅ Ready | After Epic 5.5 (ordering only) |

All 10 HEADLESS, 1 EDITOR. No mixed stories. All comply with PLANNER.md classification rules.

### Source Code State
- `Mordecai/Magic/`: SpellBase, SpellDataAsset, SpellTypes + 7 spell implementations (Fireball, ConeOfCold, MagicMissile, StoneSkin, Blink, Sleep, FireWard). No Bless or Restoration yet (US-022 WIP).
- `Mordecai/StatusEffects/Effects/`: Burning, Bleeding, Poisoned, MicroStunned (from US-014 + partial US-015)
- `Mordecai/UI/`: CombatHUDWidget, HealthBarWidget, StaminaBarWidget, PostureBarWidget (from US-052)
- Test directories: Arena, Attributes, Camera, Character, Combat, Enemy, Feats, Foundation, HUD, Input, Level, Magic, Skills, Stamina, StatusEffects

### Dependency Graph (Unchanged)

```
IN PROGRESS:
  US-022 (Bless/Restoration) ──> done

NEXT — Parallelizable pair:
  US-016 (Weakened/Brittle/Exposed/Corroded) ─┬──> US-023 (Snare/Enfeeble/EnchantWeapon)
  US-017 (Silenced/Rooted) ───────────────────┤
                                                └──> US-059 (Blinded/Fear/Cursed) ──> US-018 (Drenched/Focused)

INDEPENDENT (can run anytime after US-019):
  US-060 (Illusion/Blur)

INTEGRATION MILESTONE (Epic 5.5):
  US-055 (Spell HUD) ──────┐
  US-056 (Status Attacks) ──┤──> US-057 (Playable Magic Arena — EDITOR)
  All spells complete ──────┘

BLOCKED:
  US-015 (Frostbitten/Shocked) ──> US-018 ACs 3/4 (Drenched synergies)
```

### Blockers / Decisions Needed
1. **US-015 (Frostbitten/Shocked) remains BLOCKED.** Same as previous runs. Multiple agent attempts failed. Needs manual investigation by Jeff. Blocks US-018 ACs 3/4 only. All other stories are unblocked.
2. **US-022 recovery:** Still WIP. Next coding agent should check code state and either continue or restart. No Bless/Restoration C++ files exist yet, so a clean start is likely needed.

### Next Session Recommendation

1. **US-022** (Bless, Restoration) — **FINISH FIRST.** HEADLESS. In-progress WIP. Small story, 8 ACs, 2 simple spells (self-buff + HoT). No code written yet despite WIP status — agent should start clean.

2. **US-016 + US-017** — **PARALLELIZE.** Both HEADLESS, both ready. US-016 (19 ACs, Weakened/Brittle/Exposed/Corroded) and US-017 (11 ACs, Silenced/Rooted). Together they unblock US-023, US-059, and US-060. **Note:** US-017 was updated this run — coding agent should update existing spell abilities with Silenced blocking tag.

3. **US-060** (Illusion/Blur) — **CAN START IMMEDIATELY** after US-022 finishes. No Epic 4 dependency. HEADLESS.

4. **US-059** (Blinded/Fear/Cursed) — After US-017. HEADLESS.

5. **US-023** (Snare/Enfeeble/EnchantWeapon) — After US-016 + US-017. HEADLESS.

6. **US-018** (Drenched/Focused) — After US-059. HEADLESS. Implement non-Frostbitten/Shocked ACs. Mark ACs 3/4 as `BLOCKED_BY_US015`.

7. **US-055 + US-056** — After all spells + remaining Epic 4. HEADLESS. Can parallelize.

8. **US-057** — EDITOR. After US-055 + US-056. Playable Magic Arena.

9. **US-015** — Still needs Jeff's manual investigation. Only long-standing blocker.

---

## 2026-04-04 Nightly Planning Run

### Completed Since Last Run
- None. US-022 (Support Spells) still in progress.

### Currently In Progress
- **US-022** (Bless, Restoration) — WIP has advanced since last run. 750 lines of code now exist across 5 files:
  - `MordecaiGA_Bless.cpp/h` (187 lines) — spell ability implementation
  - `MordecaiGA_Restoration.cpp/h` (198 lines) — spell ability implementation
  - `MordecaiSupportSpellTests.cpp` (365 lines) — automation tests
  - `MordecaiGameplayTags.cpp/h` — modified (uncommitted) with new Bless/Restoration tags
  - All files are untracked/uncommitted. Next coding agent should **continue from this WIP state**, not restart.

### New Stories Created
- None. Backlog of 11 stories is sufficient and covers the full priority chain through Epic 6 gate (US-024). No gaps found in coverage.

### Backlog Health Check
All 11 backlog stories verified:

| Story | Epic | Mode | Status | Notes |
|-------|------|------|--------|-------|
| US-015 | 4 | HEADLESS | BLOCKED | Needs Jeff's manual investigation |
| US-016 | 4 | HEADLESS | Ready | After US-022 |
| US-017 | 4 | HEADLESS | Ready | After US-022 (updates 9 existing spell abilities) |
| US-059 | 4 | HEADLESS | After US-017 | Shares SpellPointRegenMultiplier attr |
| US-018 | 4 | HEADLESS | After US-059 | Partially blocked by US-015 (ACs 3/4) |
| US-023 | 5 | HEADLESS | After US-016+017 | Reuses Weakened/Rooted GEs |
| US-060 | 5 | HEADLESS | Ready | Independent (only needs US-019 done) |
| US-055 | 5.5 | HEADLESS | Ready | After spells complete |
| US-056 | 5.5 | HEADLESS | Ready | No hard deps beyond US-002/US-013 |
| US-057 | 5.5 | EDITOR | After US-055+056 | Integration milestone |
| US-024 | 6 | HEADLESS | Ready | After Epic 5.5 (ordering only) |

All 10 HEADLESS, 1 EDITOR. No mixed stories. No changes needed.

### Coverage Verification
- **Status effects:** All 16 launch-set statuses have stories (3 done via US-014, 2 blocked in US-015, 11 in backlog across US-016/017/059/018)
- **Spells:** All 14 launch spells have stories (7 done, 2 in progress via US-022, 5 in backlog across US-023/060)
- **Integration:** Epic 5.5 (US-055/056/057) covers the Playability-First Rule after system epics 4+5

### Blockers / Decisions Needed
1. **US-015 (Frostbitten/Shocked) remains BLOCKED.** Same as previous runs. Only blocks US-018 ACs 3/4 (Drenched synergies with Shocked/Frostbitten). All other stories unblocked.
2. No new decisions needed. All open items marked as `TODO(DECISION)` in existing stories.

### Next Session Recommendation

1. **US-022** (Bless, Restoration) — **FINISH FIRST.** HEADLESS. WIP code exists (750 lines, untracked). Agent should compile, run tests, fix any failures, commit. Small remaining work.

2. **US-016 + US-017** — **PARALLELIZE.** Both HEADLESS, both ready once US-022 done. US-016 (Weakened/Brittle/Exposed/Corroded, 19 ACs) and US-017 (Silenced/Rooted, 11 ACs). Together they unblock US-023 and US-059.

3. **US-060** (Illusion/Blur) — **CAN RUN IN PARALLEL** with US-016/017. No Epic 4 dependency. HEADLESS. 8 ACs.

4. **US-059** (Blinded/Fear/Cursed) — After US-017. HEADLESS. 17 ACs.

5. **US-023** (Snare/Enfeeble/EnchantWeapon) — After US-016+017. HEADLESS. 9 ACs.

6. **US-018** (Drenched/Focused) — After US-059. HEADLESS. Skip ACs 3/4 (blocked by US-015). 15 ACs.

7. **US-055 + US-056** — After all Epic 4+5 spells. HEADLESS. Parallelize.

8. **US-057** — EDITOR. After US-055+056. Playable Magic Arena integration.

9. **US-015** — Still blocked. Needs Jeff's manual investigation.

## 2026-04-09 Nightly Planning Run

### Completed Since Last Run
- **US-016: Combat Modifier Debuffs** (Weakened, Brittle, Exposed, Corroded) — moved to done
- **US-017: Action-Restricting Debuffs** (Silenced, Rooted) — moved to done
- **US-022: Support Spells** (Bless, Restoration) — moved to done (WIP recovery completed)
- **US-023: Tactical Spells** (Snare, Enfeeble, Enchant Weapon) — moved to done
- **US-055: Spell HUD & Status Effect Indicators** — moved to done (Epic 10 Phase 1 complete)
- **US-061: Combat Feedback — Combo Counter** — moved to done
- **US-062: Death Screen & Respawn Countdown** — moved to done
- **US-063: Kill Counter Display** — moved to done

### Currently In Progress
- (none — backlog is queued and ready)

### New Stories Created
- **US-064: Enemy Indicator System Migration** (`stories/backlog/US-064-enemy-indicator-system.md`) — HEADLESS
  - Migrate enemy world-space indicators to Lyra's IndicatorSystem (SActorCanvas projection)
  - Enemy health bar, name, and status icons above heads; distance-based visibility
  - 9 automation tests
- **US-065: Floating Damage Numbers** (`stories/backlog/US-065-floating-damage-numbers.md`) — HEADLESS
  - Integrate Lyra's NumberPops system for color-coded damage/healing numbers
  - Color by damage type (Physical=white, Fire=orange, etc.), critical hit emphasis, healing green
  - Pooled pops with max concurrent cap
  - 10 automation tests
- **US-069: Pause Menu Framework** (`stories/backlog/US-069-pause-menu-framework.md`) — HEADLESS
  - CommonUI-based tabbed pause menu with Game→Menu input mode switching
  - Tab registration API for future content (character sheet, skills, feats)
  - Pause/unpause game lifecycle via subsystem
  - 11 automation tests

### PLAN.md Updates
- Marked US-016, US-017, US-022, US-023, US-055 as ✅
- Updated Epic 4 count: 5/7 done (US-015 still blocked)
- Updated Epic 5 count: 5/6 done
- Reorganized priority 8 into batches: Batch 1 (US-059+060+056 parallel), Batch 2 (US-018), Batch 3 (US-057 EDITOR)
- Added priority 9: Epic 10 Phase 2-3 with newly scoped stories

### Blockers / Decisions Needed
- **US-015 (Frostbitten/Shocked)** — Still BLOCKED. Multiple agent attempts failed. Needs Jeff's manual investigation. This partially blocks US-018 (Drenched's frost/lightning synergy ACs) but US-018 can proceed with TODO stubs for those interactions.
- **US-018 AC-018.3/AC-018.4** — Drenched+Shocked and Drenched+Frostbitten interactions cannot be fully tested until US-015 is resolved. Coding agent should implement the checks but mark those test assertions as TODO.

### Backlog Summary (10 stories, priority order)
| # | Story | Mode | ACs | Dependencies | Status |
|---|-------|------|-----|-------------|--------|
| 1 | US-059 (Blinded/Fear/Cursed) | HEADLESS | 17 | US-013 ✅, US-017 ✅ | Ready |
| 2 | US-060 (Illusion/Blur) | HEADLESS | 8 | US-019 ✅, US-051 ✅ | Ready |
| 3 | US-056 (Status-Applying Attacks) | HEADLESS | 7 | US-002 ✅, US-004 ✅, US-013 ✅ | Ready |
| 4 | US-018 (Drenched/Focused) | HEADLESS | 15 | US-059, partial US-015 🔴 | After Batch 1 |
| 5 | US-057 (Magic Arena Integration) | EDITOR | 12 | US-056, US-019-022 ✅ | After US-056 |
| 6 | US-064 (Enemy Indicators) | HEADLESS | 10 | US-050 ✅, US-052 ✅ | Ready (Phase 2) |
| 7 | US-065 (Floating Damage Numbers) | HEADLESS | 10 | US-064 | After US-064 |
| 8 | US-069 (Pause Menu Framework) | HEADLESS | 13 | None | Ready (Phase 3) |
| 9 | US-024 (Weapon Class Framework) | HEADLESS | 13 | US-002 ✅, US-004 ✅ | Ready (Epic 6) |
| 10 | US-015 (Frostbitten/Shocked) | HEADLESS | 15 | 🔴 BLOCKED | Needs Jeff |

### Next Session Recommendation

**Batch 1 — Parallelize (all HEADLESS, all independent):**
1. **US-059** (Blinded/Fear/Cursed) — 17 ACs, ~11 tests. Follows established GE + attribute modifier pattern from US-014/016/017.
2. **US-060** (Illusion/Blur) — 8 ACs, ~8 tests. New actor type (IllusionActor) + projectile pipeline modification.
3. **US-056** (Status-Applying Attack Profiles) — 7 ACs, ~6 tests. Extends attack profile data model + melee/projectile hit processing.

**Batch 2 — Sequential (after Batch 1):**
4. **US-018** (Drenched/Focused) — 15 ACs, ~14 tests. Drenched elemental synergies + PerfectActionTracker component. Skip Frostbitten/Shocked interaction tests (US-015 blocked).

**Batch 3 — EDITOR (after US-056):**
5. **US-057** (Playable Magic Arena) — EDITOR. Wire spells + status attacks + HUD into playable arena. Jeff can play-test magic.

**Batch 4 — Phase 2+3 (after Batch 3, parallelize):**
6. **US-064** (Enemy Indicators) + **US-069** (Pause Menu) — both HEADLESS, independent. Can parallelize.

**Batch 5 — After Batch 4:**
7. **US-065** (Floating Damage Numbers) — HEADLESS, after US-064.

**Still blocked:**
- **US-015** — Needs Jeff's investigation before Drenched synergy ACs can be fully verified.

---

## 2026-04-10 Nightly Planning Run

### Completed Since Last Run
- (none — no new stories moved to done since 2026-04-09 run)

### Currently In Progress
- (none — backlog is queued and ready for coding agents)

### New Stories Created

**Epic 10 Phase 3 — Menus & Progression UI (3 stories, all HEADLESS)**

These complete the Phase 3 coverage. All three depend on US-069 (Pause Menu Framework) for the tab registration API. They read from existing C++ components (ASC, SkillComponent, FeatComponent) that are already implemented.

- **US-066: Character Sheet — Attributes & Derived Stats** (`stories/backlog/US-066-character-sheet-widget.md`) — HEADLESS
  - `UMordecaiCharacterSheetWidget` binds to the player's ASC via attribute change delegates
  - Displays 9 primary attributes grouped into Physical/Resilience/Magical columns
  - Shows 4 core resource pools (Health, Stamina, SP, Posture) as current/max
  - Shows 5 primary-derived multipliers + 9 secondary-derived stats as percentages
  - Real-time updates via `GetGameplayAttributeValueChangeDelegate()`
  - 9 ACs, 8 tests. Depends on US-069 (tab API), US-010 (attribute scaling).

- **US-067: Skill Tree Display** (`stories/backlog/US-067-skill-tree-display.md`) — HEADLESS
  - `UMordecaiSkillTreeWidget` binds to `UMordecaiSkillComponent`
  - Skills grouped by 5 categories with category switching
  - Rank display (1–20) with milestone markers at 1/5/10/15/20
  - Detail panel shows milestone descriptions from `UMordecaiSkillDataAsset::RankDescriptions`
  - Skill point allocation via `TryAllocateSkillPoint()` with disabled state when 0 points or max rank
  - Live updates via `OnSkillMilestoneReachedBP` delegate
  - 11 ACs, 10 tests. Depends on US-069 (tab API), US-011 (skill framework).

- **US-068: Feat Display** (`stories/backlog/US-068-feat-display.md`) — HEADLESS
  - `UMordecaiFeatDisplayWidget` binds to `UMordecaiFeatComponent`
  - Shows unlocked feats with tier color-coding (Common=gray, Rare=blue, Legendary=gold)
  - Detail panel shows description, applied effects/abilities/drawback counts
  - Locked feats section with unlock condition progress (StatName: current/threshold)
  - Summary header with total unlocked + per-tier breakdown
  - Live unlock notifications via `OnFeatUnlockedBP` delegate
  - 9 ACs, 8 tests. Depends on US-069 (tab API), US-012 (feat system).

### PLAN.md Updates
- Updated Epic 10 Phase 3 with detailed story descriptions and Execution Mode tags for US-066, US-067, US-068
- No priority order changes — these stories slot into existing Batch 6 (after US-069)

### Backlog Summary (13 stories, priority order)

| # | Story | Epic | Mode | ACs | Tests | Dependencies | Status |
|---|-------|------|------|-----|-------|-------------|--------|
| 1 | US-059 (Blinded/Fear/Cursed) | 4 | HEADLESS | 17 | 11 | US-013 ✅, US-017 ✅ | Ready |
| 2 | US-060 (Illusion/Blur) | 5 | HEADLESS | 8 | 8 | US-019 ✅, US-051 ✅ | Ready |
| 3 | US-056 (Status-Applying Attacks) | 5.5 | HEADLESS | 7 | 6 | US-002 ✅, US-013 ✅ | Ready |
| 4 | US-018 (Drenched/Focused) | 4 | HEADLESS | 15 | 14 | US-059, partial US-015 🔴 | After Batch 1 |
| 5 | US-057 (Magic Arena Integration) | 5.5 | EDITOR | 12 | Manual | US-056, spells ✅ | After US-056 |
| 6 | US-064 (Enemy Indicators) | 10 | HEADLESS | 10 | 9 | US-050 ✅, US-052 ✅ | Ready (Phase 2) |
| 7 | US-065 (Floating Damage Numbers) | 10 | HEADLESS | 10 | 10 | US-064 | After US-064 |
| 8 | US-069 (Pause Menu Framework) | 10 | HEADLESS | 13 | 11 | None | Ready (Phase 3) |
| 9 | US-066 (Character Sheet) | 10 | HEADLESS | 9 | 8 | US-069, US-010 ✅ | After US-069 |
| 10 | US-067 (Skill Tree Display) | 10 | HEADLESS | 11 | 10 | US-069, US-011 ✅ | After US-069 |
| 11 | US-068 (Feat Display) | 10 | HEADLESS | 9 | 8 | US-069, US-012 ✅ | After US-069 |
| 12 | US-024 (Weapon Class Framework) | 6 | HEADLESS | 13 | 11 | US-002 ✅, US-004 ✅ | Ready (Epic 6) |
| 13 | US-015 (Frostbitten/Shocked) | 4 | HEADLESS | 15 | 13 | 🔴 BLOCKED | Needs Jeff |

All 12 HEADLESS, 1 EDITOR. No mixed stories. All comply with PLANNER.md classification rules.

### Source Code State (unchanged from last run)
- `Mordecai/AbilitySystem/` — ASC, AttributeSet (40+ attributes), AttributeScaling
- `Mordecai/Camera/` — Diorama camera mode
- `Mordecai/Combat/` — Full combat suite (attacks, hit detection, block, parry, dodge, posture, stamina, projectiles, aim assist)
- `Mordecai/Enemy/` — EnemyCharacter, EnemyAIController, EnemyAITypes
- `Mordecai/Skills/` — SkillComponent, SkillDataAsset, SkillTypes
- `Mordecai/Feats/` — FeatComponent, FeatDataAsset, FeatTypes
- `Mordecai/StatusEffects/` — Framework + 8 status GEs (Burning, Bleeding, Poisoned, MicroStunned, Weakened, Brittle, Exposed, Corroded, Silenced, Rooted)
- `Mordecai/Magic/` — SpellBase, SpellDataAsset + 11 spells (Fireball, ConeOfCold, MagicMissile, StoneSkin, Blink, Sleep, FireWard, Bless, Restoration, Snare, Enfeeble, EnchantWeapon)
- `Mordecai/UI/` — CombatHUD, HealthBar, StaminaBar, PostureBar, EnemyHealthBar, SpellPointsBar, SpellCooldown, StatusEffectBar, StatusEffectIndicator, CombatFeedback, ComboCounter, DeathScreen, KillCounter (26 files)
- Tests: 25+ test files across 13 subdirectories

### Dependency Graph

```
CURRENT PRIORITY — Batch 1 (parallel, all HEADLESS):
  US-059 (Blinded/Fear/Cursed) ──┐
  US-060 (Illusion/Blur) ────────┤
  US-056 (Status-Applying Attacks) ──┘

Batch 2 (after Batch 1):
  US-018 (Drenched/Focused) ──> partially blocked by US-015 🔴

Batch 3 (after US-056):
  US-057 (Playable Magic Arena — EDITOR)

Batch 4 (Phase 2+3, parallel):
  US-064 (Enemy Indicators) ──> US-065 (Floating Damage Numbers)
  US-069 (Pause Menu) ──┬──> US-066 (Character Sheet)
                         ├──> US-067 (Skill Tree) ←── NEW
                         └──> US-068 (Feat Display) ←── NEW

After Phase 3:
  US-024 (Weapon Framework) ──> Epic 6 weapon type stories (US-025+)

BLOCKED:
  US-015 (Frostbitten/Shocked) ──> US-018 ACs 3/4
```

### Blockers / Decisions Needed
- **US-015 (Frostbitten/Shocked)** — Still BLOCKED. Multiple agent attempts failed. Needs Jeff's manual investigation. Partially blocks US-018 (Drenched synergies). All other stories unblocked.
- No new TODO(DECISION) items. All open items carried from prior runs.

### Coverage Check
- **Epic 10 Phase 3** is now fully scoped: US-069 (framework) → US-066/067/068 (content tabs). All HEADLESS.
- **Epic 10 Phase 4** (Blueprint polish: US-070, US-076, US-075) remains unscoped — these are EDITOR stories and lower priority. Will scope when Phase 3 stories approach completion.
- **Epic 6** (Weapons): US-024 is scoped as the gate story. US-025–030 remain placeholders — will scope as Epic 5.5 + Epic 10 Phase 2-3 approach completion.

### Next Session Recommendation

**Batch 1 — Parallelize (all HEADLESS, all independent, all ready NOW):**
1. **US-059** (Blinded/Fear/Cursed) — 17 ACs, 11 tests. Follows established GE + attribute modifier pattern.
2. **US-060** (Illusion/Blur) — 8 ACs, 8 tests. New actor type + projectile miss-chance pipeline.
3. **US-056** (Status-Applying Attack Profiles) — 7 ACs, 6 tests. Extends attack profile + hit processing.

**Batch 2 — Sequential (after Batch 1):**
4. **US-018** (Drenched/Focused) — 15 ACs, 14 tests. Skip Frostbitten/Shocked ACs (US-015 blocked).

**Batch 3 — EDITOR (after US-056):**
5. **US-057** (Playable Magic Arena) — EDITOR. Wire spells + status attacks + HUD. Jeff can play-test magic.

**Batch 4 — Phase 2+3 (after Batch 3, parallelize):**
6. **US-064** (Enemy Indicators) + **US-069** (Pause Menu) — both HEADLESS, independent. Can parallelize.

**Batch 5 — After Batch 4:**
7. **US-065** (Floating Damage Numbers) — HEADLESS, after US-064.

**Batch 6 — Phase 3 content (after US-069, parallelize):**
8. **US-066** (Character Sheet) + **US-067** (Skill Tree) + **US-068** (Feat Display) — all HEADLESS, all independent once US-069 is done. Can parallelize all three.

**Still blocked:**
- **US-015** — Needs Jeff's investigation before Drenched synergy ACs can be fully verified.

---

## 2026-04-14 Nightly Planning Run

### Completed Since Last Run
Massive progress since the last planning run. All remaining batch items through Epic 10 Phase 3 are done:

**Epic 4 (Status Effects) — COMPLETE:**
- US-015: Frostbitten & Shocked (was BLOCKED, now resolved and done)
- US-059: Blinded, Fear, Cursed ✅
- US-018: Drenched & Focused Buff ✅

**Epic 5 (Magic) — COMPLETE:**
- US-060: Illusion & Blur ✅

**Epic 5.5 (Playable Magic Slice) — COMPLETE:**
- US-056: Status-Applying Attack Profiles ✅
- US-057: Playable Magic Arena Integration ✅

**Epic 10 Phase 2 (World-Space Indicators) — COMPLETE:**
- US-064: Enemy Indicator System Migration ✅
- US-065: Floating Damage Numbers ✅

**Epic 10 Phase 3 (Menus & Progression UI) — COMPLETE:**
- US-069: Pause Menu Framework ✅
- US-066: Character Sheet Widget ✅
- US-067: Skill Tree Display ✅
- US-068: Feat Display ✅

**Epic 6 (Weapons) — Gate story done:**
- US-024: Weapon Class Framework ✅

### Currently In Progress
- (none — between batches)

### New Stories Created

- **US-026: Blunt & Polearm Weapon Profiles** (`stories/backlog/US-026-blunt-polearm-weapon-profiles.md`) — HEADLESS
  - Factory methods for Axe, Mace, Spear, Quarterstaff, Unarmed
  - Full attack profiles with timing, damage, shape, and stamina values for each weapon
  - 14 automation tests covering profile specs, cross-cutting rules, full 9-weapon relative ordering
  - Axe: MainHand Slash, 2 sweep lights + slam heavy, high burst damage
  - Mace: MainHand Blunt, sweep+slam lights + slam heavy, highest PostureDamageBonus (5)
  - Spear: TwoHand Pierce, 2 thrust + sweep finisher lights + lunge heavy, longest melee range (280)
  - Quarterstaff: TwoHand Blunt, sweep+thrust+sweep lights + 360 sweep heavy, jack-of-all-trades
  - Unarmed: MainHand Blunt, 3 thrust lights + slam heavy, fastest but weakest, zero BaseDamage
  - Note: Axe, Mace, Quarterstaff, and Unarmed profiles are planner-scoped (design docs describe identity but not specific numbers). Values follow attack taxonomy patterns and are marked as initial tuning placeholders.

- **US-077: Weapon Cycling & Equipped Weapon Display** (`stories/backlog/US-077-weapon-cycling-hud.md`) — HEADLESS
  - Adds AvailableWeapons list + CycleNextWeapon/CyclePrevWeapon to EquipmentComponent
  - OnWeaponChanged multicast delegate for UI binding
  - UMordecaiEquippedWeaponWidget (weapon name/type display, "Unarmed" fallback)
  - 12 automation tests covering cycling, wrapping, empty list, delegate firing, widget updates

- **US-078: Playable Weapon Arena Integration** (`stories/backlog/US-078-playable-weapon-arena.md`) — EDITOR
  - Creates DataAssets for 5 representative weapons: Longsword, Dagger, Greatsword, Spear, Mace
  - Weapon cycling input (Tab/Y) wired to EquipmentComponent
  - Combat HUD updated with weapon display widget
  - Player spawns with all 5 weapons available, Longsword equipped by default
  - PIE verification: cycle weapons, each feels distinct, damage/speed/range differences visible

### PLAN.md Updates
- Epic 4: Marked COMPLETE (US-015 blocker resolved)
- Epic 5: Marked COMPLETE (US-060 done)
- Epic 5.5: Marked COMPLETE
- Epic 10 Phase 2: Marked COMPLETE
- Epic 10 Phase 3: Marked COMPLETE
- Epic 6: Updated with scoped stories (US-025 in backlog, US-026 scoped)
- Epic 6.5: NEW integration milestone added (US-077 + US-078)
- Priority order updated to reflect current state — item 10 is now active
- US-027-030 marked as needing design input

### Source Code State
- `Mordecai/Weapons/` — NEW: MordecaiWeaponDataAsset, MordecaiEquipmentComponent, MordecaiWeaponTypes (5 files from US-024)
- `Mordecai/UI/` — NEW: MordecaiFeatDisplayWidget (from US-068)
- `Tests/Weapons/` — NEW: MordecaiWeaponTests.cpp (from US-024)
- `Tests/UI/` — NEW: MordecaiFeatDisplayTests.cpp (from US-068)
- All prior systems unchanged from last run + US-015/059/018/060/056/057/064/065/066/067/068/069 additions

### Dependency Graph

```
CURRENT PRIORITY — Epic 6 Melee Weapons:

  US-025 (Blade Family profiles) ──┐
  US-026 (Blunt/Polearm profiles) ─┤ (both HEADLESS, can parallelize)
                                    │
                                    v
  US-077 (Weapon Cycling + HUD widget, HEADLESS)
                                    │
                                    v
  US-078 (Playable Weapon Arena, EDITOR — integration milestone)

AFTER Epic 6.5:
  Epic 6 ranged weapons (US-027-030) — BLOCKED on design input
  Epic 10 Phase 4 (US-070, US-076, US-075) — Blueprint polish, lower priority
  Epic 7 — Inventory
```

### Design Gaps / Decisions Needed
- **Ranged weapons (US-027):** Longbow and Shortbow have sufficient design detail. Crossbow, Throwables, and Wands do NOT — they need specific attack mechanics, charge patterns, and interaction with existing spell/projectile systems designed before scoping.
- **Two-weapon fighting (US-028):** OffHand equip slot exists but dual-wield rules (damage penalties, combo chains, interaction with TwoHand) are completely undefined in design docs.
- **Shield system (US-030):** Block mechanic exists from US-006 but shield types, durability, and equip slot interaction are undefined.
- **Armor system (US-029):** Item schema has Armor as an ItemType. Defense formula has mitigation concept. But armor slots, stat values, weight/encumbrance, and resistance system are undefined.
- **Heavy attack input routing:** The melee ability (US-004) handles light combo chains. Whether it routes InputSlot::Heavy to HeavyAttackProfile needs verification during US-077/078. Noted as TODO(DECISION) in US-078.
- **Weapon numeric tuning:** US-026 profiles for Axe, Mace, Quarterstaff, and Unarmed are planner-scoped from attack taxonomy identity descriptions. Jeff may want to tune these after play-testing in US-078.

### Next Session Recommendation

**Batch 1 — Parallelize (both HEADLESS, independent, ready NOW):**
1. **US-025** (Blade Family profiles) — 12 tests. Well-scoped, factory pattern for Longsword/Greatsword/Shortsword/Dagger.
2. **US-026** (Blunt & Polearm profiles) — 14 tests. Same factory pattern for Axe/Mace/Spear/Quarterstaff/Unarmed.

**Batch 2 — Sequential (after Batch 1):**
3. **US-077** (Weapon Cycling + HUD) — 12 tests. Small C++ story, extends EquipmentComponent.

**Batch 3 — EDITOR (after US-077):**
4. **US-078** (Playable Weapon Arena) — Create DataAssets, wire input, configure arena. Jeff can play-test all melee weapons.

**After Epic 6.5:**
- Ranged weapon stories need design input from Jeff before scoping
- Epic 10 Phase 4 (Blueprint polish) is ready to scope if there's a gap
- Epic 7 (Inventory) can be scoped in parallel

---

## 2026-04-16 Nightly Planning Run

### Completed Since Last Run
Epic 6.5 (Playable Weapon Arena) completed — both stories landed:
- US-077: Weapon Cycling & Equipped Weapon Display ✅ (HEADLESS)
- US-078: Playable Weapon Arena Integration ✅ (EDITOR)

All of Epic 6 melee (US-024, US-025, US-026) + Epic 6.5 (US-077, US-078) is now in `done/`. Backlog and in-progress are empty.

### Currently In Progress
- (none — clean state)

### New Stories Created
All three Epic 7 stories scoped. All HEADLESS. All placed in `stories/backlog/`.

- **US-032: Item Definition & Categories** (`stories/backlog/US-032-item-definition-categories.md`) — HEADLESS
  - Foundation story. Delivers `EMordecaiItemType`, `EMordecaiCarryModel`, `EMordecaiStorageDomain`, `EMordecaiSortPriority`, `EMordecaiBindType` enums
  - `UMordecaiItemDefinition` (UDataAsset) with identity/classification/storage/stack/ownership fields mirroring `item_schema_v2.md`
  - `UMordecaiItemLibrary` with `CompareSortPriority`, `GetTypeDefaultAutoStore`, tag accessors
  - Reuses existing `EMordecaiItemRarity` and `EMordecaiModifierOp` from Weapons — does NOT duplicate
  - Does NOT refactor `UMordecaiWeaponDataAsset` to inherit from the new class (deferred)
  - 13 automation tests — enums, fields, helpers, tag mapping
  - **IMPLEMENT FIRST in Epic 7 — US-031 and US-033 depend on this.**

- **US-031: Flat Inventory & Auto-Store System** (`stories/backlog/US-031-flat-inventory-auto-store.md`) — HEADLESS
  - `FMordecaiItemInstance` struct (InstanceId, ItemDefinition, Quantity, AffixRolls, IsEquipped)
  - `UMordecaiInventoryComponent` (replicated, flat list, unlimited carry): AddItem, RemoveItem, ConsumeByDefinition (atomic), GetAllItems/ByType/Sorted, FindInstance, GetTotalQuantityOfDefinition
  - `UMordecaiResourceLedger` (replicated component, TMap<FName,int32>): AddResource, GetResourceCount, ConsumeResource (atomic), GetAllResources
  - `PickupItem` unified entry point — routes by `IsAutoStored()`: Materials/TownResources go to ledger, everything else to inventory
  - OnInventoryChanged and OnResourceChanged multicast delegates
  - Stack merging with overflow, no capacity cap (enforces locked rule)
  - AMordecaiCharacter wires both components as subobjects
  - 18 automation tests — routing, stacking, atomicity, filters, delegates, no-cap invariant
  - Flagged TODO(DECISION): ledger location may migrate to PlayerState when town persistence lands (future story)

- **US-033: Unidentified Items & Identification Service** (`stories/backlog/US-033-unidentified-items-town-gating.md`) — HEADLESS
  - Adds `EMordecaiIdentificationState` enum (Identified/Unidentified)
  - `UMordecaiItemDefinition` gains UsesIdentification/DefaultIdentificationState/RequiresIdentificationToEquip/ShowPartialInfoBeforeIdentify/IdentificationService fields
  - `FMordecaiItemInstance` gains IdentificationState field; instance creation respects `Def->DefaultIdentificationState` but forces Identified when UsesIdentification=false
  - `UMordecaiItemLibrary` adds GetDisplayName/GetDescription/GetVisibleTags helpers — returns partial info (e.g., "Unidentified Purple Sword") for unidentified items when ShowPartialInfoBeforeIdentify is true
  - CanEquipInstance blocks equip when RequiresIdentificationToEquip + Unidentified
  - `UMordecaiIdentificationService` (UGameInstanceSubsystem) with IdentifyInstance + OnItemIdentified delegate — callable headlessly, no NPC/UI (future town story)
  - 17 automation tests — field defaults, partial-info rendering, tag filtering, equip gating, service state flip, delegate firing
  - No cost model; flagged TODO(DECISION): identification is currently free — gold/time/reagent cost is future design decision

### PLAN.md Updates
- Epic 7 marked "3/3 scoped, ready for implementation" with explicit implementation order (US-032 → US-031 → US-033)
- Priority order updated: item 10 (Epic 6 + 6.5) marked complete; item 11 (Epic 7) is now the CURRENT PRIORITY
- Added note that next integration milestone candidate is US-071 (Inventory UI, EDITOR) — expected after Epic 7 completes, in line with Playability-First Rule

### Source Code State
- `Mordecai/Weapons/` — US-024 foundation + blade/blunt/polearm profiles + cycling
- `Mordecai/UI/` — Combat HUD widgets + character sheet + skill tree + feat display + equipped weapon
- `Mordecai/Items/` — does NOT exist yet. US-032 creates this directory.
- All prior systems unchanged from 2026-04-14 run + US-025/026/077/078 additions

### Dependency Graph

```
CURRENT — Epic 7 Inventory:

  US-032 (Item Definition & Categories, HEADLESS)
                        │
                        ▼
  US-031 (Flat Inventory & Auto-Store, HEADLESS)
                        │
                        ▼
  US-033 (Unidentified Items & ID Service, HEADLESS)

AFTER Epic 7:
  US-071 (Inventory UI, EDITOR) — integration milestone for Epic 7
  Epic 10 Phase 4 (Blueprint polish: US-070, US-076, US-075) — EDITOR, lower priority
  Epic 6 ranged weapons (US-027–030) — BLOCKED on design input
```

### Design Gaps / Decisions Needed
- **Identification cost model:** Current scope makes identification free and instant. A future decision is needed on gold cost, in-game time cost, reagent requirements, or NPC-service gating. Flagged as TODO(DECISION) in US-033.
- **Ledger ownership:** Resource ledger is per-character for launch (US-031). When town persistence / shared storage lands, this likely migrates to PlayerState or a GameInstance subsystem. Out of scope here, flagged in US-031.
- **Weapon/Item definition relationship:** `UMordecaiWeaponDataAsset` (US-024) does NOT currently inherit from the new `UMordecaiItemDefinition` (US-032). They coexist for now. A future refactor story can unify them — scoping that is risky (touches all weapon profile factories) and NOT in scope for Epic 7.
- **Ranged weapons (US-027):** Still blocked on design for crossbow, throwables, wands. Longbow/Shortbow are scopable (clear intent in attack_taxonomy) but have no numeric values — planner-scoped initial tuning would be needed like US-026. Noted but not scoped this run.
- **Shield/Armor/Two-weapon (US-028/029/030):** Still blocked on design input.

### Next Session Recommendation

**Batch 1 — Sequential (HEADLESS, all ready NOW):**
1. **US-032** (Item Definition & Categories) — foundation. 13 tests. Well-scoped, follows DataAsset pattern from US-024.
2. **US-031** (Flat Inventory & Auto-Store) — after US-032. 18 tests. Replicated component + resource ledger.
3. **US-033** (Unidentified Items & ID Service) — after US-031. 17 tests. Per-instance state + library helpers + subsystem.

Epic 7 is **three sequential HEADLESS stories**. None can parallelize (each depends on the previous). Total: 48 tests across three stories. All end with PIE smoke test per the "PIE smoke tests run every time" rule.

**Playability-First note:** Epic 6.5 just concluded as an integration milestone. Epic 7 is pure system work. After Epic 7, the next integration candidate is US-071 (Inventory UI, EDITOR) to make the inventory/auto-store/identification flow actually visible and testable by Jeff in-game.

**Still blocked (unchanged):**
- Ranged weapons (US-027), two-weapon fighting (US-028), armor (US-029), shields (US-030) — need design input before scoping

---

## 2026-04-17 Nightly Planning Run

### Completed Since Last Run
- (none) No implementation commits have landed since the 2026-04-16 run. HEAD is still at `bdc0119` ([US-078] Mark DoD complete), which pre-dates the last planner entry.

### Currently In Progress
- (none — `stories/in-progress/` is empty)

### New Stories Created
- (none) Epic 7 was fully scoped in the 2026-04-16 run. All three stories remain in `stories/backlog/` and are still valid:
  - `US-032-item-definition-categories.md` (HEADLESS) — foundation, go first
  - `US-031-flat-inventory-auto-store.md` (HEADLESS) — depends on US-032
  - `US-033-unidentified-items-town-gating.md` (HEADLESS) — depends on US-031

### PLAN.md Updates
- No changes this run. Priority order is unchanged: Epic 7 (Inventory) remains CURRENT PRIORITY.
- Note: `stories/PLAN.md` and `stories/PLANNER_LOG.md` edits from the 2026-04-16 run plus the three backlog story files are still uncommitted locally (working-tree only). They were authored by the previous planner but were never staged. No action required — the next commit (from either a coding agent picking up US-032 or the user manually) will pick them up. Flagging so it is not surprising.

### Source Code State
- Unchanged from 2026-04-16 run. `Mordecai/Items/` still does not exist — US-032 will create it.

### Dependency Graph

```
CURRENT — Epic 7 Inventory (unchanged from 2026-04-16):

  US-032 (Item Definition & Categories, HEADLESS)
                        │
                        ▼
  US-031 (Flat Inventory & Auto-Store, HEADLESS)
                        │
                        ▼
  US-033 (Unidentified Items & ID Service, HEADLESS)

AFTER Epic 7:
  US-071 (Inventory UI, EDITOR) — integration milestone for Epic 7
  Epic 10 Phase 4 (Blueprint polish: US-070, US-076, US-075) — EDITOR, lower priority
  Epic 6 ranged weapons (US-027–030) — BLOCKED on design input
```

### Design Gaps / Decisions Needed
Unchanged from 2026-04-16 run:
- Identification cost model (US-033) — free/instant for now, future TODO(DECISION)
- Ledger ownership (US-031) — per-character for launch; may migrate to PlayerState with town persistence
- WeaponDataAsset ↔ ItemDefinition relationship — coexist for now, unification deferred
- Ranged weapons / two-weapon / shields / armor — still blocked on design input

### Next Session Recommendation

**No scoping work needed — coding agents should pick up Epic 7 in order.**

1. **US-032** (Item Definition & Categories, HEADLESS) — ready NOW, foundation story. 13 tests.
2. **US-031** (Flat Inventory & Auto-Store, HEADLESS) — after US-032 lands. 18 tests.
3. **US-033** (Unidentified Items & ID Service, HEADLESS) — after US-031 lands. 17 tests.

All three are sequential (each depends on the previous). No parallelization opportunity inside Epic 7.

**After Epic 7:** Per the Playability-First Rule, the next integration milestone candidate is **US-071 (Inventory UI, EDITOR)**. That is the natural place to scope once US-033 lands — it turns the headless inventory/auto-store/identification plumbing into something Jeff can see and exercise in PIE. Not scoping it this run because US-071 currently has no backlog file and writing it today would just stale before Epic 7 completes; scope it when Epic 7 is finishing up so it stays fresh.

**Still blocked (unchanged):**
- Ranged weapons (US-027), two-weapon fighting (US-028), armor (US-029), shields (US-030) — need design input before scoping

---

## 2026-04-18 Nightly Planning Run

### Completed Since Last Run
- (none) HEAD is still `bdc0119` — no implementation commits since the 2026-04-16 / 2026-04-17 runs.

### Currently In Progress
- (none — `stories/in-progress/` is empty)

### New Stories Created
- (none) Epic 7 backlog from 2026-04-16 is still queued and valid:
  - `US-032-item-definition-categories.md` (HEADLESS) — foundation, go first
  - `US-031-flat-inventory-auto-store.md` (HEADLESS) — depends on US-032
  - `US-033-unidentified-items-town-gating.md` (HEADLESS) — depends on US-031

### PLAN.md Updates
- No changes this run. Priority order unchanged: Epic 7 (Inventory) is CURRENT PRIORITY.
- The 2026-04-16 working-tree changes (PLAN.md + PLANNER_LOG.md edits, three backlog files) are still uncommitted. Same situation flagged on 2026-04-17 — will be picked up by the next coding-agent commit. No action.

### Source Code State
- Unchanged. `Source/LyraGame/Mordecai/Items/` still does not exist. US-032 will create it.

### Dependency Graph
Unchanged from 2026-04-17:
```
CURRENT — Epic 7 Inventory:
  US-032 (HEADLESS) ──▶ US-031 (HEADLESS) ──▶ US-033 (HEADLESS)

AFTER Epic 7:
  US-071 (Inventory UI, EDITOR) — integration milestone, scope when US-033 is finishing
  Epic 10 Phase 4 (US-070, US-076, US-075) — EDITOR Blueprint/VFX polish
  Epic 6 ranged/two-weapon/armor/shields (US-027–030) — BLOCKED on design input
```

### Design Gaps / Decisions Needed
Unchanged from 2026-04-17 — no new decisions are blocking Epic 7 scoping.

### Next Session Recommendation

No scoping work needed. Coding agents should pick up Epic 7 in order:

1. **US-032** (Item Definition & Categories, HEADLESS) — ready NOW. 13 tests.
2. **US-031** (Flat Inventory & Auto-Store, HEADLESS) — after US-032 lands. 18 tests.
3. **US-033** (Unidentified Items & ID Service, HEADLESS) — after US-031 lands. 17 tests.

After US-033 lands, the planner should scope **US-071 (Inventory UI, EDITOR)** as the next integration milestone per the Playability-First Rule — that turns the inventory plumbing into something Jeff can exercise in PIE.

**Still blocked (unchanged):** US-027 (ranged), US-028 (two-weapon), US-029 (armor), US-030 (shields) — need design input.

---

## 2026-04-19 Nightly Planning Run

### Completed Since Last Run
- (none) HEAD is still `bdc0119` — no implementation commits since 2026-04-16. This is the **fourth consecutive planner run** with zero forward motion on Epic 7.

### Currently In Progress
- (none — `stories/in-progress/` is empty)

### New Stories Created
- (none) Epic 7 backlog from 2026-04-16 remains queued and valid:
  - `US-032-item-definition-categories.md` (HEADLESS) — foundation, go first
  - `US-031-flat-inventory-auto-store.md` (HEADLESS) — depends on US-032
  - `US-033-unidentified-items-town-gating.md` (HEADLESS) — depends on US-031

### PLAN.md Updates
- No changes this run.
- The 2026-04-16 working-tree edits (PLAN.md + PLANNER_LOG.md + three backlog story files under `stories/backlog/`) are **still uncommitted** in the working tree — flagged on 04-17 and 04-18, now flagged for a fourth day. A coding agent picking up US-032 will incidentally commit them; if no agent runs, they will drift further. Low risk (local-only) but worth noting.

### Source Code State
- Verified unchanged: `Source/LyraGame/Mordecai/` contains `AbilitySystem, Camera, Combat, Enemy, Feats, Magic, Skills, StatusEffects, UI, Weapons` — no `Items/` folder. US-032 will create it.

### Dependency Graph
Unchanged:
```
CURRENT — Epic 7 Inventory:
  US-032 (HEADLESS) ──▶ US-031 (HEADLESS) ──▶ US-033 (HEADLESS)

AFTER Epic 7:
  US-071 (Inventory UI, EDITOR) — integration milestone, scope when US-033 is finishing
  Epic 10 Phase 4 (US-070, US-076, US-075) — EDITOR Blueprint/VFX polish
  Epic 6 ranged/two-weapon/armor/shields (US-027–030) — BLOCKED on design input
```

### Design Gaps / Decisions Needed
Unchanged — nothing blocking Epic 7 scoping.

### Stall Pattern — Flagging For Jeff
Four nightly planner runs (04-16, 04-17, 04-18, 04-19) have completed without any coding agent picking up US-032. The backlog is healthy and ready; the blocker is strictly *agent invocation*, not scoping. If this pattern continues, the planner's output will continue to be no-ops. Possible causes (not acting on any of these — flagging only):
- Jeff has been away / not starting coding sessions
- Coding agents are being invoked on other work not visible from this repo
- An environment issue is preventing agents from picking up backlog

No action from the planner. Surfacing this so it is visible rather than quietly repeating.

### Next Session Recommendation

**No new scoping work.** Coding agents should pick up Epic 7 in order:

1. **US-032** (Item Definition & Categories, HEADLESS) — ready NOW. 13 tests.
2. **US-031** (Flat Inventory & Auto-Store, HEADLESS) — after US-032 lands. 18 tests.
3. **US-033** (Unidentified Items & ID Service, HEADLESS) — after US-031 lands. 17 tests.

After US-033 lands, scope **US-071 (Inventory UI, EDITOR)** as the next integration milestone per the Playability-First Rule.

**Still blocked (unchanged):** US-027 (ranged), US-028 (two-weapon), US-029 (armor), US-030 (shields) — need design input.

---

## 2026-04-20 Nightly Planning Run

### Completed Since Last Run
Epic 7 finished in a single burst. All three HEADLESS inventory stories landed between 2026-04-19 and 2026-04-20 (commits `dbf46d7` through `1666193`):
- **US-032** (Item Definition & Categories) — `UMordecaiItemDefinition`, enums, sort/stack helpers, item tags
- **US-031** (Flat Inventory & Auto-Store) — `UMordecaiInventoryComponent`, `UMordecaiResourceLedger`, pickup routing
- **US-033** (Unidentified Items & ID Service) — per-instance state, partial-info API, `UMordecaiIdentificationService`

Epic 7 is now fully complete. Four consecutive no-op planner runs (04-16 → 04-19) ended — the stall pattern flagged on 04-19 broke today.

### Currently In Progress
- (none — `stories/in-progress/` is empty)

### New Stories Created
Scoping the Epic 7 integration milestone per Playability-First Rule (Epic 7 was pure HEADLESS system work; a playable proof-point is now mandatory). Following the Epic 5.5 (US-057) and Epic 6.5 (US-077+US-078) template: HEADLESS widget first, then EDITOR playable integration.

- **US-071: Inventory UI — Flat List, Filter & Ledger Widget** (HEADLESS)
  - `UMordecaiInventoryWidget` content class for the Pause Menu "Inventory" tab
  - Binds to `UMordecaiInventoryComponent` + `UMordecaiResourceLedger` via change delegates (no polling)
  - Sorted flat list using `GetSortedItems`, category filter (All/Weapons/Armor/Trinkets/Consumables/Materials/Quest/Magical)
  - Rarity color map, identification badge, "Identify" action dispatching to `UMordecaiIdentificationService`
  - Ledger panel for auto-stored resources
  - Empty-state and unbound-state handling
  - 17 automation tests (bind/unbind, display, filter, ledger, identify happy path + edge cases, rebuild on delegate)
  - No drop / equip actions — those are future scope

- **US-079: Playable Inventory Arena Integration** (EDITOR)
  - `AMordecaiItemPickup` C++ actor (sphere trigger → `PickupItem`), server-authoritative, replicated
  - 6 test `UMordecaiItemDefinition` DataAssets covering every major routing case: Consumable+stackable, Material (auto-store), TownResource (auto-store), UpgradeKey (Critical sort), MagicalItem (UsesIdentification+RequiresIdentificationToEquip), Weapon (no identification)
  - Python scripts (`create_test_pickups.py`, `place_test_pickups.py`) so the arena setup is reproducible
  - Pause Menu "Inventory" tab wired to the widget from US-071
  - BP_MordecaiItemPickup with in-world name label (uses `GetDisplayName` so unidentified pickups show partial info)
  - Minimum viable BP polish on the Inventory widget (filter buttons visible/clickable, rarity colors on rows, identify button on unidentified rows)
  - `Scripts/verify_pie_inventory.py` — full PIE smoke: pickup everything → assert inventory + ledger contents → open pause menu → verify tab populated → identify amulet → verify row rebuild
  - 4 C++ automation tests on the pickup actor (overlap routing, destroy, null guard, server-auth)

### PLAN.md Updates
- Epic 7 marked complete (was "3/3 scoped, ready for implementation")
- **Epic 7.5 added**: Playable Inventory Slice with US-071 + US-079 (mirrors Epic 5.5 / 6.5 structure)
- Priority order: Epic 7.5 is now the CURRENT PRIORITY. Epic 6 (ranged/two-weapon/armor/shields) still BLOCKED on design input and falls to next position.
- Phase 5+ "Future" list updated: US-071/US-079 moved out of "future" since they are now scoped under Epic 7.5. US-072/073/074 remain future (blocked on their parent epics).

### Dependency Graph

```
CURRENT — Epic 7.5 Playable Inventory Slice:

  US-071 (Inventory Widget, HEADLESS)
                        │
                        ▼
  US-079 (Playable Inventory Arena, EDITOR)

AFTER Epic 7.5:
  Epic 10 Phase 4 (BP/VFX polish: US-070, US-076, US-075) — EDITOR, lower priority
  Epic 6 ranged/two-weapon/armor/shields (US-027–030) — BLOCKED on design input
  Epic 8 (World & Exploration) — unscoped
  Epic 9 (Town Management) — unscoped; Mage Tower NPC UI unblocks US-033's identification cost story
```

### Design Gaps / Decisions Needed
- **Identification cost model** (carried from US-033): still free/instant. The Mage Tower NPC flow in Epic 9 will likely introduce a gold/reputation cost — US-071's identify button is written to succeed or no-op, so adding a cost later is non-breaking but should be planned.
- **Equip/drop actions from the inventory widget**: intentionally out of scope for US-071. The weapon-cycling flow (US-077) already handles equipping for weapons; armor/trinkets equip via the inventory UI is a future story once Epic 6 ranged/armor stories scope.
- **Rarity enum naming**: `EMordecaiItemRarity` has canonical names (Common/Uncommon/Rare/Epic/Legendary/Mythic) while `item_system_v1.md` uses color names (Green/Blue/Purple/Red/Gold). US-071 maps each enum value to its FLinearColor — the color naming is presentation-only. Not a blocker.
- **Ranged weapons / two-weapon / shields / armor** (US-027–030): unchanged — still blocked on design input. Flagged for the 5th consecutive run.

### Next Session Recommendation

**Batch 1 — Sequential (Epic 7.5):**
1. **US-071** (Inventory Widget, HEADLESS) — ready NOW. 17 tests. Follows US-066/067/068 widget pattern.
2. **US-079** (Playable Inventory Arena, EDITOR) — after US-071 lands. 4 C++ tests + 1 PIE smoke. Depends on US-071 widget class.

Both stories are fresh and specific. The HEADLESS story is agent-friendly and lines up with the established widget pattern; the EDITOR story is the playable proof Jeff needs after three epics of inventory system work.

**After Epic 7.5:** The next scoping decision is whether to start Epic 10 Phase 4 (BP/VFX polish — low risk but also low game-feel impact) or to unblock Epic 6 ranged weapons. Ranged needs design input. BP polish is scopable today but lower priority than actual new feature work. Revisit after Epic 7.5 lands.

**Still blocked (unchanged, 5th run):** US-027 (ranged), US-028 (two-weapon), US-029 (armor), US-030 (shields) — need design input. Consider asking Jeff to make a decision on at least one of these so Epic 6 can resume after Epic 7.5.

### Addendum — Second planner invocation on 2026-04-20

Planner re-invoked later the same day. State is unchanged since the earlier 04-20 run:
- HEAD still at `1666193 [US-033] Mark all ACs complete, move story to done` — no implementation commits yet today after Epic 7.
- `stories/in-progress/` still empty.
- `stories/backlog/` still contains exactly US-071 and US-079 (Epic 7.5).
- `stories/PLAN.md` + `stories/PLANNER_LOG.md` + both backlog story files remain uncommitted in the working tree — they will be picked up by the next coding-agent commit (likely the US-071 commit).

Re-read both backlog stories to confirm they are still well-scoped and ready:
- **US-071** (HEADLESS, 17 tests) — ACs cover bind/unbind, flat list, rarity color map, category filter (with Materials/Magical/Quest group rules), ledger panel, identify action + 3 no-op edge cases, empty/filter-empty placeholders, rebuild-on-delegate. References match existing US-031/032/033 API surface (`GetSortedItems`, `OnInventoryChanged`, `UMordecaiItemLibrary::GetDisplayName`, `UMordecaiIdentificationService::IdentifyInstance`). Follows the US-066/067/068 widget pattern. Ready to pick up.
- **US-079** (EDITOR, 4 C++ tests + PIE smoke) — ACs cover pickup actor, 6 test DataAssets covering every routing case (stackable consumable, Material auto-store, TownResource auto-store, Critical-sort UpgradeKey, unidentified MagicalItem, non-identifying Weapon), arena placement, pause-menu tab wiring, BP polish floor, and a full PIE smoke covering pickup → ledger/inventory assertion → open pause menu → identify → rebuild. Reproducible via `Scripts/create_test_pickups.py` + `Scripts/place_test_pickups.py`. Correctly blocks on US-071.

No new scoping needed. No changes to PLAN.md. No new design gaps. Recommendation from the earlier 04-20 entry stands: coding agents pick up US-071 first, then US-079. After Epic 7.5 lands, the next planner run should decide between Epic 10 Phase 4 (BP/VFX polish) and attempting to unblock at least one of US-027–030 with Jeff.

---

## 2026-04-21 Nightly Planning Run

### Completed Since Last Run
- None. HEAD is still at `1666193 [US-033] Mark all ACs complete, move story to done` — no commits landed between the 04-20 addendum and now.

### Currently In Progress
- (none — `stories/in-progress/` is empty)

### New Stories Created
- None. Epic 7.5 (US-071 HEADLESS + US-079 EDITOR) was fully scoped on 04-20 and both files remain in `stories/backlog/` unchanged. Both are well-specified and ready to pick up as-written.

### Working-Tree State
- `stories/PLAN.md` — modified, uncommitted (Epic 7.5 block added 04-20)
- `stories/PLANNER_LOG.md` — modified, uncommitted
- `stories/backlog/` — untracked (US-071 + US-079 story files, authored 04-20)
- These will flow into the next coding-agent commit (expected to be the US-071 implementation commit). No planner action.

### Design Gaps / Decisions Needed
- No change from 04-20. Still pending Jeff input: identification cost model (deferred to Epic 9 Mage Tower), and design for US-027 (ranged), US-028 (two-weapon), US-029 (armor), US-030 (shields). These remain blocked for a 6th consecutive run.

### Stall Pattern
No coding-agent activity for 1 calendar day (last commit 04-20 closed Epic 7). This is not yet the multi-day stall pattern seen pre-Epic 7 — flagging only as an early observation. If 04-22 also produces no commits, escalate per the 04-19 template.

### Next Session Recommendation

**Unchanged from 04-20:**

1. **US-071** (Inventory Widget, HEADLESS) — ready NOW. 17 tests. Matches US-066/067/068 widget pattern. This is the single most valuable next unit of work.
2. **US-079** (Playable Inventory Arena, EDITOR) — after US-071 lands. 4 C++ tests + PIE smoke. Delivers the playable Epic 7 proof-point.

**After Epic 7.5:** Decide between Epic 10 Phase 4 (BP/VFX polish — low risk, scopable today) or unblocking Epic 6 ranged/two-weapon/armor/shields with Jeff. The playability-first rule is satisfied for the current cycle once US-079 ships, so either direction is acceptable from a rhythm standpoint.

**Still blocked (6th consecutive run):** US-027, US-028, US-029, US-030 — design input needed.

---

## 2026-04-26 Nightly Planning Run

### Completed Since Last Run
- None. HEAD is still at `1666193 [US-033] Mark all ACs complete, move story to done`. The 5-day gap between 04-21 and 04-26 produced no coding-agent commits.

### Currently In Progress
- (none — `stories/in-progress/` is empty)

### New Stories Created
- None. Epic 7.5 (US-071 HEADLESS + US-079 EDITOR) was scoped on 04-20 and re-verified on 04-21. Both backlog files are unchanged and remain ready to pick up as-written.

### Working-Tree State
- `stories/PLAN.md` — modified, uncommitted (Epic 7.5 block added 04-20)
- `stories/PLANNER_LOG.md` — modified, uncommitted (this entry plus 04-20/04-21)
- `stories/backlog/` — untracked (US-071 + US-079 story files, authored 04-20)
- These will flow into the next coding-agent commit (expected to be the US-071 implementation commit). No planner action.

### Verification Pass on Existing Backlog
Re-checked Source tree against the API surfaces named in US-071 / US-079:
- `Source/LyraGame/Mordecai/Items/` contains `MordecaiInventoryComponent`, `MordecaiResourceLedger`, `MordecaiItemDefinition`, `MordecaiItemInstance`, `MordecaiItemLibrary`, `MordecaiIdentificationService`, `MordecaiItemTypes` — every API surface referenced in the backlog stories exists. No drift.
- `Source/LyraGame/Mordecai/UI/` contains the US-066/067/068 widgets (`MordecaiCharacterSheetWidget`, `MordecaiSkillTreeWidget`, `MordecaiFeatDisplayWidget`) plus `MordecaiPauseMenuWidget` / `MordecaiPauseMenuSubsystem`. No `MordecaiInventoryWidget` yet (expected — US-071 still backlog).
- Design docs: `agent_rules_v2.md` last modified 2026-04-11; nothing newer than what was scoped against. No new docs warrant rescoping.

Conclusion: scoping is still correct; the bottleneck is agent invocation, not planner output.

### Design Gaps / Decisions Needed
Unchanged from 04-21:
- Identification cost model (deferred to Epic 9 Mage Tower)
- US-027 (ranged), US-028 (two-weapon), US-029 (armor), US-030 (shields) — still blocked on design input. **7th consecutive run** flagging this.

### Stall Pattern — Escalating
Five calendar days have elapsed since the last commit (04-20 → 04-26) with no coding-agent activity. This now matches or exceeds the 04-16 → 04-19 stall pattern that was flagged previously. Possible causes (planner does not act on these — surfacing only):
- Jeff has been away or focused on other work
- Coding agents are running on other repos
- An environment / invocation issue is preventing pickup

The healthy backlog (2 well-scoped stories, all dependencies satisfied) remains the same. If 04-27 also produces no commits, that's a 6-day stall and worth a direct prompt to Jeff to sanity-check the agent invocation pipeline.

### Next Session Recommendation

**Unchanged from 04-20 / 04-21:**

1. **US-071** (Inventory Widget, HEADLESS) — ready NOW. 17 tests. Matches US-066/067/068 widget pattern. Single most valuable next unit of work.
2. **US-079** (Playable Inventory Arena, EDITOR) — after US-071 lands. 4 C++ tests + PIE smoke. Delivers the playable Epic 7 proof-point per the Playability-First Rule.

**After Epic 7.5:** Same fork as before — Epic 10 Phase 4 (BP/VFX polish, scopable today) or unblock Epic 6 ranged/two-weapon/armor/shields with Jeff input. No new information to break that tie.

**Still blocked (7th consecutive run):** US-027, US-028, US-029, US-030 — design input needed.

---

## 2026-04-27 Nightly Planning Run

### Completed Since Last Run
- None. HEAD is still at `1666193 [US-033] Mark all ACs complete, move story to done`. The 04-26 → 04-27 window produced no coding-agent commits.

### Currently In Progress
- (none — `stories/in-progress/` is empty)

### New Stories Created
- None. Epic 7.5 (US-071 HEADLESS + US-079 EDITOR) was scoped on 04-20 and deep-verified on 04-26. Both backlog files remain unchanged on disk and ready to pick up as-written.

### Working-Tree State
- `stories/PLAN.md` — modified, uncommitted (Epic 7.5 block added 04-20)
- `stories/PLANNER_LOG.md` — modified, uncommitted (this entry plus all prior runs back through 04-20)
- `stories/backlog/` — untracked (US-071 + US-079 story files, authored 04-20)
- These will flow into the next coding-agent commit (expected to be the US-071 implementation commit). No planner action.

### Verification Pass
Skipped a fresh API-surface re-verification — 04-26 ran a thorough pass against `Source/LyraGame/Mordecai/Items/` and `Source/LyraGame/Mordecai/UI/` and confirmed every API named in US-071 / US-079 still exists. No commits have landed since, so nothing has drifted. Re-verifying daily would be wasted work; the next verification pass should run when either (a) a commit lands or (b) one of the referenced design docs is updated.

### Design Gaps / Decisions Needed
Unchanged from 04-26:
- Identification cost model (deferred to Epic 9 Mage Tower).
- US-027 (ranged), US-028 (two-weapon), US-029 (armor), US-030 (shields) — still blocked on design input. **8th consecutive run** flagging this.

### Stall Pattern — 6-Day Mark Reached
Six full calendar days have now elapsed since the last commit (04-20 → 04-27). This is the threshold the 04-26 entry called out as the point to surface a direct sanity-check prompt to Jeff. Restating without acting (planner does not invoke agents):
- Healthy backlog: 2 well-scoped, dependency-satisfied stories sitting in `stories/backlog/` for 7 days.
- No environment/spec drift detected — when an agent is invoked, US-071 should land cleanly against the current source tree.
- Possible causes (unchanged): Jeff is away/elsewhere, agents running on other repos, or an invocation pipeline issue.

If 04-28 also produces no commits, the stall will exceed any prior gap in this log. At that point the recommendation is to ask Jeff to confirm the coding-agent invocation pipeline is working — the planner's output side is healthy and is not the bottleneck.

### Next Session Recommendation

**Unchanged from 04-20 / 04-21 / 04-26:**

1. **US-071** (Inventory Widget, HEADLESS) — ready NOW. 17 tests. Matches the US-066/067/068 widget pattern. Single most valuable next unit of work.
2. **US-079** (Playable Inventory Arena, EDITOR) — after US-071 lands. 4 C++ tests + PIE smoke. Delivers the playable Epic 7 proof-point per the Playability-First Rule.

**After Epic 7.5:** Same fork as before — Epic 10 Phase 4 (BP/VFX polish, scopable today) or unblock Epic 6 ranged/two-weapon/armor/shields with Jeff input. No new information to break the tie.

**Still blocked (8th consecutive run):** US-027, US-028, US-029, US-030 — design input needed.

---

## 2026-04-28 Nightly Planning Run

### Completed Since Last Run
- None. HEAD is still at `1666193 [US-033] Mark all ACs complete, move story to done`. The 04-27 → 04-28 window produced no coding-agent commits.

### Currently In Progress
- (none — `stories/in-progress/` is empty)

### New Stories Created
- None. Epic 7.5 (US-071 HEADLESS + US-079 EDITOR) was scoped on 04-20 and deep-verified on 04-26. Both backlog files remain unchanged on disk.

### Working-Tree State
- `stories/PLAN.md` — modified, uncommitted (Epic 7.5 block added 04-20)
- `stories/PLANNER_LOG.md` — modified, uncommitted (this entry plus all prior runs back through 04-20)
- `stories/backlog/` — untracked (US-071 + US-079 story files, authored 04-20)
- These will flow into the next coding-agent commit (expected to be the US-071 implementation commit). No planner action.

### Verification Pass
Skipped, per 04-27's rationale: no commits have landed since 04-26's deep verification of `Source/LyraGame/Mordecai/Items/` and `Source/LyraGame/Mordecai/UI/` against the API surfaces named in US-071/US-079. Nothing has drifted. Re-verification triggers remain (a) a commit lands or (b) a referenced design doc is updated — neither has fired.

### Design Gaps / Decisions Needed
Unchanged from 04-26 / 04-27:
- Identification cost model (deferred to Epic 9 Mage Tower).
- US-027 (ranged), US-028 (two-weapon), US-029 (armor), US-030 (shields) — still blocked on design input. **9th consecutive run** flagging this.

### Stall Pattern — Now Exceeds Any Prior Gap
Seven full calendar days have now elapsed since the last commit (04-20 → 04-28). This exceeds the 04-16 → 04-19 gap and any other gap in this log. The 04-27 entry called out 04-28 as the threshold to surface a direct sanity-check prompt to Jeff. Restating the situation:

- **Healthy backlog:** 2 well-scoped, dependency-satisfied stories sitting in `stories/backlog/` for 8 days.
- **No spec drift:** every API surface referenced by US-071/US-079 still exists; no design doc updates since scoping.
- **Bottleneck is not on the planner side.** Output is healthy; the gap is between scoping and agent invocation.

**Recommendation to Jeff:** Sanity-check the coding-agent invocation pipeline. If the pipeline is fine and you've simply been away or focused elsewhere, no action is needed — US-071 will land cleanly when the next agent picks it up. If the pipeline is broken, that's the unblock.

### Next Session Recommendation

**Unchanged from 04-20 / 04-21 / 04-26 / 04-27:**

1. **US-071** (Inventory Widget, HEADLESS) — ready NOW. 17 tests. Matches the US-066/067/068 widget pattern. Single most valuable next unit of work.
2. **US-079** (Playable Inventory Arena, EDITOR) — after US-071 lands. 4 C++ tests + PIE smoke. Delivers the playable Epic 7 proof-point per the Playability-First Rule.

**After Epic 7.5:** Same fork as before — Epic 10 Phase 4 (BP/VFX polish, scopable today) or unblock Epic 6 ranged/two-weapon/armor/shields with Jeff input. No new information to break the tie.

**Still blocked (9th consecutive run):** US-027, US-028, US-029, US-030 — design input needed.

---

## 2026-04-29 Nightly Planning Run

### Completed Since Last Run
- None. HEAD is still at `1666193 [US-033] Mark all ACs complete, move story to done` (committed 04-19). The 04-28 → 04-29 window produced no coding-agent commits.

### Currently In Progress
- (none — `stories/in-progress/` is empty)

### New Stories Created
- None. Epic 7.5 (US-071 HEADLESS + US-079 EDITOR) was scoped on 04-20 and deep-verified on 04-26. Both backlog files remain unchanged on disk and ready to pick up as-written.

### Working-Tree State
- `stories/PLAN.md` — modified, uncommitted (Epic 7.5 block added 04-20)
- `stories/PLANNER_LOG.md` — modified, uncommitted (this entry plus all prior runs back through 04-20)
- `stories/backlog/` — untracked (US-071 + US-079 story files, authored 04-20)
- These will flow into the next coding-agent commit (expected to be the US-071 implementation commit). No planner action.

### Verification Pass
Skipped, per 04-27/04-28 rationale. Re-verification triggers (commit lands or referenced design doc updated) have not fired since 04-26's deep pass against `Source/LyraGame/Mordecai/Items/` and `Source/LyraGame/Mordecai/UI/`. Nothing has drifted.

### Design Gaps / Decisions Needed
Unchanged from 04-26 / 04-27 / 04-28:
- Identification cost model (deferred to Epic 9 Mage Tower).
- US-027 (ranged), US-028 (two-weapon), US-029 (armor), US-030 (shields) — still blocked on design input. **10th consecutive run** flagging this.

### Stall Pattern — Day 9, Threshold Already Crossed
Eight full calendar days since the last commit (04-20 authoring → today). The 04-28 entry already crossed the "exceeds any prior gap" threshold and surfaced the direct sanity-check prompt to Jeff. Restating here only because daily entries are the standing format — the situation and recommendation are identical:

- **Healthy backlog:** 2 well-scoped, dependency-satisfied stories sitting in `stories/backlog/` for 9 days.
- **No spec drift.** Every API surface referenced by US-071/US-079 still exists.
- **Bottleneck is downstream of the planner.** The 04-28 prompt to Jeff still stands: sanity-check the coding-agent invocation pipeline.

To avoid log churn while the stall continues, the next several runs (until a commit lands or a design doc is updated) will be one-paragraph "no change, see 04-28" entries instead of full reruns.

### Next Session Recommendation

**Unchanged from 04-20 / 04-21 / 04-26 / 04-27 / 04-28:**

1. **US-071** (Inventory Widget, HEADLESS) — ready NOW. 17 tests. Matches the US-066/067/068 widget pattern. Single most valuable next unit of work.
2. **US-079** (Playable Inventory Arena, EDITOR) — after US-071 lands. 4 C++ tests + PIE smoke. Delivers the playable Epic 7 proof-point per the Playability-First Rule.

**After Epic 7.5:** Same fork — Epic 10 Phase 4 (BP/VFX polish) vs unblocking Epic 6 ranged/two-weapon/armor/shields with Jeff input.

**Still blocked (10th consecutive run):** US-027, US-028, US-029, US-030 — design input needed.

---

## 2026-05-15 Nightly Planning Run

### Status: No change since 04-29. Stall gap has grown.

- **HEAD:** still `1666193 [US-033] Mark all ACs complete, move story to done` (committed 04-19). 26 calendar days since the last commit.
- **Planner-side gap:** 16 calendar days since the prior planner entry (04-29). The "one-paragraph entries while the stall continues" cadence promised on 04-29 was not honored daily — the planner itself was also not invoked. Surfacing this so it's visible: the bottleneck has been the invocation pipeline on both sides (coding agent and planner), not the work queue.
- **Backlog:** `stories/backlog/` still holds **US-071** (Inventory Widget, HEADLESS) and **US-079** (Playable Inventory Arena, EDITOR), both authored 04-20 and last deep-verified 04-26. No design doc updates since then; no spec drift expected.
- **Working-tree state:** identical to 04-29 — `PLAN.md` and `PLANNER_LOG.md` modified-uncommitted, `stories/backlog/` untracked. These will flow into the next coding-agent commit.
- **Design gaps:** unchanged. Identification cost model (deferred to Epic 9). US-027/028/029/030 still blocked on design input — **11th consecutive run** flagging this.

### Recommendation to Jeff
The 04-28 prompt still stands and is now more urgent: please sanity-check the coding-agent invocation pipeline. If you've simply been away and the pipeline is fine, no action is needed — US-071 will land cleanly when the next agent picks it up. If the pipeline is broken, that's the unblock and nothing downstream will move until it's fixed.

### Next Session Recommendation (unchanged)
1. **US-071** (Inventory Widget, HEADLESS) — ready. 17 tests. Single most valuable next unit of work.
2. **US-079** (Playable Inventory Arena, EDITOR) — after US-071 lands. Delivers the playable Epic 7 proof-point.

After Epic 7.5: same fork — Epic 10 Phase 4 (BP/VFX polish) vs unblocking Epic 6 ranged/two-weapon/armor/shields with Jeff input.

---

## 2026-05-16 Nightly Planning Run

### Status: No change since 05-15. Stall now at 27 days.

- **HEAD:** still `1666193 [US-033] Mark all ACs complete, move story to done` (committed 04-19). 27 calendar days since the last commit.
- **Backlog:** unchanged — US-071 (Inventory Widget, HEADLESS) and US-079 (Playable Inventory Arena, EDITOR), both authored 04-20, last deep-verified 04-26.
- **Working tree:** unchanged from 05-15. `PLAN.md` + `PLANNER_LOG.md` modified-uncommitted; `stories/backlog/` untracked. Will flow into the next coding-agent commit.
- **Design docs:** none modified since 04-11 (`agent_rules_v2.md`). No spec drift. Re-verification triggers still have not fired.
- **Design gaps:** unchanged. Identification cost model deferred to Epic 9. US-027/028/029/030 still blocked on design input — **12th consecutive run** flagging this.

### Recommendation to Jeff (unchanged, restated)
The 04-28 / 05-15 prompt still stands: please sanity-check the coding-agent invocation pipeline. The planner-side queue is healthy; nothing downstream will move until a coding agent is invoked against US-071. If you're simply away, no action needed — US-071 will land cleanly when picked up.

### Next Session Recommendation (unchanged)
1. **US-071** (Inventory Widget, HEADLESS) — ready.
2. **US-079** (Playable Inventory Arena, EDITOR) — after US-071 lands.

After Epic 7.5: same fork as 04-21+ — Epic 10 Phase 4 (BP/VFX polish) vs unblocking Epic 6 ranged/two-weapon/armor/shields with Jeff input.

---

## 2026-05-17 Nightly Planning Run

### Status: No change since 05-16. Stall now at 28 days.

- **HEAD:** still `1666193 [US-033] Mark all ACs complete, move story to done` (committed 04-19). 28 calendar days since the last commit.
- **Backlog:** unchanged — US-071 (Inventory Widget, HEADLESS) and US-079 (Playable Inventory Arena, EDITOR), both authored 04-20, last deep-verified 04-26.
- **In-progress:** empty.
- **Working tree:** unchanged from 05-16. `PLAN.md` + `PLANNER_LOG.md` modified-uncommitted; `stories/backlog/` untracked. Will flow into the next coding-agent commit.
- **Design docs:** newest is `agent_rules_v2.md` at 04-11. No modifications since the 04-26 deep-verification pass. Re-verification triggers still have not fired.
- **Design gaps:** unchanged. Identification cost model deferred to Epic 9. US-027/028/029/030 still blocked on design input — **13th consecutive run** flagging this.

### Recommendation to Jeff (unchanged, restated)
The 04-28 / 05-15 / 05-16 prompt still stands: please sanity-check the coding-agent invocation pipeline. The planner-side queue is healthy; nothing downstream will move until a coding agent is invoked against US-071. If you're simply away, no action needed — US-071 will land cleanly when picked up.

### Next Session Recommendation (unchanged)
1. **US-071** (Inventory Widget, HEADLESS) — ready.
2. **US-079** (Playable Inventory Arena, EDITOR) — after US-071 lands.

After Epic 7.5: same fork — Epic 10 Phase 4 (BP/VFX polish) vs unblocking Epic 6 ranged/two-weapon/armor/shields with Jeff input.

---

## 2026-06-08 Nightly Planning Run

### Status: No change since 05-17. Stall now at 50 days (last commit 04-19).

This entry consolidates the 05-17 → 06-08 window (22 days, planner not invoked in between).

- **HEAD:** still `1666193 [US-033] Mark all ACs complete, move story to done` (committed 2026-04-19). **50 calendar days** since the last coding-agent commit.
- **Backlog:** unchanged — **US-071** (Inventory Widget, HEADLESS) and **US-079** (Playable Inventory Arena, EDITOR), both authored 04-20, last deep-verified 04-26.
- **In-progress:** empty.
- **Working tree:** unchanged — `PLAN.md` + `PLANNER_LOG.md` modified-uncommitted; `stories/backlog/` untracked. Will flow into the next coding-agent commit.
- **Design docs:** newest is `agent_rules_v2.md` (Specs: Apr 14, alt docs: Apr 11). No modifications since the 04-26 deep-verification pass. Neither re-verification trigger (commit lands / referenced doc updated) has fired, so the backlog API surfaces are not re-checked this run — nothing has drifted.
- **Design gaps:** unchanged. Identification cost model deferred to Epic 9 (Mage Tower). US-027 (ranged) / US-028 (two-weapon) / US-029 (armor) / US-030 (shields) still blocked on design input — **14th consecutive run** flagging this.

### Assessment
The planner side has nothing left to do that adds value: the queue is healthy, dependency-satisfied, and spec-current. Writing additional stories into a queue that has gone 50 days unconsumed would be churn, not progress. The bottleneck is entirely downstream of the planner — between scoping and coding-agent invocation.

### Recommendation to Jeff (now urgent)
Please sanity-check the **coding-agent invocation pipeline**. The planner queue is healthy and has been for 50 days; nothing downstream will move until a coding agent is invoked against **US-071**. If you've simply been away and the pipeline is fine, no action is needed — US-071 will land cleanly when the next agent picks it up. If the pipeline is broken, that is the single unblock the whole project is waiting on.

Secondary, lower-urgency unblock: provide design input for US-027/028/029/030 (ranged weapons, two-weapon fighting, armor, shields) so Epic 6 can be fully scoped. This has been blocked for 14 runs.

### Next Session Recommendation (unchanged)
1. **US-071** (Inventory Widget, HEADLESS) — ready. 17 tests. Single most valuable next unit of work.
2. **US-079** (Playable Inventory Arena, EDITOR) — after US-071 lands. Delivers the playable Epic 7 proof-point per the Playability-First Rule.

After Epic 7.5: same fork as 04-21+ — Epic 10 Phase 4 (BP/VFX polish: US-070/075/076, scopable today) vs unblocking Epic 6 ranged/two-weapon/armor/shields with Jeff input.

---

## 2026-06-09 Nightly Planning Run

### Status: Stall now 51 days (last commit 04-19). Used this run to extend the unblocked runway.

- **HEAD:** still `1666193 [US-033] ...` (2026-04-19). **51 calendar days** with no coding-agent commit.
- **Backlog before this run:** US-071, US-079 (Epic 7.5) — both still ready, unconsumed since 04-20.
- **In-progress:** empty. **Done:** 50 stories.
- **Working tree:** `PLAN.md` + `PLANNER_LOG.md` modified-uncommitted; `stories/backlog/` untracked (now contains the new US-080/081/082 in addition to US-071/079).

### The dominant issue is unchanged and is NOT a planning problem
The 51-day stall is entirely downstream of the planner. The queue has been healthy, dependency-satisfied, and spec-current the whole time. **Nothing moves until a coding agent is invoked against US-071.** This remains the single most important unblock — see Recommendation to Jeff.

### Why I scoped new stories anyway (this is not queue-churn)
Prior runs (05-17, 06-08) correctly declined to add stories to an unconsumed queue. The difference this run: prior runs treated **all** of Epic 8 as "future / needs design input." On a closer read of `map_style_technical_design_v1.md` and `agentic_map_pipeline_v1.md`, I found that **Epic 8 Phase 1 (the Cell Framework foundation, Milestone 1) is genuinely unblocked, HEADLESS, and highly testable** — it implements the *already-locked* data model and markup framework and does not touch the human-approval gates (gate-type introduction, biome graph, height-tier rules, connection sockets, progression sequence). So this is net-new, dependency-satisfied scoping that extends the runway from 2 stories to 5 — not a re-churn of existing work. If the pipeline unstalls, there is now a clear unblocked path well past Epic 7.5.

### New Stories Created (all HEADLESS, all in `stories/backlog/`)
- **US-080: Cell Framework — Metadata Asset & Core World Types** — `UMordecaiCellMetadata` (full §5.2 schema), enums for biome/cell-type/height-tier/gate-requirement/exit-direction/path-role, `FMordecaiCellExit`, `UMordecaiWorldLibrary` naming/opposite helpers. 10 tests. Pure data + pure functions. First Epic 8 story; no dependencies beyond existing patterns.
- **US-081: Cell Framework — Gameplay Markup Actors & Cell Registry** — 19 `PM_*` markup types, `AMordecaiGameplayMarker` base + `AMordecaiGateMarker`/`AMordecaiSpawnGroupMarker` subclasses, queryable `UMordecaiCellMarkupSubsystem` (gates-by-requirement, cells-missing-type). 11 tests. Depends on US-080 enums.
- **US-082: Cell Framework — Validation Library & Report** — deterministic Critical/High/Medium checks (metadata completeness, exit connectivity, gate mismatch, combat-no-spawn, secret-no-hint, gate-no-marker, shortcut-not-instantiated), `FMordecaiCellValidationReport`, per-cell + cross-cell entry points. 14 tests. Depends on US-080 + US-081.

Strict implementation order: **US-080 → US-081 → US-082**.

### PLAN.md Updates
- Expanded Epic 8 into Phase 1 (Cell Framework Foundation — US-080/081/082, scoped) and Phase 2 (skills + gates — blocked).
- Marked US-034/035 (movement/exploration skills) BLOCKED: skill_sheet_v1.1 rank values are `{x}*rank` placeholders with no decided constants.
- Marked US-036 (world gates) and the Milestone-2 biome slice BLOCKED on human approval per agent_rules_v2 / agentic_map_pipeline_v1 §8.
- Inserted Epic 8 Phase 1 into the priority order at #14 (after Epic 7.5) as the no-blocker pipeline filler.

### Blockers / Decisions Needed
1. **CODING-AGENT PIPELINE (urgent, 51 days):** unchanged from 06-08. Nothing ships until an agent runs against US-071.
2. **Epic 8 movement/exploration skill values** (NEW): `skill_sheet_v1.1.md` specifies effects as `{x} * rank` / `{y} * rank` with no concrete `{x}`/`{y}`. US-034/035 can't be scoped without these constants. Need Jeff to fill in per-skill numbers (climb speed %, swim breath seconds, sneak detection-radius %, perception range/units, lockpick window %, trap arm-time/damage, etc.).
3. **Epic 8 world gates / biome content** (NEW): introducing gate types, the biome graph, height-tier rules, connection-socket standards, or progression sequence requires explicit human approval (Stage A "World Intent"). Agents may build cell content only *after* that approval. US-036 + the Milestone-2 vertical slice are gated on this.
4. **Epic 6 design input** (carried, 15th run): US-027 ranged / US-028 two-weapon / US-029 armor / US-030 shields still under-specified.

### Recommendation to Jeff (still urgent)
Same as 06-08: **please sanity-check the coding-agent invocation pipeline.** The planner queue is now 5 deep (US-071, US-079, US-080, US-081, US-082), all dependency-satisfied and spec-current. If you've simply been away, no action needed — US-071 lands cleanly when the next agent picks it up. If the pipeline is broken, that is the one unblock the whole project waits on. Secondary: provide the Epic 8 skill constants (blocker #2) and Epic 6 design input (blocker #4) when convenient.

### Next Session Recommendation
1. **US-071** (Inventory Widget, HEADLESS) — single most valuable next unit. 17 tests.
2. **US-079** (Playable Inventory Arena, EDITOR) — after US-071. Epic 7.5 playable proof-point.
3. **US-080 → US-081 → US-082** (Cell Framework Foundation, HEADLESS) — unblocked Epic 8 runway, pickable in parallel with / after Epic 7.5. Strict order.

---

## 2026-06-11 Nightly Planning Run

### Status: Stall now 53 days (HEAD `1666193`, 2026-04-19). No coding-agent commit since. Queue healthy and unchanged.

- **HEAD:** still `1666193 [US-033]` (2026-04-19). `git log --since=2026-04-20` is empty — zero commits in 53 calendar days.
- **Backlog (5, all ready):** US-071, US-079 (Epic 7.5); US-080, US-081, US-082 (Epic 8 Phase 1). All dependency-satisfied and spec-current.
- **In-progress:** empty. **Done:** 50 stories.

### No new stories this run — deliberately
The queue is 5 deep, dependency-satisfied, and unconsumed. Every genuinely-unblocked path was already scoped by the 06-09 run (which correctly extended the runway from Epic 7.5 into Epic 8 Phase 1). The remaining unscoped work is all gated:
- **Epic 8 Phase 2** (US-034/035 movement/exploration skills) — blocked on undecided `{x}*rank` skill constants in `skill_sheet_v1.1.md`.
- **Epic 8 gates / biome content** (US-036, Milestone-2 slice) — blocked on human "World Intent" approval per agentic_map_pipeline_v1 §8.
- **Epic 6** (US-027 ranged / US-028 two-weapon / US-029 armor / US-030 shields) — blocked on design input.
- **Epic 9** (Town) — would be premature scoping while 5 stories sit unconsumed and several epics intervene.

Adding to an unconsumed queue is the queue-churn the 05-17 and 06-08 runs correctly rejected. The dominant issue remains downstream of the planner: **nothing ships until a coding agent runs against US-071.**

### Durability fix (new this run)
`stories/backlog/` has been **untracked in git** and PLAN.md / PLANNER_LOG.md modified-uncommitted across several prior runs — the planning work was not durable. Committed PLAN.md, PLANNER_LOG.md, and all 5 backlog stories this run so the queue is safe in git history regardless of when the pipeline resumes.

### Blockers / Decisions Needed (unchanged from 06-09)
1. **CODING-AGENT PIPELINE (urgent, 53 days):** nothing moves until an agent is invoked against US-071. Single most important unblock.
2. **Epic 8 skill constants:** need concrete per-skill numbers for US-034/035.
3. **Epic 8 World Intent approval:** required before US-036 / Milestone-2 biome content.
4. **Epic 6 design input:** ranged/two-weapon/armor/shields under-specified.

### Recommendation to Jeff (still urgent)
**Please sanity-check the coding-agent invocation pipeline.** If you've simply been away, no action needed — US-071 lands cleanly when the next agent picks it up. If the pipeline is broken, that is the one unblock the whole project waits on.

### Next Session Recommendation
1. **US-071** (Inventory Widget, HEADLESS) — single most valuable next unit.
2. **US-079** (Playable Inventory Arena, EDITOR) — after US-071.
3. **US-080 → US-081 → US-082** (Cell Framework Foundation, HEADLESS) — strict order.

---

## 2026-06-12 Nightly Planning Run

### Status: Stall now 54 days (last coding commit `1666193`, 2026-04-19). Queue healthy and unchanged.

- **HEAD:** `a754566 [planning] 2026-06-11 ...` (yesterday's planner commit that made the queue durable in git). Last **coding-agent** commit is still `1666193 [US-033]` (2026-04-19) — `git log --since=2026-04-20` excluding planning commits is empty. **54 calendar days** with no implementation.
- **Backlog (5, all ready):** US-071, US-079 (Epic 7.5); US-080, US-081, US-082 (Epic 8 Phase 1). All dependency-satisfied and spec-current.
- **In-progress:** empty. **Done:** 50 stories. **Working tree:** clean (06-11 run committed PLAN.md, PLANNER_LOG.md, and all 5 backlog files).

### No new stories this run — deliberately (1 day after a full scoping run)
Yesterday's run already extended the runway from 2 to 5 stories and committed the whole queue. Every genuinely-unblocked path is scoped. The remaining unscoped work is all gated and unchanged from 06-09/06-11:
- **Epic 8 Phase 2** (US-034/035) — blocked on undecided `{x}*rank` skill constants in `skill_sheet_v1.1.md`.
- **Epic 8 gates / biome content** (US-036, Milestone-2 slice) — blocked on human "World Intent" approval (agentic_map_pipeline_v1 §8).
- **Epic 6** (US-027 ranged / US-028 two-weapon / US-029 armor / US-030 shields) — blocked on design input (**16th consecutive run** flagging).
- **Epic 9** (Town) — premature to scope while 5 stories sit unconsumed.

Adding to an unconsumed queue is churn, not progress. The dominant issue remains entirely downstream of the planner: **nothing ships until a coding agent runs against US-071.**

### Verification
No re-verification triggered: no coding commit has landed and no referenced design doc changed since the 04-26 deep pass (newest doc `agent_rules_v2.md`, unchanged). Backlog API surfaces have not drifted.

### Blockers / Decisions Needed (unchanged from 06-11)
1. **CODING-AGENT PIPELINE (urgent, 54 days):** nothing moves until an agent is invoked against US-071. Single most important unblock.
2. **Epic 8 skill constants:** concrete per-skill numbers needed for US-034/035.
3. **Epic 8 World Intent approval:** required before US-036 / Milestone-2 biome content.
4. **Epic 6 design input:** ranged/two-weapon/armor/shields under-specified.

### Recommendation to Jeff (still urgent)
**Please sanity-check the coding-agent invocation pipeline.** The planner queue has been healthy for 54 days; if you've simply been away, no action is needed — US-071 lands cleanly when the next agent picks it up. If the pipeline is broken, that is the single unblock the whole project is waiting on.

### Next Session Recommendation
1. **US-071** (Inventory Widget, HEADLESS) — single most valuable next unit. 17 tests.
2. **US-079** (Playable Inventory Arena, EDITOR) — after US-071. Epic 7.5 playable proof-point.
3. **US-080 → US-081 → US-082** (Cell Framework Foundation, HEADLESS) — strict order.

---

## 2026-06-13 Nightly Planning Run

### Status: Stall 55 days. No change. (HEAD = yesterday's planner commit; last coding commit `1666193` [US-033], 2026-04-19.)

- **Verified:** `git log --since=2026-04-20 --invert-grep --grep="[planning]"` is empty — zero implementation commits in 55 days. Working tree clean.
- **Backlog (5, all ready, unchanged):** US-071, US-079 (Epic 7.5); US-080 → US-081 → US-082 (Epic 8 Phase 1). All dependency-satisfied, spec-current.
- **In-progress:** empty. **Done:** 50 stories.

### No new stories — and note on cadence
Queue is unconsumed; adding to it is churn, not progress (consistent with 05-17 / 06-08 / 06-11 / 06-12). Every unblocked path is already scoped. Remaining work is gated and unchanged: Epic 8 Phase 2 skill constants, Epic 8 World Intent approval, Epic 6 design input (now **17th run** flagging), Epic 9 premature.

**Cadence flag:** this is the 5th straight night that produces only a log entry. The nightly planner has nothing left to do until the pipeline resumes. Recommend pausing nightly planner runs until a coding commit lands or Jeff provides one of the gated decisions below — continuing to append "stall +1 day" entries is itself low-value noise.

### Blockers / Decisions Needed (unchanged)
1. **CODING-AGENT PIPELINE (urgent, 55 days):** nothing ships until an agent runs against US-071. The one unblock the whole project waits on.
2. **Epic 8 skill constants** for US-034/035 (`{x}*rank` placeholders).
3. **Epic 8 World Intent approval** before US-036 / Milestone-2 biome content.
4. **Epic 6 design input:** ranged/two-weapon/armor/shields.

### Recommendation to Jeff
**Sanity-check the coding-agent invocation pipeline.** If you've been away, no action needed — US-071 lands cleanly when the next agent picks it up. If it's broken, that's the single unblock. Consider pausing this nightly planner until then.

### Next Session Recommendation
1. **US-071** (Inventory Widget, HEADLESS) — single most valuable next unit.
2. **US-079** (Playable Inventory Arena, EDITOR) — after US-071.
3. **US-080 → US-081 → US-082** (Cell Framework Foundation, HEADLESS) — strict order.

---

## 2026-06-14 Nightly Planning Run

### Status: Stall 56 days. No change. (HEAD = `d2f3a8f`, yesterday's planner commit; last coding commit `1666193` [US-033], 2026-04-19.)

- **Verified:** `git log --since=2026-04-20 --invert-grep --grep="[planning]"` is empty — zero implementation commits in 56 days. Working tree clean.
- **Backlog (5, all ready, unchanged):** US-071, US-079 (Epic 7.5); US-080 → US-081 → US-082 (Epic 8 Phase 1). All have correct Execution Mode tags (4 HEADLESS, 1 EDITOR), are dependency-satisfied, and remain spec-current — no referenced design doc has changed since the 04-26 deep-verification pass.
- **In-progress:** empty. **Done:** 50 stories.

### No new stories — pause recommendation now stands a 2nd night
Consistent with 05-17 / 06-08 / 06-11 / 06-12 / 06-13: adding to an unconsumed 5-deep queue is churn, not progress. Every unblocked path is already scoped. Remaining work is gated and unchanged: Epic 8 Phase 2 skill constants, Epic 8 World Intent approval, Epic 6 design input (**18th run** flagging), Epic 9 premature.

This is the **6th straight night producing only a log entry.** The 06-13 run recommended pausing nightly planner runs until a coding commit lands or Jeff resolves a gated decision; that recommendation is reaffirmed. Continuing to append "stall +1 day" entries is itself low-value noise.

### Blockers / Decisions Needed (unchanged)
1. **CODING-AGENT PIPELINE (urgent, 56 days):** nothing ships until an agent runs against US-071. The one unblock the whole project waits on.
2. **Epic 8 skill constants** for US-034/035 (`{x}*rank` placeholders in `skill_sheet_v1.1.md`).
3. **Epic 8 World Intent approval** before US-036 / Milestone-2 biome content.
4. **Epic 6 design input:** ranged / two-weapon / armor / shields.

### Recommendation to Jeff
**Sanity-check the coding-agent invocation pipeline, and consider pausing this nightly planner until it resumes.** If you've been away, no action needed — US-071 lands cleanly when the next agent picks it up. If it's broken, that's the single unblock.

### Next Session Recommendation
1. **US-071** (Inventory Widget, HEADLESS) — single most valuable next unit.
2. **US-079** (Playable Inventory Arena, EDITOR) — after US-071.
3. **US-080 → US-081 → US-082** (Cell Framework Foundation, HEADLESS) — strict order.

---

## 2026-06-15 Nightly Planning Run

### Status: Stall 57 days. No change. (7th straight log-only night.)
HEAD = `0e5ceed` (yesterday's planner commit); last coding commit `1666193` [US-033], 2026-04-19. Verified: `git log --since=2026-04-20 --invert-grep --grep="[planning]"` empty; working tree clean; in-progress empty; done 50; backlog 5 (US-071, US-079, US-080->081->082) — all ready, dependency-satisfied, spec-current. No in-session cron exists to pause (nightly trigger is external).

### No new stories
Queue is unconsumed; scoping more is churn. Every unblocked path is already scoped. Gated work unchanged: Epic 8 skill constants, Epic 8 World Intent approval, Epic 6 design input, Epic 9 premature.

### The one ask (unchanged, 57 days)
**Sanity-check the coding-agent invocation pipeline.** Nothing ships until an agent runs against **US-071**. If Jeff has been away, no action needed — US-071 lands cleanly when the next agent picks it up. If the pipeline is broken, that is the single unblock for the whole project. Keeping entries short from here; the situation, not the wording, is what needs to change.

### Next Session Recommendation
US-071 -> US-079 -> US-080 -> US-081 -> US-082 (unchanged order).
