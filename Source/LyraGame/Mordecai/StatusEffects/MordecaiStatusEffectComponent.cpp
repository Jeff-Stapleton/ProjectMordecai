// Project Mordecai — Status Effect Component (US-013)

#include "Mordecai/StatusEffects/MordecaiStatusEffectComponent.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectTypes.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"

UMordecaiStatusEffectComponent::UMordecaiStatusEffectComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FActiveGameplayEffectHandle UMordecaiStatusEffectComponent::ApplyStatusEffect(
	TSubclassOf<UGameplayEffect> GEClass,
	AActor* Instigator,
	float Level)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !GEClass)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	if (Instigator)
	{
		Context.AddInstigator(Instigator, Instigator);
	}

	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(GEClass, Level, Context);
	if (!Spec.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	return ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void UMordecaiStatusEffectComponent::RemoveStatusEffect(FGameplayTag StatusTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !StatusTag.IsValid())
	{
		return;
	}

	// Remove all active GEs that grant this status tag
	FGameplayTagContainer TagsToRemove;
	TagsToRemove.AddTag(StatusTag);
	ASC->RemoveActiveEffectsWithGrantedTags(TagsToRemove);
}

void UMordecaiStatusEffectComponent::RemoveAllStatusEffects()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	// Remove GEs that grant any Mordecai.Status.* tag
	static const FGameplayTag StatusRoot = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Status")), false);
	if (!StatusRoot.IsValid())
	{
		return;
	}

	FGameplayTagContainer TagsToRemove;
	TagsToRemove.AddTag(StatusRoot);
	ASC->RemoveActiveEffectsWithGrantedTags(TagsToRemove);
}

void UMordecaiStatusEffectComponent::RemoveStatusEffectsByCategory(FGameplayTag CategoryTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !CategoryTag.IsValid())
	{
		return;
	}

	// Map CategoryTag → EMordecaiStatusEffectCategory
	EMordecaiStatusEffectCategory Category = EMordecaiStatusEffectCategory::Debuff;

	using namespace MordecaiGameplayTags;
	if (CategoryTag == Status_Category_DoT)                { Category = EMordecaiStatusEffectCategory::DoT; }
	else if (CategoryTag == Status_Category_Debuff)        { Category = EMordecaiStatusEffectCategory::Debuff; }
	else if (CategoryTag == Status_Category_Control)       { Category = EMordecaiStatusEffectCategory::Control; }
	else if (CategoryTag == Status_Category_Buff)          { Category = EMordecaiStatusEffectCategory::Buff; }
	else if (CategoryTag == Status_Category_Environmental) { Category = EMordecaiStatusEffectCategory::Environmental; }

	// Get all status tags in this category and remove their GEs
	TArray<FGameplayTag> StatusTags = GetStatusTagsForCategory(Category);
	for (const FGameplayTag& StatusTag : StatusTags)
	{
		FGameplayTagContainer TagsToRemove;
		TagsToRemove.AddTag(StatusTag);
		ASC->RemoveActiveEffectsWithGrantedTags(TagsToRemove);
	}
}

bool UMordecaiStatusEffectComponent::HasStatusEffect(FGameplayTag StatusTag) const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !StatusTag.IsValid())
	{
		return false;
	}

	return ASC->HasMatchingGameplayTag(StatusTag);
}

FGameplayTagContainer UMordecaiStatusEffectComponent::GetActiveStatusTags() const
{
	FGameplayTagContainer Result;
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return Result;
	}

	// Get all owned tags from the ASC
	FGameplayTagContainer AllTags;
	ASC->GetOwnedGameplayTags(AllTags);

	// Filter to only Mordecai.Status.* tags (exclude Category subtags)
	static const FGameplayTag StatusRoot = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Status")), false);
	static const FGameplayTag CategoryRoot = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Status.Category")), false);

	for (const FGameplayTag& Tag : AllTags)
	{
		if (Tag.MatchesTag(StatusRoot) && !Tag.MatchesTag(CategoryRoot))
		{
			Result.AddTag(Tag);
		}
	}

	return Result;
}

void UMordecaiStatusEffectComponent::SetAbilitySystemComponentOverride(UAbilitySystemComponent* InASC)
{
	ASCOverride = InASC;
}

UAbilitySystemComponent* UMordecaiStatusEffectComponent::GetAbilitySystemComponent() const
{
	if (ASCOverride)
	{
		return ASCOverride;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	// Try IAbilitySystemInterface first
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
	{
		return ASI->GetAbilitySystemComponent();
	}

	// Fall back to component search
	return Owner->FindComponentByClass<UAbilitySystemComponent>();
}
