// Project Mordecai — Exposed Status Effect GE (US-016)

#include "Mordecai/StatusEffects/Effects/MordecaiGE_Exposed.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"

UMordecaiGE_Exposed::UMordecaiGE_Exposed()
{
	StatusTag = MordecaiGameplayTags::Status_Exposed;
	DurationMagnitude = FScalableFloat(ExposedDurationSec);

	// Bonus incoming damage: Additive +0.30 on 1.0 base -> 1.30 (30% more damage on next hit)
	FGameplayModifierInfo DamageMod;
	DamageMod.Attribute = UMordecaiAttributeSet::GetIncomingDamageMultiplierAttribute();
	DamageMod.ModifierOp = EGameplayModOp::Additive;
	DamageMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(ExposedBonusDamagePct));
	Modifiers.Add(DamageMod);

	// Exposed is consumed on first hit — handled by MordecaiStatusEffectComponent::NotifyDamageTaken()
	// InitializeStatusEffect() is called automatically from PostInitProperties
}
