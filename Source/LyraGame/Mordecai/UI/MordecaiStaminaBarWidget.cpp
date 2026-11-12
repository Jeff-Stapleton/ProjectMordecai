// Project Mordecai — Stamina Bar Widget (US-052)

#include "Mordecai/UI/MordecaiStaminaBarWidget.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Mordecai/UI/MordecaiHUDConstants.h"
#include "Mordecai/UI/TinTally/MordecaiUIPrimitives.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeBinding.h"
#include "Mordecai/UI/TinTally/MordecaiUITypography.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiStaminaBarWidget)

// ---------------------------------------------------------------------------
// Programmatic Fallback (empty Blueprint support)
// ---------------------------------------------------------------------------

void UMordecaiStaminaBarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!StaminaProgressBar && WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultContent();
	}
}

void UMordecaiStaminaBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	MordecaiUIThemeBinding::Bind(
		this, BoundThemeSubsystem, ThemeChangedHandle,
		this, &UMordecaiStaminaBarWidget::ApplyTheme);
}

void UMordecaiStaminaBarWidget::NativeDestruct()
{
	MordecaiUIThemeBinding::Unbind(BoundThemeSubsystem, ThemeChangedHandle);
	Super::NativeDestruct();
}

void UMordecaiStaminaBarWidget::BuildDefaultContent()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;
	const FMordecaiUIThemeColors Colors = FMordecaiUIThemeColors::MakeDiorama();
	GreenTierColor = Colors.StaminaGreen;
	YellowTierColor = Colors.StaminaYellow;
	RedTierColor = Colors.StaminaRed;
	ExhaustedTierColor = Colors.Danger;

	auto Place = [Root](UWidget* Widget, FVector2D Position, FVector2D Size)
	{
		if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Widget))
		{
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
			Slot->SetAutoSize(false);
		}
	};
	auto ConfigureFill = [](UProgressBar* Bar)
	{
		FProgressBarStyle Style = Bar->GetWidgetStyle();
		FSlateBrush Background;
		Background.DrawAs = ESlateBrushDrawType::NoDrawType;
		FSlateBrush Fill;
		Fill.DrawAs = ESlateBrushDrawType::RoundedBox;
		Fill.OutlineSettings = FSlateBrushOutlineSettings(
			FVector4(3.f), FSlateColor(FLinearColor::Transparent), 0.f);
		Style.SetBackgroundImage(Background);
		Style.SetFillImage(Fill);
		Bar->SetWidgetStyle(Style);
		Bar->SetBarFillType(EProgressBarFillType::LeftToRight);
		Bar->SetVisibility(ESlateVisibility::HitTestInvisible);
	};

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("StaminaLabel"));
	Label->SetText(FText::FromString(TEXT("ST")));
	Label->SetColorAndOpacity(FSlateColor(Colors.TextMuted));
	FSlateFontInfo LabelFont = Label->GetFont();
	LabelFont.Size = 15;
	Label->SetFont(LabelFont);
	Place(Label, FVector2D(0.f, 2.f), FVector2D(28.f, 24.f));

	UOverlay* Well = FMordecaiUIPrimitiveBuilder::Build(
		WidgetTree, FMordecaiUIPrimitiveBuilder::Well(Colors, true), TEXT("StaminaWell"));
	Place(Well, FVector2D(38.f, 5.f), FVector2D(268.f, 18.f));

	StaminaProgressBar = WidgetTree->ConstructWidget<UProgressBar>(
		UProgressBar::StaticClass(), TEXT("StaminaProgressBar"));
	ConfigureFill(StaminaProgressBar);
	Place(StaminaProgressBar, FVector2D(40.f, 7.f), FVector2D(264.f, 14.f));

	StaminaDebtBar = WidgetTree->ConstructWidget<UProgressBar>(
		UProgressBar::StaticClass(), TEXT("StaminaDebtBar"));
	ConfigureFill(StaminaDebtBar);
	Place(StaminaDebtBar, FVector2D(40.f, 7.f), FVector2D(264.f, 14.f));

	UMaterialInterface* Pattern = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/MordecaiCore/UI/TinTally/Materials/M_UI_PatternFill.M_UI_PatternFill"));
	if (Pattern)
	{
		RedTierMaterial = UMaterialInstanceDynamic::Create(Pattern, this);
		DebtMaterial = UMaterialInstanceDynamic::Create(Pattern, this);
		auto ConfigurePattern = [](UMaterialInstanceDynamic* Material,
			const FLinearColor& Base, const FLinearColor& Line, float Angle)
		{
			Material->SetVectorParameterValue(TEXT("Base"), Base);
			Material->SetVectorParameterValue(TEXT("Line"), Line);
			Material->SetScalarParameterValue(TEXT("Angle"), Angle);
			Material->SetScalarParameterValue(TEXT("Period"), 10.f);
			Material->SetScalarParameterValue(TEXT("LineWidth"), 3.5f);
		};
		ConfigurePattern(RedTierMaterial, Colors.StaminaRed,
			FLinearColor(Colors.Ink.R, Colors.Ink.G, Colors.Ink.B, 0.35f), -45.f);
		ConfigurePattern(DebtMaterial, Colors.Well, Colors.Danger, 45.f);
		FProgressBarStyle DebtStyle = StaminaDebtBar->GetWidgetStyle();
		FSlateBrush DebtFill = DebtStyle.FillImage;
		DebtFill.SetResourceObject(DebtMaterial);
		DebtFill.DrawAs = ESlateBrushDrawType::Image;
		DebtStyle.SetFillImage(DebtFill);
		StaminaDebtBar->SetWidgetStyle(DebtStyle);
		StaminaDebtBar->SetFillColorAndOpacity(FLinearColor::White);
	}

	for (const TPair<const TCHAR*, float>& Notch : {
		TPair<const TCHAR*, float>(TEXT("StaminaNotch33"), 38.f + 268.f * 0.33f),
		TPair<const TCHAR*, float>(TEXT("StaminaNotch66"), 38.f + 268.f * 0.66f) })
	{
		UBorder* Line = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(Notch.Key));
		Line->SetBrushColor(Colors.Ink);
		Line->SetPadding(FMargin(0.f));
		Line->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(Line, FVector2D(Notch.Value, 5.f), FVector2D(2.f, 18.f));
	}

	SprintPip = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SprintPip"));
	// TODO(DECISION): US-105 does not own the sprint-pip signal source; wire its full behavior when the movement owner decides it.
	FSlateBrush PipBrush;
	PipBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	PipBrush.TintColor = FSlateColor(FLinearColor::Transparent);
	PipBrush.OutlineSettings = FSlateBrushOutlineSettings(
		FVector4(7.f), FSlateColor(Colors.Lip), 2.f);
	SprintPip->SetBrush(PipBrush);
	SprintPip->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(SprintPip, FVector2D(319.f, 7.f), FVector2D(14.f));

	UOverlay* TierPill = FMordecaiUIPrimitiveBuilder::Build(
		WidgetTree, FMordecaiUIPrimitiveBuilder::Pill(Colors), TEXT("StaminaTierPill"));
	Place(TierPill, FVector2D(340.f, 0.f), FVector2D(84.f, 28.f));
	StaminaTierText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("StaminaTierText"));
	StaminaTierText->SetJustification(ETextJustify::Center);
	StaminaTierText->SetColorAndOpacity(FSlateColor(Colors.Text));
	FSlateFontInfo TierFont = StaminaTierText->GetFont();
	TierFont.Size = 15;
	StaminaTierText->SetFont(TierFont);
	Place(StaminaTierText, FVector2D(340.f, 2.f), FVector2D(84.f, 24.f));

	StaminaDebtBar->SetVisibility(ESlateVisibility::Collapsed);
	StaminaProgressBar->SetPercent(1.f);
	StaminaProgressBar->SetFillColorAndOpacity(GreenTierColor);
}

// ---------------------------------------------------------------------------
// Tier thresholds for HUD display (AC-052.8)
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Static Helpers (AC-052.7, AC-052.8, AC-052.11)
// ---------------------------------------------------------------------------

float UMordecaiStaminaBarWidget::CalcStaminaPercent(float Stamina, float MaxStamina)
{
	if (MaxStamina <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(Stamina / MaxStamina, 0.f, 1.f);
}

EMordecaiStaminaTier UMordecaiStaminaBarWidget::CalcStaminaTier(float Stamina, float MaxStamina)
{
	if (MaxStamina <= 0.f)
	{
		return EMordecaiStaminaTier::Exhausted;
	}

	const float Ratio = Stamina / MaxStamina;

	if (Ratio <= 0.f)
	{
		return EMordecaiStaminaTier::Exhausted;
	}
	if (Ratio >= MordecaiHUDConstants::StaminaGreenThreshold)
	{
		return EMordecaiStaminaTier::Green;
	}
	if (Ratio >= MordecaiHUDConstants::StaminaYellowThreshold)
	{
		return EMordecaiStaminaTier::Yellow;
	}
	return EMordecaiStaminaTier::Red;
}

// ---------------------------------------------------------------------------
// Instance Methods
// ---------------------------------------------------------------------------

float UMordecaiStaminaBarWidget::GetStaminaPercent() const
{
	return CalcStaminaPercent(CachedStamina, CachedMaxStamina);
}

EMordecaiStaminaTier UMordecaiStaminaBarWidget::GetStaminaTier() const
{
	return CalcStaminaTier(CachedStamina, CachedMaxStamina);
}

void UMordecaiStaminaBarWidget::BindToASC(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	if (UAbilitySystemComponent* Previous = BoundASC.Get())
	{
		Previous->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetStaminaAttribute()).RemoveAll(this);
		Previous->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetMaxStaminaAttribute()).RemoveAll(this);
	}
	BoundASC = ASC;

	ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetStaminaAttribute())
		.AddUObject(this, &UMordecaiStaminaBarWidget::OnStaminaChanged);

	ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetMaxStaminaAttribute())
		.AddUObject(this, &UMordecaiStaminaBarWidget::OnMaxStaminaChanged);

	// Initialize from current values
	bool bFound = false;
	CachedStamina = ASC->GetGameplayAttributeValue(UMordecaiAttributeSet::GetStaminaAttribute(), bFound);
	if (!bFound) { CachedStamina = 0.f; }

	CachedMaxStamina = ASC->GetGameplayAttributeValue(UMordecaiAttributeSet::GetMaxStaminaAttribute(), bFound);
	if (!bFound) { CachedMaxStamina = 0.f; }

	UpdateDisplay();
}

void UMordecaiStaminaBarWidget::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
	CachedStamina = Data.NewValue;
	UpdateDisplay();
}

void UMordecaiStaminaBarWidget::OnMaxStaminaChanged(const FOnAttributeChangeData& Data)
{
	CachedMaxStamina = Data.NewValue;
	UpdateDisplay();
}

void UMordecaiStaminaBarWidget::UpdateDisplay()
{
	const float Percent = GetStaminaPercent();
	const EMordecaiStaminaTier Tier = GetStaminaTier();

	if (StaminaProgressBar)
	{
		StaminaProgressBar->SetPercent(Percent);
		StaminaProgressBar->SetVisibility(
			Tier == EMordecaiStaminaTier::Exhausted
				? ESlateVisibility::Collapsed
				: ESlateVisibility::HitTestInvisible);
		if (Tier == EMordecaiStaminaTier::Red && RedTierMaterial)
		{
			FProgressBarStyle Style = StaminaProgressBar->GetWidgetStyle();
			FSlateBrush Fill = Style.FillImage;
			Fill.SetResourceObject(RedTierMaterial);
			Fill.DrawAs = ESlateBrushDrawType::Image;
			Style.SetFillImage(Fill);
			StaminaProgressBar->SetWidgetStyle(Style);
			StaminaProgressBar->SetFillColorAndOpacity(FLinearColor::White);
		}
		else
		{
			FProgressBarStyle Style = StaminaProgressBar->GetWidgetStyle();
			FSlateBrush Fill = Style.FillImage;
			Fill.SetResourceObject(nullptr);
			Fill.DrawAs = ESlateBrushDrawType::RoundedBox;
			Style.SetFillImage(Fill);
			StaminaProgressBar->SetWidgetStyle(Style);
			StaminaProgressBar->SetFillColorAndOpacity(GetColorForTier(Tier));
		}
	}
	if (StaminaDebtBar)
	{
		StaminaDebtBar->SetPercent(CachedStamina < 0.f && CachedMaxStamina > 0.f
			? FMath::Clamp(FMath::Abs(CachedStamina) / CachedMaxStamina, 0.f, 1.f)
			: 0.f);
		StaminaDebtBar->SetVisibility(CachedStamina < 0.f
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (StaminaTierText)
	{
		const TCHAR* TierCopy = TEXT("×1.0");
		switch (Tier)
		{
		case EMordecaiStaminaTier::Green: TierCopy = TEXT("×1.25"); break;
		case EMordecaiStaminaTier::Yellow: TierCopy = TEXT("×1.0"); break;
		case EMordecaiStaminaTier::Red: TierCopy = TEXT("×0.75"); break;
		case EMordecaiStaminaTier::Exhausted: TierCopy = TEXT("×0.5"); break;
		default: break;
		}
		StaminaTierText->SetText(FText::FromString(TierCopy));
		StaminaTierText->SetColorAndOpacity(FSlateColor(
			Tier == EMordecaiStaminaTier::Exhausted
				? CurrentThemeColors.DangerText
				: CurrentThemeColors.Ink));
	}
	if (UOverlay* TierPill = WidgetTree
		? Cast<UOverlay>(WidgetTree->FindWidget(TEXT("StaminaTierPill")))
		: nullptr)
	{
		const FLinearColor FillColor = Tier == EMordecaiStaminaTier::Exhausted
			? CurrentThemeColors.Ink
			: GetColorForTier(Tier);
		const FLinearColor OutlineColor = Tier == EMordecaiStaminaTier::Exhausted
			? CurrentThemeColors.Danger
			: CurrentThemeColors.Ink;
		for (int32 Index = 0; Index < TierPill->GetChildrenCount(); ++Index)
		{
			if (UBorder* Layer = Cast<UBorder>(TierPill->GetChildAt(Index)))
			{
				FSlateBrush Brush = Layer->Background;
				if (Layer->GetName().EndsWith(TEXT("_Fill")))
				{
					Brush.TintColor = FSlateColor(FillColor);
				}
				else if (Layer->GetName().EndsWith(TEXT("_Outline")))
				{
					Brush.TintColor = FSlateColor(OutlineColor);
				}
				Layer->SetBrush(Brush);
				Layer->SetBrushColor(FLinearColor::White);
			}
		}
	}
}

FLinearColor UMordecaiStaminaBarWidget::GetColorForTier(EMordecaiStaminaTier Tier) const
{
	switch (Tier)
	{
	case EMordecaiStaminaTier::Green:     return GreenTierColor;
	case EMordecaiStaminaTier::Yellow:    return YellowTierColor;
	case EMordecaiStaminaTier::Red:       return RedTierColor;
	case EMordecaiStaminaTier::Exhausted: return ExhaustedTierColor;
	default:                              return YellowTierColor;
	}
}

void UMordecaiStaminaBarWidget::ApplyTheme(
	EMordecaiUITheme Theme,
	const FMordecaiUIThemeColors& Colors)
{
	CurrentTheme = Theme;
	CurrentThemeColors = Colors;
	GreenTierColor = Colors.StaminaGreen;
	YellowTierColor = Colors.StaminaYellow;
	RedTierColor = Colors.StaminaRed;
	ExhaustedTierColor = Colors.Danger;
	if (UOverlay* Well = WidgetTree
		? Cast<UOverlay>(WidgetTree->FindWidget(TEXT("StaminaWell")))
		: nullptr)
	{
		FMordecaiUIPrimitiveBuilder::Apply(
			Well, FMordecaiUIPrimitiveBuilder::Well(Colors, true));
	}
	if (UOverlay* TierPill = WidgetTree
		? Cast<UOverlay>(WidgetTree->FindWidget(TEXT("StaminaTierPill")))
		: nullptr)
	{
		FMordecaiUIPrimitiveBuilder::Apply(
			TierPill,
			FMordecaiUIPrimitiveBuilder::Pill(
				Colors, EMordecaiUIPrintDepth::HUD, Theme));
	}
	for (const FName Name : { FName(TEXT("StaminaNotch33")), FName(TEXT("StaminaNotch66")) })
	{
		if (UBorder* Notch = WidgetTree ? Cast<UBorder>(WidgetTree->FindWidget(Name)) : nullptr)
		{
			Notch->SetBrushColor(Colors.Ink);
		}
	}
	if (SprintPip)
	{
		FSlateBrush Brush = SprintPip->GetBrush();
		Brush.OutlineSettings.Color = FSlateColor(Colors.Lip);
		SprintPip->SetBrush(Brush);
	}
	if (RedTierMaterial)
	{
		RedTierMaterial->SetVectorParameterValue(TEXT("Base"), Colors.StaminaRed);
		RedTierMaterial->SetVectorParameterValue(
			TEXT("Line"), FLinearColor(Colors.Ink.R, Colors.Ink.G, Colors.Ink.B, 0.35f));
	}
	if (DebtMaterial)
	{
		DebtMaterial->SetVectorParameterValue(TEXT("Base"), Colors.Well);
		DebtMaterial->SetVectorParameterValue(TEXT("Line"), Colors.Danger);
	}
	if (UTextBlock* Label = WidgetTree
		? Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("StaminaLabel")))
		: nullptr)
	{
		MordecaiUITypography::Apply(Label, TEXT("num_s"));
		Label->SetColorAndOpacity(FSlateColor(Colors.TextMuted));
	}
	MordecaiUITypography::Apply(StaminaTierText, TEXT("num_s"));
	if (StaminaTierText)
	{
		FSlateFontInfo Font = StaminaTierText->GetFont();
		Font.TypefaceFontName = TEXT("Bold");
		StaminaTierText->SetFont(Font);
	}
	UpdateDisplay();
}
