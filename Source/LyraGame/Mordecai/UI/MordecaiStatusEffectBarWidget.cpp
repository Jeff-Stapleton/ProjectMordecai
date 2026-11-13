// Project Mordecai — Status Effect Bar Widget (US-055)

#include "Mordecai/UI/MordecaiStatusEffectBarWidget.h"
#include "Mordecai/UI/MordecaiStatusEffectIndicatorWidget.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectTypes.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/PanelWidget.h"
#include "GameplayEffect.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Mordecai/UI/TinTally/MordecaiUITypography.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeBinding.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiStatusEffectBarWidget)

namespace
{
	int32 CategoryPriority(const EMordecaiStatusEffectCategory Category)
	{
		switch (Category)
		{
		case EMordecaiStatusEffectCategory::Control: return 0;
		case EMordecaiStatusEffectCategory::DoT: return 1;
		case EMordecaiStatusEffectCategory::Debuff: return 2;
		case EMordecaiStatusEffectCategory::Environmental: return 3;
		case EMordecaiStatusEffectCategory::Buff: return 4;
		default: return 2;
		}
	}

	bool FitsTwoRows(
		const TArray<UMordecaiStatusEffectIndicatorWidget*>& Indicators,
		const int32 Count,
		const bool bCompact,
		const bool bIncludeOverflow)
	{
		constexpr float StripWidth = 456.f;
		constexpr float HorizontalGap = 10.f;
		constexpr float OverflowWidth = 58.f;
		int32 Rows = 1;
		float RowWidth = 0.f;
		auto AddWidth = [&Rows, &RowWidth](const float Width)
		{
			const float Added = RowWidth > 0.f ? HorizontalGap + Width : Width;
			if (RowWidth > 0.f && RowWidth + Added > StripWidth)
			{
				++Rows;
				RowWidth = Width;
			}
			else
			{
				RowWidth += Added;
			}
		};
		for (int32 Index = 0; Index < Count; ++Index)
		{
			AddWidth(Indicators[Index]->GetChipWidth(bCompact));
		}
		if (bIncludeOverflow)
		{
			AddWidth(OverflowWidth);
		}
		return Rows <= 2;
	}
}

// ---------------------------------------------------------------------------
// Programmatic Fallback (empty Blueprint support)
// ---------------------------------------------------------------------------

void UMordecaiStatusEffectBarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!IndicatorContainer && WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultContent();
	}

	// Set default indicator class to C++ class if not configured
	if (!IndicatorWidgetClass)
	{
		IndicatorWidgetClass = UMordecaiStatusEffectIndicatorWidget::StaticClass();
	}
}

void UMordecaiStatusEffectBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	MordecaiUIThemeBinding::Bind(
		this, BoundThemeSubsystem, ThemeChangedHandle,
		this, &ThisClass::ApplyTheme);
}

void UMordecaiStatusEffectBarWidget::NativeDestruct()
{
	MordecaiUIThemeBinding::Unbind(BoundThemeSubsystem, ThemeChangedHandle);
	Super::NativeDestruct();
}

void UMordecaiStatusEffectBarWidget::NativeTick(
	const FGeometry& MyGeometry,
	const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	DurationRefreshAccumulator += InDeltaTime;
	if (DurationRefreshAccumulator >= 0.1f)
	{
		DurationRefreshAccumulator = FMath::Fmod(DurationRefreshAccumulator, 0.1f);
		RefreshDurationsAndStacks();
	}
}

void UMordecaiStatusEffectBarWidget::BuildDefaultContent()
{
	UWrapBox* Wrap = WidgetTree->ConstructWidget<UWrapBox>(
		UWrapBox::StaticClass(), TEXT("IndicatorContainer"));
	Wrap->SetWrapSize(456.f);
	Wrap->SetInnerSlotPadding(FVector2D(10.f, 20.f));
	IndicatorContainer = Wrap;
	WidgetTree->RootWidget = IndicatorContainer;
	OverflowChip = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("OverflowChip"));
	OverflowChip->SetJustification(ETextJustify::Center);
	MordecaiUITypography::Apply(OverflowChip, TEXT("num_s"));
	OverflowPlate = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("OverflowPlate"));
	OverflowPlate->SetPadding(FMargin(8.f, 2.f));
	OverflowPlate->SetHorizontalAlignment(HAlign_Center);
	OverflowPlate->SetVerticalAlignment(VAlign_Center);
	OverflowPlate->SetContent(OverflowChip);
	OverflowSize = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("OverflowSize"));
	OverflowSize->SetWidthOverride(58.f);
	OverflowSize->SetHeightOverride(36.f);
	OverflowSize->SetContent(OverflowPlate);
	OverflowSize->SetVisibility(ESlateVisibility::Collapsed);
	IndicatorContainer->AddChild(OverflowSize);
	ApplyTheme(EMordecaiUITheme::Diorama, FMordecaiUIThemeColors::MakeDiorama());
}

// ---------------------------------------------------------------------------
// Static Helpers (AC-055.7)
// ---------------------------------------------------------------------------

bool UMordecaiStatusEffectBarWidget::IsBuffTag(const FGameplayTag& StatusTag)
{
	const FMordecaiStatusDisplayData* Row =
		FindStatusEffectDisplayData(StatusTag);
	return Row && Row->bIsBuff;
}

FLinearColor UMordecaiStatusEffectBarWidget::GetTintForStatusTag(const FGameplayTag& StatusTag)
{
	return IsBuffTag(StatusTag)
		? FLinearColor(0.f, 1.f, 0.f, 1.f)   // Green for buffs
		: FLinearColor(1.f, 0.f, 0.f, 1.f);   // Red for debuffs
}

// ---------------------------------------------------------------------------
// ASC Binding (AC-055.6)
// ---------------------------------------------------------------------------

void UMordecaiStatusEffectBarWidget::BindToASC(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	if (UAbilitySystemComponent* Previous = BoundASC.Get())
	{
		for (const FMordecaiStatusDisplayData& Row : GetAllStatusEffectDisplayData())
		{
			Previous->RegisterGameplayTagEvent(
				Row.Tag, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
		}
		if (Previous != ASC)
		{
			for (const TPair<FGameplayTag, TObjectPtr<UMordecaiStatusEffectIndicatorWidget>>& Pair : ActiveIndicators)
			{
				if (Pair.Value)
				{
					Pair.Value->RemoveFromParent();
				}
			}
			ActiveIndicators.Reset();
			ActiveTagOrder.Reset();
		}
	}
	BoundASC = ASC;

	// Register every approved child tag directly. A parent-tag callback reports the
	// parent, not the child that changed, and cannot key one indicator per status.
	for (const FMordecaiStatusDisplayData& Row : GetAllStatusEffectDisplayData())
	{
		ASC->RegisterGameplayTagEvent(Row.Tag, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &ThisClass::OnStatusTagChanged);
		if (ASC->HasMatchingGameplayTag(Row.Tag))
		{
			ActiveTagOrder.AddUnique(Row.Tag);
		}
	}
	RebuildIndicators();
}

void UMordecaiStatusEffectBarWidget::SetGalleryPreviewStatuses(
	const TArray<FGameplayTag>& StatusTags)
{
	if (UAbilitySystemComponent* Previous = BoundASC.Get())
	{
		for (const FMordecaiStatusDisplayData& Row : GetAllStatusEffectDisplayData())
		{
			Previous->RegisterGameplayTagEvent(
				Row.Tag, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
		}
	}
	BoundASC.Reset();
	ActiveTagOrder.Reset();
	for (const FGameplayTag& Tag : StatusTags)
	{
		if (FindStatusEffectDisplayData(Tag))
		{
			ActiveTagOrder.AddUnique(Tag);
		}
	}
	RebuildIndicators();
}

// ---------------------------------------------------------------------------
// Tag Change Callback (AC-055.5, AC-055.6)
// ---------------------------------------------------------------------------

void UMordecaiStatusEffectBarWidget::OnStatusTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		AddIndicator(Tag);
	}
	else
	{
		RemoveIndicator(Tag);
	}
}

void UMordecaiStatusEffectBarWidget::AddIndicator(const FGameplayTag& StatusTag)
{
	if (!FindStatusEffectDisplayData(StatusTag))
	{
		return;
	}
	ActiveTagOrder.AddUnique(StatusTag);
	RebuildIndicators();
}

void UMordecaiStatusEffectBarWidget::RemoveIndicator(const FGameplayTag& StatusTag)
{
	ActiveTagOrder.Remove(StatusTag);
	if (TObjectPtr<UMordecaiStatusEffectIndicatorWidget>* Found =
		ActiveIndicators.Find(StatusTag))
	{
		if (*Found)
		{
			(*Found)->RemoveFromParent();
		}
	}
	ActiveIndicators.Remove(StatusTag);
	RebuildIndicators();
}

void UMordecaiStatusEffectBarWidget::RebuildIndicators()
{
	if (!IndicatorContainer || !IndicatorWidgetClass)
	{
		return;
	}
	IndicatorContainer->ClearChildren();
	if (OverflowSize)
	{
		OverflowSize->RemoveFromParent();
		OverflowSize->SetVisibility(ESlateVisibility::Collapsed);
	}

	TArray<FGameplayTag> Ordered = ActiveTagOrder;
	Ordered.StableSort([](const FGameplayTag& Left, const FGameplayTag& Right)
	{
		const FMordecaiStatusDisplayData* LeftRow =
			FindStatusEffectDisplayData(Left);
		const FMordecaiStatusDisplayData* RightRow =
			FindStatusEffectDisplayData(Right);
		return CategoryPriority(
			LeftRow ? LeftRow->Category : EMordecaiStatusEffectCategory::Debuff)
			< CategoryPriority(
				RightRow ? RightRow->Category : EMordecaiStatusEffectCategory::Debuff);
	});

	TArray<UMordecaiStatusEffectIndicatorWidget*> Indicators;
	Indicators.Reserve(Ordered.Num());
	for (const FGameplayTag& Tag : Ordered)
	{
		UMordecaiStatusEffectIndicatorWidget* Indicator =
			ActiveIndicators.FindRef(Tag);
		if (!Indicator)
		{
			Indicator = CreateWidget<UMordecaiStatusEffectIndicatorWidget>(
				this, IndicatorWidgetClass);
			if (Indicator)
			{
				const FMordecaiStatusDisplayData* Row =
					FindStatusEffectDisplayData(Tag);
				Indicator->InitializeIndicator(
					Tag, Row && Row->bIsBuff, Row ? Row->DefaultDuration : 0.f);
				ActiveIndicators.Add(Tag, Indicator);
			}
		}
		if (Indicator)
		{
			Indicators.Add(Indicator);
		}
	}

	const int32 FullCandidateCount =
		FMath::Min(Indicators.Num(), MaxVisibleIndicators);
	const bool bCompact = !FitsTwoRows(
		Indicators, FullCandidateCount, false, false);
	int32 VisibleCount = FullCandidateCount;
	bool bHasOverflow = Indicators.Num() > MaxVisibleIndicators;
	if (bHasOverflow)
	{
		VisibleCount = FMath::Min(VisibleCount, MaxVisibleIndicators - 1);
	}
	while (VisibleCount > 0
		&& !FitsTwoRows(Indicators, VisibleCount, bCompact, bHasOverflow))
	{
		bHasOverflow = true;
		VisibleCount = FMath::Min(VisibleCount - 1, MaxVisibleIndicators - 1);
	}

	for (int32 Index = 0; Index < VisibleCount; ++Index)
	{
		Indicators[Index]->SetCompact(bCompact);
		IndicatorContainer->AddChild(Indicators[Index]);
	}
	const int32 HiddenCount = Indicators.Num() - VisibleCount;
	if (HiddenCount > 0 && OverflowChip && OverflowSize)
	{
		OverflowChip->SetText(FText::Format(
			NSLOCTEXT("MordecaiStatusEffects", "Overflow", "+{0}"),
			FText::AsNumber(HiddenCount)));
		OverflowSize->SetVisibility(ESlateVisibility::HitTestInvisible);
		IndicatorContainer->AddChild(OverflowSize);
	}
	else if (Indicators.IsEmpty() && OverflowSize)
	{
		// Keep the named fallback discoverable to integration diagnostics while
		// preserving zero visible chips and no extra child in populated rows.
		IndicatorContainer->AddChild(OverflowSize);
	}
	SetVisibility(
		Indicators.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	RefreshDurationsAndStacks();
}

void UMordecaiStatusEffectBarWidget::RefreshDurationsAndStacks()
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC)
	{
		return;
	}
	for (const TPair<FGameplayTag, TObjectPtr<UMordecaiStatusEffectIndicatorWidget>>& Pair
		: ActiveIndicators)
	{
		if (!Pair.Value)
		{
			continue;
		}
		FGameplayEffectQuery Query;
		Query.OwningTagQuery =
			FGameplayTagQuery::MakeQuery_MatchAnyTags(FGameplayTagContainer(Pair.Key));
		const TArray<TPair<float, float>> Times =
			ASC->GetActiveEffectsTimeRemainingAndDuration(Query);
		float Remaining = 0.f;
		for (const TPair<float, float>& Time : Times)
		{
			Remaining = FMath::Max(Remaining, Time.Key);
		}
		Pair.Value->UpdateDuration(Remaining);
		Pair.Value->SetStackCount(ASC->GetAggregatedStackCount(Query));
	}
}

void UMordecaiStatusEffectBarWidget::ApplyTheme(
	const EMordecaiUITheme Theme,
	const FMordecaiUIThemeColors& Colors)
{
	if (OverflowPlate)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Colors.Plate);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(
			FVector4(10.f), FSlateColor(Colors.Lip),
			Theme == EMordecaiUITheme::HighContrast ? 4.f : 2.f);
		OverflowPlate->SetBrush(Brush);
	}
	if (OverflowChip)
	{
		OverflowChip->SetColorAndOpacity(FSlateColor(Colors.Text));
		FSlateFontInfo Font = OverflowChip->GetFont();
		Font.TypefaceFontName = TEXT("Bold");
		Font.Size = 15;
		OverflowChip->SetFont(Font);
	}
}
