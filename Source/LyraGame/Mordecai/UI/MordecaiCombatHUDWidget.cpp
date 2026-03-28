// Project Mordecai — Combat HUD Root Widget (US-052)

#include "Mordecai/UI/MordecaiCombatHUDWidget.h"
#include "Mordecai/UI/MordecaiHealthBarWidget.h"
#include "Mordecai/UI/MordecaiStaminaBarWidget.h"
#include "Mordecai/UI/MordecaiPostureBarWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiCombatHUDWidget)

void UMordecaiCombatHUDWidget::BindToASC(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

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
}
