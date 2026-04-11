// Project Mordecai — Status Effect Indicator Widget (US-055)

#include "Mordecai/UI/MordecaiStatusEffectIndicatorWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiStatusEffectIndicatorWidget)

// ---------------------------------------------------------------------------
// Programmatic Fallback (empty Blueprint support)
// ---------------------------------------------------------------------------

void UMordecaiStatusEffectIndicatorWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!StatusNameText && WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultContent();
	}
}

void UMordecaiStatusEffectIndicatorWidget::BuildDefaultContent()
{
	UHorizontalBox* Root = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	StatusNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusNameText"));
	UHorizontalBoxSlot* NameSlot = Root->AddChildToHorizontalBox(StatusNameText);
	NameSlot->SetPadding(FMargin(2.f, 0.f));

	StatusNameText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo NameFont = StatusNameText->GetFont();
	NameFont.Size = 10;
	StatusNameText->SetFont(NameFont);

	DurationText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DurationText"));
	UHorizontalBoxSlot* DurSlot = Root->AddChildToHorizontalBox(DurationText);
	DurSlot->SetPadding(FMargin(2.f, 0.f));

	DurationText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f)));
	FSlateFontInfo DurFont = DurationText->GetFont();
	DurFont.Size = 9;
	DurationText->SetFont(DurFont);
}

// ---------------------------------------------------------------------------
// Static Helper
// ---------------------------------------------------------------------------

float UMordecaiStatusEffectIndicatorWidget::CalcDurationRemaining(float StartTime, float Duration, float CurrentTime)
{
	if (Duration <= 0.f)
	{
		return 0.f;
	}
	return FMath::Max(0.f, (StartTime + Duration) - CurrentTime);
}

// ---------------------------------------------------------------------------
// Instance Methods (AC-055.4)
// ---------------------------------------------------------------------------

void UMordecaiStatusEffectIndicatorWidget::InitializeIndicator(FGameplayTag InStatusTag, bool bIsBuff, float InDuration)
{
	StatusTag = InStatusTag;
	bIsBuffIndicator = bIsBuff;
	TotalDuration = InDuration;

	// Set display name from the last segment of the tag (e.g., "Mordecai.Status.Burning" -> "Burning")
	if (StatusNameText)
	{
		const FString TagString = StatusTag.ToString();
		FString DisplayName;
		TagString.Split(TEXT("."), nullptr, &DisplayName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		if (DisplayName.IsEmpty())
		{
			DisplayName = TagString;
		}
		StatusNameText->SetText(FText::FromString(DisplayName));
	}

	// Set tint color: green for buff, red for debuff
	if (TintImage)
	{
		const FLinearColor TintColor = bIsBuff
			? FLinearColor(0.f, 1.f, 0.f, 1.f)
			: FLinearColor(1.f, 0.f, 0.f, 1.f);
		TintImage->SetColorAndOpacity(TintColor);
	}

	UpdateDuration(InDuration);
}

void UMordecaiStatusEffectIndicatorWidget::UpdateDuration(float RemainingSeconds)
{
	if (DurationText)
	{
		if (RemainingSeconds > 0.f)
		{
			DurationText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), RemainingSeconds)));
		}
		else
		{
			DurationText->SetText(FText::GetEmpty());
		}
	}
}
