// Project Mordecai — Skill Tree Widget Tests (US-067)
// All NullRHI-compatible headless tests covering category filtering, rank display,
// milestone distinction, allocation gating, and live updates via delegate.

#include "Misc/AutomationTest.h"
#include "Abilities/GameplayAbility.h"

#include "Mordecai/Skills/MordecaiSkillComponent.h"
#include "Mordecai/Skills/MordecaiSkillDataAsset.h"
#include "Mordecai/Skills/MordecaiSkillTypes.h"
#include "Mordecai/UI/MordecaiSkillTreeWidget.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace MordecaiSkillTreeTestHelpers
{
	static UMordecaiSkillDataAsset* CreateSkill(
		FName SkillName,
		EMordecaiSkillCategory Category,
		bool bWithAbilities = false)
	{
		UMordecaiSkillDataAsset* Asset = NewObject<UMordecaiSkillDataAsset>();
		Asset->SkillName = SkillName;
		Asset->Category = Category;

		Asset->RankDescriptions.Add(1,  FText::FromString(FString::Printf(TEXT("%s rank 1"),  *SkillName.ToString())));
		Asset->RankDescriptions.Add(5,  FText::FromString(FString::Printf(TEXT("%s rank 5"),  *SkillName.ToString())));
		Asset->RankDescriptions.Add(10, FText::FromString(FString::Printf(TEXT("%s rank 10"), *SkillName.ToString())));
		Asset->RankDescriptions.Add(15, FText::FromString(FString::Printf(TEXT("%s rank 15"), *SkillName.ToString())));
		Asset->RankDescriptions.Add(20, FText::FromString(FString::Printf(TEXT("%s rank 20"), *SkillName.ToString())));

		if (bWithAbilities)
		{
			Asset->MilestoneAbilities.Add(1,  UGameplayAbility::StaticClass());
			Asset->MilestoneAbilities.Add(5,  UGameplayAbility::StaticClass());
			Asset->MilestoneAbilities.Add(10, UGameplayAbility::StaticClass());
			Asset->MilestoneAbilities.Add(15, UGameplayAbility::StaticClass());
			Asset->MilestoneAbilities.Add(20, UGameplayAbility::StaticClass());
		}
		return Asset;
	}

	static UMordecaiSkillComponent* CreateComponentWithSkills()
	{
		UMordecaiSkillComponent* Comp = NewObject<UMordecaiSkillComponent>();
		Comp->RegisterSkillDataAsset(CreateSkill(FName(TEXT("Longswords")),  EMordecaiSkillCategory::Weapon));
		Comp->RegisterSkillDataAsset(CreateSkill(FName(TEXT("Greatswords")), EMordecaiSkillCategory::Weapon));
		Comp->RegisterSkillDataAsset(CreateSkill(FName(TEXT("Dodging")),     EMordecaiSkillCategory::DefenseArmor));
		Comp->RegisterSkillDataAsset(CreateSkill(FName(TEXT("Sneaking")),    EMordecaiSkillCategory::MovementExploration));
		Comp->RegisterSkillDataAsset(CreateSkill(FName(TEXT("Fireball")),    EMordecaiSkillCategory::Magic, /*withAbilities*/true));
		return Comp;
	}

	static UMordecaiSkillTreeWidget* CreateWidget()
	{
		return NewObject<UMordecaiSkillTreeWidget>(GetTransientPackage());
	}
}

using namespace MordecaiSkillTreeTestHelpers;

// ===========================================================================
// 1. Mordecai.UI.SkillTree.DisplaysSkillsByCategory (AC-067.3, AC-067.4)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_SkillTree_DisplaysSkillsByCategory,
	"Mordecai.UI.SkillTree.DisplaysSkillsByCategory",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_SkillTree_DisplaysSkillsByCategory::RunTest(const FString& Parameters)
{
	UMordecaiSkillComponent* Comp = CreateComponentWithSkills();
	UMordecaiSkillTreeWidget* Widget = CreateWidget();

	Widget->BindToSkillComponent(Comp);

	// Weapon category (default)
	Widget->SetActiveCategory(EMordecaiSkillCategory::Weapon);
	TArray<FMordecaiSkillTreeRow> WeaponRows = Widget->GetVisibleSkills();
	TestEqual("2 weapon skills visible", WeaponRows.Num(), 2);
	for (const FMordecaiSkillTreeRow& Row : WeaponRows)
	{
		TestEqual("Row category matches active", Row.Category, EMordecaiSkillCategory::Weapon);
	}

	// Magic category
	Widget->SetActiveCategory(EMordecaiSkillCategory::Magic);
	TArray<FMordecaiSkillTreeRow> MagicRows = Widget->GetVisibleSkills();
	TestEqual("1 magic skill visible", MagicRows.Num(), 1);
	TestEqual("Magic skill is Fireball", MagicRows[0].SkillName, FName(TEXT("Fireball")));

	// Empty category
	Widget->SetActiveCategory(EMordecaiSkillCategory::UtilityInteraction);
	TestEqual("0 utility skills", Widget->GetVisibleSkills().Num(), 0);

	return true;
}

// ===========================================================================
// 2. Mordecai.UI.SkillTree.ShowsRankAndMaxRank (AC-067.4)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_SkillTree_ShowsRankAndMaxRank,
	"Mordecai.UI.SkillTree.ShowsRankAndMaxRank",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_SkillTree_ShowsRankAndMaxRank::RunTest(const FString& Parameters)
{
	UMordecaiSkillComponent* Comp = CreateComponentWithSkills();
	Comp->SetSkillRank(FName(TEXT("Longswords")), 7);

	UMordecaiSkillTreeWidget* Widget = CreateWidget();
	Widget->BindToSkillComponent(Comp);
	Widget->SetActiveCategory(EMordecaiSkillCategory::Weapon);

	bool bFoundLongswords = false;
	for (const FMordecaiSkillTreeRow& Row : Widget->GetVisibleSkills())
	{
		TestEqual("Max rank is 20", Row.MaxRank, 20);
		if (Row.SkillName == FName(TEXT("Longswords")))
		{
			TestEqual("Longswords rank 7", Row.CurrentRank, 7);
			TestFalse("Row has display name", Row.DisplayName.IsEmpty());
			bFoundLongswords = true;
		}
	}
	TestTrue("Longswords row present", bFoundLongswords);
	return true;
}

// ===========================================================================
// 3. Mordecai.UI.SkillTree.MilestonesDistinguished (AC-067.5)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_SkillTree_MilestonesDistinguished,
	"Mordecai.UI.SkillTree.MilestonesDistinguished",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_SkillTree_MilestonesDistinguished::RunTest(const FString& Parameters)
{
	const TArray<int32>& Milestones = UMordecaiSkillTreeWidget::GetMilestoneRanks();
	TestEqual("5 milestone ranks", Milestones.Num(), 5);
	TestTrue("Contains rank 1",  Milestones.Contains(1));
	TestTrue("Contains rank 5",  Milestones.Contains(5));
	TestTrue("Contains rank 10", Milestones.Contains(10));
	TestTrue("Contains rank 15", Milestones.Contains(15));
	TestTrue("Contains rank 20", Milestones.Contains(20));

	// IsMilestoneRank classifies correctly
	TestTrue("1 is milestone",   UMordecaiSkillTreeWidget::IsMilestoneRank(1));
	TestTrue("5 is milestone",   UMordecaiSkillTreeWidget::IsMilestoneRank(5));
	TestTrue("10 is milestone",  UMordecaiSkillTreeWidget::IsMilestoneRank(10));
	TestTrue("15 is milestone",  UMordecaiSkillTreeWidget::IsMilestoneRank(15));
	TestTrue("20 is milestone",  UMordecaiSkillTreeWidget::IsMilestoneRank(20));
	TestFalse("2 is not milestone",  UMordecaiSkillTreeWidget::IsMilestoneRank(2));
	TestFalse("7 is not milestone",  UMordecaiSkillTreeWidget::IsMilestoneRank(7));
	TestFalse("19 is not milestone", UMordecaiSkillTreeWidget::IsMilestoneRank(19));
	return true;
}

// ===========================================================================
// 4. Mordecai.UI.SkillTree.DetailPanelShowsMilestoneDescriptions (AC-067.6, AC-067.7)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_SkillTree_DetailPanelShowsMilestoneDescriptions,
	"Mordecai.UI.SkillTree.DetailPanelShowsMilestoneDescriptions",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_SkillTree_DetailPanelShowsMilestoneDescriptions::RunTest(const FString& Parameters)
{
	UMordecaiSkillComponent* Comp = CreateComponentWithSkills();
	// Fireball was registered with milestone abilities
	Comp->SetSkillRank(FName(TEXT("Fireball")), 10);

	UMordecaiSkillTreeWidget* Widget = CreateWidget();
	Widget->BindToSkillComponent(Comp);
	Widget->SetSelectedSkill(FName(TEXT("Fireball")));

	TArray<FMordecaiSkillTreeMilestoneRow> Rows = Widget->GetMilestonesForSelectedSkill();
	TestEqual("5 milestone rows", Rows.Num(), 5);

	// Rows must be sorted by rank ascending
	TestEqual("Row[0] rank 1", Rows[0].Rank, 1);
	TestEqual("Row[1] rank 5", Rows[1].Rank, 5);
	TestEqual("Row[2] rank 10", Rows[2].Rank, 10);
	TestEqual("Row[3] rank 15", Rows[3].Rank, 15);
	TestEqual("Row[4] rank 20", Rows[4].Rank, 20);

	// Reached/unreached flags
	TestTrue("Rank 1 reached",   Rows[0].bReached);
	TestTrue("Rank 5 reached",   Rows[1].bReached);
	TestTrue("Rank 10 reached",  Rows[2].bReached);
	TestFalse("Rank 15 not reached", Rows[3].bReached);
	TestFalse("Rank 20 not reached", Rows[4].bReached);

	// Ability grant flags (Fireball has abilities at every milestone)
	TestTrue("Rank 1 ability granted",  Rows[0].bAbilityGranted);
	TestTrue("Rank 5 ability granted",  Rows[1].bAbilityGranted);
	TestTrue("Rank 10 ability granted", Rows[2].bAbilityGranted);
	TestFalse("Rank 15 ability NOT granted", Rows[3].bAbilityGranted);
	TestFalse("Rank 20 ability NOT granted", Rows[4].bAbilityGranted);

	// All rows have description text
	for (const FMordecaiSkillTreeMilestoneRow& Row : Rows)
	{
		TestFalse("Description non-empty", Row.Description.IsEmpty());
		TestTrue("Row marked bHasDescription", Row.bHasDescription);
	}
	return true;
}

// ===========================================================================
// 5. Mordecai.UI.SkillTree.DisplaysAvailablePoints (AC-067.8)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_SkillTree_DisplaysAvailablePoints,
	"Mordecai.UI.SkillTree.DisplaysAvailablePoints",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_SkillTree_DisplaysAvailablePoints::RunTest(const FString& Parameters)
{
	UMordecaiSkillComponent* Comp = CreateComponentWithSkills();
	Comp->SetAvailableSkillPoints(7);

	UMordecaiSkillTreeWidget* Widget = CreateWidget();
	Widget->BindToSkillComponent(Comp);

	TestEqual("Widget reflects component points", Widget->GetAvailableSkillPoints(), 7);
	TestTrue("HasSkillPoints true when >0", Widget->HasSkillPoints());

	Comp->SetAvailableSkillPoints(0);
	TestEqual("Widget reflects updated points", Widget->GetAvailableSkillPoints(), 0);
	TestFalse("HasSkillPoints false when 0", Widget->HasSkillPoints());
	return true;
}

// ===========================================================================
// 6. Mordecai.UI.SkillTree.AllocatePointIncrementsRank (AC-067.9)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_SkillTree_AllocatePointIncrementsRank,
	"Mordecai.UI.SkillTree.AllocatePointIncrementsRank",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_SkillTree_AllocatePointIncrementsRank::RunTest(const FString& Parameters)
{
	UMordecaiSkillComponent* Comp = CreateComponentWithSkills();
	Comp->SetAvailableSkillPoints(3);

	UMordecaiSkillTreeWidget* Widget = CreateWidget();
	Widget->BindToSkillComponent(Comp);

	const int32 PrevRank   = Comp->GetSkillRank(FName(TEXT("Dodging")));
	const int32 PrevPoints = Comp->GetAvailableSkillPoints();

	TestTrue("RequestAllocateSkillPoint succeeds",
		Widget->RequestAllocateSkillPoint(FName(TEXT("Dodging"))));

	TestEqual("Component rank incremented by 1",
		Comp->GetSkillRank(FName(TEXT("Dodging"))), PrevRank + 1);
	TestEqual("Component points decremented by 1",
		Comp->GetAvailableSkillPoints(), PrevPoints - 1);
	TestEqual("Widget reflects decremented points",
		Widget->GetAvailableSkillPoints(), PrevPoints - 1);

	return true;
}

// ===========================================================================
// 7. Mordecai.UI.SkillTree.AllocateDisabledWhenNoPoints (AC-067.10)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_SkillTree_AllocateDisabledWhenNoPoints,
	"Mordecai.UI.SkillTree.AllocateDisabledWhenNoPoints",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_SkillTree_AllocateDisabledWhenNoPoints::RunTest(const FString& Parameters)
{
	UMordecaiSkillComponent* Comp = CreateComponentWithSkills();
	Comp->SetAvailableSkillPoints(0);

	UMordecaiSkillTreeWidget* Widget = CreateWidget();
	Widget->BindToSkillComponent(Comp);
	Widget->SetActiveCategory(EMordecaiSkillCategory::Weapon);

	TestFalse("CanAllocate false with 0 points",
		Widget->CanAllocateSkillPoint(FName(TEXT("Longswords"))));

	// Every visible row also surfaces bCanAllocate=false
	for (const FMordecaiSkillTreeRow& Row : Widget->GetVisibleSkills())
	{
		TestFalse("Row bCanAllocate=false with 0 points", Row.bCanAllocate);
	}

	// Request should hard-fail
	TestFalse("RequestAllocate returns false with 0 points",
		Widget->RequestAllocateSkillPoint(FName(TEXT("Longswords"))));
	TestEqual("Rank unchanged",
		Comp->GetSkillRank(FName(TEXT("Longswords"))), 0);
	return true;
}

// ===========================================================================
// 8. Mordecai.UI.SkillTree.AllocateDisabledAtMaxRank (AC-067.10)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_SkillTree_AllocateDisabledAtMaxRank,
	"Mordecai.UI.SkillTree.AllocateDisabledAtMaxRank",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_SkillTree_AllocateDisabledAtMaxRank::RunTest(const FString& Parameters)
{
	UMordecaiSkillComponent* Comp = CreateComponentWithSkills();
	Comp->SetAvailableSkillPoints(10);
	Comp->SetSkillRank(FName(TEXT("Sneaking")), 20);

	UMordecaiSkillTreeWidget* Widget = CreateWidget();
	Widget->BindToSkillComponent(Comp);
	Widget->SetActiveCategory(EMordecaiSkillCategory::MovementExploration);

	TestFalse("CanAllocate false at max rank",
		Widget->CanAllocateSkillPoint(FName(TEXT("Sneaking"))));

	bool bFound = false;
	for (const FMordecaiSkillTreeRow& Row : Widget->GetVisibleSkills())
	{
		if (Row.SkillName == FName(TEXT("Sneaking")))
		{
			TestFalse("Sneaking row bCanAllocate=false at max", Row.bCanAllocate);
			bFound = true;
		}
	}
	TestTrue("Sneaking row present", bFound);

	TestFalse("RequestAllocate returns false at max",
		Widget->RequestAllocateSkillPoint(FName(TEXT("Sneaking"))));
	TestEqual("Rank still 20",
		Comp->GetSkillRank(FName(TEXT("Sneaking"))), 20);
	return true;
}

// ===========================================================================
// 9. Mordecai.UI.SkillTree.MilestoneReachedUpdatesUI (AC-067.11)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_SkillTree_MilestoneReachedUpdatesUI,
	"Mordecai.UI.SkillTree.MilestoneReachedUpdatesUI",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_SkillTree_MilestoneReachedUpdatesUI::RunTest(const FString& Parameters)
{
	UMordecaiSkillComponent* Comp = CreateComponentWithSkills();
	Comp->SetAvailableSkillPoints(5);

	UMordecaiSkillTreeWidget* Widget = CreateWidget();
	Widget->BindToSkillComponent(Comp);
	Widget->SetSelectedSkill(FName(TEXT("Fireball")));

	// Before rank 1 — milestone 1 not reached
	TArray<FMordecaiSkillTreeMilestoneRow> Rows = Widget->GetMilestonesForSelectedSkill();
	TestFalse("Rank 1 not reached before alloc", Rows[0].bReached);

	// Allocate a point → milestone 1 reached. Delegate must have fired and
	// the widget's last observed milestone state must now reflect rank 1.
	Widget->RequestAllocateSkillPoint(FName(TEXT("Fireball")));
	TestEqual("Last observed milestone rank", Widget->GetLastObservedMilestoneRank(), 1);
	TestEqual("Last observed milestone skill", Widget->GetLastObservedMilestoneSkill(), FName(TEXT("Fireball")));

	// Detail panel now shows rank 1 as reached
	Rows = Widget->GetMilestonesForSelectedSkill();
	TestTrue("Rank 1 reached after alloc", Rows[0].bReached);
	TestTrue("Rank 1 ability granted after alloc", Rows[0].bAbilityGranted);
	return true;
}

// ===========================================================================
// 10. Mordecai.UI.SkillTree.GracefulWithoutComponent (AC-067.2)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_SkillTree_GracefulWithoutComponent,
	"Mordecai.UI.SkillTree.GracefulWithoutComponent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_SkillTree_GracefulWithoutComponent::RunTest(const FString& Parameters)
{
	UMordecaiSkillTreeWidget* Widget = CreateWidget();

	// No component bound
	TestFalse("Not bound by default", Widget->IsBoundToComponent());
	TestEqual("0 available points without component", Widget->GetAvailableSkillPoints(), 0);
	TestEqual("0 visible skills without component", Widget->GetVisibleSkills().Num(), 0);
	TestFalse("CanAllocate false without component",
		Widget->CanAllocateSkillPoint(FName(TEXT("Anything"))));
	TestFalse("RequestAllocate false without component",
		Widget->RequestAllocateSkillPoint(FName(TEXT("Anything"))));
	TestEqual("0 milestone rows without component",
		Widget->GetMilestonesForSelectedSkill().Num(), 0);

	// Nullptr bind is a no-op (no crash)
	Widget->BindToSkillComponent(nullptr);
	TestFalse("Still unbound after nullptr", Widget->IsBoundToComponent());

	// Tab id / display name constants (AC-067.1)
	TestEqual("Tab id is 'skills'", UMordecaiSkillTreeWidget::GetSkillsTabId(), FName(TEXT("skills")));
	TestFalse("Tab display name non-empty", UMordecaiSkillTreeWidget::GetSkillsTabDisplayName().IsEmpty());
	return true;
}
