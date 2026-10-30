// Project Mordecai — Test world for the Epic R recovery tests: a primary game layout with real layer stacks (US-086)

#pragma once

#include "Blueprint/WidgetTree.h"
#include "PrimaryGameLayout.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

#include "MordecaiRecoveryTestLayout.generated.h"

/**
 * The game's primary layout comes from the UI policy (W_OverallUILayout, one per UCommonLocalPlayer), which a headless
 * test world does not have. This one registers the CommonUI layer stacks a test asks for, so a push and a removal run
 * the stacks' own code (the widget pool included). Headless a stack has no Slate switcher: a pushed widget joins the
 * stack's widget list but is never activated, and only a removal through the stack takes it out again.
 */
UCLASS()
class UMordecaiRecoveryTestLayout : public UPrimaryGameLayout
{
	GENERATED_BODY()

public:
	UMordecaiRecoveryTestLayout(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get())
		: Super(ObjectInitializer)
	{
	}

	/** A layer stack registered under LayerTag. Call after Initialize(), which creates the widget tree. */
	UCommonActivatableWidgetContainerBase* AddTestLayer(FGameplayTag LayerTag)
	{
		UCommonActivatableWidgetStack* Stack = WidgetTree->ConstructWidget<UCommonActivatableWidgetStack>(
			UCommonActivatableWidgetStack::StaticClass());
		RegisterLayer(LayerTag, Stack);
		return Stack;
	}
};
