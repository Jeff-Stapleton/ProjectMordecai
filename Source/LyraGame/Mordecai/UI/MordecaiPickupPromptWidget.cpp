// Project Mordecai — Pickup Prompt Widget (US-079)

#include "Mordecai/UI/MordecaiPickupPromptWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Mordecai/Items/MordecaiItemPickup.h"
#include "Mordecai/Items/MordecaiPickupInteractionComponent.h"
#include "Mordecai/UI/TinTally/MordecaiUIPrimitives.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeBinding.h"
#include "Mordecai/UI/TinTally/MordecaiUITypography.h"

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

void UMordecaiPickupPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();
	MordecaiUIThemeBinding::Bind(
		this, BoundThemeSubsystem, ThemeChangedHandle,
		this, &UMordecaiPickupPromptWidget::ApplyTheme);
}

void UMordecaiPickupPromptWidget::NativeDestruct()
{
	Unbind();
	MordecaiUIThemeBinding::Unbind(BoundThemeSubsystem, ThemeChangedHandle);
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
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;
	const FMordecaiUIThemeColors Colors = FMordecaiUIThemeColors::MakeDiorama();
	auto Place = [Root](UWidget* Widget, FVector2D Position, FVector2D Size, int32 ZOrder = 0)
		-> UCanvasPanelSlot*
	{
		if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Widget))
		{
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
			Slot->SetAutoSize(false);
			Slot->SetZOrder(ZOrder);
			return Slot;
		}
		return nullptr;
	};

	UOverlay* Tag = FMordecaiUIPrimitiveBuilder::Build(
		WidgetTree,
		FMordecaiUIPrimitiveBuilder::Tag(
			Colors, EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::Diorama),
		TEXT("PromptTag"));
	PromptTag = Tag;
	if (UCanvasPanelSlot* TagSlot = Place(
		Tag, FVector2D(-180.f, 0.f), FVector2D(540.f, 60.f), 0))
	{
		TagSlot->SetAnchors(FAnchors(1.f, 0.5f));
		TagSlot->SetAlignment(FVector2D(1.f, 0.5f));
		TagSlot->SetAutoSize(true);
	}

	PromptText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PromptText"));
	PromptText->SetJustification(ETextJustify::Right);
	PromptText->SetColorAndOpacity(FSlateColor(Colors.Ink));
	PromptText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
	FSlateFontInfo PromptFont = PromptText->GetFont();
	PromptFont.Size = 17;
	PromptText->SetFont(PromptFont);
	USizeBox* PromptTextBox = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("PromptTextBox"));
	PromptTextBox->SetMaxDesiredWidth(484.f);
	PromptTextBox->SetMinDesiredHeight(60.f);
	PromptTextBox->AddChild(PromptText);
	if (UOverlaySlot* TextSlot = Tag->AddChildToOverlay(PromptTextBox))
	{
		TextSlot->SetPadding(FMargin(40.f, 0.f, 16.f, 0.f));
		TextSlot->SetHorizontalAlignment(HAlign_Right);
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}

	PromptSocket = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(), TEXT("PromptSocket"));
	PromptSocket->SetVisibility(ESlateVisibility::Collapsed);
	Place(PromptSocket, FVector2D(500.f, 19.f), FVector2D(22.f), 2);
	InteractKey = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(), TEXT("InteractKey"));
	InteractKey->SetVisibility(ESlateVisibility::Collapsed);
	Place(InteractKey, FVector2D(40.f, 10.f), FVector2D(40.f), 2);

	UOverlay* Pill = FMordecaiUIPrimitiveBuilder::Build(
		WidgetTree,
		FMordecaiUIPrimitiveBuilder::Pill(
			Colors, EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::Diorama),
		TEXT("HintPill"));
	HintPill = Pill;
	Place(Pill, FVector2D(552.f, 10.f), FVector2D(151.f, 40.f), 0);
	InventoryHintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryHintText"));
	InventoryHintText->SetJustification(ETextJustify::Center);
	InventoryHintText->SetColorAndOpacity(FSlateColor(Colors.TextMuted));
	InventoryHintText->SetText(GetInventoryHintText());
	FSlateFontInfo HintFont = InventoryHintText->GetFont();
	HintFont.Size = 15;
	InventoryHintText->SetFont(HintFont);
	Place(InventoryHintText, FVector2D(564.f, 18.f), FVector2D(127.f, 24.f), 1);
	HintKey = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("HintKey"));
	HintKey->SetVisibility(ESlateVisibility::Collapsed);
	Place(HintKey, FVector2D(560.f, 14.f), FVector2D(32.f), 2);
}

void UMordecaiPickupPromptWidget::SetGalleryPreviewPrompt(const FText& Prompt)
{
	CachedPromptText = Prompt;
	bPromptVisible = !Prompt.IsEmpty();
	RefreshVisuals();
}

void UMordecaiPickupPromptWidget::RefreshVisuals()
{
	if (PromptText)
	{
		PromptText->SetText(CachedPromptText);
		PromptText->SetVisibility(bPromptVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (PromptTag)
	{
		PromptTag->SetVisibility(
			bPromptVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (InventoryHintText)
	{
		InventoryHintText->SetText(GetInventoryHintText());
		InventoryHintText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (HintPill)
	{
		HintPill->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UMordecaiPickupPromptWidget::ApplyTheme(
	EMordecaiUITheme Theme,
	const FMordecaiUIThemeColors& Colors)
{
	CurrentTheme = Theme;
	CurrentThemeColors = Colors;
	if (UOverlay* Tag = Cast<UOverlay>(PromptTag))
	{
		FMordecaiUIPrimitiveBuilder::Apply(
			Tag,
			FMordecaiUIPrimitiveBuilder::Tag(
				Colors, EMordecaiUIPrintDepth::HUD, Theme));
	}
	if (UOverlay* Pill = Cast<UOverlay>(HintPill))
	{
		FMordecaiUIPrimitiveBuilder::Apply(
			Pill,
			FMordecaiUIPrimitiveBuilder::Pill(
				Colors, EMordecaiUIPrintDepth::HUD, Theme));
	}
	MordecaiUITypography::Apply(PromptText, TEXT("ui_body"));
	if (PromptText)
	{
		FSlateFontInfo Font = PromptText->GetFont();
		Font.TypefaceFontName = TEXT("Bold");
		PromptText->SetFont(Font);
		PromptText->SetColorAndOpacity(FSlateColor(Colors.Ink));
	}
	MordecaiUITypography::Apply(InventoryHintText, TEXT("ui_label"));
	if (InventoryHintText)
	{
		InventoryHintText->SetColorAndOpacity(FSlateColor(Colors.TextMuted));
	}
	RefreshVisuals();
}

#undef LOCTEXT_NAMESPACE
