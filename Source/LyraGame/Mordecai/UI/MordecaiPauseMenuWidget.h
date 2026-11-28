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
struct FBindUIActionArgs;

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
 *
 * Menu input mode blocks every key without a CommonUI UI action binding
 * (US-095, issues #23 and #32): Escape / gamepad B close the menu through the
 * default back action (bIsBackHandler), and NativeOnInitialized registers UI
 * actions for LB / RB (previous / next tab, wrapping) and close-on-release
 * handling for I, Escape and Start. CommonUI keeps those bindings across pooled
 * reopens and re-attaches them on every rebuild of the Slate tree. A close key
 * opened outside the menu does not close it on that opening press's release.
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

	/** Switch to the previous tab in tab-bar order, wrapping from the first to the last (LB, US-095). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|PauseMenu")
	void SelectPreviousTab();

	/** Switch to the next tab in tab-bar order, wrapping from the last to the first (RB, US-095). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|PauseMenu")
	void SelectNextTab();

	/**
	 * The CommonUI UI action bindings NativeOnInitialized registers: LB/RB tab switching, I close-on-release, and
	 * held-safe Escape/B/Start close handling. Public so headless tests can create exactly these bindings: Tier 1 has
	 * no CommonUI action router.
	 */
	TArray<FBindUIActionArgs> MakeUIActionBindingArgs();

	/** Action names (the UI action tag names) of the bindings registered with the CommonUI action router; empty without one. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|PauseMenu")
	TArray<FName> GetUIActionBindingNames() const;

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

	/** I pressed in the open menu: arm the close (US-095). */
	void HandleInventoryPressed();

	/** I released: close the menu the way the back action does, if an I press in this open menu armed it (US-095). */
	void HandleInventoryReleased();

	/** Escape or Start pressed while this menu owns input: arm its release to close (#178). */
	void HandlePauseClosePressed();

	/** Close on an armed Escape/Start release; also consumes the back-key release (#178). */
	void HandlePauseCloseReleased();

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

	/** An I press reached this open menu; its release closes it. Reset on every activation and deactivation. */
	bool bInventoryCloseArmed = false;

	/** An Escape/Start press reached this open menu; its release closes it. */
	bool bPauseCloseArmed = false;
};
