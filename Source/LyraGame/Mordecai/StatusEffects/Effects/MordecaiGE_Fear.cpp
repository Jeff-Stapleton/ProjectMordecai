// Project Mordecai — Fear Status Effect GE (US-059)

#include "Mordecai/StatusEffects/Effects/MordecaiGE_Fear.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"

UMordecaiGE_Fear::UMordecaiGE_Fear()
{
	StatusTag = MordecaiGameplayTags::Status_Fear;
	DurationMagnitude = FScalableFloat(FearDurationSec);

	// Stamina tier penalty amplification: Additive +0.50 on 1.0 base -> 1.50 (50% worse tier penalties)
	FGameplayModifierInfo StaminaPenaltyMod;
	StaminaPenaltyMod.Attribute = UMordecaiAttributeSet::GetStaminaTierPenaltyMultiplierAttribute();
	StaminaPenaltyMod.ModifierOp = EGameplayModOp::Additive;
	StaminaPenaltyMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(FearStaminaTierPenaltyMultiplier - 1.0f));
	Modifiers.Add(StaminaPenaltyMod);

	// Guard stability reduction: Additive -0.25 on 1.0 base -> 0.75 (25% less guard stability)
	FGameplayModifierInfo BlockStabilityMod;
	BlockStabilityMod.Attribute = UMordecaiAttributeSet::GetBlockStabilityMultiplierAttribute();
	BlockStabilityMod.ModifierOp = EGameplayModOp::Additive;
	BlockStabilityMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-FearGuardStabilityReductionPct));
	Modifiers.Add(BlockStabilityMod);

	// Aim drift: Additive -0.15 on 1.0 base -> 0.85 (15% less ranged accuracy)
	FGameplayModifierInfo AimDriftMod;
	AimDriftMod.Attribute = UMordecaiAttributeSet::GetRangedAccuracyMultiplierAttribute();
	AimDriftMod.ModifierOp = EGameplayModOp::Additive;
	AimDriftMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-FearAimDriftPct));
	Modifiers.Add(AimDriftMod);

	// AC-059.11: Resolve mechanic — PerfectBlock or PerfectParry while Fear is active
	// grants Mordecai.Status.Resolved which cancels Fear immediately. Interface only —
	// wiring deferred until block/parry system integration listens for Fear tag.

	// InitializeStatusEffect() is called automatically from PostInitProperties
}
