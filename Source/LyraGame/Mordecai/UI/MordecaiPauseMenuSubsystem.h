// Project Mordecai — Pause Menu Subsystem (US-069)

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Mordecai/UI/MordecaiPauseMenuTypes.h"

#include "MordecaiPauseMenuSubsystem.generated.h"

class UMordecaiPauseMenuWidget;

/**
 * UMordecaiPauseMenuSubsystem
 *
 * GameInstance subsystem that manages the pause menu widget lifecycle.
 * Provides TogglePauseMenu / OpenPauseMenu / ClosePauseMenu.
 * Persists across level loads. Holds menu state and last-active tab.
 *
 * TODO(DECISION): Multiplayer pause behavior (AC-069.13).
 * For now bCanPauseGame defaults to true (single-player first).
 */
UCLASS()
class LYRAGAME_API UMordecaiPauseMenuSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Toggle the pause menu open/closed. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|PauseMenu")
	void TogglePauseMenu();

	/** Open the pause menu (no-op if already open). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|PauseMenu")
	void OpenPauseMenu();

	/** Close the pause menu (no-op if already closed). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|PauseMenu")
	void ClosePauseMenu();

	/** Whether the pause menu is currently open. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|PauseMenu")
	bool IsMenuOpen() const;

	/** The tab that was active when the menu was last closed. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|PauseMenu")
	FName GetLastActiveTabId() const;

	/**
	 * If false, opening the menu only switches input mode without pausing
	 * the game. Intended for future multiplayer (AC-069.13).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mordecai|PauseMenu")
	bool bCanPauseGame = true;

	/** Direct access to the menu state (for tests). */
	FMordecaiPauseMenuState& GetMenuState() { return MenuState; }
	const FMordecaiPauseMenuState& GetMenuState() const { return MenuState; }

private:
	void PushMenuWidget();
	void PopMenuWidget();

	FMordecaiPauseMenuState MenuState;

	UPROPERTY(Transient)
	TWeakObjectPtr<UMordecaiPauseMenuWidget> ActiveMenuWidget;
};
