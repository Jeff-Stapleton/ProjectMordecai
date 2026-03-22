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
