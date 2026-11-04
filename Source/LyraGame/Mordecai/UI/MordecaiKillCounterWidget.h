// Project Mordecai — Kill Counter Widget (US-063, US-086)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "MordecaiKillCounterWidget.generated.h"

class AMordecaiGameMode;
class UAbilitySystemComponent;
class UTextBlock;

/**
 * UMordecaiKillCounterWidget
 *
 * Persistent kill counter in the combat HUD: "Kills: N", where N is AMordecaiGameMode's kill count, the number the
 * agent bridge reports as player.kills. The game mode counts the player's kills and resets the count when the player
 * dies (issue #6); this widget only shows it and follows its changes.
 *
 * Like the combat HUD it serves the local player of a standalone or listen-server game: a remote client has no
 * authority game mode and shows 0 (issue #77).
 */
UCLASS()
class LYRAGAME_API UMordecaiKillCounterWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Format kill count as display text. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	static FString FormatKillCount(int32 Kills);

	/** Show the kill count of the authority game mode in the ASC's world (the combat HUD's BindToASC chain). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void BindToASC(UAbilitySystemComponent* ASC);

	/** Show this game mode's kill count and follow its changes; with none, show 0. */
	void BindToGameMode(AMordecaiGameMode* GameMode);

	/** The kill count on display. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	int32 GetKillCount() const { return KillCount; }

	/** The game mode whose count this shows, or null (smoke tests check the binding). */
	UFUNCTION(BlueprintPure, Category = "Mordecai|HUD")
	AMordecaiGameMode* GetBoundGameMode() const { return BoundGameMode.Get(); }

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KillCountText;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	int32 KillCount = 0;

private:
	void BuildDefaultContent();
	void OnKillCountChanged(int32 NewCount);
	void UpdateDisplay();

	TWeakObjectPtr<AMordecaiGameMode> BoundGameMode;
	FDelegateHandle KillCountChangedHandle;
};
