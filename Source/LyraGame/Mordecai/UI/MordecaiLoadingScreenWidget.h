// Project Mordecai — Loading Screen Widget (issue #39)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "MordecaiLoadingScreenWidget.generated.h"

class UTextBlock;

/**
 * UMordecaiLoadingScreenWidget
 *
 * The game's loading screen. Config/DefaultGame.ini sets CommonLoadingScreenSettings.LoadingScreenWidget to this
 * class; it was Lyra's W_LoadingScreen_Host, which showed the LYRA logo on every load (issue #39).
 * ULoadingScreenManager creates it with UUserWidget::CreateWidgetInstance(GameInstance), puts its Slate widget on top
 * of the viewport while a map or experience loads, and drops it afterwards (2 s later outside the editor).
 *
 * - Builds its own tree in Initialize(), not NativeOnInitialized(): the manager gives it a player context only when the
 *   first local player has a player controller, and without one NativeOnInitialized never runs (a blank screen).
 * - Opaque: the manager renders the world again while it holds the screen up for texture streaming.
 * - No content assets (the engine's default font), so there is nothing to cook and no path-string load.
 * - Ignores ULyraLoadingScreenSubsystem's content widget: Lyra's front-end playlists set it to Lyra's logo.
 * - Logs "MordecaiLoadingScreen: shown" each time it goes on screen: playtest_boot.py's loading_screen check reads it.
 */
UCLASS()
class LYRAGAME_API UMordecaiLoadingScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The large line: the game's name. */
	static FText GetTitleText();

	/** The line under the title. */
	static FText GetStatusText();

	/** The class ULoadingScreenManager shows: CommonLoadingScreenSettings.LoadingScreenWidget, loaded. Null if unset or
	 *  not a widget. The settings class is private to its plugin, so this reads it by reflection. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|UI")
	static TSubclassOf<UUserWidget> GetConfiguredLoadingScreenWidgetClass();

	virtual bool Initialize() override;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

private:
	/** An opaque full-screen background, the title, the status line and a throbber. */
	void BuildDefaultContent();
};
