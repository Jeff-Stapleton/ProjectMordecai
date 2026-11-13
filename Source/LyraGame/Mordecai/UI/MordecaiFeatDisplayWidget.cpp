// Project Mordecai — Feat Display Widget (US-068)

#include "Mordecai/UI/MordecaiFeatDisplayWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Mordecai/Feats/MordecaiFeatComponent.h"
#include "Mordecai/Feats/MordecaiFeatDataAsset.h"
#include "Mordecai/Skills/MordecaiSkillComponent.h"
#include "Mordecai/UI/MordecaiPauseMenuWidget.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeBinding.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeSubsystem.h"
#include "Mordecai/UI/TinTally/MordecaiUITypography.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiFeatDisplayWidget)

#define LOCTEXT_NAMESPACE "MordecaiFeatDisplay"

namespace MordecaiFeatDisplayLayout
{
	constexpr float OuterPadding = 16.f;
	constexpr float SectionGap = 12.f;
	constexpr float RowGap = 8.f;
	constexpr float TierWidth = 116.f;
	constexpr float RowMinHeight = 70.f;

	const TCHAR* TierLabel(const EMordecaiFeatTier Tier)
	{
		switch (Tier)
		{
		case EMordecaiFeatTier::Common:
			return TEXT("Common");
		case EMordecaiFeatTier::Rare:
			return TEXT("Rare");
		case EMordecaiFeatTier::Legendary:
			return TEXT("Legendary");
		default:
			return TEXT("Common");
		}
	}

	FString WidgetSafeName(const FName Name)
	{
		FString Result = Name.ToString();
		Result.ReplaceInline(TEXT(" "), TEXT("_"));
		Result.ReplaceInline(TEXT("-"), TEXT("_"));
		return Result;
	}
}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

FText UMordecaiFeatDisplayWidget::GetFeatsTabDisplayName()
{
	return LOCTEXT("FeatsTabName", "Feats");
}

void UMordecaiFeatDisplayWidget::RegisterWithPauseMenu(UMordecaiPauseMenuWidget* PauseMenu, TSubclassOf<UMordecaiFeatDisplayWidget> WidgetClass)
{
	if (!PauseMenu)
	{
		return;
	}
	TSubclassOf<UUserWidget> ContentClass = WidgetClass ? *WidgetClass : UMordecaiFeatDisplayWidget::StaticClass();
	PauseMenu->RegisterTab(GetFeatsTabId(), GetFeatsTabDisplayName(), ContentClass);
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void UMordecaiFeatDisplayWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Initialize default tier colors per spec
	TierColors.Add(EMordecaiFeatTier::Common, FLinearColor(0.8f, 0.8f, 0.8f));
	TierColors.Add(EMordecaiFeatTier::Rare, FLinearColor(0.3f, 0.5f, 1.0f));
	TierColors.Add(EMordecaiFeatTier::Legendary, FLinearColor(1.0f, 0.75f, 0.0f));
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultContent();
	}
	RefreshContent();
}

void UMordecaiFeatDisplayWidget::NativeConstruct()
{
	Super::NativeConstruct();
	MordecaiUIThemeBinding::Bind(
		this,
		BoundThemeSubsystem,
		ThemeChangedHandle,
		this,
		&ThisClass::ApplyTheme);
}

void UMordecaiFeatDisplayWidget::NativeDestruct()
{
	UnbindFromFeatComponent();
	MordecaiUIThemeBinding::Unbind(BoundThemeSubsystem, ThemeChangedHandle);
	Super::NativeDestruct();
}

void UMordecaiFeatDisplayWidget::BuildDefaultContent()
{
	using namespace MordecaiFeatDisplayLayout;

	ContentPlate = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("FeatContentPlate"));
	ContentPlate->SetPadding(FMargin(OuterPadding));
	WidgetTree->RootWidget = ContentPlate;

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("FeatContent"));
	ContentPlate->SetContent(Root);

	auto MakeStaticText = [this](
		const FName Name,
		const FText& Text,
		const TCHAR* Style,
		const bool bMuted = false) -> UTextBlock*
	{
		UTextBlock* Result = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), Name);
		Result->SetText(Text);
		Result->SetAutoWrapText(true);
		MordecaiUITypography::Apply(Result, Style);
		(bMuted ? StaticMutedTextWidgets : StaticMainTextWidgets).Add(Result);
		return Result;
	};

	FeatsTitle = MakeStaticText(
		TEXT("FeatsTitle"), LOCTEXT("FeatsTitle", "Feats"), TEXT("ui_title"));
	if (UVerticalBoxSlot* TitleSlot = Root->AddChildToVerticalBox(FeatsTitle))
	{
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	}

	FeatSummaryText = MakeStaticText(
		TEXT("FeatSummaryText"),
		LOCTEXT(
			"FeatSummaryInitial",
			"Feats: 0 unlocked \u2014 0 Common, 0 Rare, 0 Legendary"),
		TEXT("ui_title"));
	if (UVerticalBoxSlot* SummarySlot = Root->AddChildToVerticalBox(FeatSummaryText))
	{
		SummarySlot->SetPadding(FMargin(0.f, 0.f, 0.f, SectionGap));
	}

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(
		UScrollBox::StaticClass(), TEXT("FeatListBox"));
	Scroll->SetScrollBarVisibility(ESlateVisibility::Visible);
	if (UVerticalBoxSlot* ScrollSlot = Root->AddChildToVerticalBox(Scroll))
	{
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ScrollSlot->SetHorizontalAlignment(HAlign_Fill);
		ScrollSlot->SetVerticalAlignment(VAlign_Fill);
	}
	FeatList = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("FeatList"));
	Scroll->AddChild(FeatList);

	ApplyCurrentTheme();
}

void UMordecaiFeatDisplayWidget::RefreshContent()
{
	using namespace MordecaiFeatDisplayLayout;
	if (!FeatList)
	{
		return;
	}

	FeatList->ClearChildren();
	DynamicMainTextWidgets.Reset();
	DynamicMutedTextWidgets.Reset();
	DynamicRowPanels.Reset();
	CommonTierTexts.Reset();
	RareTierTexts.Reset();
	LegendaryTierTexts.Reset();

	const FMordecaiFeatDisplaySummary Summary = GetSummary();
	if (FeatSummaryText)
	{
		FeatSummaryText->SetText(FText::Format(
			LOCTEXT(
				"FeatSummaryFormat",
				"Feats: {0} unlocked \u2014 {1} Common, {2} Rare, {3} Legendary"),
			FText::AsNumber(Summary.TotalUnlocked),
			FText::AsNumber(Summary.CommonCount),
			FText::AsNumber(Summary.RareCount),
			FText::AsNumber(Summary.LegendaryCount)));
	}

	auto MakeDynamicText = [this](
		const FName Name,
		const FText& Text,
		const TCHAR* Style,
		const bool bMuted = false) -> UTextBlock*
	{
		UTextBlock* Result = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), Name);
		Result->SetText(Text);
		Result->SetAutoWrapText(true);
		MordecaiUITypography::Apply(Result, Style);
		(bMuted ? DynamicMutedTextWidgets : DynamicMainTextWidgets).Add(Result);
		return Result;
	};

	auto TrackTierText = [this](UTextBlock* Text, const EMordecaiFeatTier Tier)
	{
		switch (Tier)
		{
		case EMordecaiFeatTier::Common:
			CommonTierTexts.Add(Text);
			break;
		case EMordecaiFeatTier::Rare:
			RareTierTexts.Add(Text);
			break;
		case EMordecaiFeatTier::Legendary:
			LegendaryTierTexts.Add(Text);
			break;
		}
	};

	auto AddSectionHeading = [this, &MakeDynamicText](
		const TCHAR* WidgetName,
		const FText& Label)
	{
		UTextBlock* Heading = MakeDynamicText(
			FName(WidgetName), Label, TEXT("ui_label"), true);
		if (UVerticalBoxSlot* HeadingSlot =
			FeatList->AddChildToVerticalBox(Heading))
		{
			HeadingSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		}
	};

	auto AddFeatRow = [
		this,
		&MakeDynamicText,
		&TrackTierText](
		const FMordecaiFeatDisplayRow& Row,
		const int32 Index,
		const int32 Count)
	{
		const FString SafeName = WidgetSafeName(Row.FeatName);
		USizeBox* RowSize = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			FName(*FString::Printf(TEXT("FeatRowBox_%s"), *SafeName)));
		RowSize->SetMinDesiredHeight(RowMinHeight);
		UBorder* RowPanel = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			FName(*FString::Printf(TEXT("FeatRow_%s"), *SafeName)));
		RowPanel->SetPadding(FMargin(14.f, 10.f));
		RowSize->SetContent(RowPanel);
		DynamicRowPanels.Add(RowPanel);

		UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass());
		RowPanel->SetContent(RowBox);

		UVerticalBox* Copy = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass());
		if (UHorizontalBoxSlot* CopySlot = RowBox->AddChildToHorizontalBox(Copy))
		{
			CopySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			CopySlot->SetVerticalAlignment(VAlign_Center);
		}
		UTextBlock* Name = MakeDynamicText(
			FName(*FString::Printf(TEXT("FeatName_%s"), *SafeName)),
			Row.DisplayName,
			TEXT("ui_title"),
			!Row.bUnlocked);
		Copy->AddChildToVerticalBox(Name);
		UTextBlock* Description = MakeDynamicText(
			FName(*FString::Printf(TEXT("FeatDescription_%s"), *SafeName)),
			Row.Description,
			TEXT("ui_label"),
			true);
		Copy->AddChildToVerticalBox(Description);

		USizeBox* TierBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			FName(*FString::Printf(TEXT("FeatTierBox_%s"), *SafeName)));
		TierBox->SetMinDesiredWidth(TierWidth);
		UTextBlock* Tier = MakeDynamicText(
			FName(*FString::Printf(TEXT("FeatTier_%s"), *SafeName)),
			FText::FromString(TierLabel(Row.Tier)),
			TEXT("ui_label"));
		Tier->SetJustification(ETextJustify::Right);
		TrackTierText(Tier, Row.Tier);
		if (USizeBoxSlot* TierTextSlot =
			Cast<USizeBoxSlot>(TierBox->AddChild(Tier)))
		{
			TierTextSlot->SetHorizontalAlignment(HAlign_Fill);
			TierTextSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (UHorizontalBoxSlot* TierBoxSlot =
			RowBox->AddChildToHorizontalBox(TierBox))
		{
			TierBoxSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (UVerticalBoxSlot* RowSlot = FeatList->AddChildToVerticalBox(RowSize))
		{
			RowSlot->SetPadding(FMargin(
				0.f,
				Index > 0 ? RowGap * 0.5f : 0.f,
				0.f,
				Index + 1 < Count ? RowGap * 0.5f : SectionGap));
		}
	};

	const TArray<FMordecaiFeatDisplayRow> UnlockedRows = GetUnlockedFeatRows();
	AddSectionHeading(
		TEXT("UnlockedHeading"),
		FText::Format(
			LOCTEXT("UnlockedHeadingFormat", "Unlocked ({0})"),
			FText::AsNumber(UnlockedRows.Num())));
	if (UnlockedRows.IsEmpty())
	{
		UTextBlock* Empty = MakeDynamicText(
			TEXT("UnlockedEmptyText"),
			LOCTEXT(
				"UnlockedEmpty",
				"No feats unlocked yet.\nFeats are earned, not picked."),
			TEXT("ui_body"),
			true);
		if (UVerticalBoxSlot* EmptyUnlockedSlot =
			FeatList->AddChildToVerticalBox(Empty))
		{
			EmptyUnlockedSlot->SetPadding(FMargin(12.f, 8.f, 12.f, SectionGap));
		}
	}
	else
	{
		for (int32 Index = 0; Index < UnlockedRows.Num(); ++Index)
		{
			AddFeatRow(UnlockedRows[Index], Index, UnlockedRows.Num());
		}
	}

	const TArray<FMordecaiFeatDisplayRow> LockedRows = GetLockedFeatRows();
	AddSectionHeading(
		TEXT("LockedHeading"),
		FText::Format(
			LOCTEXT("LockedHeadingFormat", "Locked ({0})"),
			FText::AsNumber(LockedRows.Num())));
	if (LockedRows.IsEmpty())
	{
		UTextBlock* Empty = MakeDynamicText(
			TEXT("LockedEmptyText"),
			LOCTEXT("LockedEmpty", "No feats registered."),
			TEXT("ui_body"),
			true);
		if (UVerticalBoxSlot* EmptyLockedSlot =
			FeatList->AddChildToVerticalBox(Empty))
		{
			EmptyLockedSlot->SetPadding(FMargin(12.f, 8.f));
		}
	}
	else
	{
		for (int32 Index = 0; Index < LockedRows.Num(); ++Index)
		{
			AddFeatRow(LockedRows[Index], Index, LockedRows.Num());
		}
	}

	ApplyCurrentTheme();
}

void UMordecaiFeatDisplayWidget::ApplyCurrentTheme()
{
	if (UMordecaiUIThemeSubsystem* ThemeSubsystem =
		UMordecaiUIThemeSubsystem::ResolveForWidget(this))
	{
		ApplyTheme(
			ThemeSubsystem->GetActiveTheme(),
			ThemeSubsystem->GetActiveColors());
	}
	else
	{
		ApplyTheme(
			EMordecaiUITheme::Diorama,
			FMordecaiUIThemeColors::MakeDiorama());
	}
}

void UMordecaiFeatDisplayWidget::ApplyTheme(
	EMordecaiUITheme Theme,
	const FMordecaiUIThemeColors& Colors)
{
	TierColors.Add(EMordecaiFeatTier::Common, Colors.FeatCommon);
	TierColors.Add(EMordecaiFeatTier::Rare, Colors.FeatRare);
	TierColors.Add(EMordecaiFeatTier::Legendary, Colors.FeatLegendary);
	if (ContentPlate)
	{
		ContentPlate->SetBrushColor(Colors.Plate);
	}
	for (UBorder* Panel : DynamicRowPanels)
	{
		if (Panel)
		{
			Panel->SetBrushColor(Colors.PlateRaised);
		}
	}
	for (UTextBlock* Text : StaticMainTextWidgets)
	{
		if (Text)
		{
			Text->SetColorAndOpacity(FSlateColor(Colors.Text));
		}
	}
	for (UTextBlock* Text : StaticMutedTextWidgets)
	{
		if (Text)
		{
			Text->SetColorAndOpacity(FSlateColor(Colors.TextMuted));
		}
	}
	for (UTextBlock* Text : DynamicMainTextWidgets)
	{
		if (Text)
		{
			Text->SetColorAndOpacity(FSlateColor(Colors.Text));
		}
	}
	for (UTextBlock* Text : DynamicMutedTextWidgets)
	{
		if (Text)
		{
			Text->SetColorAndOpacity(FSlateColor(Colors.TextMuted));
		}
	}
	for (UTextBlock* Text : CommonTierTexts)
	{
		if (Text)
		{
			Text->SetColorAndOpacity(FSlateColor(Colors.FeatCommon));
		}
	}
	for (UTextBlock* Text : RareTierTexts)
	{
		if (Text)
		{
			Text->SetColorAndOpacity(FSlateColor(Colors.FeatRare));
		}
	}
	for (UTextBlock* Text : LegendaryTierTexts)
	{
		if (Text)
		{
			Text->SetColorAndOpacity(FSlateColor(Colors.FeatLegendary));
		}
	}
}

// ---------------------------------------------------------------------------
// Binding
// ---------------------------------------------------------------------------

void UMordecaiFeatDisplayWidget::BindToFeatComponent(UMordecaiFeatComponent* InComponent)
{
	UnbindFromFeatComponent();
	if (!InComponent)
	{
		return;
	}

	BoundComponent = InComponent;
	InComponent->OnFeatUnlockedBP.AddDynamic(this, &UMordecaiFeatDisplayWidget::HandleFeatUnlocked);
	RefreshContent();
}

void UMordecaiFeatDisplayWidget::UnbindFromFeatComponent()
{
	if (UMordecaiFeatComponent* Comp = BoundComponent.Get())
	{
		Comp->OnFeatUnlockedBP.RemoveDynamic(this, &UMordecaiFeatDisplayWidget::HandleFeatUnlocked);
	}
	BoundComponent.Reset();
	RefreshContent();
}

void UMordecaiFeatDisplayWidget::HandleFeatUnlocked(FName FeatName)
{
	LastUnlockedFeatName = FeatName;
	RefreshContent();
}

// ---------------------------------------------------------------------------
// Unlocked feats
// ---------------------------------------------------------------------------

TArray<FMordecaiFeatDisplayRow> UMordecaiFeatDisplayWidget::GetUnlockedFeatRows() const
{
	TArray<FMordecaiFeatDisplayRow> Result;

	UMordecaiFeatComponent* Comp = BoundComponent.Get();
	if (!Comp)
	{
		return Result;
	}

	const TSet<FName>& UnlockedNames = Comp->GetUnlockedFeats();
	const auto& RegisteredFeats = Comp->GetRegisteredFeats();

	Result.Reserve(UnlockedNames.Num());
	for (const FName& FeatName : UnlockedNames)
	{
		const TObjectPtr<UMordecaiFeatDataAsset>* AssetPtr = RegisteredFeats.Find(FeatName);
		if (!AssetPtr || !(*AssetPtr))
		{
			continue;
		}

		const UMordecaiFeatDataAsset* Asset = *AssetPtr;
		FMordecaiFeatDisplayRow Row;
		Row.FeatName = Asset->FeatName;
		Row.DisplayName = Asset->DisplayName;
		Row.Description = Asset->Description;
		Row.Tier = Asset->Tier;
		Row.bUnlocked = true;
		Row.AppliedRecord = Comp->GetFeatAppliedRecord(FeatName);
		Result.Add(Row);
	}

	return Result;
}

// ---------------------------------------------------------------------------
// Locked feats
// ---------------------------------------------------------------------------

TArray<FMordecaiFeatDisplayRow> UMordecaiFeatDisplayWidget::GetLockedFeatRows() const
{
	TArray<FMordecaiFeatDisplayRow> Result;

	UMordecaiFeatComponent* Comp = BoundComponent.Get();
	if (!Comp)
	{
		return Result;
	}

	const TSet<FName>& UnlockedNames = Comp->GetUnlockedFeats();
	const auto& RegisteredFeats = Comp->GetRegisteredFeats();

	for (const auto& Pair : RegisteredFeats)
	{
		if (UnlockedNames.Contains(Pair.Key))
		{
			continue; // Skip unlocked feats
		}

		const UMordecaiFeatDataAsset* Asset = Pair.Value;
		if (!Asset)
		{
			continue;
		}

		FMordecaiFeatDisplayRow Row;
		Row.FeatName = Asset->FeatName;
		Row.DisplayName = Asset->DisplayName;
		Row.Description = LOCTEXT("LockedFeatDescription", "???");
		Row.Tier = Asset->Tier;
		Row.bUnlocked = false;
		Row.ConditionProgress = BuildConditionProgress(Asset->UnlockConditions);
		Result.Add(Row);
	}

	return Result;
}

// ---------------------------------------------------------------------------
// Summary
// ---------------------------------------------------------------------------

FMordecaiFeatDisplaySummary UMordecaiFeatDisplayWidget::GetSummary() const
{
	FMordecaiFeatDisplaySummary Summary;

	UMordecaiFeatComponent* Comp = BoundComponent.Get();
	if (!Comp)
	{
		return Summary;
	}

	const TSet<FName>& UnlockedNames = Comp->GetUnlockedFeats();
	const auto& RegisteredFeats = Comp->GetRegisteredFeats();

	Summary.TotalUnlocked = UnlockedNames.Num();

	for (const FName& FeatName : UnlockedNames)
	{
		const TObjectPtr<UMordecaiFeatDataAsset>* AssetPtr = RegisteredFeats.Find(FeatName);
		if (!AssetPtr || !(*AssetPtr))
		{
			continue;
		}

		switch ((*AssetPtr)->Tier)
		{
		case EMordecaiFeatTier::Common:    Summary.CommonCount++;    break;
		case EMordecaiFeatTier::Rare:      Summary.RareCount++;      break;
		case EMordecaiFeatTier::Legendary: Summary.LegendaryCount++; break;
		}
	}

	return Summary;
}

// ---------------------------------------------------------------------------
// Detail panel
// ---------------------------------------------------------------------------

void UMordecaiFeatDisplayWidget::SetSelectedFeat(FName FeatName)
{
	SelectedFeat = FeatName;
}

FMordecaiFeatDisplayRow UMordecaiFeatDisplayWidget::GetSelectedFeatDetail() const
{
	FMordecaiFeatDisplayRow Detail;

	UMordecaiFeatComponent* Comp = BoundComponent.Get();
	if (!Comp || SelectedFeat.IsNone())
	{
		return Detail;
	}

	const auto& RegisteredFeats = Comp->GetRegisteredFeats();
	const TObjectPtr<UMordecaiFeatDataAsset>* AssetPtr = RegisteredFeats.Find(SelectedFeat);
	if (!AssetPtr || !(*AssetPtr))
	{
		return Detail;
	}

	const UMordecaiFeatDataAsset* Asset = *AssetPtr;
	Detail.FeatName = Asset->FeatName;
	Detail.DisplayName = Asset->DisplayName;
	Detail.Tier = Asset->Tier;

	if (Comp->HasFeat(SelectedFeat))
	{
		// Unlocked: show full description and applied record
		Detail.bUnlocked = true;
		Detail.Description = Asset->Description;
		Detail.AppliedRecord = Comp->GetFeatAppliedRecord(SelectedFeat);
	}
	else
	{
		// Locked: show teaser and condition progress
		Detail.bUnlocked = false;
		Detail.Description = LOCTEXT("LockedFeatDescription", "???");
		Detail.ConditionProgress = BuildConditionProgress(Asset->UnlockConditions);
	}

	return Detail;
}

// ---------------------------------------------------------------------------
// Tier colors
// ---------------------------------------------------------------------------

FLinearColor UMordecaiFeatDisplayWidget::GetTierColor(EMordecaiFeatTier Tier) const
{
	const FLinearColor* Color = TierColors.Find(Tier);
	if (Color)
	{
		return *Color;
	}

	// Fallback defaults (same as spec, in case TierColors wasn't initialized via NativeOnInitialized)
	switch (Tier)
	{
	case EMordecaiFeatTier::Common:    return FLinearColor(0.8f, 0.8f, 0.8f);
	case EMordecaiFeatTier::Rare:      return FLinearColor(0.3f, 0.5f, 1.0f);
	case EMordecaiFeatTier::Legendary: return FLinearColor(1.0f, 0.75f, 0.0f);
	default:                           return FLinearColor::White;
	}
}

// ---------------------------------------------------------------------------
// Condition progress
// ---------------------------------------------------------------------------

TArray<FMordecaiFeatConditionProgress> UMordecaiFeatDisplayWidget::BuildConditionProgress(
	const TArray<FMordecaiFeatCondition>& Conditions) const
{
	TArray<FMordecaiFeatConditionProgress> Result;

	UMordecaiFeatComponent* Comp = BoundComponent.Get();
	if (!Comp)
	{
		return Result;
	}

	Result.Reserve(Conditions.Num());
	for (const FMordecaiFeatCondition& Cond : Conditions)
	{
		FMordecaiFeatConditionProgress Prog;
		Prog.ConditionType = Cond.ConditionType;
		Prog.ThresholdValue = Cond.ThresholdValue;

		switch (Cond.ConditionType)
		{
		case EMordecaiFeatConditionType::StatThreshold:
		case EMordecaiFeatConditionType::EventCount:
		{
			Prog.StatName = Cond.StatName;
			Prog.CurrentValue = Comp->GetFeatStatValue(Cond.StatName);
			Prog.bMet = (Prog.CurrentValue >= Cond.ThresholdValue);
			break;
		}

		case EMordecaiFeatConditionType::SkillRank:
		{
			Prog.StatName = Cond.StatName;
			UMordecaiSkillComponent* SkillComp = FindSkillComponent();
			Prog.CurrentValue = SkillComp ? SkillComp->GetSkillRank(Cond.StatName) : 0;
			Prog.bMet = (Prog.CurrentValue >= Cond.ThresholdValue);
			break;
		}

		case EMordecaiFeatConditionType::TagPresent:
		{
			Prog.RequiredTag = Cond.RequiredTag;
			Prog.CurrentValue = Comp->HasGrantedFeatTag(Cond.RequiredTag) ? 1 : 0;
			Prog.ThresholdValue = 1;
			Prog.bMet = (Prog.CurrentValue >= 1);
			break;
		}
		}

		Result.Add(Prog);
	}

	return Result;
}

UMordecaiSkillComponent* UMordecaiFeatDisplayWidget::FindSkillComponent() const
{
	UMordecaiFeatComponent* Comp = BoundComponent.Get();
	return Comp ? Comp->GetSkillComponent() : nullptr;
}

#undef LOCTEXT_NAMESPACE
