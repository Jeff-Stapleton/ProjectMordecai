// Project Mordecai — Posture Bar Widget (US-052)

#include "Mordecai/UI/MordecaiPostureBarWidget.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiPostureBarWidget)

// ---------------------------------------------------------------------------
// Static Helper (AC-052.9, AC-052.11)
// ---------------------------------------------------------------------------

float UMordecaiPostureBarWidget::CalcPosturePercent(float Posture, float MaxPosture)
{
	if (MaxPosture <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(Posture / MaxPosture, 0.f, 1.f);
}

// ---------------------------------------------------------------------------
// Instance Methods
// ---------------------------------------------------------------------------

float UMordecaiPostureBarWidget::GetPosturePercent() const
{
	return CalcPosturePercent(CachedPosture, CachedMaxPosture);
}

bool UMordecaiPostureBarWidget::IsPostureBroken() const
{
	return bPostureBroken;
}

void UMordecaiPostureBarWidget::BindToASC(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	BoundASC = ASC;

	ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetPostureAttribute())
		.AddUObject(this, &UMordecaiPostureBarWidget::OnPostureChanged);

	ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetMaxPostureAttribute())
		.AddUObject(this, &UMordecaiPostureBarWidget::OnMaxPostureChanged);

	// Listen for posture broken tag (AC-052.4)
	ASC->RegisterGameplayTagEvent(MordecaiGameplayTags::State_PostureBroken, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UMordecaiPostureBarWidget::OnPostureBrokenTagChanged);

	// Initialize from current values
	bool bFound = false;
	CachedPosture = ASC->GetGameplayAttributeValue(UMordecaiAttributeSet::GetPostureAttribute(), bFound);
	if (!bFound) { CachedPosture = 0.f; }

	CachedMaxPosture = ASC->GetGameplayAttributeValue(UMordecaiAttributeSet::GetMaxPostureAttribute(), bFound);
	if (!bFound) { CachedMaxPosture = 0.f; }

	bPostureBroken = ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_PostureBroken);

	UpdateDisplay();
}

void UMordecaiPostureBarWidget::OnPostureChanged(const FOnAttributeChangeData& Data)
{
	CachedPosture = Data.NewValue;
	UpdateDisplay();
}

void UMordecaiPostureBarWidget::OnMaxPostureChanged(const FOnAttributeChangeData& Data)
{
	CachedMaxPosture = Data.NewValue;
	UpdateDisplay();
}

void UMordecaiPostureBarWidget::OnPostureBrokenTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	bPostureBroken = (NewCount > 0);
	UpdateDisplay();
}

void UMordecaiPostureBarWidget::UpdateDisplay()
{
	const float Percent = GetPosturePercent();

	if (PostureProgressBar)
	{
		PostureProgressBar->SetPercent(Percent);
		PostureProgressBar->SetFillColorAndOpacity(bPostureBroken ? BrokenPostureColor : NormalPostureColor);
	}
}
