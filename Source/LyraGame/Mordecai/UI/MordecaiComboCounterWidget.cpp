// Project Mordecai — Combo Counter Widget (US-061)

#include "Mordecai/UI/MordecaiComboCounterWidget.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiComboCounterWidget)

// ---------------------------------------------------------------------------
// Programmatic Fallback (empty Blueprint support)
// ---------------------------------------------------------------------------

void UMordecaiComboCounterWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!ComboText && WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultContent();
	}
}

void UMordecaiComboCounterWidget::BuildDefaultContent()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	ComboText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ComboText"));
	UCanvasPanelSlot* TextSlot = Root->AddChildToCanvas(ComboText);
	TextSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	TextSlot->SetOffsets(FMargin(0.f));

	ComboText->SetText(FText::GetEmpty());
	ComboText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.84f, 0.f))); // Gold
	ComboText->SetJustification(ETextJustify::Center);

	FSlateFontInfo FontInfo = ComboText->GetFont();
	FontInfo.Size = 24;
	ComboText->SetFont(FontInfo);
}

// ---------------------------------------------------------------------------
// Static Helper
// ---------------------------------------------------------------------------

FString UMordecaiComboCounterWidget::FormatComboHit(int32 ComboIndex)
{
	if (ComboIndex < 0)
	{
		return FString();
	}
	return FString::Printf(TEXT("Hit %d"), ComboIndex + 1);
}

// ---------------------------------------------------------------------------
// Instance Methods
// ---------------------------------------------------------------------------

void UMordecaiComboCounterWidget::BindToASC(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	BoundASC = ASC;

	ASC->RegisterGameplayTagEvent(MordecaiGameplayTags::State_ComboWindow, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UMordecaiComboCounterWidget::OnComboWindowTagChanged);

	// Start hidden
	SetComboVisible(false);
}

void UMordecaiComboCounterWidget::SetComboIndex(int32 ComboIndex)
{
	CachedComboIndex = ComboIndex;
	UpdateDisplay();
}

void UMordecaiComboCounterWidget::SetComboVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UMordecaiComboCounterWidget::OnComboWindowTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		SetComboVisible(true);
	}
	else
	{
		// Combo window closed — hide after brief display
		SetComboVisible(false);
		CachedComboIndex = -1;
	}
}

void UMordecaiComboCounterWidget::UpdateDisplay()
{
	if (ComboText)
	{
		ComboText->SetText(FText::FromString(FormatComboHit(CachedComboIndex)));
	}
}
