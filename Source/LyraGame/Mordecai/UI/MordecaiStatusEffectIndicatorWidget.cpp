// Project Mordecai — Status Effect Indicator Widget (US-055)

#include "Mordecai/UI/MordecaiStatusEffectIndicatorWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiStatusEffectIndicatorWidget)

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
