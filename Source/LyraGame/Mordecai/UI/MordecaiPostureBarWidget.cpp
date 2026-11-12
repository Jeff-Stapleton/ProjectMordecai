// Project Mordecai — Posture Bar Widget (US-052)

#include "Mordecai/UI/MordecaiPostureBarWidget.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/ProgressBar.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Mordecai/UI/TinTally/MordecaiUIPrimitives.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeBinding.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiPostureBarWidget)

// ---------------------------------------------------------------------------
// Programmatic Fallback (empty Blueprint support)
// ---------------------------------------------------------------------------

void UMordecaiPostureBarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!PostureProgressBar && WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultContent();
	}
}

void UMordecaiPostureBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	MordecaiUIThemeBinding::Bind(
		this, BoundThemeSubsystem, ThemeChangedHandle,
		this, &UMordecaiPostureBarWidget::ApplyTheme);
}

void UMordecaiPostureBarWidget::NativeDestruct()
{
	MordecaiUIThemeBinding::Unbind(BoundThemeSubsystem, ThemeChangedHandle);
	Super::NativeDestruct();
}

void UMordecaiPostureBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AdvanceVisuals(InDeltaTime);
}

void UMordecaiPostureBarWidget::BuildDefaultContent()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;
	const FMordecaiUIThemeColors Colors = FMordecaiUIThemeColors::MakeDiorama();
	NormalPostureColor = Colors.Posture;
	BrokenPostureColor = Colors.Posture;

	UOverlay* Well = FMordecaiUIPrimitiveBuilder::Build(
		WidgetTree, FMordecaiUIPrimitiveBuilder::Well(Colors, true), TEXT("PostureWell"));
	if (UCanvasPanelSlot* WellSlot = Root->AddChildToCanvas(Well))
	{
		WellSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		WellSlot->SetOffsets(FMargin(0.f));
	}

	PostureProgressBar = WidgetTree->ConstructWidget<UProgressBar>(
		UProgressBar::StaticClass(), TEXT("PostureProgressBar"));
	if (UCanvasPanelSlot* BarSlot = Root->AddChildToCanvas(PostureProgressBar))
	{
		BarSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BarSlot->SetOffsets(FMargin(1.f));
	}
	FProgressBarStyle Style = PostureProgressBar->GetWidgetStyle();
	FSlateBrush Background;
	Background.DrawAs = ESlateBrushDrawType::NoDrawType;
	FSlateBrush Fill;
	Fill.DrawAs = ESlateBrushDrawType::RoundedBox;
	Fill.OutlineSettings = FSlateBrushOutlineSettings(
		FVector4(2.f), FSlateColor(FLinearColor::Transparent), 0.f);
	Style.SetBackgroundImage(Background);
	Style.SetFillImage(Fill);
	PostureProgressBar->SetWidgetStyle(Style);
	PostureProgressBar->SetBarFillType(EProgressBarFillType::LeftToRight);
	PostureProgressBar->SetPercent(1.f);
	PostureProgressBar->SetFillColorAndOpacity(NormalPostureColor);

	UMaterialInterface* Pattern = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/MordecaiCore/UI/TinTally/Materials/M_UI_PatternFill.M_UI_PatternFill"));
	if (Pattern)
	{
		BrokenPatternMaterial = UMaterialInstanceDynamic::Create(Pattern, this);
		BrokenPatternMaterial->SetVectorParameterValue(TEXT("Base"), Colors.Posture);
		BrokenPatternMaterial->SetVectorParameterValue(TEXT("Line"), Colors.Ink);
		BrokenPatternMaterial->SetScalarParameterValue(TEXT("Angle"), 45.f);
		BrokenPatternMaterial->SetScalarParameterValue(TEXT("Period"), 10.f);
		BrokenPatternMaterial->SetScalarParameterValue(TEXT("LineWidth"), 3.f);
	}
}

// ---------------------------------------------------------------------------
// Static Helper (AC-052.9, AC-052.11)
// ---------------------------------------------------------------------------

float UMordecaiPostureBarWidget::CalcPosturePercent(float Posture, float MaxPosture)
{
	if (MaxPosture <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(Posture / MaxPosture, 0.f, 1.f);
}

// ---------------------------------------------------------------------------
// Instance Methods
// ---------------------------------------------------------------------------

float UMordecaiPostureBarWidget::GetPosturePercent() const
{
	return CalcPosturePercent(CachedPosture, CachedMaxPosture);
}

bool UMordecaiPostureBarWidget::IsPostureBroken() const
{
	return bPostureBroken;
}

void UMordecaiPostureBarWidget::BindToASC(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	if (UAbilitySystemComponent* Previous = BoundASC.Get())
	{
		Previous->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetPostureAttribute()).RemoveAll(this);
		Previous->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetMaxPostureAttribute()).RemoveAll(this);
		Previous->RegisterGameplayTagEvent(MordecaiGameplayTags::State_PostureBroken,
			EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
	}
	BoundASC = ASC;

	ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetPostureAttribute())
		.AddUObject(this, &UMordecaiPostureBarWidget::OnPostureChanged);

	ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetMaxPostureAttribute())
		.AddUObject(this, &UMordecaiPostureBarWidget::OnMaxPostureChanged);

	// TODO(DECISION): the combat owner still must decide the player-posture-break signal and consequences.
	// Listen for the existing posture broken tag so the HUD is ready for that decision (AC-052.4).
	ASC->RegisterGameplayTagEvent(MordecaiGameplayTags::State_PostureBroken, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UMordecaiPostureBarWidget::OnPostureBrokenTagChanged);

	// Initialize from current values
	bool bFound = false;
	CachedPosture = ASC->GetGameplayAttributeValue(UMordecaiAttributeSet::GetPostureAttribute(), bFound);
	if (!bFound) { CachedPosture = 0.f; }

	CachedMaxPosture = ASC->GetGameplayAttributeValue(UMordecaiAttributeSet::GetMaxPostureAttribute(), bFound);
	if (!bFound) { CachedMaxPosture = 0.f; }

	bPostureBroken = ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_PostureBroken);

	UpdateDisplay();
}

void UMordecaiPostureBarWidget::OnPostureChanged(const FOnAttributeChangeData& Data)
{
	CachedPosture = Data.NewValue;
	UpdateDisplay();
}

void UMordecaiPostureBarWidget::OnMaxPostureChanged(const FOnAttributeChangeData& Data)
{
	CachedMaxPosture = Data.NewValue;
	UpdateDisplay();
}

void UMordecaiPostureBarWidget::OnPostureBrokenTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	bPostureBroken = (NewCount > 0);
	UpdateDisplay();
}

void UMordecaiPostureBarWidget::UpdateDisplay()
{
	const float Percent = GetPosturePercent();
	UOverlay* PostureWell = WidgetTree
		? Cast<UOverlay>(WidgetTree->FindWidget(TEXT("PostureWell")))
		: nullptr;
	const bool bAtFull = !bPostureBroken && Percent >= 1.f;
	if (!bAtFull)
	{
		bWaitingToHide = false;
		FullPostureDelayRemaining = 0.f;
		SetVisibility(ESlateVisibility::HitTestInvisible);
		if (PostureWell)
		{
			PostureWell->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
	else if (!bWaitingToHide && GetVisibility() != ESlateVisibility::Hidden)
	{
		bWaitingToHide = true;
		FullPostureDelayRemaining = 3.f;
	}

	if (PostureProgressBar)
	{
		PostureProgressBar->SetPercent(bPostureBroken ? 1.f : Percent);
		PostureProgressBar->SetFillColorAndOpacity(bPostureBroken ? BrokenPostureColor : NormalPostureColor);
		FProgressBarStyle Style = PostureProgressBar->GetWidgetStyle();
		FSlateBrush Fill = Style.FillImage;
		if (bPostureBroken && BrokenPatternMaterial)
		{
			Fill.SetResourceObject(BrokenPatternMaterial);
			Fill.DrawAs = ESlateBrushDrawType::Image;
			PostureProgressBar->SetFillColorAndOpacity(FLinearColor::White);
		}
		else
		{
			Fill.SetResourceObject(nullptr);
			Fill.DrawAs = ESlateBrushDrawType::RoundedBox;
		}
		Style.SetFillImage(Fill);
		PostureProgressBar->SetWidgetStyle(Style);
		PostureProgressBar->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (UBorder* WellLayer = WidgetTree
		? Cast<UBorder>(WidgetTree->FindWidget(TEXT("PostureWell_Well")))
		: nullptr)
	{
		FSlateBrush Brush = WellLayer->Background;
		Brush.OutlineSettings.Color = FSlateColor(
			bPostureBroken ? CurrentThemeColors.Rim : CurrentThemeColors.Lip);
		WellLayer->SetBrush(Brush);
		WellLayer->SetBrushColor(FLinearColor::White);
	}
}

void UMordecaiPostureBarWidget::AdvanceVisuals(float DeltaTime)
{
	if (!bWaitingToHide || bPostureBroken || GetPosturePercent() < 1.f)
	{
		return;
	}
	FullPostureDelayRemaining = FMath::Max(
		0.f, FullPostureDelayRemaining - FMath::Max(0.f, DeltaTime));
	if (FullPostureDelayRemaining <= KINDA_SMALL_NUMBER)
	{
		bWaitingToHide = false;
		SetVisibility(ESlateVisibility::Hidden);
		if (UWidget* PostureWell = WidgetTree
			? WidgetTree->FindWidget(TEXT("PostureWell"))
			: nullptr)
		{
			PostureWell->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UMordecaiPostureBarWidget::ApplyTheme(
	EMordecaiUITheme Theme,
	const FMordecaiUIThemeColors& Colors)
{
	CurrentTheme = Theme;
	CurrentThemeColors = Colors;
	NormalPostureColor = Colors.Posture;
	BrokenPostureColor = Colors.Posture;
	if (UOverlay* Well = WidgetTree
		? Cast<UOverlay>(WidgetTree->FindWidget(TEXT("PostureWell")))
		: nullptr)
	{
		FMordecaiUIPrimitiveBuilder::Apply(
			Well, FMordecaiUIPrimitiveBuilder::Well(Colors, true));
	}
	if (BrokenPatternMaterial)
	{
		BrokenPatternMaterial->SetVectorParameterValue(TEXT("Base"), Colors.Posture);
		BrokenPatternMaterial->SetVectorParameterValue(TEXT("Line"), Colors.Ink);
	}
	UpdateDisplay();
}
