// Project Mordecai — Kill Counter Widget (US-063)

#include "Mordecai/UI/MordecaiKillCounterWidget.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Components/TextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiKillCounterWidget)

// ---------------------------------------------------------------------------
// Static Helper
// ---------------------------------------------------------------------------

FString UMordecaiKillCounterWidget::FormatKillCount(int32 Kills)
{
	return FString::Printf(TEXT("Kills: %d"), Kills);
}

// ---------------------------------------------------------------------------
// ASC Binding
// ---------------------------------------------------------------------------

void UMordecaiKillCounterWidget::BindToASC(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	BoundASC = ASC;

	// Listen for enemy kill events (increment)
	ASC->RegisterGameplayTagEvent(MordecaiGameplayTags::Event_EnemyKill, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UMordecaiKillCounterWidget::OnEnemyKillEvent);

	// Listen for player death events (reset)
	ASC->RegisterGameplayTagEvent(MordecaiGameplayTags::Event_PlayerDeath, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UMordecaiKillCounterWidget::OnPlayerDeathEvent);

	UpdateDisplay();
}

void UMordecaiKillCounterWidget::OnEnemyKillEvent(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		KillCount++;
		UpdateDisplay();
	}
}

void UMordecaiKillCounterWidget::OnPlayerDeathEvent(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		ResetKillCount();
	}
}

void UMordecaiKillCounterWidget::ResetKillCount()
{
	KillCount = 0;
	UpdateDisplay();
}

void UMordecaiKillCounterWidget::UpdateDisplay()
{
	if (KillCountText)
	{
		KillCountText->SetText(FText::FromString(FormatKillCount(KillCount)));
	}
}
