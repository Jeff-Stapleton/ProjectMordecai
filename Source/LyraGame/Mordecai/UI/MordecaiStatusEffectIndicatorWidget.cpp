// Project Mordecai — Status Effect Indicator Widget (US-055)

#include "Mordecai/UI/MordecaiStatusEffectIndicatorWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectTypes.h"
#include "Mordecai/UI/TinTally/MordecaiUITypography.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeBinding.h"
#include "Styling/CoreStyle.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiStatusEffectIndicatorWidget)

namespace
{
	FSlateBrush RoundedBrush(
		const FLinearColor& Fill,
		const FLinearColor& Outline,
		const float OutlineWidth,
		const FVector4& Radius)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings =
			FSlateBrushOutlineSettings(Radius, FSlateColor(Outline), OutlineWidth);
		return Brush;
	}

	const FText& CachedDurationText(const int32 Tenths)
	{
		static const TArray<FText> Cache = []
		{
			TArray<FText> Result;
			Result.Reserve(1000);
			for (int32 Value = 0; Value < 1000; ++Value)
			{
				Result.Add(FText::FromString(FString::Printf(
					TEXT("%d.%d"), Value / 10, Value % 10)));
			}
			return Result;
		}();
		return Cache[FMath::Clamp(Tenths, 0, Cache.Num() - 1)];
	}
}

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

void UMordecaiStatusEffectIndicatorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	MordecaiUIThemeBinding::Bind(
		this, BoundThemeSubsystem, ThemeChangedHandle,
		this, &ThisClass::ApplyTheme);
}

void UMordecaiStatusEffectIndicatorWidget::NativeDestruct()
{
	MordecaiUIThemeBinding::Unbind(BoundThemeSubsystem, ThemeChangedHandle);
	Super::NativeDestruct();
}

void UMordecaiStatusEffectIndicatorWidget::BuildDefaultContent()
{
	ChipSizeBox = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("ChipSizeBox"));
	ChipSizeBox->SetWidthOverride(FullChipWidth);
	ChipSizeBox->SetHeightOverride(36.f);
	WidgetTree->RootWidget = ChipSizeBox;
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("Root"));
	ChipSizeBox->AddChild(Root);

	auto Place = [Root](
		UWidget* Widget,
		const FVector2D Position,
		const FVector2D Size,
		const int32 ZOrder = 0)
	{
		if (UCanvasPanelSlot* CanvasSlot = Root->AddChildToCanvas(Widget))
		{
			CanvasSlot->SetPosition(Position);
			CanvasSlot->SetSize(Size);
			CanvasSlot->SetAutoSize(false);
			CanvasSlot->SetZOrder(ZOrder);
		}
	};

	TintImage = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(), TEXT("TintImage"));
	Place(TintImage, FVector2D::ZeroVector, FVector2D(FullChipWidth, 36.f), 0);

	BuffTab = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("BuffTab"));
	BuffTab->SetPadding(FMargin(0.f));
	Place(BuffTab, FVector2D(4.f, -10.f), FVector2D(30.f, 10.f), 1);
	DebuffTeeth = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("DebuffTeeth"));
	DebuffTeeth->SetPadding(FMargin(0.f));
	Place(DebuffTeeth, FVector2D(4.f, 36.f), FVector2D(FullChipWidth - 8.f, 7.f), 1);

	StatusGlyph = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(), TEXT("StatusGlyph"));
	Place(StatusGlyph, FVector2D(8.f, 6.f), FVector2D(24.f), 2);

	auto AddCenteredText = [this, Root, &Place](
		const TCHAR* BoxName,
		const TCHAR* TextName,
		const FVector2D Position,
		const FVector2D Size) -> UTextBlock*
	{
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), FName(BoxName));
		Box->SetMinDesiredHeight(Size.Y);
		Place(Box, Position, Size, 2);
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), FName(TextName));
		Text->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
		if (USizeBoxSlot* TextSlot = Cast<USizeBoxSlot>(Box->AddChild(Text)))
		{
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}
		return Text;
	};

	StatusNameText = AddCenteredText(
		TEXT("StatusNameBox"), TEXT("StatusNameText"),
		FVector2D(38.f, 0.f), FVector2D(54.f, 36.f));
	MordecaiUITypography::Apply(StatusNameText, TEXT("ui_label"));
	FSlateFontInfo NameFont = StatusNameText->GetFont();
	NameFont.TypefaceFontName = TEXT("SemiBold");
	NameFont.Size = 15;
	StatusNameText->SetFont(NameFont);

	StackPips = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(), TEXT("StackPips"));
	Place(StackPips, FVector2D(96.f, 11.f), FVector2D(0.f, 14.f), 2);

	DurationText = AddCenteredText(
		TEXT("DurationBox"), TEXT("DurationText"),
		FVector2D(FullChipWidth - 46.f, 0.f), FVector2D(38.f, 36.f));
	DurationText->SetJustification(ETextJustify::Left);
	MordecaiUITypography::Apply(DurationText, TEXT("num_s"));
	FSlateFontInfo DurFont = DurationText->GetFont();
	DurFont.Size = 15;
	DurationText->SetFont(DurFont);
	ApplyTheme(EMordecaiUITheme::Diorama, FMordecaiUIThemeColors::MakeDiorama());
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
	// InStatusTag's display row owns polarity. The bool remains only as the
	// backwards-compatible fallback for an unknown/internal status.
	bIsBuffIndicator = bIsBuff;
	TotalDuration = InDuration;
	DurationBoxWidth = TotalDuration >= 10.f ? 51.f : 38.f;
	FixedValueText = FText::GetEmpty();
	LastDurationTenths = INDEX_NONE;

	const FMordecaiStatusDisplayData* DisplayData =
		FindStatusEffectDisplayData(StatusTag);
	if (DisplayData)
	{
		bIsBuffIndicator =
			DisplayData->DisplayPolarity == EMordecaiStatusDisplayPolarity::Buff;
		GlyphColorToken = DisplayData->GlyphColorToken;
		MaxVisibleStacks = DisplayData->MaxVisibleStacks;
		FixedValueText = DisplayData->FixedValueText;
		if (StatusNameText)
		{
			StatusNameText->SetText(DisplayData->DisplayName);
		}
		const FString GlyphPath = FString::Printf(
			TEXT("/MordecaiCore/UI/TinTally/Textures/Glyphs/T_UI_Glyph_%s.T_UI_Glyph_%s"),
			*DisplayData->GlyphName.ToString(),
			*DisplayData->GlyphName.ToString());
		if (StatusGlyph)
		{
			StatusGlyph->SetBrushFromTexture(
				LoadObject<UTexture2D>(nullptr, *GlyphPath), true);
		}
		const TSharedRef<FSlateFontMeasure> FontMeasure =
			FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		const FSlateFontInfo MeasureFont =
			FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 15);
		const float NameWidth = FMath::Clamp(
			FMath::CeilToFloat(FontMeasure->Measure(
				DisplayData->DisplayName, MeasureFont).X),
			50.f, 130.f);
		const float PipsWidth = MaxVisibleStacks > 0
			? MaxVisibleStacks * 14.f + (MaxVisibleStacks - 1) * 6.f
			: 0.f;
		const float PipsAndGap = PipsWidth > 0.f ? PipsWidth + 6.f : 0.f;
		const float DurationWidthDelta = DurationBoxWidth - 38.f;
		FullChipWidth =
			84.f + NameWidth + PipsAndGap + DurationWidthDelta;
		CompactChipWidth =
			90.f + PipsAndGap + DurationWidthDelta;
	}

	RebuildPips();
	SetStackCount(MaxVisibleStacks > 0 ? 1 : 0);
	SetCompact(bCompact);
	ApplyTheme(
		BoundThemeSubsystem.IsValid()
			? BoundThemeSubsystem->GetActiveTheme()
			: EMordecaiUITheme::Diorama,
		BoundThemeSubsystem.IsValid()
			? BoundThemeSubsystem->GetActiveColors()
			: FMordecaiUIThemeColors::MakeDiorama());
	UpdateDuration(InDuration);
}

void UMordecaiStatusEffectIndicatorWidget::UpdateDuration(float RemainingSeconds)
{
	const int32 Tenths = RemainingSeconds > 0.f
		? FMath::Max(0, FMath::CeilToInt(RemainingSeconds * 10.f))
		: 0;
	if (DurationText && Tenths != LastDurationTenths)
	{
		DurationText->SetText(!FixedValueText.IsEmpty()
			? FixedValueText
			: Tenths > 0 ? CachedDurationText(Tenths) : FText::GetEmpty());
		LastDurationTenths = Tenths;
	}
}

void UMordecaiStatusEffectIndicatorWidget::SetStackCount(const int32 StackCount)
{
	const int32 Clamped = FMath::Clamp(StackCount, 0, MaxVisibleStacks);
	if (Clamped == CurrentStackCount && StackPips
		&& StackPips->GetChildrenCount() == MaxVisibleStacks)
	{
		return;
	}
	CurrentStackCount = Clamped;
	for (int32 Index = 0; StackPips && Index < StackPips->GetChildrenCount(); ++Index)
	{
		if (UImage* Pip = Cast<UImage>(StackPips->GetChildAt(Index)))
		{
			FLinearColor StatusColor = CurrentThemeColors.Text;
			CurrentThemeColors.TryGetColorToken(GlyphColorToken, StatusColor);
			Pip->SetBrush(RoundedBrush(
				Index < CurrentStackCount ? StatusColor : FLinearColor::Transparent,
				Index < CurrentStackCount ? StatusColor : CurrentThemeColors.Lip,
				2.f,
				FVector4(7.f)));
		}
	}
}

void UMordecaiStatusEffectIndicatorWidget::SetCompact(const bool bInCompact)
{
	bCompact = bInCompact;
	if (StatusNameText)
	{
		StatusNameText->SetVisibility(
			bCompact ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	RefreshChipWidth();
}

float UMordecaiStatusEffectIndicatorWidget::GetChipWidth(const bool bForCompact) const
{
	return bForCompact ? CompactChipWidth : FullChipWidth;
}

void UMordecaiStatusEffectIndicatorWidget::RebuildPips()
{
	if (!StackPips)
	{
		return;
	}
	StackPips->ClearChildren();
	for (int32 Index = 0; Index < MaxVisibleStacks; ++Index)
	{
		UImage* Pip = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			NAME_None);
		FSlateBrush Brush;
		Brush.ImageSize = FVector2D(14.f);
		Pip->SetBrush(Brush);
		if (UHorizontalBoxSlot* PipSlot = StackPips->AddChildToHorizontalBox(Pip))
		{
			PipSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			PipSlot->SetPadding(FMargin(0.f, 0.f, Index + 1 < MaxVisibleStacks ? 6.f : 0.f, 0.f));
		}
	}
}

void UMordecaiStatusEffectIndicatorWidget::RefreshChipWidth()
{
	const float Width = GetChipWidth(bCompact);
	if (ChipSizeBox)
	{
		ChipSizeBox->SetWidthOverride(Width);
	}
	auto Resize = [this](UWidget* Widget, const FVector2D& Position, const FVector2D& Size)
	{
		if (UCanvasPanelSlot* CanvasSlot =
			Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr)
		{
			CanvasSlot->SetPosition(Position);
			CanvasSlot->SetSize(Size);
		}
	};
	Resize(TintImage, FVector2D::ZeroVector, FVector2D(Width, 36.f));
	Resize(DebuffTeeth, FVector2D(4.f, 36.f), FVector2D(Width - 8.f, 7.f));
	const float PipsWidth = MaxVisibleStacks > 0
		? MaxVisibleStacks * 14.f + (MaxVisibleStacks - 1) * 6.f
		: 0.f;
	const float PipsAndGap = PipsWidth > 0.f ? PipsWidth + 6.f : 0.f;
	const float NameWidth = FMath::Max(0.f, Width - 84.f - PipsAndGap);
	const float DurationX = Width - 8.f - DurationBoxWidth;
	const float PipsX = DurationX - 6.f - PipsWidth;
	Resize(
		StatusNameText ? StatusNameText->GetParent() : nullptr,
		FVector2D(32.f, 0.f),
		FVector2D(NameWidth, 36.f));
	Resize(StackPips, FVector2D(PipsX, 11.f), FVector2D(PipsWidth, 14.f));
	Resize(
		DurationText ? DurationText->GetParent() : nullptr,
		FVector2D(DurationX, 0.f),
		FVector2D(DurationBoxWidth, 36.f));
}

void UMordecaiStatusEffectIndicatorWidget::ApplyTheme(
	const EMordecaiUITheme Theme,
	const FMordecaiUIThemeColors& Colors)
{
	CurrentThemeColors = Colors;
	const float Outline = Theme == EMordecaiUITheme::HighContrast ? 4.f : 3.f;
	if (TintImage)
	{
		TintImage->SetBrush(RoundedBrush(
			bIsBuffIndicator ? Colors.Enamel : Colors.Plate,
			Colors.Ink,
			Outline,
			bIsBuffIndicator ? FVector4(10.f) : FVector4(10.f, 10.f, 3.f, 3.f)));
		TintImage->SetColorAndOpacity(FLinearColor::White);
	}
	if (BuffTab)
	{
		BuffTab->SetBrush(RoundedBrush(
			Colors.Buff, Colors.Ink, Outline, FVector4(4.f, 4.f, 0.f, 0.f)));
		BuffTab->SetVisibility(
			bIsBuffIndicator ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (DebuffTeeth)
	{
		DebuffTeeth->SetBrush(RoundedBrush(
			Colors.Debuff, Colors.Ink, Outline, FVector4(0.f, 0.f, 3.f, 3.f)));
		DebuffTeeth->SetVisibility(
			bIsBuffIndicator ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (StatusNameText)
	{
		StatusNameText->SetColorAndOpacity(FSlateColor(
			bIsBuffIndicator ? Colors.Ink : Colors.Text));
	}
	if (DurationText)
	{
		DurationText->SetColorAndOpacity(FSlateColor(
			bIsBuffIndicator ? Colors.Ink : Colors.Text));
	}
	if (StatusGlyph)
	{
		FLinearColor GlyphColor = bIsBuffIndicator ? Colors.Ink : Colors.Text;
		Colors.TryGetColorToken(GlyphColorToken, GlyphColor);
		StatusGlyph->SetColorAndOpacity(GlyphColor);
	}
	const int32 PreviousStackCount = CurrentStackCount;
	CurrentStackCount = INDEX_NONE;
	SetStackCount(PreviousStackCount);
}
