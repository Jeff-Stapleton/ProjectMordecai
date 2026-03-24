# US-013: Status Effect Framework (GAS-Based)

## Execution Mode
**Mode:** HEADLESS

## Overview
Build the GAS infrastructure that all 16 launch status effects share. This story creates the base classes, gameplay tags, application/removal patterns, duration management, periodic tick support, stacking interfaces, cleanse/immunity hooks, and a query API. No individual status effects are implemented here — this is pure framework.

## References
- Design doc: `status_effects_v1.md` — raw status effect catalog
- Design doc: `status_effects_launch_set_v1.md` — curated 16-status launch set with identity, mechanics, counterplay, sources, cleanses
- Design doc: `game_design_v2.md` — Section 10 (Status Effects principle: "statuses must have distinct gameplay identity")
- Design doc: `combat_system_v1.md` — Damage Formula baseline (`StatusModifier` slot)
- Design doc: `damage_types_v1.md` — Damage type → status relationships (Fire→Burning, Frost→Frostbitten, Lightning→Shocked, Poison→Poisoned, Corrosive→Corroded)
- Agent rules: `agent_rules_v2.md` — GAS for all status effects; no parallel systems outside GAS; `TODO(DECISION)` for stacking policy (Open Item #2)

---

## Acceptance Criteria
- [x] AC-013.1: Native gameplay tags registered for all 16 launch statuses: `Mordecai.Status.Burning`, `Mordecai.Status.Bleeding`, `Mordecai.Status.Poisoned`, `Mordecai.Status.Frostbitten`, `Mordecai.Status.Shocked`, `Mordecai.Status.Weakened`, `Mordecai.Status.Brittle`, `Mordecai.Status.Silenced`, `Mordecai.Status.Rooted`, `Mordecai.Status.Blinded`, `Mordecai.Status.Fear`, `Mordecai.Status.Cursed`, `Mordecai.Status.Exposed`, `Mordecai.Status.Corroded`, `Mordecai.Status.Drenched`, `Mordecai.Status.Focused`
- [x] AC-013.2: Status effect category tags registered: `Mordecai.Status.Category.DoT`, `Mordecai.Status.Category.Debuff`, `Mordecai.Status.Category.Control`, `Mordecai.Status.Category.Buff`, `Mordecai.Status.Category.Environmental`
- [x] AC-013.3: `UMordecaiStatusEffectGameplayEffect` (UGameplayEffect subclass) exists as a base class for all status effect GEs, with configurable `DurationPolicy` (Instant, HasDuration, Infinite), `Period` for tick effects, and a `StatusTag` property that is applied/removed with the GE
- [x] AC-013.4: `UMordecaiStatusEffectComponent` (UActorComponent) exists on actors that can receive status effects. Provides: `ApplyStatusEffect(TSubclassOf<UGameplayEffect>, AActor* Instigator, float Level)`, `RemoveStatusEffect(FGameplayTag StatusTag)`, `RemoveAllStatusEffects()`, `HasStatusEffect(FGameplayTag StatusTag) → bool`, `GetActiveStatusTags() → FGameplayTagContainer`
- [x] AC-013.5: Status effects are applied by granting a GE through the target's ASC. The GE adds the status gameplay tag via `InheritableOwnedTagsContainer`. Removing the GE removes the tag.
- [x] AC-013.6: Duration-based status effects auto-remove after their duration expires (standard GAS GE duration behavior)
- [x] AC-013.7: Periodic (tick) status effects execute their periodic effect at the configured `Period` interval (standard GAS periodic GE behavior) — framework only, no specific tick logic
- [x] AC-013.8: Immunity interface: if the target's ASC has a tag matching `Mordecai.Immunity.<StatusName>` (e.g., `Mordecai.Immunity.Burning`), the status effect GE application is blocked. Implemented via GE `ApplicationTagRequirements` (target must NOT have the immunity tag).
- [x] AC-013.9: Cleanse interface: `RemoveStatusEffectsByCategory(FGameplayTag CategoryTag)` removes all active status GEs whose `StatusTag` is a child of the given category. E.g., cleansing `Mordecai.Status.Category.DoT` removes Burning, Bleeding, Poisoned.
- [x] AC-013.10: Stacking behavior: `TODO(DECISION)` — per agent_rules_v2.md Open Item #2. For now, default to **duration refresh** on re-application (same status re-applied refreshes duration, does not stack magnitude). Expose a `StackingPolicy` enum (`Refresh`, `StackCount`, `StackDuration`, `Independent`) on the base GE class as a configurable field, defaulting to `Refresh`. Mark with `TODO(DECISION)` comment.
- [x] AC-013.11: Immunity tags registered for all 16 statuses: `Mordecai.Immunity.Burning`, `Mordecai.Immunity.Bleeding`, ..., `Mordecai.Immunity.Focused`
- [x] AC-013.12: `EMordecaiStatusEffectCategory` enum exists with values: `DoT`, `Debuff`, `Control`, `Buff`, `Environmental`
- [x] AC-013.13: Status-to-category mapping function exists: `GetStatusEffectCategory(FGameplayTag StatusTag) → EMordecaiStatusEffectCategory`

## Technical Notes
- Place framework code in `Source/LyraGame/Mordecai/StatusEffects/`
- Place gameplay tags in existing `MordecaiGameplayTags.h/.cpp` (extend from US-002's tag registration)
- `UMordecaiStatusEffectGameplayEffect`: subclass `UGameplayEffect`. Override default values. The key pattern: each concrete status GE inherits from this base, sets its `StatusTag`, duration, period, and specific modifiers.
- `UMordecaiStatusEffectComponent`: lightweight component that wraps ASC status queries. This is a convenience layer — actual status state lives in the ASC's active GE list. The component delegates to ASC for all operations.
- Tag hierarchy: `Mordecai.Status` is the root. Individual statuses are direct children. Category tags are separate under `Mordecai.Status.Category`.
- Immunity tags under `Mordecai.Immunity` — kept flat, not nested under Status, to avoid tag hierarchy confusion.
- Stacking: use `TODO(DECISION)` extensively. The base GE sets `StackingType`, `StackLimitCount`, `StackDurationRefreshPolicy` etc. per GAS stacking API. Default: `AggregateBySource`, limit 1, refresh duration.
- Do NOT implement any specific status effect logic (DoT ticks, speed reduction, etc.) — that's US-014 through US-018.
- Do NOT create GE Blueprints or DataAssets — this is C++ only.

## Tests Required
- [x] `Mordecai.StatusEffect.AllStatusTagsRegistered` — verify all 16 `Mordecai.Status.*` tags exist in the gameplay tag manager (AC-013.1)
- [x] `Mordecai.StatusEffect.CategoryTagsRegistered` — verify all 5 `Mordecai.Status.Category.*` tags exist (AC-013.2)
- [x] `Mordecai.StatusEffect.ImmunityTagsRegistered` — verify all 16 `Mordecai.Immunity.*` tags exist (AC-013.11)
- [x] `Mordecai.StatusEffect.BaseGEClassExists` — verify `UMordecaiStatusEffectGameplayEffect` can be instantiated and has expected default properties (AC-013.3)
- [x] `Mordecai.StatusEffect.ComponentAppliesStatusTag` — create actor with ASC + component, apply a test status GE, verify the status gameplay tag is active on the ASC (AC-013.4, AC-013.5)
- [x] `Mordecai.StatusEffect.ComponentRemovesStatusByTag` — apply status, call `RemoveStatusEffect`, verify tag removed (AC-013.4)
- [x] `Mordecai.StatusEffect.ComponentHasStatusQuery` — apply status, verify `HasStatusEffect` returns true; remove, verify false (AC-013.4)
- [x] `Mordecai.StatusEffect.ComponentGetActiveStatusTags` — apply two different statuses, verify `GetActiveStatusTags` returns both (AC-013.4)
- [x] `Mordecai.StatusEffect.DurationAutoRemoves` — apply a status GE with 2s duration, verify tag present immediately, absent after duration (AC-013.6)
- [x] `Mordecai.StatusEffect.ImmunityBlocksApplication` — add immunity tag to target ASC, attempt to apply corresponding status GE, verify it does not apply (AC-013.8)
- [x] `Mordecai.StatusEffect.CleanseByCategory` — apply Burning + Poisoned (DoT category), call `RemoveStatusEffectsByCategory(DoT)`, verify both removed while a non-DoT status remains (AC-013.9)
- [x] `Mordecai.StatusEffect.DefaultStackingRefreshesDuration` — apply status GE, wait 1s, re-apply, verify duration resets (not stacked) (AC-013.10)
- [x] `Mordecai.StatusEffect.CategoryEnumHasAllValues` — verify `EMordecaiStatusEffectCategory` has DoT, Debuff, Control, Buff, Environmental (AC-013.12)
- [x] `Mordecai.StatusEffect.CategoryMappingCorrect` — verify `GetStatusEffectCategory` returns correct category for each of the 16 statuses (AC-013.13)

---

## Definition of Done
- [x] All tests written and failing (red phase confirmed)
- [x] All implementation complete
- [x] All tests passing (green)
- [x] Project compiles with zero errors
- [x] Code committed and pushed with `[US-013]` prefix
