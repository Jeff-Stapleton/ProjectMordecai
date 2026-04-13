// Project Mordecai — Pause Menu Placeholder Widget (US-069)

#include "Mordecai/UI/MordecaiPauseMenuPlaceholderWidget.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiPauseMenuPlaceholderWidget)

UMordecaiPauseMenuPlaceholderWidget::UMordecaiPauseMenuPlaceholderWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UMordecaiPauseMenuPlaceholderWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultContent();
	}
}

void UMordecaiPauseMenuPlaceholderWidget::BuildDefaultContent()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PlaceholderText"));
	Label->SetText(FText::FromString(TEXT("Coming Soon")));

	UCanvasPanelSlot* LabelSlot = Root->AddChildToCanvas(Label);
	if (LabelSlot)
	{
		LabelSlot->SetAutoSize(true);
	}
}
