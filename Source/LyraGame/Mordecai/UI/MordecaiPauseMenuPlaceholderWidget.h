// Project Mordecai — Pause Menu Placeholder Widget (US-069)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "MordecaiPauseMenuPlaceholderWidget.generated.h"

/**
 * UMordecaiPauseMenuPlaceholderWidget
 *
 * Trivial placeholder displayed in pause menu tabs that haven't been
 * populated with real content yet. Shows "Coming Soon" text.
 * Actual tab content is added by US-066/067/068 via RegisterTab().
 */
UCLASS()
class LYRAGAME_API UMordecaiPauseMenuPlaceholderWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UMordecaiPauseMenuPlaceholderWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnInitialized() override;

private:
	void BuildDefaultContent();
};
