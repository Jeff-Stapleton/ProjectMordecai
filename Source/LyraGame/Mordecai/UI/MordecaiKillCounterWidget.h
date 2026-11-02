// Project Mordecai — Kill Counter Widget (US-063)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "MordecaiKillCounterWidget.generated.h"

class UAbilitySystemComponent;
class UTextBlock;
class AMordecaiGameMode;

/**
 * UMordecaiKillCounterWidget
 *
 * Persistent kill counter in the combat HUD. Displays AMordecaiGameMode's authoritative
 * per-life kill count; it does not maintain a second incrementing count.
 */
UCLASS()
class LYRAGAME_API UMordecaiKillCounterWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Format kill count as display text. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	static FString FormatKillCount(int32 Kills);

	/** Bind through the player's context to the authoritative arena game mode. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void BindToASC(UAbilitySystemComponent* ASC);

	/** Bind directly to the arena's authoritative kill count (also used by headless tests). */
	void BindToGameMode(AMordecaiGameMode* GameMode);

	/** Get the current kill count. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	int32 GetKillCount() const { return KillCount; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KillCountText;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	int32 KillCount = 0;

private:
	void BuildDefaultContent();
	void UnbindFromGameMode();
	void OnKillCountChanged(int32 NewKillCount);
	void UpdateDisplay();

	TWeakObjectPtr<AMordecaiGameMode> BoundGameMode;
	FDelegateHandle KillCountChangedHandle;
};
