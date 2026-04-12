// Project Mordecai — Drenched Status Effect GE (US-018)

#include "Mordecai/StatusEffects/Effects/MordecaiGE_Drenched.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"

UMordecaiGE_Drenched::UMordecaiGE_Drenched()
{
	StatusTag = MordecaiGameplayTags::Status_Drenched;
	DurationMagnitude = FScalableFloat(DrenchedDurationSec);

	// AC-018.2: Reduce fire damage taken by DrenchedFireDamageReductionPct (25%)
	// Additive -0.25 on base 1.0 → 0.75 (25% fire damage reduction)
	FGameplayModifierInfo FireDamageMod;
	FireDamageMod.Attribute = UMordecaiAttributeSet::GetFireDamageReceivedMultiplierAttribute();
	FireDamageMod.ModifierOp = EGameplayModOp::Additive;
	FireDamageMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-DrenchedFireDamageReductionPct));
	Modifiers.Add(FireDamageMod);

	// InitializeStatusEffect() is called automatically from PostInitProperties
}

float UMordecaiGE_Drenched::GetShockedChanceMultiplier(const UAbilitySystemComponent* TargetASC)
{
	if (!TargetASC)
	{
		return 1.0f;
	}

	if (TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_Drenched))
	{
		return GetDefault<UMordecaiGE_Drenched>()->DrenchedShockedMultiplier;
	}

	return 1.0f;
}

int32 UMordecaiGE_Drenched::GetFrostBonusStacks(const UAbilitySystemComponent* TargetASC)
{
	if (!TargetASC)
	{
		return 0;
	}

	if (TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_Drenched))
	{
		return GetDefault<UMordecaiGE_Drenched>()->DrenchedFrostBonusStacks;
	}

	return 0;
}
