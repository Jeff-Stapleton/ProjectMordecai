// Project Mordecai — Combat Feedback Widget (US-061)

#include "Mordecai/UI/MordecaiCombatFeedbackWidget.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiCombatFeedbackWidget)

// ---------------------------------------------------------------------------
// Programmatic Fallback (empty Blueprint support)
// ---------------------------------------------------------------------------

void UMordecaiCombatFeedbackWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!FeedbackText && WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultContent();
	}
}

void UMordecaiCombatFeedbackWidget::BuildDefaultContent()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	FeedbackText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FeedbackText"));
	UCanvasPanelSlot* TextSlot = Root->AddChildToCanvas(FeedbackText);
	TextSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	TextSlot->SetOffsets(FMargin(0.f));

	FeedbackText->SetText(FText::GetEmpty());
	FeedbackText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FeedbackText->SetJustification(ETextJustify::Center);

	FSlateFontInfo FontInfo = FeedbackText->GetFont();
	FontInfo.Size = 20;
	FeedbackText->SetFont(FontInfo);
}

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
