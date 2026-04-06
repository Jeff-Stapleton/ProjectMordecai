// Project Mordecai — Spell Points Bar Widget (US-055)

#include "Mordecai/UI/MordecaiSpellPointsBarWidget.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiSpellPointsBarWidget)

// ---------------------------------------------------------------------------
// Static Helpers (AC-055.1, AC-055.3)
// ---------------------------------------------------------------------------

float UMordecaiSpellPointsBarWidget::CalcSpellPointsPercent(float SpellPoints, float MaxSpellPoints)
{
	if (MaxSpellPoints <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(SpellPoints / MaxSpellPoints, 0.f, 1.f);
}

FString UMordecaiSpellPointsBarWidget::FormatSpellPoints(float SpellPoints, float MaxSpellPoints)
{
	const int32 Current = FMath::FloorToInt32(FMath::Max(SpellPoints, 0.f));
	const int32 Max = FMath::FloorToInt32(FMath::Max(MaxSpellPoints, 0.f));
	return FString::Printf(TEXT("%d / %d"), Current, Max);
}

// ---------------------------------------------------------------------------
// Instance Methods
// ---------------------------------------------------------------------------

float UMordecaiSpellPointsBarWidget::GetSpellPointsPercent() const
{
	return CalcSpellPointsPercent(CachedSpellPoints, CachedMaxSpellPoints);
}

void UMordecaiSpellPointsBarWidget::BindToASC(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	BoundASC = ASC;

	ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetSpellPointsAttribute())
		.AddUObject(this, &UMordecaiSpellPointsBarWidget::OnSpellPointsChanged);

	ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetMaxSpellPointsAttribute())
		.AddUObject(this, &UMordecaiSpellPointsBarWidget::OnMaxSpellPointsChanged);

	// Initialize from current values
	bool bFound = false;
	CachedSpellPoints = ASC->GetGameplayAttributeValue(UMordecaiAttributeSet::GetSpellPointsAttribute(), bFound);
	if (!bFound) { CachedSpellPoints = 0.f; }

	CachedMaxSpellPoints = ASC->GetGameplayAttributeValue(UMordecaiAttributeSet::GetMaxSpellPointsAttribute(), bFound);
	if (!bFound) { CachedMaxSpellPoints = 0.f; }

	UpdateDisplay();
}

void UMordecaiSpellPointsBarWidget::OnSpellPointsChanged(const FOnAttributeChangeData& Data)
{
	CachedSpellPoints = Data.NewValue;
	UpdateDisplay();
}

void UMordecaiSpellPointsBarWidget::OnMaxSpellPointsChanged(const FOnAttributeChangeData& Data)
{
	CachedMaxSpellPoints = Data.NewValue;
	UpdateDisplay();
}

void UMordecaiSpellPointsBarWidget::UpdateDisplay()
{
	const float Percent = GetSpellPointsPercent();
	if (SpellPointsProgressBar)
	{
		SpellPointsProgressBar->SetPercent(Percent);
	}
	if (SpellPointsText)
	{
		SpellPointsText->SetText(FText::FromString(FormatSpellPoints(CachedSpellPoints, CachedMaxSpellPoints)));
	}
}
