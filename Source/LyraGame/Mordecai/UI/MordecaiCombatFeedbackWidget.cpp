// Project Mordecai — Combat Feedback Widget (US-061)

#include "Mordecai/UI/MordecaiCombatFeedbackWidget.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Components/TextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiCombatFeedbackWidget)

// ---------------------------------------------------------------------------
// Static color constants
// ---------------------------------------------------------------------------
namespace MordecaiFeedbackColors
{
	static const FLinearColor Gold(1.f, 0.84f, 0.f, 1.f);
	static const FLinearColor Red(1.f, 0.2f, 0.2f, 1.f);
	static const FLinearColor White(1.f, 1.f, 1.f, 1.f);
}

// ---------------------------------------------------------------------------
// Static Helpers
// ---------------------------------------------------------------------------

FString UMordecaiCombatFeedbackWidget::GetFeedbackTextForTag(const FGameplayTag& Tag)
{
	if (Tag == MordecaiGameplayTags::State_PerfectDodge)
	{
		return TEXT("Perfect Dodge!");
	}
	if (Tag == MordecaiGameplayTags::State_PerfectBlock)
	{
		return TEXT("Perfect Block!");
	}
	if (Tag == MordecaiGameplayTags::State_Parried)
	{
		return TEXT("Parried!");
	}
	if (Tag == MordecaiGameplayTags::State_ParryWhiff)
	{
		return TEXT("Parry Failed!");
	}
	if (Tag == MordecaiGameplayTags::State_PostureBroken)
	{
		return TEXT("Posture Broken!");
	}
	return FString();
}

bool UMordecaiCombatFeedbackWidget::IsNegativeFeedback(const FGameplayTag& Tag)
{
	return Tag == MordecaiGameplayTags::State_ParryWhiff;
}

FLinearColor UMordecaiCombatFeedbackWidget::GetFeedbackColorForTag(const FGameplayTag& Tag)
{
	if (IsNegativeFeedback(Tag))
	{
		return MordecaiFeedbackColors::Red;
	}

	// Check if it's a known feedback tag at all
	const FString Text = GetFeedbackTextForTag(Tag);
	if (Text.IsEmpty())
	{
		return MordecaiFeedbackColors::White;
	}

	return MordecaiFeedbackColors::Gold;
}

// ---------------------------------------------------------------------------
// ASC Binding
// ---------------------------------------------------------------------------

void UMordecaiCombatFeedbackWidget::BindToASC(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	BoundASC = ASC;

	// Register for all feedback-relevant tags
	const FGameplayTag FeedbackTags[] = {
		MordecaiGameplayTags::State_PerfectDodge,
		MordecaiGameplayTags::State_PerfectBlock,
		MordecaiGameplayTags::State_Parried,
		MordecaiGameplayTags::State_ParryWhiff,
		MordecaiGameplayTags::State_PostureBroken,
	};

	for (const FGameplayTag& Tag : FeedbackTags)
	{
		ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UMordecaiCombatFeedbackWidget::OnFeedbackTagChanged);
	}

	// Start hidden
	SetVisibility(ESlateVisibility::Collapsed);
}

void UMordecaiCombatFeedbackWidget::OnFeedbackTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		const FString Text = GetFeedbackTextForTag(Tag);
		if (!Text.IsEmpty())
		{
			ShowFeedback(Text, GetFeedbackColorForTag(Tag));
		}
	}
}

void UMordecaiCombatFeedbackWidget::ShowFeedback(const FString& Text, const FLinearColor& Color)
{
	if (FeedbackText)
	{
		FeedbackText->SetText(FText::FromString(Text));
		FeedbackText->SetColorAndOpacity(FSlateColor(Color));
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	RemainingDisplayTime = FeedbackDisplayDuration;
}

// ---------------------------------------------------------------------------
// Tick — auto-hide after display duration
// ---------------------------------------------------------------------------

void UMordecaiCombatFeedbackWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (RemainingDisplayTime > 0.f)
	{
		RemainingDisplayTime -= InDeltaTime;
		if (RemainingDisplayTime <= 0.f)
		{
			SetVisibility(ESlateVisibility::Collapsed);
			RemainingDisplayTime = 0.f;
		}
	}
}
