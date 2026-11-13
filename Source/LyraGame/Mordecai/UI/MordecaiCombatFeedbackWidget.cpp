// Project Mordecai — Combat Feedback Widget (US-061)

#include "Mordecai/UI/MordecaiCombatFeedbackWidget.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Mordecai/UI/TinTally/MordecaiUITypography.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeBinding.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiCombatFeedbackWidget)

namespace
{
	FSlateBrush RoundedBrush(
		const FLinearColor& Fill,
		const FLinearColor& Outline = FLinearColor::Transparent,
		const float OutlineWidth = 0.f)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(
			FVector4(12.f), FSlateColor(Outline), OutlineWidth);
		return Brush;
	}

	const FText& FeedbackDisplayText(const FGameplayTag& Tag)
	{
		static const FText PerfectDodge =
			NSLOCTEXT("MordecaiCombatFeedback", "PerfectDodge", "Perfect Dodge!");
		static const FText PerfectBlock =
			NSLOCTEXT("MordecaiCombatFeedback", "PerfectBlock", "Perfect Block!");
		static const FText Parried =
			NSLOCTEXT("MordecaiCombatFeedback", "Parried", "Parried!");
		static const FText ParryFailed =
			NSLOCTEXT("MordecaiCombatFeedback", "ParryFailed", "Parry Failed!");
		static const FText PostureBroken =
			NSLOCTEXT("MordecaiCombatFeedback", "PostureBroken", "Posture Broken!");
		static const FText Empty = FText::GetEmpty();
		if (Tag == MordecaiGameplayTags::State_PerfectDodge) return PerfectDodge;
		if (Tag == MordecaiGameplayTags::State_PerfectBlock) return PerfectBlock;
		if (Tag == MordecaiGameplayTags::State_Parried) return Parried;
		if (Tag == MordecaiGameplayTags::State_ParryWhiff) return ParryFailed;
		if (Tag == MordecaiGameplayTags::State_PostureBroken) return PostureBroken;
		return Empty;
	}
}

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

void UMordecaiCombatFeedbackWidget::NativeConstruct()
{
	Super::NativeConstruct();
	MordecaiUIThemeBinding::Bind(
		this, BoundThemeSubsystem, ThemeChangedHandle,
		this, &ThisClass::ApplyTheme);
}

void UMordecaiCombatFeedbackWidget::NativeDestruct()
{
	MordecaiUIThemeBinding::Unbind(BoundThemeSubsystem, ThemeChangedHandle);
	Super::NativeDestruct();
}

void UMordecaiCombatFeedbackWidget::BuildDefaultContent()
{
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(
		UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* PlateInk = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("CalloutPlateInk"));
	PlateInk->SetPadding(FMargin(0.f));
	PlateInk->SetHorizontalAlignment(HAlign_Center);
	PlateInk->SetVerticalAlignment(VAlign_Center);
	if (UOverlaySlot* InkSlot = Root->AddChildToOverlay(PlateInk))
	{
		InkSlot->SetHorizontalAlignment(HAlign_Center);
		InkSlot->SetVerticalAlignment(VAlign_Bottom);
		InkSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	}

	CalloutPlate = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("CalloutPlate"));
	CalloutPlate->SetHorizontalAlignment(HAlign_Center);
	CalloutPlate->SetVerticalAlignment(VAlign_Center);
	PlateInk->SetContent(CalloutPlate);
	FeedbackText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("FeedbackText"));
	FeedbackText->SetText(FText::GetEmpty());
	FeedbackText->SetJustification(ETextJustify::Center);
	FeedbackText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
	MordecaiUITypography::Apply(FeedbackText, TEXT("callout"));
	FSlateFontInfo FontInfo = FeedbackText->GetFont();
	FontInfo.TypefaceFontName = TEXT("Bold");
	FontInfo.Size = CurrentFontSizePt;
	FeedbackText->SetFont(FontInfo);
	CalloutPlate->SetContent(FeedbackText);
	ApplyTheme(EMordecaiUITheme::Diorama, FMordecaiUIThemeColors::MakeDiorama());
}

// ---------------------------------------------------------------------------
// Static Helpers
// ---------------------------------------------------------------------------

FString UMordecaiCombatFeedbackWidget::GetFeedbackTextForTag(const FGameplayTag& Tag)
{
	return FeedbackDisplayText(Tag).ToString();
}

bool UMordecaiCombatFeedbackWidget::IsNegativeFeedback(const FGameplayTag& Tag)
{
	return Tag == MordecaiGameplayTags::State_ParryWhiff;
}

FLinearColor UMordecaiCombatFeedbackWidget::GetFeedbackColorForTag(const FGameplayTag& Tag)
{
	if (IsNegativeFeedback(Tag))
	{
		return FMordecaiUIThemeColors::MakeDiorama().DangerText;
	}

	// Check if it's a known feedback tag at all
	if (FeedbackDisplayText(Tag).IsEmpty())
	{
		return FMordecaiUIThemeColors::MakeDiorama().Text;
	}

	return FMordecaiUIThemeColors::MakeDiorama().Good;
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

	if (UAbilitySystemComponent* Previous = BoundASC.Get())
	{
		const FGameplayTag PreviousTags[] = {
			MordecaiGameplayTags::State_PerfectDodge,
			MordecaiGameplayTags::State_PerfectBlock,
			MordecaiGameplayTags::State_Parried,
			MordecaiGameplayTags::State_ParryWhiff,
			MordecaiGameplayTags::State_PostureBroken,
		};
		for (const FGameplayTag& Tag : PreviousTags)
		{
			Previous->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
		}
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
		const FText& Text = FeedbackDisplayText(Tag);
		if (!Text.IsEmpty())
		{
			bCurrentFeedbackNegative = IsNegativeFeedback(Tag);
			ShowFeedbackText(
				Text,
				bCurrentFeedbackNegative
					? CurrentThemeColors.DangerText
					: CurrentThemeColors.Good);
		}
	}
}

void UMordecaiCombatFeedbackWidget::ShowFeedback(const FString& Text, const FLinearColor& Color)
{
	bCurrentFeedbackNegative = Color.Equals(CurrentThemeColors.DangerText);
	ShowFeedbackText(FText::FromString(Text), Color);
}

void UMordecaiCombatFeedbackWidget::ShowFeedbackText(
	const FText& Text,
	const FLinearColor& Color)
{
	if (FeedbackText)
	{
		FeedbackText->SetText(Text);
		FeedbackText->SetColorAndOpacity(FSlateColor(Color));
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	RemainingDisplayTime = FeedbackDisplayDuration;
}

void UMordecaiCombatFeedbackWidget::SetCalloutFontSize(const float FontSizePt)
{
	CurrentFontSizePt = FontSizePt;
	if (FeedbackText)
	{
		FSlateFontInfo Font = FeedbackText->GetFont();
		Font.Size = FontSizePt;
		FeedbackText->SetFont(Font);
	}
}

void UMordecaiCombatFeedbackWidget::ApplyTheme(
	const EMordecaiUITheme Theme,
	const FMordecaiUIThemeColors& Colors)
{
	CurrentTheme = Theme;
	CurrentThemeColors = Colors;
	UBorder* PlateInk = WidgetTree
		? Cast<UBorder>(WidgetTree->FindWidget(TEXT("CalloutPlateInk")))
		: nullptr;
	if (Theme == EMordecaiUITheme::HighContrast)
	{
		if (PlateInk)
		{
			PlateInk->SetBrush(RoundedBrush(Colors.Ink));
			PlateInk->SetPadding(FMargin(4.f));
		}
		if (CalloutPlate)
		{
			CalloutPlate->SetBrush(RoundedBrush(Colors.Plate, Colors.Rim, 2.f));
			CalloutPlate->SetPadding(FMargin(8.f, 2.f));
		}
	}
	else
	{
		FSlateBrush NoDraw;
		NoDraw.DrawAs = ESlateBrushDrawType::NoDrawType;
		if (PlateInk)
		{
			PlateInk->SetBrush(NoDraw);
			PlateInk->SetPadding(FMargin(0.f));
		}
		if (CalloutPlate)
		{
			CalloutPlate->SetBrush(NoDraw);
			CalloutPlate->SetPadding(FMargin(0.f));
		}
	}
	if (FeedbackText)
	{
		FSlateFontInfo Font = FeedbackText->GetFont();
		Font.OutlineSettings.OutlineSize =
			Theme == EMordecaiUITheme::HighContrast ? 0 : 4;
		Font.OutlineSettings.OutlineColor = Colors.Ink.ToFColorSRGB();
		FeedbackText->SetFont(Font);
		FeedbackText->SetShadowOffset(
			Theme == EMordecaiUITheme::HighContrast
				? FVector2D::ZeroVector
				: FVector2D(0.f, 4.f));
		FeedbackText->SetShadowColorAndOpacity(
			Theme == EMordecaiUITheme::HighContrast
				? FLinearColor::Transparent
				: Colors.Ink);
		if (!FeedbackText->GetText().IsEmpty())
		{
			FeedbackText->SetColorAndOpacity(FSlateColor(
				bCurrentFeedbackNegative ? Colors.DangerText : Colors.Good));
		}
	}
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
