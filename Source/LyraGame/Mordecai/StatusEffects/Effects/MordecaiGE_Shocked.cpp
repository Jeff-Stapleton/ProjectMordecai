// Project Mordecai — Shocked Status Effect GE (US-015)

#include "Mordecai/StatusEffects/Effects/MordecaiGE_Shocked.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_MicroStunned.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"

// ---------------------------------------------------------------------------
// UMordecaiShockedBlockCostMMC
// ---------------------------------------------------------------------------

float UMordecaiShockedBlockCostMMC::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const int32 StackCount = Spec.GetStackCount();
	const UMordecaiGE_Shocked* ShockedGE = Cast<UMordecaiGE_Shocked>(Spec.Def);
	const float PerStack = ShockedGE ? ShockedGE->ShockedBlockStaminaCostMultiplierPerStack : 0.20f;
	return PerStack * StackCount;
}

// ---------------------------------------------------------------------------
// UMordecaiGE_Shocked
// ---------------------------------------------------------------------------

UMordecaiGE_Shocked::UMordecaiGE_Shocked()
{
	StatusTag = MordecaiGameplayTags::Status_Shocked;
	DurationMagnitude = FScalableFloat(ShockedDurationSec);

	// Stack count up to max (AC-015.10)
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	StackLimitCount = ShockedMaxStacks;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::NeverReset;
	StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::ClearEntireStack;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	// Block stamina cost modifier: per-stack via custom MMC (AC-015.12)
	FGameplayModifierInfo BlockCostMod;
	BlockCostMod.Attribute = UMordecaiAttributeSet::GetBlockStaminaCostMultiplierAttribute();
	BlockCostMod.ModifierOp = EGameplayModOp::Additive;
	BlockCostMod.ModifierMagnitude.MagnitudeCalculationType = EGameplayEffectMagnitudeCalculation::CustomCalculationClass;
	BlockCostMod.ModifierMagnitude.CustomMagnitude.CalculationClassMagnitude = UMordecaiShockedBlockCostMMC::StaticClass();
	Modifiers.Add(BlockCostMod);

	// InitializeStatusEffect() is called automatically from PostInitProperties
}

bool UMordecaiGE_Shocked::TryMicroStun(UAbilitySystemComponent* TargetASC, float Chance)
{
	if (!TargetASC)
	{
		return false;
	}

	// Roll for micro-stun
	if (FMath::FRand() < Chance)
	{
		// Apply MicroStunned GE for brief duration (AC-015.11)
		FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = TargetASC->MakeOutgoingSpec(UMordecaiGE_MicroStunned::StaticClass(), 1.0f, Context);
		if (Spec.IsValid())
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}

		return true;
	}

	return false;
}

bool UMordecaiGE_Shocked::TryShockedCastInterrupt(UAbilitySystemComponent* TargetASC, float Chance)
{
	if (!TargetASC)
	{
		return false;
	}

	// Only interrupt if target is currently casting (AC-015.13)
	if (!TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Casting))
	{
		return false;
	}

	// Roll for interrupt
	if (FMath::FRand() < Chance)
	{
		FGameplayEventData EventData;
		TargetASC->HandleGameplayEvent(MordecaiGameplayTags::Event_CastInterrupted, &EventData);
		return true;
	}

	return false;
}
