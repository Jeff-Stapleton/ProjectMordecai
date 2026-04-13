// Project Mordecai — Skill Tree Widget (US-067)

#include "Mordecai/UI/MordecaiSkillTreeWidget.h"
#include "Mordecai/Skills/MordecaiSkillComponent.h"
#include "Mordecai/Skills/MordecaiSkillDataAsset.h"
#include "Mordecai/UI/MordecaiPauseMenuWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiSkillTreeWidget)

#define LOCTEXT_NAMESPACE "MordecaiSkillTree"

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
}

void UMordecaiSkillTreeWidget::NativeDestruct()
{
	UnbindFromSkillComponent();
	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------
// Binding
// ---------------------------------------------------------------------------

void UMordecaiSkillTreeWidget::BindToSkillComponent(UMordecaiSkillComponent* InComponent)
{
	UnbindFromSkillComponent();
	if (!InComponent)
	{
		return;
	}

	BoundComponent = InComponent;
	InComponent->OnSkillMilestoneReachedBP.AddDynamic(this, &UMordecaiSkillTreeWidget::HandleMilestoneReached);
}

void UMordecaiSkillTreeWidget::UnbindFromSkillComponent()
{
	if (UMordecaiSkillComponent* Comp = BoundComponent.Get())
	{
		Comp->OnSkillMilestoneReachedBP.RemoveDynamic(this, &UMordecaiSkillTreeWidget::HandleMilestoneReached);
	}
	BoundComponent.Reset();
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
	ActiveCategory = Category;
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
		Row.DisplayName = FText::FromName(Info.SkillName);
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
	return Comp->TryAllocateSkillPoint(SkillName);
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
