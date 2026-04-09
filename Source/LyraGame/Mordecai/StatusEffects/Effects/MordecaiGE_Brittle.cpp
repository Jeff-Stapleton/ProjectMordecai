// Project Mordecai — Brittle Status Effect GE (US-016)

#include "Mordecai/StatusEffects/Effects/MordecaiGE_Brittle.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"

UMordecaiGE_Brittle::UMordecaiGE_Brittle()
{
	StatusTag = MordecaiGameplayTags::Status_Brittle;
	DurationMagnitude = FScalableFloat(BrittleDurationSec);

	// Incoming posture damage increase: Additive +0.35 on 1.0 base -> 1.35 (35% more posture damage taken)
	FGameplayModifierInfo PostureDamageMod;
	PostureDamageMod.Attribute = UMordecaiAttributeSet::GetIncomingPostureDamageMultiplierAttribute();
	PostureDamageMod.ModifierOp = EGameplayModOp::Additive;
	PostureDamageMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(BrittlePostureDamageIncreasePct));
	Modifiers.Add(PostureDamageMod);

	// MaxPosture reduction: Multiply by (1.0 - reduction) = 0.80 (20% lower posture threshold)
	FGameplayModifierInfo MaxPostureMod;
	MaxPostureMod.Attribute = UMordecaiAttributeSet::GetMaxPostureAttribute();
	MaxPostureMod.ModifierOp = EGameplayModOp::Multiplicitive;
	MaxPostureMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.0f - BrittleMaxPostureReductionPct));
	Modifiers.Add(MaxPostureMod);

	// InitializeStatusEffect() is called automatically from PostInitProperties
}
