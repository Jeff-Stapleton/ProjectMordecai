// Project Mordecai — Pause Menu Types (US-069)
// Pure C++ data types for pause menu tab model and state.
// Testable without a game world or widget rendering.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

class UUserWidget;

/**
 * A single tab entry in the pause menu.
 */
struct FMordecaiPauseMenuTabEntry
{
	FName TabId;
	FText DisplayName;
	TSubclassOf<UUserWidget> ContentWidgetClass;
};

/**
 * Pure data model for pause menu tabs.
 * Manages tab list, active tab, registration/unregistration.
 * Decoupled from UMG so it can be tested headlessly.
 */
struct FMordecaiPauseMenuTabModel
{
	TArray<FMordecaiPauseMenuTabEntry> Tabs;
	FName ActiveTabId;

	/** Populate default tabs: Character, Skills, Feats, Inventory, Settings. */
	void InitDefaults(TSubclassOf<UUserWidget> PlaceholderClass);

	int32 GetTabCount() const;
	bool HasTab(FName TabId) const;
	const FMordecaiPauseMenuTabEntry* FindTab(FName TabId) const;

	/** Register or replace a tab's content widget class. */
	void RegisterTab(FName TabId, FText DisplayName, TSubclassOf<UUserWidget> ContentClass);

	/** Remove a tab entirely. Returns true if removed. */
	bool UnregisterTab(FName TabId);

	/** Switch active tab. Returns true if tab exists and was switched. */
	bool SetActiveTab(FName TabId);
};

/**
 * Pure state machine for pause menu open/close logic.
 * Decoupled from UE game systems so it can be tested headlessly.
 */
struct FMordecaiPauseMenuState
{
	bool bIsOpen = false;
	bool bCanPauseGame = true;
	FName LastActiveTabId;

	/** Open the menu. Returns true if state changed. */
	bool Open();

	/** Close the menu, remembering the currently active tab. Returns true if state changed. */
	bool Close(FName CurrentActiveTabId);

	/** Toggle open/close. Returns true if state changed. */
	bool Toggle(FName CurrentActiveTabId);

	/** Whether the game should be paused (menu open + pause enabled). */
	bool ShouldBePaused() const;
};
