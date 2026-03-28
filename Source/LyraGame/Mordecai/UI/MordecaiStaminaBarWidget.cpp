// Project Mordecai — Stamina Bar Widget (US-052)

#include "Mordecai/UI/MordecaiStaminaBarWidget.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiStaminaBarWidget)

// ---------------------------------------------------------------------------
// Tier thresholds for HUD display (AC-052.8)
// ---------------------------------------------------------------------------
namespace MordecaiHUDConstants
{
	static constexpr float GreenThreshold = 0.66f;
	static constexpr float YellowThreshold = 0.33f;
}

// ---------------------------------------------------------------------------
// Static Helpers (AC-052.7, AC-052.8, AC-052.11)
// ---------------------------------------------------------------------------

float UMordecaiStaminaBarWidget::CalcStaminaPercent(float Stamina, float MaxStamina)
{
	if (MaxStamina <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(Stamina / MaxStamina, 0.f, 1.f);
}

EMordecaiStaminaTier UMordecaiStaminaBarWidget::CalcStaminaTier(float Stamina, float MaxStamina)
{
	if (MaxStamina <= 0.f)
	{
		return EMordecaiStaminaTier::Exhausted;
	}

	const float Ratio = Stamina / MaxStamina;

	if (Ratio <= 0.f)
	{
		return EMordecaiStaminaTier::Exhausted;
	}
	if (Ratio >= MordecaiHUDConstants::GreenThreshold)
	{
		return EMordecaiStaminaTier::Green;
	}
	if (Ratio >= MordecaiHUDConstants::YellowThreshold)
	{
		return EMordecaiStaminaTier::Yellow;
	}
	return EMordecaiStaminaTier::Red;
}

// ---------------------------------------------------------------------------
// Instance Methods
// ---------------------------------------------------------------------------

float UMordecaiStaminaBarWidget::GetStaminaPercent() const
{
	return CalcStaminaPercent(CachedStamina, CachedMaxStamina);
}

EMordecaiStaminaTier UMordecaiStaminaBarWidget::GetStaminaTier() const
{
	return CalcStaminaTier(CachedStamina, CachedMaxStamina);
}

void UMordecaiStaminaBarWidget::BindToASC(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	BoundASC = ASC;

	ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetStaminaAttribute())
		.AddUObject(this, &UMordecaiStaminaBarWidget::OnStaminaChanged);

	ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetMaxStaminaAttribute())
		.AddUObject(this, &UMordecaiStaminaBarWidget::OnMaxStaminaChanged);

	// Initialize from current values
	bool bFound = false;
	CachedStamina = ASC->GetGameplayAttributeValue(UMordecaiAttributeSet::GetStaminaAttribute(), bFound);
	if (!bFound) { CachedStamina = 0.f; }

	CachedMaxStamina = ASC->GetGameplayAttributeValue(UMordecaiAttributeSet::GetMaxStaminaAttribute(), bFound);
	if (!bFound) { CachedMaxStamina = 0.f; }

	UpdateDisplay();
}

void UMordecaiStaminaBarWidget::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
	CachedStamina = Data.NewValue;
	UpdateDisplay();
}

void UMordecaiStaminaBarWidget::OnMaxStaminaChanged(const FOnAttributeChangeData& Data)
{
	CachedMaxStamina = Data.NewValue;
	UpdateDisplay();
}

void UMordecaiStaminaBarWidget::UpdateDisplay()
{
	const float Percent = GetStaminaPercent();
	const EMordecaiStaminaTier Tier = GetStaminaTier();

	if (StaminaProgressBar)
	{
		StaminaProgressBar->SetPercent(Percent);
		StaminaProgressBar->SetFillColorAndOpacity(GetColorForTier(Tier));
	}
}

FLinearColor UMordecaiStaminaBarWidget::GetColorForTier(EMordecaiStaminaTier Tier) const
{
	switch (Tier)
	{
	case EMordecaiStaminaTier::Green:     return GreenTierColor;
	case EMordecaiStaminaTier::Yellow:    return YellowTierColor;
	case EMordecaiStaminaTier::Red:       return RedTierColor;
	case EMordecaiStaminaTier::Exhausted: return ExhaustedTierColor;
	default:                              return YellowTierColor;
	}
}
