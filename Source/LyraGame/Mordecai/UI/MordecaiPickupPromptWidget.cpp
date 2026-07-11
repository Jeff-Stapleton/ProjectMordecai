// Project Mordecai — Pickup Prompt Widget (US-079)

#include "Mordecai/UI/MordecaiPickupPromptWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Mordecai/Items/MordecaiItemPickup.h"
#include "Mordecai/Items/MordecaiPickupInteractionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiPickupPromptWidget)

#define LOCTEXT_NAMESPACE "MordecaiPickupPrompt"

// ---------------------------------------------------------------------------
// Prompt composition
// ---------------------------------------------------------------------------

FText UMordecaiPickupPromptWidget::MakePromptText(const FText& ItemDisplayName)
{
	return FText::Format(LOCTEXT("PickupPromptFormat", "[F] Pick up {0}"), ItemDisplayName);
}

FText UMordecaiPickupPromptWidget::GetInventoryHintText()
{
	return LOCTEXT("InventoryHint", "[I] Inventory");
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void UMordecaiPickupPromptWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
	RefreshVisuals();
}

void UMordecaiPickupPromptWidget::NativeDestruct()
{
	Unbind();
	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------
// Binding
// ---------------------------------------------------------------------------

void UMordecaiPickupPromptWidget::BindToInteraction(UMordecaiPickupInteractionComponent* Interaction)
{
	Unbind();

	if (!Interaction)
	{
		return;
	}

	BoundInteraction = Interaction;
	Interaction->OnFocusedPickupChanged.AddDynamic(this, &UMordecaiPickupPromptWidget::HandleFocusedPickupChanged);

	// Prime from the current focus state
	HandleFocusedPickupChanged(Interaction->GetFocusedPickup(),
		Interaction->GetFocusedPickup() ? Interaction->GetFocusedPickup()->GetItemDisplayName() : FText::GetEmpty());
}

void UMordecaiPickupPromptWidget::Unbind()
{
	if (UMordecaiPickupInteractionComponent* Interaction = BoundInteraction.Get())
	{
		Interaction->OnFocusedPickupChanged.RemoveDynamic(this, &UMordecaiPickupPromptWidget::HandleFocusedPickupChanged);
	}
	BoundInteraction.Reset();
}

// ---------------------------------------------------------------------------
// Internal
// ---------------------------------------------------------------------------

void UMordecaiPickupPromptWidget::HandleFocusedPickupChanged(AMordecaiItemPickup* FocusedPickup, const FText& ItemDisplayName)
{
	bPromptVisible = FocusedPickup != nullptr;
	CachedPromptText = bPromptVisible ? MakePromptText(ItemDisplayName) : FText::GetEmpty();
	RefreshVisuals();
}

void UMordecaiPickupPromptWidget::BuildDefaultLayout()
{
	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	PromptText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PromptText"));
	PromptText->SetJustification(ETextJustify::Center);
	PromptText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	if (UVerticalBoxSlot* PromptSlot = Root->AddChildToVerticalBox(PromptText))
	{
		PromptSlot->SetHorizontalAlignment(HAlign_Center);
	}

	InventoryHintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryHintText"));
	InventoryHintText->SetJustification(ETextJustify::Center);
	InventoryHintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f, 0.9f)));
	InventoryHintText->SetText(GetInventoryHintText());
	if (UVerticalBoxSlot* HintSlot = Root->AddChildToVerticalBox(InventoryHintText))
	{
		HintSlot->SetHorizontalAlignment(HAlign_Center);
	}
}

void UMordecaiPickupPromptWidget::RefreshVisuals()
{
	if (PromptText)
	{
		PromptText->SetText(CachedPromptText);
		PromptText->SetVisibility(bPromptVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (InventoryHintText)
	{
		InventoryHintText->SetText(GetInventoryHintText());
	}
}

#undef LOCTEXT_NAMESPACE
