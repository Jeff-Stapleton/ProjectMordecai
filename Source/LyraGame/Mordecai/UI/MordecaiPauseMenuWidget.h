// Project Mordecai — Pause Menu Widget (US-069)

#pragma once

#include "CoreMinimal.h"
#include "UI/LyraActivatableWidget.h"
#include "Mordecai/UI/MordecaiPauseMenuTypes.h"

#include "MordecaiPauseMenuWidget.generated.h"

class UUserWidget;
class UMordecaiPauseMenuSubsystem;

/**
 * UMordecaiPauseMenuWidget
 *
 * Tabbed pause menu using Lyra's CommonUI stack.
 * Activated by the subsystem, switches input to Menu mode,
 * and provides a tab registration API for future content
 * (character sheet, skills, feats, inventory, settings).
 *
 * Tab content starts as placeholder widgets. US-066/067/068
 * plug in real content via RegisterTab().
 */
UCLASS()
class LYRAGAME_API UMordecaiPauseMenuWidget : public ULyraActivatableWidget
{
	GENERATED_BODY()

public:
	UMordecaiPauseMenuWidget(const FObjectInitializer& ObjectInitializer);

	/** Register or replace a tab's content widget class. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|PauseMenu")
	void RegisterTab(FName TabId, FText TabDisplayName, TSubclassOf<UUserWidget> ContentWidgetClass);

	/** Remove a tab entirely. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|PauseMenu")
	void UnregisterTab(FName TabId);

	/** Get the currently active tab ID. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|PauseMenu")
	FName GetActiveTabId() const;

	/** Switch to a specific tab. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|PauseMenu")
	void SetActiveTab(FName TabId);

	/** Number of registered tabs. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|PauseMenu")
	int32 GetTabCount() const;

	/** Direct access to the tab data model (for subsystem/tests). */
	FMordecaiPauseMenuTabModel& GetTabModel() { return TabModel; }
	const FMordecaiPauseMenuTabModel& GetTabModel() const { return TabModel; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnDeactivated() override;

private:
	void InitDefaultTabs();
	void RebuildContentArea();

	FMordecaiPauseMenuTabModel TabModel;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> CurrentContentWidget;
};
