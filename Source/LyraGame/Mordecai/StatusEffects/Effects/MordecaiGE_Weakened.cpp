// Project Mordecai — Weakened Status Effect GE (US-016)

#include "Mordecai/StatusEffects/Effects/MordecaiGE_Weakened.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"

UMordecaiGE_Weakened::UMordecaiGE_Weakened()
{
	StatusTag = MordecaiGameplayTags::Status_Weakened;
	DurationMagnitude = FScalableFloat(WeakenedDurationSec);

	// Outgoing posture damage reduction: Additive -0.40 on 1.0 base -> 0.60 (40% reduction)
	FGameplayModifierInfo PostureDamageMod;
	PostureDamageMod.Attribute = UMordecaiAttributeSet::GetOutgoingPostureDamageMultiplierAttribute();
	PostureDamageMod.ModifierOp = EGameplayModOp::Additive;
	PostureDamageMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-WeakenedPostureDamageReductionPct));
	Modifiers.Add(PostureDamageMod);

	// InitializeStatusEffect() is called automatically from PostInitProperties
}
