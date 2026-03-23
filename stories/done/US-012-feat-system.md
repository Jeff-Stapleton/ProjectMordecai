# US-012: Feat System (Achievement-Based Unlocks)

## Overview
Implement the feat system framework — feats are earned via gameplay achievements (not picked from a list). Feats have rarity tiers (Common/Rare/Legendary) with escalating power and drawbacks. This story builds the tracking, evaluation, and GAS integration infrastructure. Specific feat definitions will be authored in later stories as combat and progression systems mature.

Per game_design_v2.md pillar #3: "Feats are earned, not picked. Unlocked via gameplay achievements. Tiered (Common/Rare/Legendary). Strong feats can include drawbacks (DOS2-like)."

## Execution Mode
**Mode:** HEADLESS

## References
- **Primary:** `game_design_v2.md` §2 (Pillar #3: feats), §5 (Progression — feats section)
- **Agent rules:** `agent_rules_v2.md` §6 (GAS for abilities/attributes/effects/tags)

---

## Acceptance Criteria

### Feat Data Model
- [x] AC-012.1: `UMordecaiFeatDataAsset` exists as a `UPrimaryDataAsset` with fields: `FeatName` (FName), `DisplayName` (FText), `Description` (FText), `Tier` (enum: Common/Rare/Legendary), `GrantedEffects` (TArray<TSubclassOf<UGameplayEffect>>), `GrantedAbilities` (TArray<TSubclassOf<UGameplayAbility>>), `DrawbackEffects` (TArray<TSubclassOf<UGameplayEffect>>), `FeatTag` (FGameplayTag)
- [x] AC-012.2: `EMordecaiFeatTier` enum defines three tiers: `Common`, `Rare`, `Legendary`
- [x] AC-012.3: Feat data assets are queryable at runtime — given a feat ID, retrieve its tier, effects, and description

### Feat Condition System
- [x] AC-012.4: `FMordecaiFeatCondition` struct defines an unlock condition with: `ConditionType` (enum: `StatThreshold`, `EventCount`, `TagPresent`, `SkillRank`), condition-specific parameters (tag, threshold value, comparison operator)
- [x] AC-012.5: `UMordecaiFeatDataAsset` includes a `TArray<FMordecaiFeatCondition>` — ALL conditions must be met for the feat to unlock (AND logic)
- [x] AC-012.6: `StatThreshold` condition evaluates true when a tracked gameplay stat (e.g., "EnemiesBurnedSimultaneously") meets or exceeds a threshold
- [x] AC-012.7: `EventCount` condition evaluates true when a tracked cumulative event count (e.g., "TotalPerfectParries") meets or exceeds a threshold
- [x] AC-012.8: `TagPresent` condition evaluates true when the character currently has a specified gameplay tag
- [x] AC-012.9: `SkillRank` condition evaluates true when a specified skill rank meets or exceeds a threshold

### Feat Tracking Component
- [x] AC-012.10: `UMordecaiFeatComponent` (ActorComponent on PlayerState) maintains: a set of unlocked feats (TSet<FName>), a map of tracked gameplay stats (TMap<FName, int32>)
- [x] AC-012.11: `IncrementFeatStat(FName StatName, int32 Amount)` updates a tracked stat and triggers condition evaluation for all feats that reference it
- [x] AC-012.12: When all conditions for a feat are met, the feat auto-unlocks: its `GrantedEffects`, `GrantedAbilities`, and `DrawbackEffects` are applied to the character's ASC
- [x] AC-012.13: `HasFeat(FName FeatName)` returns whether a feat has been unlocked
- [x] AC-012.14: Unlocked feats are replicated to clients
- [x] AC-012.15: A feat can only be unlocked once — duplicate unlock attempts are no-ops

### Feat Tier Behavior
- [x] AC-012.16: Common feats apply `GrantedEffects` only (no `DrawbackEffects`)
- [x] AC-012.17: Rare feats apply both `GrantedEffects` and `DrawbackEffects`
- [x] AC-012.18: Legendary feats apply both `GrantedEffects` and `DrawbackEffects` — drawback effects are non-optional and applied automatically

### Gameplay Tag Integration
- [x] AC-012.19: Each unlocked feat grants its `FeatTag` (pattern `Mordecai.Feat.<FeatName>`) to the character — other systems can query whether a feat is active via gameplay tags
- [x] AC-012.20: A `Mordecai.Event.FeatUnlocked` gameplay event fires when a feat is earned, with the feat name as payload

### Sample Feat Validation
- [x] AC-012.21: A sample Common feat ("Pyromaniac" — unlock condition: burn 6 enemies simultaneously) can be defined in code, its condition tracked via `IncrementFeatStat`, and its effect applied when the threshold is met

## Technical Notes

### Architecture
- **`UMordecaiFeatDataAsset`**: Extends `UPrimaryDataAsset`. One asset per feat. Contains all static data (tier, conditions, rewards, drawbacks). References GEs for both positive and negative effects.
- **`UMordecaiFeatComponent`**: `UActorComponent` attached to `AMordecaiPlayerState`. Manages mutable state: unlocked feats, tracked stats, pending condition evaluations. Provides public API.
- **`FMordecaiFeatCondition`**: Lightweight struct that can be evaluated against current character state. The component evaluates all conditions for all registered feats when a relevant stat changes. Since feat count is small (<100), brute-force evaluation on stat change is acceptable.
- **Condition evaluation flow**: `IncrementFeatStat("EnemiesBurnedSimultaneously", 6)` → component iterates registered feats → finds "Pyromaniac" has a `StatThreshold` condition on "EnemiesBurnedSimultaneously" ≥ 6 → all conditions met → feat unlocks → GE/abilities applied to ASC.
- **Drawback modeling**: Drawbacks are standard GEs with negative modifiers (e.g., a Legendary feat might grant +50% fire damage but apply a GE that reduces frost resistance by 25%). The tier enum is informational — the actual power/drawback balance is in the GE data, not enforced by code.
- **Integration with US-011**: The `SkillRank` condition type queries `UMordecaiSkillComponent::GetSkillRank()`. This creates a dependency on US-011 for that condition type. The component should gracefully handle the case where the skill component doesn't exist (condition evaluates false).

### Feat stat tracking vs GAS attributes
Feat stats (e.g., "EnemiesBurnedSimultaneously", "TotalPerfectParries") are NOT GAS attributes. They're lightweight event counters on the feat component. The actual gameplay systems (hit detection, status effects, etc.) call `IncrementFeatStat` at the appropriate moments. This keeps feat tracking decoupled from the attribute set.

### What's NOT in scope
- **Defining the full feat list**: Only a sample feat ("Pyromaniac") is implemented to validate the pipeline. The full feat list requires design input.
- **UI for feat display**: Epic 10 (UI & HUD).
- **Feat removal/respec**: Not in current design. If feats are permanent, the system is simpler.
- **Feat stat persistence**: Stat counters need to survive sessions via PlayFab. The component provides the data; persistence wiring comes with the PlayFab integration.

### TODO(DECISION): Complete feat list
The design mentions feats are earned via "achievements/patterns" with the single example "burn 6 enemies at once → Pyromaniac". A full feat list with conditions, effects, and drawbacks needs design authoring. This story provides the framework; actual feat content is deferred.

### TODO(DECISION): Feat stat reset behavior
Should feat stats reset per-session, per-expedition, or be cumulative lifetime stats? Example: if "burn 6 enemies simultaneously" is a per-combat-encounter stat, it resets between fights. If cumulative, it tracks the max-ever-achieved value. Default: stats are persistent peak values (max watermark) for threshold-type conditions, and cumulative for count-type conditions.

## Tests Required
- [x] `Mordecai.Feats.DataAssetHasRequiredFields` — verifies AC-012.1
- [x] `Mordecai.Feats.TierEnumMatchesDesign` — verifies AC-012.2
- [x] `Mordecai.Feats.DataAssetQueryable` — verifies AC-012.3
- [x] `Mordecai.Feats.ConditionStructDefinesTypes` — verifies AC-012.4
- [x] `Mordecai.Feats.AllConditionsMustBeMet` — verifies AC-012.5
- [x] `Mordecai.Feats.StatThresholdConditionEvaluates` — verifies AC-012.6
- [x] `Mordecai.Feats.EventCountConditionEvaluates` — verifies AC-012.7
- [x] `Mordecai.Feats.TagPresentConditionEvaluates` — verifies AC-012.8
- [x] `Mordecai.Feats.SkillRankConditionEvaluates` — verifies AC-012.9
- [x] `Mordecai.Feats.ComponentTracksUnlockedFeats` — verifies AC-012.10
- [x] `Mordecai.Feats.IncrementStatTriggersEvaluation` — verifies AC-012.11
- [x] `Mordecai.Feats.AutoUnlockAppliesEffects` — verifies AC-012.12
- [x] `Mordecai.Feats.HasFeatQueryWorks` — verifies AC-012.13
- [x] `Mordecai.Feats.UnlockedFeatsReplicated` — verifies AC-012.14
- [x] `Mordecai.Feats.DuplicateUnlockIsNoOp` — verifies AC-012.15
- [x] `Mordecai.Feats.CommonFeatHasNoDrawback` — verifies AC-012.16
- [x] `Mordecai.Feats.RareFeatAppliesDrawback` — verifies AC-012.17
- [x] `Mordecai.Feats.LegendaryFeatAppliesDrawback` — verifies AC-012.18
- [x] `Mordecai.Feats.FeatTagGrantedOnUnlock` — verifies AC-012.19
- [x] `Mordecai.Feats.FeatUnlockedEventFires` — verifies AC-012.20
- [x] `Mordecai.Feats.PyromaniacSamplePipeline` — verifies AC-012.21

---

## Definition of Done
- [x] All tests written and failing (red phase confirmed)
- [x] All implementation complete
- [x] All tests passing (green)
- [x] Project compiles with zero errors
- [x] Code committed and pushed with `[US-012]` prefix
