// Project Mordecai — Skill Tree Widget (US-067)

#include "Mordecai/UI/MordecaiSkillTreeWidget.h"
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
#include "Mordecai/Skills/MordecaiSkillComponent.h"
#include "Mordecai/Skills/MordecaiSkillDataAsset.h"
#include "Mordecai/UI/MordecaiPauseMenuWidget.h"
#include "Mordecai/UI/MordecaiTabButton.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeBinding.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeSubsystem.h"
#include "Mordecai/UI/TinTally/MordecaiUITypography.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiSkillTreeWidget)

#define LOCTEXT_NAMESPACE "MordecaiSkillTree"

namespace MordecaiSkillTreeLayout
{
	constexpr float OuterPadding = 10.f;
	constexpr float SectionGap = 8.f;
	constexpr float CategoryGap = 4.f;
	constexpr float CategoryWidth = 190.f;
	constexpr float RowGap = 4.f;
	constexpr float RankWidth = 112.f;
	constexpr float SkillPointsWidth = 180.f;
	constexpr float RowMinHeight = 58.f;

	struct FCategoryPresentation
	{
		EMordecaiSkillCategory Category;
		const TCHAR* Name;
		const TCHAR* Label;
	};

	const FCategoryPresentation Categories[] = {
		{ EMordecaiSkillCategory::Weapon, TEXT("Weapon"), TEXT("Weapon") },
		{ EMordecaiSkillCategory::DefenseArmor, TEXT("DefenseArmor"), TEXT("Defense & Armor") },
		{ EMordecaiSkillCategory::MovementExploration, TEXT("MovementExploration"), TEXT("Movement & Exploration") },
		{ EMordecaiSkillCategory::UtilityInteraction, TEXT("UtilityInteraction"), TEXT("Utility & Interaction") },
		{ EMordecaiSkillCategory::Magic, TEXT("Magic"), TEXT("Magic") }
	};

	const FCategoryPresentation& GetCategoryPresentation(const EMordecaiSkillCategory Category)
	{
		for (const FCategoryPresentation& Presentation : Categories)
		{
			if (Presentation.Category == Category)
			{
				return Presentation;
			}
		}
		return Categories[0];
	}

}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

FText UMordecaiSkillTreeWidget::GetSkillsTabDisplayName()
{
	return LOCTEXT("SkillsTabName", "Skills");
}

void UMordecaiSkillTreeWidget::RegisterWithPauseMenu(UMordecaiPauseMenuWidget* PauseMenu, TSubclassOf<UMordecaiSkillTreeWidget> WidgetClass)
{
	if (!PauseMenu)
	{
		return;
	}
	TSubclassOf<UUserWidget> ContentClass = WidgetClass ? *WidgetClass : UMordecaiSkillTreeWidget::StaticClass();
	PauseMenu->RegisterTab(GetSkillsTabId(), GetSkillsTabDisplayName(), ContentClass);
}

const TArray<int32>& UMordecaiSkillTreeWidget::GetMilestoneRanks()
{
	static const TArray<int32> Ranks = { 1, 5, 10, 15, 20 };
	return Ranks;
}

bool UMordecaiSkillTreeWidget::IsMilestoneRank(int32 Rank)
{
	return UMordecaiSkillComponent::IsMilestoneRank(Rank);
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void UMordecaiSkillTreeWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultContent();
	}
	RefreshContent();
}

void UMordecaiSkillTreeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	MordecaiUIThemeBinding::Bind(
		this,
		BoundThemeSubsystem,
		ThemeChangedHandle,
		this,
		&ThisClass::ApplyTheme);
}

void UMordecaiSkillTreeWidget::NativeDestruct()
{
	UnbindFromSkillComponent();
	MordecaiUIThemeBinding::Unbind(BoundThemeSubsystem, ThemeChangedHandle);
	Super::NativeDestruct();
}

void UMordecaiSkillTreeWidget::BuildDefaultContent()
{
	using namespace MordecaiSkillTreeLayout;

	ContentPlate = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("SkillContentPlate"));
	ContentPlate->SetPadding(FMargin(OuterPadding));
	WidgetTree->RootWidget = ContentPlate;

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("SkillContent"));
	ContentPlate->SetContent(Root);

	auto MakeStaticText = [this](
		const FName Name,
		const FText& Text,
		const TCHAR* Style,
		const bool bMuted = false,
		const bool bAutoWrap = false) -> UTextBlock*
	{
		UTextBlock* Result = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), Name);
		Result->SetText(Text);
		Result->SetAutoWrapText(bAutoWrap);
		MordecaiUITypography::Apply(Result, Style);
		(bMuted ? StaticMutedTextWidgets : StaticMainTextWidgets).Add(Result);
		return Result;
	};

	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(), TEXT("SkillHeader"));
	if (UVerticalBoxSlot* HeaderSlot = Root->AddChildToVerticalBox(Header))
	{
		HeaderSlot->SetHorizontalAlignment(HAlign_Fill);
		HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, SectionGap));
	}
	SkillsTitle = MakeStaticText(
		TEXT("SkillsTitle"), LOCTEXT("SkillsTitle", "Skills"), TEXT("ui_title"));
	if (UHorizontalBoxSlot* TitleSlot = Header->AddChildToHorizontalBox(SkillsTitle))
	{
		TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TitleSlot->SetVerticalAlignment(VAlign_Center);
	}
	SkillPointsText = MakeStaticText(
		TEXT("SkillPointsText"),
		LOCTEXT("SkillPointsInitial", "Skill Points: 0"),
		TEXT("num_m"));
	USizeBox* SkillPointsBox = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("SkillPointsBox"));
	SkillPointsBox->SetWidthOverride(SkillPointsWidth);
	SkillPointsBox->SetContent(SkillPointsText);
	if (UHorizontalBoxSlot* PointsSlot = Header->AddChildToHorizontalBox(SkillPointsBox))
	{
		PointsSlot->SetVerticalAlignment(VAlign_Center);
	}

	UHorizontalBox* CategoriesBox = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(), TEXT("CategoryRail"));
	if (UVerticalBoxSlot* CategoriesSlot = Root->AddChildToVerticalBox(CategoriesBox))
	{
		CategoriesSlot->SetHorizontalAlignment(HAlign_Fill);
		CategoriesSlot->SetPadding(FMargin(0.f, 0.f, 0.f, SectionGap));
	}
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Categories); ++Index)
	{
		const FCategoryPresentation& Presentation = Categories[Index];
		UMordecaiTabButton* CategoryButton =
			WidgetTree->ConstructWidget<UMordecaiTabButton>(
				UMordecaiTabButton::StaticClass(),
				FName(*FString::Printf(TEXT("CategoryButton_%s"), Presentation.Name)));
		FString CategoryId(Presentation.Name);
		CategoryId.ToLowerInline();
		CategoryButton->InitButton(FName(*CategoryId));
		CategoryButton->OnClickedWithId.AddUniqueDynamic(
			this,
			&UMordecaiSkillTreeWidget::HandleCategoryClicked);
		CategoryButtons.Add(CategoryButton);

		UTextBlock* CategoryText = MakeStaticText(
			FName(*FString::Printf(TEXT("Category_%s"), Presentation.Name)),
			FText::FromString(Presentation.Label),
			TEXT("ui_label"),
			Presentation.Category != ActiveCategory);
		CategoryText->SetJustification(ETextJustify::Center);
		CategoryButton->AddChild(CategoryText);
		USizeBox* CategorySize = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass());
		CategorySize->SetMinDesiredWidth(CategoryWidth);
		CategorySize->SetContent(CategoryButton);
		if (UHorizontalBoxSlot* CategorySlot =
			CategoriesBox->AddChildToHorizontalBox(CategorySize))
		{
			CategorySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			CategorySlot->SetVerticalAlignment(VAlign_Center);
			CategorySlot->SetPadding(FMargin(
				Index > 0 ? CategoryGap * 0.5f : 0.f,
				0.f,
				Index + 1 < UE_ARRAY_COUNT(Categories) ? CategoryGap * 0.5f : 0.f,
				0.f));
		}
	}

	CategoryMetaText = MakeStaticText(
		TEXT("CategoryMetaText"),
		FText::GetEmpty(),
		TEXT("ui_label"),
		true);
	if (UVerticalBoxSlot* MetaSlot = Root->AddChildToVerticalBox(CategoryMetaText))
	{
		MetaSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(
		UScrollBox::StaticClass(), TEXT("SkillListBox"));
	Scroll->SetScrollBarVisibility(ESlateVisibility::Visible);
	if (UVerticalBoxSlot* ScrollSlot = Root->AddChildToVerticalBox(Scroll))
	{
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ScrollSlot->SetHorizontalAlignment(HAlign_Fill);
		ScrollSlot->SetVerticalAlignment(VAlign_Fill);
	}
	SkillList = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("SkillList"));
	Scroll->AddChild(SkillList);

	ApplyCurrentTheme();
}

void UMordecaiSkillTreeWidget::RefreshContent()
{
	using namespace MordecaiSkillTreeLayout;
	if (!SkillList)
	{
		return;
	}

	++ContentRebuildCount;
	SkillList->ClearChildren();
	DynamicMainTextWidgets.Reset();
	DynamicMutedTextWidgets.Reset();
	DynamicRowPanels.Reset();
	SkillNameTextById.Reset();
	SkillRankTextById.Reset();
	SkillRankBoxById.Reset();
	SkillRowPanelById.Reset();

	const int32 AvailablePoints = GetAvailableSkillPoints();
	if (SkillPointsText)
	{
		SkillPointsText->SetText(FText::Format(
			LOCTEXT("SkillPointsFormat", "Skill Points: {0}"),
			FText::AsNumber(AvailablePoints)));
	}

	const TArray<FMordecaiSkillTreeRow> Rows = GetVisibleSkills();
	const FCategoryPresentation& Category = GetCategoryPresentation(ActiveCategory);
	if (CategoryMetaText)
	{
		CategoryMetaText->SetText(FText::Format(
			LOCTEXT("CategoryCountFormat", "{0} \u00b7 {1} skills"),
			FText::FromString(Category.Label),
			FText::AsNumber(Rows.Num())));
	}

	auto MakeDynamicText = [this](
		const FName Name,
		const FText& Text,
		const TCHAR* Style,
		const bool bMuted = false,
		const bool bAutoWrap = false) -> UTextBlock*
	{
		UTextBlock* Result = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), Name);
		Result->SetText(Text);
		Result->SetAutoWrapText(bAutoWrap);
		MordecaiUITypography::Apply(Result, Style);
		(bMuted ? DynamicMutedTextWidgets : DynamicMainTextWidgets).Add(Result);
		return Result;
	};

	if (Rows.IsEmpty())
	{
		UTextBlock* Empty = MakeDynamicText(
			TEXT("SkillEmptyText"),
			LOCTEXT("SkillEmpty", "No skills in this category yet."),
			TEXT("ui_body"),
			true,
			true);
		if (UVerticalBoxSlot* EmptySlot = SkillList->AddChildToVerticalBox(Empty))
		{
			EmptySlot->SetPadding(FMargin(12.f));
			EmptySlot->SetHorizontalAlignment(HAlign_Center);
			EmptySlot->SetVerticalAlignment(VAlign_Center);
		}
		ApplyCurrentTheme();
		return;
	}

	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		const FMordecaiSkillTreeRow& Row = Rows[Index];

		USizeBox* RowSize = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), NAME_None);
		RowSize->SetMinDesiredHeight(RowMinHeight);
		UBorder* RowPanel = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), NAME_None);
		RowPanel->SetPadding(FMargin(10.f, 6.f));
		RowSize->SetContent(RowPanel);
		DynamicRowPanels.Add(RowPanel);
		SkillRowPanelById.Add(Row.SkillName, RowPanel);

		UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), NAME_None);
		RowPanel->SetContent(RowBox);

		UTextBlock* NameText = MakeDynamicText(
			NAME_None,
			Row.DisplayName,
			TEXT("ui_title"));
		SkillNameTextById.Add(Row.SkillName, NameText);
		if (UHorizontalBoxSlot* NameSlot = RowBox->AddChildToHorizontalBox(NameText))
		{
			NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			NameSlot->SetVerticalAlignment(VAlign_Center);
		}

		UTextBlock* RankLabel = MakeDynamicText(
			NAME_None,
			LOCTEXT("RankLabel", "Rank"),
			TEXT("ui_label"),
			true);
		if (UHorizontalBoxSlot* RankLabelSlot =
			RowBox->AddChildToHorizontalBox(RankLabel))
		{
			RankLabelSlot->SetVerticalAlignment(VAlign_Center);
			RankLabelSlot->SetPadding(FMargin(8.f, 0.f));
		}

		USizeBox* RankBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), NAME_None);
		RankBox->SetWidthOverride(RankWidth);
		SkillRankBoxById.Add(Row.SkillName, RankBox);
		UTextBlock* RankText = MakeDynamicText(
			NAME_None,
			FText::FromString(FString::Printf(
				TEXT("%d / %d"), Row.CurrentRank, Row.MaxRank)),
			TEXT("num_m"));
		SkillRankTextById.Add(Row.SkillName, RankText);
		RankText->SetJustification(ETextJustify::Right);
		if (USizeBoxSlot* RankTextSlot =
			Cast<USizeBoxSlot>(RankBox->AddChild(RankText)))
		{
			RankTextSlot->SetHorizontalAlignment(HAlign_Fill);
			RankTextSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (UHorizontalBoxSlot* RankBoxSlot = RowBox->AddChildToHorizontalBox(RankBox))
		{
			RankBoxSlot->SetVerticalAlignment(VAlign_Center);
		}

		if (UVerticalBoxSlot* RowSlot = SkillList->AddChildToVerticalBox(RowSize))
		{
			RowSlot->SetPadding(FMargin(
				0.f,
				Index > 0 ? RowGap * 0.5f : 0.f,
				0.f,
				Index + 1 < Rows.Num() ? RowGap * 0.5f : 0.f));
		}
	}

	ApplyCurrentTheme();
}

void UMordecaiSkillTreeWidget::RefreshValues()
{
	if (SkillPointsText)
	{
		SkillPointsText->SetText(FText::Format(
			LOCTEXT("SkillPointsFormat", "Skill Points: {0}"),
			FText::AsNumber(GetAvailableSkillPoints())));
	}

	if (UMordecaiSkillComponent* Component = BoundComponent.Get())
	{
		for (const TPair<FName, TObjectPtr<UTextBlock>>& Pair : SkillRankTextById)
		{
			if (Pair.Value)
			{
				Pair.Value->SetText(FText::FromString(FString::Printf(
					TEXT("%d / %d"),
					Component->GetSkillRank(Pair.Key),
					UMordecaiSkillComponent::MaxSkillRank)));
			}
		}
	}
}

void UMordecaiSkillTreeWidget::ApplyCurrentTheme()
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

void UMordecaiSkillTreeWidget::ApplyTheme(
	EMordecaiUITheme Theme,
	const FMordecaiUIThemeColors& Colors)
{
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
	for (int32 Index = 0;
		Index < CategoryButtons.Num()
			&& Index < UE_ARRAY_COUNT(MordecaiSkillTreeLayout::Categories);
		++Index)
	{
		if (UMordecaiTabButton* CategoryButton = CategoryButtons[Index])
		{
			const bool bActive =
				MordecaiSkillTreeLayout::Categories[Index].Category == ActiveCategory;
			CategoryButton->SetBackgroundColor(
				bActive ? Colors.Enamel : Colors.PlateRaised);
			if (UTextBlock* CategoryText =
				Cast<UTextBlock>(CategoryButton->GetContent()))
			{
				CategoryText->SetColorAndOpacity(FSlateColor(
					bActive ? Colors.Ink : Colors.TextMuted));
			}
		}
	}
}

UTextBlock* UMordecaiSkillTreeWidget::FindSkillNameText(FName SkillName) const
{
	const TObjectPtr<UTextBlock>* Found = SkillNameTextById.Find(SkillName);
	return Found ? Found->Get() : nullptr;
}

UTextBlock* UMordecaiSkillTreeWidget::FindSkillRankText(FName SkillName) const
{
	const TObjectPtr<UTextBlock>* Found = SkillRankTextById.Find(SkillName);
	return Found ? Found->Get() : nullptr;
}

USizeBox* UMordecaiSkillTreeWidget::FindSkillRankBox(FName SkillName) const
{
	const TObjectPtr<USizeBox>* Found = SkillRankBoxById.Find(SkillName);
	return Found ? Found->Get() : nullptr;
}

UBorder* UMordecaiSkillTreeWidget::FindSkillRowPanel(FName SkillName) const
{
	const TObjectPtr<UBorder>* Found = SkillRowPanelById.Find(SkillName);
	return Found ? Found->Get() : nullptr;
}

// ---------------------------------------------------------------------------
// Binding
// ---------------------------------------------------------------------------

void UMordecaiSkillTreeWidget::BindToSkillComponent(UMordecaiSkillComponent* InComponent)
{
	if (BoundComponent.Get() == InComponent)
	{
		return;
	}

	UnbindFromSkillComponent();
	if (!InComponent)
	{
		return;
	}

	BoundComponent = InComponent;
	InComponent->OnSkillMilestoneReachedBP.AddDynamic(this, &UMordecaiSkillTreeWidget::HandleMilestoneReached);
	InComponent->OnSkillStateChanged.AddUObject(
		this,
		&UMordecaiSkillTreeWidget::HandleSkillStateChanged);
	if (LastBoundComponent.Get() == InComponent)
	{
		RefreshValues();
	}
	else
	{
		RefreshContent();
	}
	LastBoundComponent = InComponent;
}

void UMordecaiSkillTreeWidget::UnbindFromSkillComponent()
{
	if (UMordecaiSkillComponent* Comp = BoundComponent.Get())
	{
		Comp->OnSkillMilestoneReachedBP.RemoveDynamic(this, &UMordecaiSkillTreeWidget::HandleMilestoneReached);
		Comp->OnSkillStateChanged.RemoveAll(this);
	}
	BoundComponent.Reset();
}

void UMordecaiSkillTreeWidget::HandleSkillStateChanged()
{
	RefreshValues();
}

void UMordecaiSkillTreeWidget::HandleMilestoneReached(FName SkillName, int32 MilestoneRank)
{
	LastObservedMilestoneSkill = SkillName;
	LastObservedMilestoneRank = MilestoneRank;
}

// ---------------------------------------------------------------------------
// Category
// ---------------------------------------------------------------------------

void UMordecaiSkillTreeWidget::SetActiveCategory(EMordecaiSkillCategory Category)
{
	if (ActiveCategory == Category)
	{
		return;
	}
	ActiveCategory = Category;
	RefreshContent();
}

void UMordecaiSkillTreeWidget::HandleCategoryClicked(FName CategoryId)
{
	for (const MordecaiSkillTreeLayout::FCategoryPresentation& Presentation :
		MordecaiSkillTreeLayout::Categories)
	{
		FString ExpectedId(Presentation.Name);
		ExpectedId.ToLowerInline();
		if (CategoryId == FName(*ExpectedId))
		{
			SetActiveCategory(Presentation.Category);
			return;
		}
	}
}

TArray<FMordecaiSkillTreeRow> UMordecaiSkillTreeWidget::GetVisibleSkills() const
{
	TArray<FMordecaiSkillTreeRow> Result;

	UMordecaiSkillComponent* Comp = BoundComponent.Get();
	if (!Comp)
	{
		return Result;
	}

	const int32 AvailablePoints = Comp->GetAvailableSkillPoints();
	TArray<FMordecaiSkillRankInfo> Infos = Comp->GetAllSkillsByCategory(ActiveCategory);

	Result.Reserve(Infos.Num());
	for (const FMordecaiSkillRankInfo& Info : Infos)
	{
		FMordecaiSkillTreeRow Row;
		Row.SkillName = Info.SkillName;
		if (const UMordecaiSkillDataAsset* SkillData =
			Comp->GetRegisteredSkillDataAsset(Info.SkillName))
		{
			Row.DisplayName = SkillData->DisplayName.IsEmpty()
				? FText::FromName(Info.SkillName)
				: SkillData->DisplayName;
		}
		else
		{
			Row.DisplayName = FText::FromName(Info.SkillName);
		}
		Row.CurrentRank = Info.CurrentRank;
		Row.MaxRank = UMordecaiSkillComponent::MaxSkillRank;
		Row.Category = ActiveCategory;
		Row.bCanAllocate = (AvailablePoints > 0) && (Info.CurrentRank < UMordecaiSkillComponent::MaxSkillRank);
		Result.Add(Row);
	}
	return Result;
}

// ---------------------------------------------------------------------------
// Skill points
// ---------------------------------------------------------------------------

int32 UMordecaiSkillTreeWidget::GetAvailableSkillPoints() const
{
	UMordecaiSkillComponent* Comp = BoundComponent.Get();
	return Comp ? Comp->GetAvailableSkillPoints() : 0;
}

// ---------------------------------------------------------------------------
// Allocation
// ---------------------------------------------------------------------------

bool UMordecaiSkillTreeWidget::CanAllocateSkillPoint(FName SkillName) const
{
	UMordecaiSkillComponent* Comp = BoundComponent.Get();
	if (!Comp)
	{
		return false;
	}
	if (!Comp->GetRegisteredSkillDataAsset(SkillName))
	{
		return false;
	}
	if (Comp->GetAvailableSkillPoints() <= 0)
	{
		return false;
	}
	if (Comp->GetSkillRank(SkillName) >= UMordecaiSkillComponent::MaxSkillRank)
	{
		return false;
	}
	return true;
}

bool UMordecaiSkillTreeWidget::RequestAllocateSkillPoint(FName SkillName)
{
	UMordecaiSkillComponent* Comp = BoundComponent.Get();
	if (!Comp)
	{
		return false;
	}
	const bool bAllocated = Comp->TryAllocateSkillPoint(SkillName);
	return bAllocated;
}

// ---------------------------------------------------------------------------
// Detail panel
// ---------------------------------------------------------------------------

void UMordecaiSkillTreeWidget::SetSelectedSkill(FName SkillName)
{
	SelectedSkill = SkillName;
}

UMordecaiSkillDataAsset* UMordecaiSkillTreeWidget::FindSkillDataAsset(FName SkillName) const
{
	UMordecaiSkillComponent* Comp = BoundComponent.Get();
	return Comp ? Comp->GetRegisteredSkillDataAsset(SkillName) : nullptr;
}

TArray<FMordecaiSkillTreeMilestoneRow> UMordecaiSkillTreeWidget::GetMilestonesForSelectedSkill() const
{
	TArray<FMordecaiSkillTreeMilestoneRow> Result;

	UMordecaiSkillComponent* Comp = BoundComponent.Get();
	if (!Comp || SelectedSkill.IsNone())
	{
		return Result;
	}

	UMordecaiSkillDataAsset* DataAsset = FindSkillDataAsset(SelectedSkill);
	const int32 CurrentRank = Comp->GetSkillRank(SelectedSkill);

	const TArray<int32>& Milestones = GetMilestoneRanks();
	Result.Reserve(Milestones.Num());

	for (int32 MilestoneRank : Milestones)
	{
		FMordecaiSkillTreeMilestoneRow Row;
		Row.Rank = MilestoneRank;
		Row.bReached = (CurrentRank >= MilestoneRank);
		Row.bAbilityGranted = Comp->HasGrantedMilestoneAbility(SelectedSkill, MilestoneRank);

		if (DataAsset)
		{
			if (const FText* Desc = DataAsset->RankDescriptions.Find(MilestoneRank))
			{
				Row.Description = *Desc;
				Row.bHasDescription = true;
			}
		}

		if (!Row.bHasDescription)
		{
			Row.Description = LOCTEXT("MilestoneNoData", "No data available.");
		}

		Result.Add(Row);
	}
	return Result;
}

#undef LOCTEXT_NAMESPACE
