// Project Mordecai — Spell-point regeneration GameplayEffect (US-090)

#include "Mordecai/AbilitySystem/Effects/MordecaiGE_SpellPointRegen.h"

#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/MordecaiGameplayTags.h"

UMordecaiGE_SpellPointRegen::UMordecaiGE_SpellPointRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(RegenPeriodSeconds);
	bExecutePeriodicEffectOnApplication = false;

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::NeverRefresh;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::NeverReset;
	StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::ClearEntireStack;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	FAttributeBasedFloat RegenMagnitude;
	RegenMagnitude.Coefficient = FScalableFloat(BaseRegenPerSecond);
	RegenMagnitude.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
		UMordecaiAttributeSet::GetSpellPointsRegenMultiplierAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		/*bSnapshot=*/false);

	FGameplayModifierInfo& SpellPointModifier = Modifiers.AddDefaulted_GetRef();
	SpellPointModifier.Attribute = UMordecaiAttributeSet::GetSpellPointsAttribute();
	SpellPointModifier.ModifierOp = EGameplayModOp::Additive;
	SpellPointModifier.ModifierMagnitude =
		FGameplayEffectModifierMagnitude(RegenMagnitude);

}

void UMordecaiGE_SpellPointRegen::PostInitProperties()
{
	Super::PostInitProperties();

	bool bHasAssetTags = false;
	bool bHasRequirements = false;
	for (UGameplayEffectComponent* Component : GEComponents)
	{
		bHasAssetTags |= Cast<UAssetTagsGameplayEffectComponent>(Component) != nullptr;
		bHasRequirements |=
			Cast<UTargetTagRequirementsGameplayEffectComponent>(Component) != nullptr;
	}

	if (!bHasAssetTags)
	{
		UAssetTagsGameplayEffectComponent* AssetTags =
			NewObject<UAssetTagsGameplayEffectComponent>(this);
		if (AssetTags)
		{
			FInheritedTagContainer AssetTagChanges;
			AssetTagChanges.AddTag(MordecaiGameplayTags::Effect_SpellPointRegen);
			AssetTags->SetAndApplyAssetTagChanges(AssetTagChanges);
			GEComponents.Add(AssetTags);
		}
	}

	if (!bHasRequirements)
	{
		UTargetTagRequirementsGameplayEffectComponent* Requirements =
			NewObject<UTargetTagRequirementsGameplayEffectComponent>(this);
		if (Requirements)
		{
			Requirements->OngoingTagRequirements.IgnoreTags.AddTag(
				MordecaiGameplayTags::State_Dead);
			Requirements->OngoingTagRequirements.IgnoreTags.AddTag(
				MordecaiGameplayTags::Status_Silenced);
			GEComponents.Add(Requirements);
		}
	}
}
