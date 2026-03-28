// Project Mordecai — Enemy Health Bar Widget (US-052)

#include "Mordecai/UI/MordecaiEnemyHealthBarWidget.h"
#include "Mordecai/UI/MordecaiHealthBarWidget.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiEnemyHealthBarWidget)

// ---------------------------------------------------------------------------
// Instance Methods
// ---------------------------------------------------------------------------

float UMordecaiEnemyHealthBarWidget::GetHealthPercent() const
{
	return UMordecaiHealthBarWidget::CalcHealthPercent(CachedHealth, CachedMaxHealth);
}

void UMordecaiEnemyHealthBarWidget::BindToASC(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	BoundASC = ASC;

	ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetHealthAttribute())
		.AddUObject(this, &UMordecaiEnemyHealthBarWidget::OnHealthChanged);

	ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &UMordecaiEnemyHealthBarWidget::OnMaxHealthChanged);

	// Initialize from current values
	bool bFound = false;
	CachedHealth = ASC->GetGameplayAttributeValue(UMordecaiAttributeSet::GetHealthAttribute(), bFound);
	if (!bFound) { CachedHealth = 0.f; }

	CachedMaxHealth = ASC->GetGameplayAttributeValue(UMordecaiAttributeSet::GetMaxHealthAttribute(), bFound);
	if (!bFound) { CachedMaxHealth = 0.f; }

	UpdateDisplay();
}

void UMordecaiEnemyHealthBarWidget::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	CachedHealth = Data.NewValue;
	UpdateDisplay();
}

void UMordecaiEnemyHealthBarWidget::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	CachedMaxHealth = Data.NewValue;
	UpdateDisplay();
}

void UMordecaiEnemyHealthBarWidget::UpdateDisplay()
{
	const float Percent = GetHealthPercent();
	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(Percent);
	}
}
