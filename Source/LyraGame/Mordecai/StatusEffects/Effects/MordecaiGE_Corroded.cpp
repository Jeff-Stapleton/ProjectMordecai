// Project Mordecai — Corroded Status Effect GE (US-016)

#include "Mordecai/StatusEffects/Effects/MordecaiGE_Corroded.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"

UMordecaiGE_Corroded::UMordecaiGE_Corroded()
{
	StatusTag = MordecaiGameplayTags::Status_Corroded;
	DurationMagnitude = FScalableFloat(CorrodedDurationSec);

	// Block stability reduction: Additive -0.30 on 1.0 base -> 0.70 (30% less block stability)
	FGameplayModifierInfo BlockStabilityMod;
	BlockStabilityMod.Attribute = UMordecaiAttributeSet::GetBlockStabilityMultiplierAttribute();
	BlockStabilityMod.ModifierOp = EGameplayModOp::Additive;
	BlockStabilityMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-CorrodedBlockStabilityReductionPct));
	Modifiers.Add(BlockStabilityMod);

	// Armor efficiency reduction: Additive -0.25 on 1.0 base -> 0.75 (25% less armor efficiency)
	FGameplayModifierInfo ArmorMod;
	ArmorMod.Attribute = UMordecaiAttributeSet::GetArmorEfficiencyMultiplierAttribute();
	ArmorMod.ModifierOp = EGameplayModOp::Additive;
	ArmorMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-CorrodedArmorReductionPct));
	Modifiers.Add(ArmorMod);

	// AC-016.18: While Corroded, Sharpen/Repair buffs are disabled.
	// Those GEs don't exist yet — when created in Epic 6, they should add
	// Mordecai.Status.Corroded to their ApplicationTagRequirements.IgnoreTags
	// (i.e., they cannot be applied while Corroded is active).

	// InitializeStatusEffect() is called automatically from PostInitProperties
}
