// Project Mordecai — Kill Counter Widget (US-063)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"

#include "MordecaiKillCounterWidget.generated.h"

class UAbilitySystemComponent;
class UTextBlock;

/**
 * UMordecaiKillCounterWidget
 *
 * Persistent kill counter in the combat HUD. Listens for Mordecai.Event.EnemyKill
 * gameplay events and increments the count. Resets on player respawn.
 */
UCLASS()
class LYRAGAME_API UMordecaiKillCounterWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Format kill count as display text. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	static FString FormatKillCount(int32 Kills);

	/** Bind to ASC to listen for kill and death events. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void BindToASC(UAbilitySystemComponent* ASC);

	/** Get the current kill count. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	int32 GetKillCount() const { return KillCount; }

	/** Reset kill count to 0 (called on player respawn). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void ResetKillCount();

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KillCountText;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	int32 KillCount = 0;

private:
	void BuildDefaultContent();
	void OnEnemyKillEvent(const FGameplayTag Tag, int32 NewCount);
	void OnPlayerDeathEvent(const FGameplayTag Tag, int32 NewCount);
	void UpdateDisplay();

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
};
