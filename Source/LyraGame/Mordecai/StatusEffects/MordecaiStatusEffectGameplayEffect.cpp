// Project Mordecai — Status Effect Base Gameplay Effect (US-013)

#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectTypes.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"

UMordecaiStatusEffectGameplayEffect::UMordecaiStatusEffectGameplayEffect()
{
	// Default: duration-based status effect (5 seconds)
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(5.0f);

	// Stacking: AggregateByTarget so all sources contribute to one stack per target.
	// Subclasses override StackLimitCount for multi-stack effects (Frostbitten=5, Shocked=3).
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::NeverReset;
	StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::ClearEntireStack;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

void UMordecaiStatusEffectGameplayEffect::PostInitProperties()
{
	Super::PostInitProperties();

	// Initialize after CDO construction is complete (NewObject is now safe)
	InitializeStatusEffect();
}

void UMordecaiStatusEffectGameplayEffect::InitializeStatusEffect()
{
	if (!StatusTag.IsValid())
	{
		return;
	}

	// Guard: don't double-initialize (check if TargetTags component already exists)
	for (UGameplayEffectComponent* Comp : GEComponents)
	{
		if (Cast<UTargetTagsGameplayEffectComponent>(Comp))
		{
			return;
		}
	}

	// AC-013.5: Grant the status gameplay tag to the target while this GE is active.
	FInheritedTagContainer TagContainer;
	TagContainer.AddTag(StatusTag);

	UTargetTagsGameplayEffectComponent* TargetTagComp = NewObject<UTargetTagsGameplayEffectComponent>(this);
	if (TargetTagComp)
	{
		TargetTagComp->SetAndApplyTargetTagChanges(TagContainer);
		GEComponents.Add(TargetTagComp);
	}

	// AC-013.8: Immunity — block application if target has the immunity tag.
	FGameplayTag ImmunityTag = GetImmunityTagForStatus(StatusTag);
	if (ImmunityTag.IsValid())
	{
		UTargetTagRequirementsGameplayEffectComponent* ReqComp = NewObject<UTargetTagRequirementsGameplayEffectComponent>(this);
		if (ReqComp)
		{
			FGameplayTagRequirements AppReqs;
			AppReqs.IgnoreTags.AddTag(ImmunityTag);
			ReqComp->ApplicationTagRequirements = AppReqs;
			GEComponents.Add(ReqComp);
		}
	}
}
