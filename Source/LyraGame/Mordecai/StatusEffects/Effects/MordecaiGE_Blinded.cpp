// Project Mordecai — Blinded Status Effect GE (US-059)

#include "Mordecai/StatusEffects/Effects/MordecaiGE_Blinded.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"

UMordecaiGE_Blinded::UMordecaiGE_Blinded()
{
	StatusTag = MordecaiGameplayTags::Status_Blinded;
	DurationMagnitude = FScalableFloat(BlindedDurationSec);

	// Ranged accuracy reduction: Additive -0.40 on 1.0 base -> 0.60 (40% less accuracy)
	FGameplayModifierInfo AccuracyMod;
	AccuracyMod.Attribute = UMordecaiAttributeSet::GetRangedAccuracyMultiplierAttribute();
	AccuracyMod.ModifierOp = EGameplayModOp::Additive;
	AccuracyMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-BlindedAccuracyReductionPct));
	Modifiers.Add(AccuracyMod);

	// Aim assist reduction: Additive -0.50 on 1.0 base -> 0.50 (50% less aim assist)
	FGameplayModifierInfo AimAssistMod;
	AimAssistMod.Attribute = UMordecaiAttributeSet::GetAimAssistMultiplierAttribute();
	AimAssistMod.ModifierOp = EGameplayModOp::Additive;
	AimAssistMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-BlindedAimAssistReductionPct));
	Modifiers.Add(AimAssistMod);

	// Stealth detection reduction: Additive -0.60 on 1.0 base -> 0.40 (60% less detection)
	FGameplayModifierInfo DetectionMod;
	DetectionMod.Attribute = UMordecaiAttributeSet::GetStealthDetectionMultiplierAttribute();
	DetectionMod.ModifierOp = EGameplayModOp::Additive;
	DetectionMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-BlindedDetectionReductionPct));
	Modifiers.Add(DetectionMod);

	// AC-059.5: Perfect dodge clarity window — interface only.
	// When PerfectDodge triggers while Blinded, temporarily suppress Blinded modifiers
	// for BlindedClarityWindowMs. Wiring deferred until dodge system integration.

	// InitializeStatusEffect() is called automatically from PostInitProperties
}
