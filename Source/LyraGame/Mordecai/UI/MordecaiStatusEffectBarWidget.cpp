// Project Mordecai — Status Effect Bar Widget (US-055)

#include "Mordecai/UI/MordecaiStatusEffectBarWidget.h"
#include "Mordecai/UI/MordecaiStatusEffectIndicatorWidget.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiStatusEffectBarWidget)

// ---------------------------------------------------------------------------
// Buff Tag Set (AC-055.7)
// ---------------------------------------------------------------------------

const FGameplayTagContainer& UMordecaiStatusEffectBarWidget::GetBuffTagContainer()
{
	static FGameplayTagContainer BuffTags;
	static bool bBuffTagsInitialized = false;
	if (!bBuffTagsInitialized)
	{
		BuffTags.AddTag(MordecaiGameplayTags::Status_Focused);
		BuffTags.AddTag(MordecaiGameplayTags::Status_Blessed);
		BuffTags.AddTag(MordecaiGameplayTags::Status_StoneSkin);
		BuffTags.AddTag(MordecaiGameplayTags::Status_FireWard);
		BuffTags.AddTag(MordecaiGameplayTags::Status_Restoration);
		bBuffTagsInitialized = true;
	}
	return BuffTags;
}

// ---------------------------------------------------------------------------
// Static Helpers (AC-055.7)
// ---------------------------------------------------------------------------

bool UMordecaiStatusEffectBarWidget::IsBuffTag(const FGameplayTag& StatusTag)
{
	return GetBuffTagContainer().HasTagExact(StatusTag);
}

FLinearColor UMordecaiStatusEffectBarWidget::GetTintForStatusTag(const FGameplayTag& StatusTag)
{
	return IsBuffTag(StatusTag)
		? FLinearColor(0.f, 1.f, 0.f, 1.f)   // Green for buffs
		: FLinearColor(1.f, 0.f, 0.f, 1.f);   // Red for debuffs
}

// ---------------------------------------------------------------------------
// ASC Binding (AC-055.6)
// ---------------------------------------------------------------------------

void UMordecaiStatusEffectBarWidget::BindToASC(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	BoundASC = ASC;

	// Register for any child tag change under Mordecai.Status
	const FGameplayTag StatusRootTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Status")));
	ASC->RegisterGameplayTagEvent(StatusRootTag, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UMordecaiStatusEffectBarWidget::OnStatusTagChanged);

	// Check for any already-active status tags and create indicators
	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);
	for (const FGameplayTag& Tag : OwnedTags)
	{
		if (Tag.MatchesTag(StatusRootTag) && !Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Status.Category")))))
		{
			AddIndicator(Tag);
		}
	}
}

// ---------------------------------------------------------------------------
// Tag Change Callback (AC-055.5, AC-055.6)
// ---------------------------------------------------------------------------

void UMordecaiStatusEffectBarWidget::OnStatusTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// Skip category tags (Mordecai.Status.Category.*)
	if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Status.Category")))))
	{
		return;
	}

	if (NewCount > 0)
	{
		AddIndicator(Tag);
	}
	else
	{
		RemoveIndicator(Tag);
	}
}

void UMordecaiStatusEffectBarWidget::AddIndicator(const FGameplayTag& StatusTag)
{
	// Don't duplicate
	if (ActiveIndicators.Contains(StatusTag))
	{
		return;
	}

	// Enforce max visible limit
	if (ActiveIndicators.Num() >= MaxVisibleIndicators)
	{
		return;
	}

	if (!IndicatorWidgetClass)
	{
		return;
	}

	UMordecaiStatusEffectIndicatorWidget* Indicator = CreateWidget<UMordecaiStatusEffectIndicatorWidget>(this, IndicatorWidgetClass);
	if (!Indicator)
	{
		return;
	}

	const bool bIsBuff = IsBuffTag(StatusTag);

	// Query remaining duration from ASC if available
	float Duration = 0.f;
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		// Query for active GEs that grant this specific status tag
		FGameplayEffectQuery Query;
		Query.OwningTagQuery = FGameplayTagQuery::MakeQuery_MatchAnyTags(FGameplayTagContainer(StatusTag));
		TArray<FActiveGameplayEffectHandle> MatchingEffects = ASC->GetActiveEffects(Query);
		for (const FActiveGameplayEffectHandle& Handle : MatchingEffects)
		{
			const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(Handle);
			if (ActiveGE)
			{
				Duration = ActiveGE->GetDuration();
				break;
			}
		}
	}

	Indicator->InitializeIndicator(StatusTag, bIsBuff, Duration);

	if (IndicatorContainer)
	{
		IndicatorContainer->AddChildToHorizontalBox(Indicator);
	}

	ActiveIndicators.Add(StatusTag, Indicator);
}

void UMordecaiStatusEffectBarWidget::RemoveIndicator(const FGameplayTag& StatusTag)
{
	TObjectPtr<UMordecaiStatusEffectIndicatorWidget>* Found = ActiveIndicators.Find(StatusTag);
	if (!Found || !(*Found))
	{
		return;
	}

	(*Found)->RemoveFromParent();
	ActiveIndicators.Remove(StatusTag);
}
