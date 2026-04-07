// Project Mordecai — Death Screen Widget (US-062)

#include "Mordecai/UI/MordecaiDeathScreenWidget.h"
#include "Components/TextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiDeathScreenWidget)

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiDeathScreenWidget::UMordecaiDeathScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// GameAndMenu so respawn flow is not blocked by modal input capture
	InputConfig = ELyraWidgetInputMode::GameAndMenu;
}

// ---------------------------------------------------------------------------
// Static Helpers
// ---------------------------------------------------------------------------

FString UMordecaiDeathScreenWidget::FormatRespawnCountdown(float TimeRemaining)
{
	if (TimeRemaining <= 0.f)
	{
		return FString();
	}
	const int32 CeilSeconds = FMath::CeilToInt32(TimeRemaining);
	return FString::Printf(TEXT("%d"), CeilSeconds);
}

FString UMordecaiDeathScreenWidget::FormatKillCount(int32 Kills)
{
	return FString::Printf(TEXT("Kills: %d"), Kills);
}

// ---------------------------------------------------------------------------
// Instance Methods
// ---------------------------------------------------------------------------

void UMordecaiDeathScreenWidget::InitializeDeathScreen(float RespawnDelay, int32 KillCount)
{
	RemainingRespawnTime = RespawnDelay;
	CachedKillCount = KillCount;

	if (DeathText)
	{
		DeathText->SetText(FText::FromString(TEXT("You Died")));
	}

	if (KillCountText)
	{
		KillCountText->SetText(FText::FromString(FormatKillCount(CachedKillCount)));
	}

	UpdateCountdownDisplay();
}

// ---------------------------------------------------------------------------
// Tick — countdown
// ---------------------------------------------------------------------------

void UMordecaiDeathScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (RemainingRespawnTime > 0.f)
	{
		RemainingRespawnTime -= InDeltaTime;
		UpdateCountdownDisplay();
	}
}

void UMordecaiDeathScreenWidget::UpdateCountdownDisplay()
{
	if (CountdownText)
	{
		CountdownText->SetText(FText::FromString(FormatRespawnCountdown(RemainingRespawnTime)));
	}
}
