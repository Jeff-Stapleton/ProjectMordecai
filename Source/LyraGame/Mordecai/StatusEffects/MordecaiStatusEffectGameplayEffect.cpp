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

	// TODO(DECISION): Stacking policy per agent_rules_v2.md Open Item #2.
	// Default: AggregateBySource, limit 1, refresh duration on re-application.
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateBySource;
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::NeverReset;
	StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::ClearEntireStack;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

void UMordecaiStatusEffectGameplayEffect::InitializeStatusEffect()
{
	if (!StatusTag.IsValid())
	{
		return;
	}

	// AC-013.5: Grant the status gameplay tag to the target while this GE is active.
	// UE 5.3+: Use UTargetTagsGameplayEffectComponent to grant tags to the target.
	FInheritedTagContainer TagContainer;
	TagContainer.AddTag(StatusTag);

	UTargetTagsGameplayEffectComponent* TargetTagComp = NewObject<UTargetTagsGameplayEffectComponent>(this);
	if (TargetTagComp)
	{
		TargetTagComp->SetAndApplyTargetTagChanges(TagContainer);
		GEComponents.Add(TargetTagComp);
	}

	// AC-013.8: Immunity — block application if target has the immunity tag.
	// UE 5.3+: Use UTargetTagRequirementsGameplayEffectComponent for application requirements.
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
