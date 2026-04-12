// Project Mordecai — Focused Status Effect GE (US-018)

#include "Mordecai/StatusEffects/Effects/MordecaiGE_Focused.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"

UMordecaiGE_Focused::UMordecaiGE_Focused()
{
	StatusTag = MordecaiGameplayTags::Status_Focused;

	// AC-018.8: Infinite duration — stays until broken by hit
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	// AC-018.10: Stamina tier penalties forgiven — set multiplier to 0
	// StaminaTierPenaltyMultiplier base is 1.0; additive -1.0 → 0.0
	FGameplayModifierInfo StaminaTierMod;
	StaminaTierMod.Attribute = UMordecaiAttributeSet::GetStaminaTierPenaltyMultiplierAttribute();
	StaminaTierMod.ModifierOp = EGameplayModOp::Additive;
	StaminaTierMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-1.0f));
	Modifiers.Add(StaminaTierMod);

	// AC-018.11: Outgoing posture damage bonus (20%)
	FGameplayModifierInfo PostureDamageMod;
	PostureDamageMod.Attribute = UMordecaiAttributeSet::GetOutgoingPostureDamageMultiplierAttribute();
	PostureDamageMod.ModifierOp = EGameplayModOp::Additive;
	PostureDamageMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(FocusedPostureDamageBonus));
	Modifiers.Add(PostureDamageMod);

	// InitializeStatusEffect() is called automatically from PostInitProperties
}
