// Project Mordecai — Spell Points Bar Widget (US-055)

#include "Mordecai/UI/MordecaiSpellPointsBarWidget.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

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

void UMordecaiSpellPointsBarWidget::BuildDefaultContent()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	SpellPointsProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("SpellPointsProgressBar"));
	UCanvasPanelSlot* BarSlot = Root->AddChildToCanvas(SpellPointsProgressBar);
	BarSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BarSlot->SetOffsets(FMargin(0.f));

	SpellPointsProgressBar->SetPercent(1.f);
	SpellPointsProgressBar->SetFillColorAndOpacity(FLinearColor(0.4f, 0.2f, 0.8f)); // Purple for spell points

	SpellPointsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SpellPointsText"));
	UCanvasPanelSlot* TextSlot = Root->AddChildToCanvas(SpellPointsText);
	TextSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	TextSlot->SetOffsets(FMargin(0.f));
	TextSlot->SetAlignment(FVector2D(0.5f, 0.5f));

	SpellPointsText->SetText(FText::FromString(FormatSpellPoints(CachedSpellPoints, CachedMaxSpellPoints)));
	SpellPointsText->SetColorAndOpacity(FSlateColor(FLinearColor::White));

	FSlateFontInfo FontInfo = SpellPointsText->GetFont();
	FontInfo.Size = 10;
	SpellPointsText->SetFont(FontInfo);
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
}
