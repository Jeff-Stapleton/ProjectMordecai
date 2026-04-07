// Project Mordecai — Combat HUD Root Widget (US-052, US-055, US-061)

#include "Mordecai/UI/MordecaiCombatHUDWidget.h"
#include "Mordecai/UI/MordecaiHealthBarWidget.h"
#include "Mordecai/UI/MordecaiStaminaBarWidget.h"
#include "Mordecai/UI/MordecaiPostureBarWidget.h"
#include "Mordecai/UI/MordecaiSpellPointsBarWidget.h"
#include "Mordecai/UI/MordecaiStatusEffectBarWidget.h"
#include "Mordecai/UI/MordecaiSpellCooldownWidget.h"
#include "Mordecai/UI/MordecaiComboCounterWidget.h"
#include "Mordecai/UI/MordecaiCombatFeedbackWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiCombatHUDWidget)

void UMordecaiCombatHUDWidget::BindToASC(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	// Core resource bars (US-052)
	if (HealthBar)
	{
		HealthBar->BindToASC(ASC);
	}
	if (StaminaBar)
	{
		StaminaBar->BindToASC(ASC);
	}
	if (PostureBar)
	{
		PostureBar->BindToASC(ASC);
	}

	// Spell HUD (US-055)
	if (SpellPointsBar)
	{
		SpellPointsBar->BindToASC(ASC);
	}
	if (StatusEffectBar)
	{
		StatusEffectBar->BindToASC(ASC);
	}
	// Note: SpellCooldownA/B are bound individually via BindToCooldownTag()
	// from the player controller or experience setup, since each slot needs
	// its specific cooldown tag and duration.

	// Combat feedback (US-061)
	if (ComboCounter)
	{
		ComboCounter->BindToASC(ASC);
	}
	if (CombatFeedback)
	{
		CombatFeedback->BindToASC(ASC);
	}
}
