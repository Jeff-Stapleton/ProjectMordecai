// Project Mordecai — Pause Menu Widget (US-069)

#pragma once

#include "CoreMinimal.h"
#include "UI/LyraActivatableWidget.h"
#include "Mordecai/UI/MordecaiPauseMenuTypes.h"

#include "MordecaiPauseMenuWidget.generated.h"

class UBorder;
class UHorizontalBox;
class UTextBlock;
class UUserWidget;
class UMordecaiPauseMenuSubsystem;

/**
 * UMordecaiPauseMenuWidget
 *
 * Tabbed pause menu using Lyra's CommonUI stack.
 * Activated by the subsystem, switches input to Menu mode,
 * and provides a tab registration API for content tabs.
 *
 * Real content registered so far: Character (US-066), Inventory (US-079).
 * Remaining tabs use placeholder widgets until their stories land.
 *
 * Visuals (US-079): a programmatic fallback layout (title, tab bar,
 * content area) is built when no Blueprint tree exists, mirroring the
 * combat HUD pattern. The active tab's content widget class is
 * instantiated into the content area and — for known content types —
 * bound to the owning player's data sources.
 *
 * CommonUI pools this widget (US-094, issue #28): a reopen reuses the same
 * instance without NativeOnInitialized, after the close released its Slate
 * tree (the content's NativeDestruct unbinds it). So the active tab's content
 * is rebound to the current pawn on every activation and unbound on every
 * deactivation.
 */
UCLASS()
class LYRAGAME_API UMordecaiPauseMenuWidget : public ULyraActivatableWidget
{
	GENERATED_BODY()

public:
	UMordecaiPauseMenuWidget(const FObjectInitializer& ObjectInitializer);

	/**
	 * Populate default tabs and register the real content widgets
	 * (character sheet, inventory). Called from NativeOnInitialized;
	 * public so headless tests can drive it on a NewObject'd widget.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|PauseMenu")
	void InitializeTabs();

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

	/** The instantiated content widget for the active tab (null headless). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|PauseMenu")
	UUserWidget* GetCurrentContentWidget() const { return CurrentContentWidget; }

	/** Direct access to the tab data model (for subsystem/tests). */
	FMordecaiPauseMenuTabModel& GetTabModel() { return TabModel; }
	const FMordecaiPauseMenuTabModel& GetTabModel() const { return TabModel; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	// --- Programmatic fallback layout (US-079) ---

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> TabBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> TabContentBorder;

private:
	UFUNCTION()
	void HandleTabButtonClicked(FName TabId);

	void InitDefaultTabs();
	void EnsureDefaultLayout();
	void RebuildTabBar();
	void RebuildContentArea();

	/** Wire known content widget types to the owning player's data sources. */
	void BindTabContent(UUserWidget* ContentWidget);

	/** Drop a content widget's data-source subscriptions (its last drawn state stays). */
	void UnbindTabContent(UUserWidget* ContentWidget);

	FMordecaiPauseMenuTabModel TabModel;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> CurrentContentWidget;
};
