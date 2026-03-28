// Project Mordecai — Health Bar Widget (US-052)

#include "Mordecai/UI/MordecaiHealthBarWidget.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiHealthBarWidget)

// ---------------------------------------------------------------------------
// Static Helper (AC-052.6, AC-052.11)
// ---------------------------------------------------------------------------

float UMordecaiHealthBarWidget::CalcHealthPercent(float Health, float MaxHealth)
{
	if (MaxHealth <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(Health / MaxHealth, 0.f, 1.f);
}

// ---------------------------------------------------------------------------
// Instance Methods
// ---------------------------------------------------------------------------

float UMordecaiHealthBarWidget::GetHealthPercent() const
{
	return CalcHealthPercent(CachedHealth, CachedMaxHealth);
}

void UMordecaiHealthBarWidget::BindToASC(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	BoundASC = ASC;

	ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetHealthAttribute())
		.AddUObject(this, &UMordecaiHealthBarWidget::OnHealthChanged);

	ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &UMordecaiHealthBarWidget::OnMaxHealthChanged);

	// Initialize from current values
	bool bFound = false;
	CachedHealth = ASC->GetGameplayAttributeValue(UMordecaiAttributeSet::GetHealthAttribute(), bFound);
	if (!bFound) { CachedHealth = 0.f; }

	CachedMaxHealth = ASC->GetGameplayAttributeValue(UMordecaiAttributeSet::GetMaxHealthAttribute(), bFound);
	if (!bFound) { CachedMaxHealth = 0.f; }

	UpdateDisplay();
}

void UMordecaiHealthBarWidget::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	CachedHealth = Data.NewValue;
	UpdateDisplay();
}

void UMordecaiHealthBarWidget::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	CachedMaxHealth = Data.NewValue;
	UpdateDisplay();
}

void UMordecaiHealthBarWidget::UpdateDisplay()
{
	const float Percent = GetHealthPercent();
	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(Percent);
	}
}
