// Project Mordecai — Spell Points Bar Widget (US-055)

#include "Mordecai/UI/MordecaiSpellPointsBarWidget.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Mordecai/UI/TinTally/MordecaiUIPrimitives.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeBinding.h"
#include "Mordecai/UI/TinTally/MordecaiUITypography.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiSpellPointsBarWidget)

// ---------------------------------------------------------------------------
// Programmatic Fallback (empty Blueprint support)
// ---------------------------------------------------------------------------

void UMordecaiSpellPointsBarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!SpellPointsProgressBar && WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultContent();
	}
}

void UMordecaiSpellPointsBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	MordecaiUIThemeBinding::Bind(
		this, BoundThemeSubsystem, ThemeChangedHandle,
		this, &UMordecaiSpellPointsBarWidget::ApplyTheme);
}

void UMordecaiSpellPointsBarWidget::NativeDestruct()
{
	MordecaiUIThemeBinding::Unbind(BoundThemeSubsystem, ThemeChangedHandle);
	Super::NativeDestruct();
}

void UMordecaiSpellPointsBarWidget::BuildDefaultContent()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;
	const FMordecaiUIThemeColors Colors = FMordecaiUIThemeColors::MakeDiorama();

	auto Place = [Root](UWidget* Widget, FVector2D Position, FVector2D Size)
	{
		if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Widget))
		{
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
			Slot->SetAutoSize(false);
		}
	};

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("SpellPointsLabel"));
	Label->SetText(FText::FromString(TEXT("SP")));
	Label->SetColorAndOpacity(FSlateColor(Colors.TextMuted));
	FSlateFontInfo LabelFont = Label->GetFont();
	LabelFont.Size = 15;
	Label->SetFont(LabelFont);
	Place(Label, FVector2D(0.f, 2.f), FVector2D(28.f, 24.f));

	UOverlay* Well = FMordecaiUIPrimitiveBuilder::Build(
		WidgetTree, FMordecaiUIPrimitiveBuilder::Well(Colors, true), TEXT("SpellPointsWell"));
	Place(Well, FVector2D(38.f, 5.f), FVector2D(268.f, 18.f));

	SpellPointsProgressBar = WidgetTree->ConstructWidget<UProgressBar>(
		UProgressBar::StaticClass(), TEXT("SpellPointsProgressBar"));
	FProgressBarStyle Style = SpellPointsProgressBar->GetWidgetStyle();
	FSlateBrush Background;
	Background.DrawAs = ESlateBrushDrawType::NoDrawType;
	FSlateBrush Fill;
	Fill.DrawAs = ESlateBrushDrawType::RoundedBox;
	Fill.OutlineSettings = FSlateBrushOutlineSettings(
		FVector4(3.f), FSlateColor(FLinearColor::Transparent), 0.f);
	Style.SetBackgroundImage(Background);
	Style.SetFillImage(Fill);
	SpellPointsProgressBar->SetWidgetStyle(Style);
	SpellPointsProgressBar->SetBarFillType(EProgressBarFillType::LeftToRight);
	SpellPointsProgressBar->SetPercent(1.f);
	SpellPointsProgressBar->SetFillColorAndOpacity(Colors.SpellPoints);
	Place(SpellPointsProgressBar, FVector2D(40.f, 7.f), FVector2D(264.f, 14.f));

	SpellPointTokens = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(), TEXT("SpellPointTokens"));
	Place(SpellPointTokens, FVector2D(38.f, 3.f), FVector2D(268.f, 22.f));

	SpellPointsText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("SpellPointsText"));
	SpellPointsText->SetText(FText::FromString(FormatSpellPoints(CachedSpellPoints, CachedMaxSpellPoints)));
	SpellPointsText->SetColorAndOpacity(FSlateColor(Colors.Text));
	SpellPointsText->SetJustification(ETextJustify::Right);
	FSlateFontInfo FontInfo = SpellPointsText->GetFont();
	FontInfo.Size = 18;
	SpellPointsText->SetFont(FontInfo);
	Place(SpellPointsText, FVector2D(316.f, 0.f), FVector2D(108.f, 28.f));
}

// ---------------------------------------------------------------------------
// Static Helpers (AC-055.1, AC-055.3)
// ---------------------------------------------------------------------------

float UMordecaiSpellPointsBarWidget::CalcSpellPointsPercent(float SpellPoints, float MaxSpellPoints)
{
	if (MaxSpellPoints <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(SpellPoints / MaxSpellPoints, 0.f, 1.f);
}

FString UMordecaiSpellPointsBarWidget::FormatSpellPoints(float SpellPoints, float MaxSpellPoints)
{
	const int32 Current = FMath::FloorToInt32(FMath::Max(SpellPoints, 0.f));
	const int32 Max = FMath::FloorToInt32(FMath::Max(MaxSpellPoints, 0.f));
	return FString::Printf(TEXT("%d / %d"), Current, Max);
}

// ---------------------------------------------------------------------------
// Instance Methods
// ---------------------------------------------------------------------------

float UMordecaiSpellPointsBarWidget::GetSpellPointsPercent() const
{
	return CalcSpellPointsPercent(CachedSpellPoints, CachedMaxSpellPoints);
}

void UMordecaiSpellPointsBarWidget::BindToASC(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	if (UAbilitySystemComponent* Previous = BoundASC.Get())
	{
		Previous->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetSpellPointsAttribute()).RemoveAll(this);
		Previous->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetMaxSpellPointsAttribute()).RemoveAll(this);
	}
	BoundASC = ASC;

	ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetSpellPointsAttribute())
		.AddUObject(this, &UMordecaiSpellPointsBarWidget::OnSpellPointsChanged);

	ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetMaxSpellPointsAttribute())
		.AddUObject(this, &UMordecaiSpellPointsBarWidget::OnMaxSpellPointsChanged);

	// Initialize from current values
	bool bFound = false;
	CachedSpellPoints = ASC->GetGameplayAttributeValue(UMordecaiAttributeSet::GetSpellPointsAttribute(), bFound);
	if (!bFound) { CachedSpellPoints = 0.f; }

	CachedMaxSpellPoints = ASC->GetGameplayAttributeValue(UMordecaiAttributeSet::GetMaxSpellPointsAttribute(), bFound);
	if (!bFound) { CachedMaxSpellPoints = 0.f; }

	UpdateDisplay();
}

void UMordecaiSpellPointsBarWidget::OnSpellPointsChanged(const FOnAttributeChangeData& Data)
{
	CachedSpellPoints = Data.NewValue;
	UpdateDisplay();
}

void UMordecaiSpellPointsBarWidget::OnMaxSpellPointsChanged(const FOnAttributeChangeData& Data)
{
	CachedMaxSpellPoints = Data.NewValue;
	UpdateDisplay();
}

void UMordecaiSpellPointsBarWidget::UpdateDisplay()
{
	const float Percent = GetSpellPointsPercent();
	if (SpellPointsProgressBar)
	{
		SpellPointsProgressBar->SetPercent(Percent);
	}
	if (SpellPointsText)
	{
		SpellPointsText->SetText(FText::FromString(FormatSpellPoints(CachedSpellPoints, CachedMaxSpellPoints)));
	}
	const int32 Current = FMath::FloorToInt(FMath::Max(0.f, CachedSpellPoints));
	const int32 Maximum = FMath::FloorToInt(FMath::Max(0.f, CachedMaxSpellPoints));
	const bool bUseTokens = Maximum > 0 && Maximum <= 16;
	// TODO(DECISION): US-090 owns the visual budget/fallback for SP maximums above 16.
	if (SpellPointTokens)
	{
		SpellPointTokens->SetVisibility(
			bUseTokens ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bUseTokens)
		{
			if (BuiltTokenMaximum != Maximum)
			{
				RebuildTokens(Current, Maximum);
			}
			else
			{
				RetintTokens(Current);
			}
		}
	}
	if (SpellPointsProgressBar)
	{
		SpellPointsProgressBar->SetVisibility(
			bUseTokens ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UMordecaiSpellPointsBarWidget::RebuildTokens(int32 Current, int32 Maximum)
{
	if (!SpellPointTokens)
	{
		return;
	}
	SpellPointTokens->ClearChildren();
	BuiltTokenMaximum = Maximum;
	const float TokenSize = Maximum <= 10 ? 22.f : Maximum <= 13 ? 16.f : 14.f;
	const float Gap = Maximum <= 10 ? 4.f : Maximum <= 13 ? 3.f : 2.f;
	const float GroupGap = Maximum <= 10 ? 12.f : Maximum <= 13 ? 8.f : 6.f;
	for (int32 Index = 0; Index < Maximum; ++Index)
	{
		UImage* Token = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			FName(*FString::Printf(TEXT("SpellPointToken_%02d"), Index)));
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.ImageSize = FVector2D(TokenSize);
		Brush.TintColor = FSlateColor(
			Index < Current ? CurrentThemeColors.SpellPoints : CurrentThemeColors.Well);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(
			FVector4(TokenSize * 0.5f), FSlateColor(
				Index < Current ? CurrentThemeColors.Ink : CurrentThemeColors.Lip), 2.f);
		Token->SetBrush(Brush);
		Token->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UHorizontalBoxSlot* TokenSlot = SpellPointTokens->AddChildToHorizontalBox(Token))
		{
			const bool bStartsGroup = Index > 0 && Index % 5 == 0;
			TokenSlot->SetPadding(FMargin(bStartsGroup ? GroupGap : (Index > 0 ? Gap : 0.f), 0.f, 0.f, 0.f));
			TokenSlot->SetVerticalAlignment(VAlign_Center);
			TokenSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}
}

void UMordecaiSpellPointsBarWidget::RetintTokens(int32 Current)
{
	if (!SpellPointTokens)
	{
		return;
	}
	for (int32 Index = 0; Index < SpellPointTokens->GetChildrenCount(); ++Index)
	{
		if (UImage* Token = Cast<UImage>(SpellPointTokens->GetChildAt(Index)))
		{
			FSlateBrush Brush = Token->GetBrush();
			Brush.TintColor = FSlateColor(
				Index < Current ? CurrentThemeColors.SpellPoints : CurrentThemeColors.Well);
			Brush.OutlineSettings.Color = FSlateColor(
				Index < Current ? CurrentThemeColors.Ink : CurrentThemeColors.Lip);
			Token->SetBrush(Brush);
		}
	}
}

void UMordecaiSpellPointsBarWidget::ApplyTheme(
	EMordecaiUITheme Theme,
	const FMordecaiUIThemeColors& Colors)
{
	CurrentTheme = Theme;
	CurrentThemeColors = Colors;
	if (UOverlay* Well = WidgetTree
		? Cast<UOverlay>(WidgetTree->FindWidget(TEXT("SpellPointsWell")))
		: nullptr)
	{
		FMordecaiUIPrimitiveBuilder::Apply(
			Well, FMordecaiUIPrimitiveBuilder::Well(Colors, true));
	}
	if (SpellPointsProgressBar)
	{
		SpellPointsProgressBar->SetFillColorAndOpacity(Colors.SpellPoints);
	}
	if (UTextBlock* Label = WidgetTree
		? Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("SpellPointsLabel")))
		: nullptr)
	{
		MordecaiUITypography::Apply(Label, TEXT("num_s"));
		Label->SetColorAndOpacity(FSlateColor(Colors.TextMuted));
	}
	MordecaiUITypography::Apply(SpellPointsText, TEXT("num_m"));
	if (SpellPointsText)
	{
		SpellPointsText->SetColorAndOpacity(FSlateColor(Colors.Text));
	}
	RetintTokens(FMath::FloorToInt(FMath::Max(0.f, CachedSpellPoints)));
}
