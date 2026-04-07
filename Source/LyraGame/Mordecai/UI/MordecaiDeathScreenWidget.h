// Project Mordecai — Death Screen Widget (US-062)

#pragma once

#include "CoreMinimal.h"
#include "UI/LyraActivatableWidget.h"
#include "GameplayTagContainer.h"

#include "MordecaiDeathScreenWidget.generated.h"

class UAbilitySystemComponent;
class UTextBlock;

/**
 * UMordecaiDeathScreenWidget
 *
 * Full-screen death overlay using CommonUI activatable widget stack.
 * Pushed to UI.Layer.Modal on player death. Shows "You Died" message
 * and respawn countdown timer. Auto-removed on respawn.
 *
 * Uses GameAndMenu input mode so respawn flow is not blocked.
 */
UCLASS()
class LYRAGAME_API UMordecaiDeathScreenWidget : public ULyraActivatableWidget
{
	GENERATED_BODY()

public:
	UMordecaiDeathScreenWidget(const FObjectInitializer& ObjectInitializer);

	/** Format respawn countdown. Rounds up to nearest integer. Empty if <= 0. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	static FString FormatRespawnCountdown(float TimeRemaining);

	/** Format kill count for display on death screen. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	static FString FormatKillCount(int32 Kills);

	/** Initialize the death screen with respawn delay and kill count. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void InitializeDeathScreen(float RespawnDelay, int32 KillCount);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DeathText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountdownText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KillCountText;

private:
	void UpdateCountdownDisplay();

	float RemainingRespawnTime = 0.f;
	int32 CachedKillCount = 0;
};
