// Project Mordecai — Health Bar Widget (US-052)

#include "Mordecai/UI/MordecaiHealthBarWidget.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Mordecai/UI/MordecaiHUDConstants.h"
#include "Mordecai/UI/TinTally/MordecaiUIPrimitives.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeBinding.h"
#include "Mordecai/UI/TinTally/MordecaiUITypography.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiHealthBarWidget)

// ---------------------------------------------------------------------------
// Programmatic Fallback (empty Blueprint support)
// ---------------------------------------------------------------------------

void UMordecaiHealthBarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!HealthProgressBar && WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultContent();
	}
}

void UMordecaiHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	MordecaiUIThemeBinding::Bind(
		this, BoundThemeSubsystem, ThemeChangedHandle,
		this, &UMordecaiHealthBarWidget::ApplyTheme);
}

void UMordecaiHealthBarWidget::NativeDestruct()
{
	MordecaiUIThemeBinding::Unbind(BoundThemeSubsystem, ThemeChangedHandle);
	Super::NativeDestruct();
}

void UMordecaiHealthBarWidget::BuildDefaultContent()
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
	auto ConfigureFill = [](UProgressBar* Bar, const FLinearColor& Color)
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
		Bar->SetFillColorAndOpacity(Color);
		Bar->SetBarFillType(EProgressBarFillType::LeftToRight);
		Bar->SetVisibility(ESlateVisibility::HitTestInvisible);
	};

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HealthLabel"));
	Label->SetText(FText::FromString(TEXT("HP")));
	Label->SetColorAndOpacity(FSlateColor(Colors.TextMuted));
	FSlateFontInfo LabelFont = Label->GetFont();
	LabelFont.Size = 15;
	Label->SetFont(LabelFont);
	Place(Label, FVector2D(0.f, 5.f), FVector2D(28.f, 24.f));

	UOverlay* Well = FMordecaiUIPrimitiveBuilder::Build(
		WidgetTree, FMordecaiUIPrimitiveBuilder::Well(Colors, true), TEXT("HealthWell"));
	Place(Well, FVector2D(38.f, 5.f), FVector2D(268.f, 24.f));

	HealthTrailBar = WidgetTree->ConstructWidget<UProgressBar>(
		UProgressBar::StaticClass(), TEXT("HealthTrailBar"));
	ConfigureFill(HealthTrailBar, Colors.VitalTrail);
	Place(HealthTrailBar, FVector2D(40.f, 7.f), FVector2D(264.f, 20.f));

	HealthLeadBar = WidgetTree->ConstructWidget<UProgressBar>(
		UProgressBar::StaticClass(), TEXT("HealthLeadBar"));
	ConfigureFill(HealthLeadBar, Colors.Heal);
	Place(HealthLeadBar, FVector2D(40.f, 7.f), FVector2D(264.f, 20.f));

	HealthProgressBar = WidgetTree->ConstructWidget<UProgressBar>(
		UProgressBar::StaticClass(), TEXT("HealthProgressBar"));
	ConfigureFill(HealthProgressBar, Colors.Vital);
	Place(HealthProgressBar, FVector2D(40.f, 7.f), FVector2D(264.f, 20.f));

	UBorder* Notch = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("HealthNotch25"));
	Notch->SetBrushColor(Colors.Ink);
	Notch->SetPadding(FMargin(0.f));
	Notch->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(Notch, FVector2D(104.f, 5.f), FVector2D(2.f, 24.f));

	HealthPercentText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("HealthPercentText"));
	HealthPercentText->SetJustification(ETextJustify::Right);
	HealthPercentText->SetColorAndOpacity(FSlateColor(Colors.Text));
	FSlateFontInfo PercentFont = HealthPercentText->GetFont();
	PercentFont.Size = 23;
	HealthPercentText->SetFont(PercentFont);
	Place(HealthPercentText, FVector2D(316.f, 0.f), FVector2D(108.f, 34.f));

	HealthTrailBar->SetPercent(1.f);
	HealthLeadBar->SetPercent(1.f);
	HealthProgressBar->SetPercent(1.f);
	HealthPercentText->SetText(FText::FromString(TEXT("100%")));
}

// ---------------------------------------------------------------------------
// Static Helper (AC-052.6, AC-052.11)
// ---------------------------------------------------------------------------

float UMordecaiHealthBarWidget::CalcHealthPercent(float Health, float MaxHealth)
{
	if (MaxHealth <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(Health / MaxHealth, 0.f, 1.f);
}

// ---------------------------------------------------------------------------
// Instance Methods
// ---------------------------------------------------------------------------

float UMordecaiHealthBarWidget::GetHealthPercent() const
{
	return CalcHealthPercent(CachedHealth, CachedMaxHealth);
}

void UMordecaiHealthBarWidget::BindToASC(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	if (UAbilitySystemComponent* Previous = BoundASC.Get())
	{
		Previous->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetHealthAttribute()).RemoveAll(this);
		Previous->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetMaxHealthAttribute()).RemoveAll(this);
	}
	BoundASC = ASC;

	ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetHealthAttribute())
		.AddUObject(this, &UMordecaiHealthBarWidget::OnHealthChanged);

	ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &UMordecaiHealthBarWidget::OnMaxHealthChanged);

	// Initialize from current values
	bool bFound = false;
	CachedHealth = ASC->GetGameplayAttributeValue(UMordecaiAttributeSet::GetHealthAttribute(), bFound);
	if (!bFound) { CachedHealth = 0.f; }

	CachedMaxHealth = ASC->GetGameplayAttributeValue(UMordecaiAttributeSet::GetMaxHealthAttribute(), bFound);
	if (!bFound) { CachedMaxHealth = 0.f; }

	bHasDisplayValue = false;
	UpdateDisplay();
}

void UMordecaiHealthBarWidget::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	CachedHealth = Data.NewValue;
	UpdateDisplay();
}

void UMordecaiHealthBarWidget::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	CachedMaxHealth = Data.NewValue;
	UpdateDisplay();
}

void UMordecaiHealthBarWidget::UpdateDisplay()
{
	const float Percent = GetHealthPercent();
	if (!bHasDisplayValue)
	{
		DisplayedHealthPercent = Percent;
		TrailPercent = Percent;
		LeadPercent = Percent;
		bHasDisplayValue = true;
	}
	else if (Percent < DisplayedHealthPercent)
	{
		TrailPercent = FMath::Max(TrailPercent, DisplayedHealthPercent);
		DisplayedHealthPercent = Percent;
		LeadPercent = Percent;
		TrailHoldRemaining = 0.35f;
		TrailStartPercent = TrailPercent;
		TrailEaseElapsed = 0.f;
		bHealing = false;
	}
	else if (Percent > DisplayedHealthPercent)
	{
		LeadPercent = Percent;
		HealStartPercent = DisplayedHealthPercent;
		HealEaseElapsed = 0.f;
		bHealing = true;
	}
	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(DisplayedHealthPercent);
	}
	if (HealthTrailBar)
	{
		HealthTrailBar->SetPercent(TrailPercent);
	}
	if (HealthLeadBar)
	{
		HealthLeadBar->SetPercent(LeadPercent);
		HealthLeadBar->SetVisibility(
			bHealing ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (HealthPercentText)
	{
		const int32 PercentNumber = CachedHealth > 0.f
			? FMath::Max(1, FMath::CeilToInt(Percent * 100.f))
			: 0;
		HealthPercentText->SetText(FText::FromString(
			FString::Printf(TEXT("%d%%"), PercentNumber)));
		HealthPercentText->SetColorAndOpacity(FSlateColor(
			Percent <= MordecaiHUDConstants::LowHealthThreshold
				? CurrentThemeColors.DangerText
				: CurrentThemeColors.Text));
	}
}

void UMordecaiHealthBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AdvanceVisuals(InDeltaTime);
}

void UMordecaiHealthBarWidget::AdvanceVisuals(float DeltaTime)
{
	const float Target = GetHealthPercent();
	if (bHealing)
	{
		HealEaseElapsed = FMath::Min(0.4f, HealEaseElapsed + FMath::Max(0.f, DeltaTime));
		const float T = HealEaseElapsed / 0.4f;
		const float EaseOut = 1.f - FMath::Square(1.f - T);
		DisplayedHealthPercent = FMath::Lerp(HealStartPercent, Target, EaseOut);
		if (HealEaseElapsed >= 0.4f)
		{
			DisplayedHealthPercent = Target;
			LeadPercent = Target;
			bHealing = false;
		}
		if (HealthProgressBar)
		{
			HealthProgressBar->SetPercent(DisplayedHealthPercent);
		}
		if (HealthLeadBar)
		{
			HealthLeadBar->SetPercent(LeadPercent);
			HealthLeadBar->SetVisibility(
				bHealing ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
	else if (TrailPercent > Target)
	{
		const float HoldStep = FMath::Min(TrailHoldRemaining, FMath::Max(0.f, DeltaTime));
		TrailHoldRemaining -= HoldStep;
		const float EaseStep = FMath::Max(0.f, DeltaTime - HoldStep);
		if (TrailHoldRemaining <= 0.f)
		{
			TrailEaseElapsed = FMath::Min(0.45f, TrailEaseElapsed + EaseStep);
			const float T = TrailEaseElapsed / 0.45f;
			const float EaseInOut = T * T * (3.f - 2.f * T);
			TrailPercent = FMath::Lerp(TrailStartPercent, Target, EaseInOut);
			if (HealthTrailBar)
			{
				HealthTrailBar->SetPercent(TrailPercent);
			}
		}
	}
}

void UMordecaiHealthBarWidget::ApplyTheme(
	EMordecaiUITheme Theme,
	const FMordecaiUIThemeColors& Colors)
{
	CurrentTheme = Theme;
	CurrentThemeColors = Colors;
	if (UOverlay* Well = WidgetTree
		? Cast<UOverlay>(WidgetTree->FindWidget(TEXT("HealthWell")))
		: nullptr)
	{
		FMordecaiUIPrimitiveBuilder::Apply(
			Well, FMordecaiUIPrimitiveBuilder::Well(Colors, true));
	}
	if (HealthProgressBar) HealthProgressBar->SetFillColorAndOpacity(Colors.Vital);
	if (HealthTrailBar) HealthTrailBar->SetFillColorAndOpacity(Colors.VitalTrail);
	if (HealthLeadBar) HealthLeadBar->SetFillColorAndOpacity(Colors.Heal);
	if (UBorder* Notch = WidgetTree
		? Cast<UBorder>(WidgetTree->FindWidget(TEXT("HealthNotch25")))
		: nullptr)
	{
		Notch->SetBrushColor(Colors.Ink);
	}
	if (UTextBlock* Label = WidgetTree
		? Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("HealthLabel")))
		: nullptr)
	{
		MordecaiUITypography::Apply(Label, TEXT("ui_label"));
		Label->SetColorAndOpacity(FSlateColor(Colors.TextMuted));
	}
	MordecaiUITypography::Apply(HealthPercentText, TEXT("num_l"));
	UpdateDisplay();
}
