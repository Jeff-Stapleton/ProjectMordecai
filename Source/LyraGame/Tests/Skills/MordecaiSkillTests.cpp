// Project Mordecai — Skill Framework Tests (US-011)

#include "Misc/AutomationTest.h"
#include "Mordecai/Skills/MordecaiSkillTypes.h"
#include "Mordecai/Skills/MordecaiSkillDataAsset.h"
#include "Mordecai/Skills/MordecaiSkillComponent.h"
#include "Mordecai/MordecaiPlayerState.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Abilities/GameplayAbility.h"

// ---------------------------------------------------------------------------
// Helper: Create a test skill data asset with optional milestone ability
// ---------------------------------------------------------------------------
static UMordecaiSkillDataAsset* CreateTestSkillAsset(
	FName SkillName,
	EMordecaiSkillCategory Category,
	bool bWithMilestoneAbilities = false)
{
	UMordecaiSkillDataAsset* Asset = NewObject<UMordecaiSkillDataAsset>();
	Asset->SkillName = SkillName;
	Asset->Category = Category;

	Asset->RankDescriptions.Add(1, FText::FromString(TEXT("Rank 1 description")));
	Asset->RankDescriptions.Add(5, FText::FromString(TEXT("Rank 5 description")));
	Asset->RankDescriptions.Add(10, FText::FromString(TEXT("Rank 10 description")));
	Asset->RankDescriptions.Add(15, FText::FromString(TEXT("Rank 15 description")));
	Asset->RankDescriptions.Add(20, FText::FromString(TEXT("Rank 20 description")));

	if (bWithMilestoneAbilities)
	{
		// Use base UGameplayAbility as a placeholder
		Asset->MilestoneAbilities.Add(1, UGameplayAbility::StaticClass());
		Asset->MilestoneAbilities.Add(5, UGameplayAbility::StaticClass());
		Asset->MilestoneAbilities.Add(10, UGameplayAbility::StaticClass());
		Asset->MilestoneAbilities.Add(15, UGameplayAbility::StaticClass());
		Asset->MilestoneAbilities.Add(20, UGameplayAbility::StaticClass());
	}

	return Asset;
}

// ===========================================================================
// AC-011.1: SkillDataAsset has required fields
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSkills_DataAssetFields,
	"Mordecai.Skills.SkillDataAssetHasRequiredFields",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSkills_DataAssetFields::RunTest(const FString& Parameters)
{
	UMordecaiSkillDataAsset* Asset = NewObject<UMordecaiSkillDataAsset>();
	TestNotNull("DataAsset created", Asset);

	// Set all required fields and verify they store correctly
	Asset->SkillName = FName(TEXT("Longswords"));
	Asset->Category = EMordecaiSkillCategory::Weapon;
	Asset->RankDescriptions.Add(1, FText::FromString(TEXT("Rank 1")));
	Asset->RankDescriptions.Add(5, FText::FromString(TEXT("Rank 5")));
	Asset->MilestoneAbilities.Add(1, UGameplayAbility::StaticClass());

	TestEqual("SkillName stored", Asset->SkillName, FName(TEXT("Longswords")));
	TestEqual("Category stored", Asset->Category, EMordecaiSkillCategory::Weapon);
	TestEqual("RankDescriptions has 2 entries", Asset->RankDescriptions.Num(), 2);
	TestTrue("RankDescriptions contains rank 1", Asset->RankDescriptions.Contains(1));
	TestTrue("RankDescriptions contains rank 5", Asset->RankDescriptions.Contains(5));
	TestEqual("MilestoneAbilities has 1 entry", Asset->MilestoneAbilities.Num(), 1);
	TestTrue("MilestoneAbilities contains rank 1", Asset->MilestoneAbilities.Contains(1));

	return true;
}

// ===========================================================================
// AC-011.2: SkillCategory enum matches design (5 categories)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSkills_CategoryEnum,
	"Mordecai.Skills.SkillCategoryEnumMatchesDesign",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSkills_CategoryEnum::RunTest(const FString& Parameters)
{
	// Verify all 5 categories exist and have distinct uint8 values
	TestEqual("Weapon value", static_cast<uint8>(EMordecaiSkillCategory::Weapon), 0);
	TestEqual("DefenseArmor value", static_cast<uint8>(EMordecaiSkillCategory::DefenseArmor), 1);
	TestEqual("MovementExploration value", static_cast<uint8>(EMordecaiSkillCategory::MovementExploration), 2);
	TestEqual("UtilityInteraction value", static_cast<uint8>(EMordecaiSkillCategory::UtilityInteraction), 3);
	TestEqual("Magic value", static_cast<uint8>(EMordecaiSkillCategory::Magic), 4);

	return true;
}

// ===========================================================================
// AC-011.3: Skill data assets can be loaded and queried at runtime
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSkills_DataAssetQueryable,
	"Mordecai.Skills.SkillDataAssetQueryable",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSkills_DataAssetQueryable::RunTest(const FString& Parameters)
{
	UMordecaiSkillDataAsset* Asset = CreateTestSkillAsset(
		FName(TEXT("Dodging")), EMordecaiSkillCategory::DefenseArmor, true);

	// Query category
	TestEqual("Category is DefenseArmor", Asset->Category, EMordecaiSkillCategory::DefenseArmor);

	// Query rank description
	const FText* Desc = Asset->RankDescriptions.Find(10);
	TestNotNull("Rank 10 description exists", Desc);

	// Query milestone ability
	const TSubclassOf<UGameplayAbility>* Ability = Asset->MilestoneAbilities.Find(5);
	TestNotNull("Milestone ability at rank 5 exists", Ability);
	TestTrue("Milestone ability class is valid", *Ability != nullptr);

	// Query primary asset ID
	FPrimaryAssetId Id = Asset->GetPrimaryAssetId();
	TestEqual("PrimaryAssetType", Id.PrimaryAssetType.GetName(), FName(TEXT("MordecaiSkill")));
	TestEqual("PrimaryAssetName", Id.PrimaryAssetName, FName(TEXT("Dodging")));

	return true;
}

// ===========================================================================
// AC-011.4: SkillComponent stores per-character skill ranks
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSkills_ComponentStoresRanks,
	"Mordecai.Skills.ComponentStoresSkillRanks",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSkills_ComponentStoresRanks::RunTest(const FString& Parameters)
{
	UMordecaiSkillComponent* Comp = NewObject<UMordecaiSkillComponent>();
	UMordecaiSkillDataAsset* Asset = CreateTestSkillAsset(
		FName(TEXT("Longswords")), EMordecaiSkillCategory::Weapon);

	Comp->RegisterSkillDataAsset(Asset);
	Comp->SetAvailableSkillPoints(5);

	// Allocate 3 points
	TestTrue("Alloc 1", Comp->TryAllocateSkillPoint(FName(TEXT("Longswords"))));
	TestTrue("Alloc 2", Comp->TryAllocateSkillPoint(FName(TEXT("Longswords"))));
	TestTrue("Alloc 3", Comp->TryAllocateSkillPoint(FName(TEXT("Longswords"))));

	TestEqual("Rank is 3", Comp->GetSkillRank(FName(TEXT("Longswords"))), 3);
	TestEqual("Remaining points", Comp->GetAvailableSkillPoints(), 2);

	return true;
}

// ===========================================================================
// AC-011.5: Skill ranks clamped to [0, 20]
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSkills_RankClamped,
	"Mordecai.Skills.RankClampedToRange",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSkills_RankClamped::RunTest(const FString& Parameters)
{
	UMordecaiSkillComponent* Comp = NewObject<UMordecaiSkillComponent>();
	UMordecaiSkillDataAsset* Asset = CreateTestSkillAsset(
		FName(TEXT("Axes")), EMordecaiSkillCategory::Weapon);
	Comp->RegisterSkillDataAsset(Asset);

	// SetSkillRank beyond max -> clamped to 20
	Comp->SetSkillRank(FName(TEXT("Axes")), 99);
	TestEqual("Rank clamped to 20", Comp->GetSkillRank(FName(TEXT("Axes"))), 20);

	// SetSkillRank below min -> clamped to 0
	Comp->SetSkillRank(FName(TEXT("Axes")), -5);
	TestEqual("Rank clamped to 0", Comp->GetSkillRank(FName(TEXT("Axes"))), 0);

	return true;
}

// ===========================================================================
// AC-011.6: Skill ranks replicated to clients
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSkills_Replicated,
	"Mordecai.Skills.SkillRanksReplicated",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSkills_Replicated::RunTest(const FString& Parameters)
{
	// Verify that SkillRanks and AvailableSkillPoints are marked for replication
	FProperty* RanksProp = UMordecaiSkillComponent::StaticClass()->FindPropertyByName(TEXT("SkillRanks"));
	TestNotNull("SkillRanks property exists", RanksProp);
	if (RanksProp)
	{
		TestTrue("SkillRanks has CPF_Net flag", (RanksProp->PropertyFlags & CPF_Net) != 0);
	}

	FProperty* PointsProp = UMordecaiSkillComponent::StaticClass()->FindPropertyByName(TEXT("AvailableSkillPoints"));
	TestNotNull("AvailableSkillPoints property exists", PointsProp);
	if (PointsProp)
	{
		TestTrue("AvailableSkillPoints has CPF_Net flag", (PointsProp->PropertyFlags & CPF_Net) != 0);
	}

	return true;
}

// ===========================================================================
// AC-011.7: GetSkillRank returns 0 for unlearned skill
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSkills_UnlearnedRank,
	"Mordecai.Skills.GetSkillRankReturnsZeroForUnlearned",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSkills_UnlearnedRank::RunTest(const FString& Parameters)
{
	UMordecaiSkillComponent* Comp = NewObject<UMordecaiSkillComponent>();

	// Unregistered skill
	TestEqual("Unregistered skill returns 0", Comp->GetSkillRank(FName(TEXT("Nonexistent"))), 0);

	// Registered but unlearned skill
	UMordecaiSkillDataAsset* Asset = CreateTestSkillAsset(
		FName(TEXT("Sneaking")), EMordecaiSkillCategory::MovementExploration);
	Comp->RegisterSkillDataAsset(Asset);
	TestEqual("Registered but unlearned returns 0", Comp->GetSkillRank(FName(TEXT("Sneaking"))), 0);

	return true;
}

// ===========================================================================
// AC-011.8: GetAllSkillsByCategory filters correctly
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSkills_CategoryFilter,
	"Mordecai.Skills.GetSkillsByCategoryFiltersCorrectly",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSkills_CategoryFilter::RunTest(const FString& Parameters)
{
	UMordecaiSkillComponent* Comp = NewObject<UMordecaiSkillComponent>();

	// Register skills in different categories
	Comp->RegisterSkillDataAsset(CreateTestSkillAsset(FName(TEXT("Longswords")), EMordecaiSkillCategory::Weapon));
	Comp->RegisterSkillDataAsset(CreateTestSkillAsset(FName(TEXT("Greatswords")), EMordecaiSkillCategory::Weapon));
	Comp->RegisterSkillDataAsset(CreateTestSkillAsset(FName(TEXT("Dodging")), EMordecaiSkillCategory::DefenseArmor));
	Comp->RegisterSkillDataAsset(CreateTestSkillAsset(FName(TEXT("Fireball")), EMordecaiSkillCategory::Magic));

	// Set some ranks
	Comp->SetSkillRank(FName(TEXT("Longswords")), 5);
	Comp->SetSkillRank(FName(TEXT("Greatswords")), 3);

	// Query weapon skills
	TArray<FMordecaiSkillRankInfo> Weapons = Comp->GetAllSkillsByCategory(EMordecaiSkillCategory::Weapon);
	TestEqual("2 weapon skills", Weapons.Num(), 2);

	// Query magic skills
	TArray<FMordecaiSkillRankInfo> Magic = Comp->GetAllSkillsByCategory(EMordecaiSkillCategory::Magic);
	TestEqual("1 magic skill", Magic.Num(), 1);
	TestEqual("Magic skill is Fireball", Magic[0].SkillName, FName(TEXT("Fireball")));
	TestEqual("Fireball rank is 0", Magic[0].CurrentRank, 0);

	// Query category with no skills
	TArray<FMordecaiSkillRankInfo> Utility = Comp->GetAllSkillsByCategory(EMordecaiSkillCategory::UtilityInteraction);
	TestEqual("0 utility skills", Utility.Num(), 0);

	// Verify weapon skills have correct ranks
	bool bFoundLongswords = false;
	for (const FMordecaiSkillRankInfo& Info : Weapons)
	{
		if (Info.SkillName == FName(TEXT("Longswords")))
		{
			TestEqual("Longswords rank is 5", Info.CurrentRank, 5);
			bFoundLongswords = true;
		}
	}
	TestTrue("Longswords found in weapon list", bFoundLongswords);

	return true;
}

// ===========================================================================
// AC-011.9: AvailableSkillPoints tracked
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSkills_PointsTracked,
	"Mordecai.Skills.AvailableSkillPointsTracked",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSkills_PointsTracked::RunTest(const FString& Parameters)
{
	UMordecaiSkillComponent* Comp = NewObject<UMordecaiSkillComponent>();

	// Default is 0
	TestEqual("Default points is 0", Comp->GetAvailableSkillPoints(), 0);

	// Set and read
	Comp->SetAvailableSkillPoints(10);
	TestEqual("Points set to 10", Comp->GetAvailableSkillPoints(), 10);

	// Clamped >= 0
	Comp->SetAvailableSkillPoints(-5);
	TestEqual("Negative clamped to 0", Comp->GetAvailableSkillPoints(), 0);

	return true;
}

// ===========================================================================
// AC-011.10: TryAllocateSkillPoint succeeds when valid
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSkills_AllocateSucceeds,
	"Mordecai.Skills.AllocatePointSucceedsWhenValid",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSkills_AllocateSucceeds::RunTest(const FString& Parameters)
{
	UMordecaiSkillComponent* Comp = NewObject<UMordecaiSkillComponent>();
	UMordecaiSkillDataAsset* Asset = CreateTestSkillAsset(
		FName(TEXT("Maces")), EMordecaiSkillCategory::Weapon);
	Comp->RegisterSkillDataAsset(Asset);
	Comp->SetAvailableSkillPoints(3);

	// Valid allocation: skill exists, rank < 20, points > 0
	bool bResult = Comp->TryAllocateSkillPoint(FName(TEXT("Maces")));
	TestTrue("Allocation succeeded", bResult);
	TestEqual("Rank incremented to 1", Comp->GetSkillRank(FName(TEXT("Maces"))), 1);
	TestEqual("Points decremented to 2", Comp->GetAvailableSkillPoints(), 2);

	// Allocate again
	bResult = Comp->TryAllocateSkillPoint(FName(TEXT("Maces")));
	TestTrue("Second allocation succeeded", bResult);
	TestEqual("Rank incremented to 2", Comp->GetSkillRank(FName(TEXT("Maces"))), 2);
	TestEqual("Points decremented to 1", Comp->GetAvailableSkillPoints(), 1);

	return true;
}

// ===========================================================================
// AC-011.11: TryAllocateSkillPoint fails gracefully
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSkills_AllocateFails,
	"Mordecai.Skills.AllocatePointFailsGracefully",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSkills_AllocateFails::RunTest(const FString& Parameters)
{
	UMordecaiSkillComponent* Comp = NewObject<UMordecaiSkillComponent>();
	UMordecaiSkillDataAsset* Asset = CreateTestSkillAsset(
		FName(TEXT("Spears")), EMordecaiSkillCategory::Weapon);
	Comp->RegisterSkillDataAsset(Asset);

	// Fail: no points available
	Comp->SetAvailableSkillPoints(0);
	TestFalse("Fails with 0 points", Comp->TryAllocateSkillPoint(FName(TEXT("Spears"))));
	TestEqual("Rank unchanged at 0", Comp->GetSkillRank(FName(TEXT("Spears"))), 0);

	// Fail: skill doesn't exist
	Comp->SetAvailableSkillPoints(5);
	TestFalse("Fails with unknown skill", Comp->TryAllocateSkillPoint(FName(TEXT("Nonexistent"))));
	TestEqual("Points unchanged at 5", Comp->GetAvailableSkillPoints(), 5);

	// Fail: already at max rank
	Comp->SetSkillRank(FName(TEXT("Spears")), 20);
	TestFalse("Fails at max rank", Comp->TryAllocateSkillPoint(FName(TEXT("Spears"))));
	TestEqual("Rank still 20", Comp->GetSkillRank(FName(TEXT("Spears"))), 20);
	TestEqual("Points still 5", Comp->GetAvailableSkillPoints(), 5);

	return true;
}

// ===========================================================================
// AC-011.12: Learning a new skill (rank 0 -> 1) costs 1 point
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSkills_LearnNewSkill,
	"Mordecai.Skills.LearningNewSkillCostsOnePoint",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSkills_LearnNewSkill::RunTest(const FString& Parameters)
{
	UMordecaiSkillComponent* Comp = NewObject<UMordecaiSkillComponent>();
	UMordecaiSkillDataAsset* Asset = CreateTestSkillAsset(
		FName(TEXT("Lockpicking")), EMordecaiSkillCategory::UtilityInteraction);
	Comp->RegisterSkillDataAsset(Asset);
	Comp->SetAvailableSkillPoints(1);

	// Skill starts at rank 0
	TestEqual("Starts at rank 0", Comp->GetSkillRank(FName(TEXT("Lockpicking"))), 0);

	// Allocate one point to learn the skill
	bool bResult = Comp->TryAllocateSkillPoint(FName(TEXT("Lockpicking")));
	TestTrue("Learning succeeded", bResult);
	TestEqual("Rank is now 1", Comp->GetSkillRank(FName(TEXT("Lockpicking"))), 1);
	TestEqual("Points decremented to 0", Comp->GetAvailableSkillPoints(), 0);

	return true;
}

// ===========================================================================
// AC-011.13: Milestone fires gameplay event (via delegate)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSkills_MilestoneEvent,
	"Mordecai.Skills.MilestoneFiresGameplayEvent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSkills_MilestoneEvent::RunTest(const FString& Parameters)
{
	UMordecaiSkillComponent* Comp = NewObject<UMordecaiSkillComponent>();
	UMordecaiSkillDataAsset* Asset = CreateTestSkillAsset(
		FName(TEXT("Parrying")), EMordecaiSkillCategory::DefenseArmor);
	Comp->RegisterSkillDataAsset(Asset);
	Comp->SetAvailableSkillPoints(5);

	// Track delegate fires
	TArray<TPair<FName, int32>> MilestonesReceived;
	Comp->OnSkillMilestoneReached.AddLambda([&MilestonesReceived](FName Name, int32 Rank)
	{
		MilestonesReceived.Add(TPair<FName, int32>(Name, Rank));
	});

	// Allocate to rank 1 (milestone)
	Comp->TryAllocateSkillPoint(FName(TEXT("Parrying")));
	TestEqual("1 milestone fired after rank 1", MilestonesReceived.Num(), 1);
	TestEqual("Milestone skill name", MilestonesReceived[0].Key, FName(TEXT("Parrying")));
	TestEqual("Milestone rank", MilestonesReceived[0].Value, 1);

	// Allocate ranks 2-4 (no milestones)
	MilestonesReceived.Empty();
	Comp->TryAllocateSkillPoint(FName(TEXT("Parrying")));
	Comp->TryAllocateSkillPoint(FName(TEXT("Parrying")));
	Comp->TryAllocateSkillPoint(FName(TEXT("Parrying")));
	TestEqual("No milestones for ranks 2-4", MilestonesReceived.Num(), 0);

	// Allocate to rank 5 (milestone)
	Comp->TryAllocateSkillPoint(FName(TEXT("Parrying")));
	TestEqual("1 milestone fired at rank 5", MilestonesReceived.Num(), 1);
	TestEqual("Milestone rank is 5", MilestonesReceived[0].Value, 5);

	return true;
}

// ===========================================================================
// AC-011.14: Milestone grants ability
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSkills_MilestoneAbility,
	"Mordecai.Skills.MilestoneGrantsAbility",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSkills_MilestoneAbility::RunTest(const FString& Parameters)
{
	UMordecaiSkillComponent* Comp = NewObject<UMordecaiSkillComponent>();
	UMordecaiSkillDataAsset* Asset = CreateTestSkillAsset(
		FName(TEXT("Fireball")), EMordecaiSkillCategory::Magic, true /* with milestone abilities */);
	Comp->RegisterSkillDataAsset(Asset);

	// Before allocation, no milestone abilities granted
	TestFalse("No ability at rank 1 before", Comp->HasGrantedMilestoneAbility(FName(TEXT("Fireball")), 1));

	// Set rank to 5 — should grant milestones at rank 1 and 5
	Comp->SetSkillRank(FName(TEXT("Fireball")), 5);
	TestTrue("Ability granted at rank 1", Comp->HasGrantedMilestoneAbility(FName(TEXT("Fireball")), 1));
	TestTrue("Ability granted at rank 5", Comp->HasGrantedMilestoneAbility(FName(TEXT("Fireball")), 5));
	TestFalse("No ability at rank 10", Comp->HasGrantedMilestoneAbility(FName(TEXT("Fireball")), 10));

	return true;
}

// ===========================================================================
// AC-011.15: SetRank grants all milestones at or below current rank
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSkills_SetRankMilestones,
	"Mordecai.Skills.SetRankGrantsAllMilestonesBelowOrEqual",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSkills_SetRankMilestones::RunTest(const FString& Parameters)
{
	UMordecaiSkillComponent* Comp = NewObject<UMordecaiSkillComponent>();
	UMordecaiSkillDataAsset* Asset = CreateTestSkillAsset(
		FName(TEXT("Dodging")), EMordecaiSkillCategory::DefenseArmor, true);
	Comp->RegisterSkillDataAsset(Asset);

	// Jump to rank 10
	Comp->SetSkillRank(FName(TEXT("Dodging")), 10);
	TestTrue("Rank 1 ability granted", Comp->HasGrantedMilestoneAbility(FName(TEXT("Dodging")), 1));
	TestTrue("Rank 5 ability granted", Comp->HasGrantedMilestoneAbility(FName(TEXT("Dodging")), 5));
	TestTrue("Rank 10 ability granted", Comp->HasGrantedMilestoneAbility(FName(TEXT("Dodging")), 10));
	TestFalse("Rank 15 ability NOT granted", Comp->HasGrantedMilestoneAbility(FName(TEXT("Dodging")), 15));
	TestFalse("Rank 20 ability NOT granted", Comp->HasGrantedMilestoneAbility(FName(TEXT("Dodging")), 20));

	// Drop rank to 3 — milestones at 5 and 10 should be revoked
	Comp->SetSkillRank(FName(TEXT("Dodging")), 3);
	TestTrue("Rank 1 ability still granted", Comp->HasGrantedMilestoneAbility(FName(TEXT("Dodging")), 1));
	TestFalse("Rank 5 ability revoked", Comp->HasGrantedMilestoneAbility(FName(TEXT("Dodging")), 5));
	TestFalse("Rank 10 ability revoked", Comp->HasGrantedMilestoneAbility(FName(TEXT("Dodging")), 10));

	return true;
}

// ===========================================================================
// AC-011.16: Milestone tags applied and removed
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSkills_MilestoneTags,
	"Mordecai.Skills.MilestoneTagsAppliedAndRemoved",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSkills_MilestoneTags::RunTest(const FString& Parameters)
{
	UMordecaiSkillComponent* Comp = NewObject<UMordecaiSkillComponent>();
	UMordecaiSkillDataAsset* Asset = CreateTestSkillAsset(
		FName(TEXT("Sneaking")), EMordecaiSkillCategory::MovementExploration);
	Comp->RegisterSkillDataAsset(Asset);

	// Construct expected tag names
	FName Tag1Name = UMordecaiSkillComponent::MakeSkillMilestoneTagName(FName(TEXT("Sneaking")), 1);
	FName Tag5Name = UMordecaiSkillComponent::MakeSkillMilestoneTagName(FName(TEXT("Sneaking")), 5);

	// Verify tag names are constructed correctly
	TestEqual("Tag1 name", Tag1Name.ToString(), FString(TEXT("Mordecai.Skill.Sneaking.Rank1")));
	TestEqual("Tag5 name", Tag5Name.ToString(), FString(TEXT("Mordecai.Skill.Sneaking.Rank5")));

	// Set rank to 5 — tags for rank 1 and 5 should be active
	Comp->SetSkillRank(FName(TEXT("Sneaking")), 5);

	TestTrue("Tag for rank 1 is active", Comp->HasMilestoneTag(FName(TEXT("Sneaking")), 1));
	TestTrue("Tag for rank 5 is active", Comp->HasMilestoneTag(FName(TEXT("Sneaking")), 5));
	TestFalse("Tag for rank 10 is NOT active", Comp->HasMilestoneTag(FName(TEXT("Sneaking")), 10));
	TestFalse("Tag for rank 15 is NOT active", Comp->HasMilestoneTag(FName(TEXT("Sneaking")), 15));
	TestFalse("Tag for rank 20 is NOT active", Comp->HasMilestoneTag(FName(TEXT("Sneaking")), 20));

	// Verify the active tag names set
	const TSet<FName>& TagNames = Comp->GetActiveMilestoneTagNames();
	TestTrue("Tag name set contains rank 1", TagNames.Contains(Tag1Name));
	TestTrue("Tag name set contains rank 5", TagNames.Contains(Tag5Name));
	TestEqual("Tag name set has exactly 2 entries", TagNames.Num(), 2);

	// Drop rank to 3 — tag for rank 5 should be removed
	Comp->SetSkillRank(FName(TEXT("Sneaking")), 3);
	TestTrue("Tag for rank 1 still active", Comp->HasMilestoneTag(FName(TEXT("Sneaking")), 1));
	TestFalse("Tag for rank 5 removed", Comp->HasMilestoneTag(FName(TEXT("Sneaking")), 5));

	return true;
}

// ===========================================================================
// AC-011.17: Longswords sample pipeline (data → allocation → milestone grant)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSkills_LongswordsPipeline,
	"Mordecai.Skills.LongswordsSamplePipeline",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSkills_LongswordsPipeline::RunTest(const FString& Parameters)
{
	// --- Create Longswords data asset with placeholder milestones ---
	UMordecaiSkillDataAsset* Longswords = NewObject<UMordecaiSkillDataAsset>();
	Longswords->SkillName = FName(TEXT("Longswords"));
	Longswords->Category = EMordecaiSkillCategory::Weapon;

	Longswords->RankDescriptions.Add(1, FText::FromString(TEXT("Arcing sweep attack")));
	Longswords->RankDescriptions.Add(5, FText::FromString(TEXT("Charged spinning attack")));
	Longswords->RankDescriptions.Add(10, FText::FromString(TEXT("3-swing combo")));
	Longswords->RankDescriptions.Add(15, FText::FromString(TEXT("Cyclone heavy attack")));
	Longswords->RankDescriptions.Add(20, FText::FromString(TEXT("Projectile combo finishers")));

	Longswords->MilestoneAbilities.Add(1, UGameplayAbility::StaticClass());
	Longswords->MilestoneAbilities.Add(5, UGameplayAbility::StaticClass());
	Longswords->MilestoneAbilities.Add(10, UGameplayAbility::StaticClass());
	Longswords->MilestoneAbilities.Add(15, UGameplayAbility::StaticClass());
	Longswords->MilestoneAbilities.Add(20, UGameplayAbility::StaticClass());

	// --- Set up component ---
	UMordecaiSkillComponent* Comp = NewObject<UMordecaiSkillComponent>();
	Comp->RegisterSkillDataAsset(Longswords);
	Comp->SetAvailableSkillPoints(20);

	// Track milestone events
	TArray<int32> MilestoneRanksHit;
	Comp->OnSkillMilestoneReached.AddLambda([&MilestoneRanksHit](FName Name, int32 Rank)
	{
		MilestoneRanksHit.Add(Rank);
	});

	// --- Allocate all 20 points one by one ---
	for (int32 i = 0; i < 20; ++i)
	{
		TestTrue(FString::Printf(TEXT("Alloc point %d"), i + 1),
			Comp->TryAllocateSkillPoint(FName(TEXT("Longswords"))));
	}

	// --- Verify final state ---
	TestEqual("Final rank is 20", Comp->GetSkillRank(FName(TEXT("Longswords"))), 20);
	TestEqual("All points spent", Comp->GetAvailableSkillPoints(), 0);

	// Verify all 5 milestones fired
	TestEqual("5 milestones hit", MilestoneRanksHit.Num(), 5);
	TestTrue("Milestone 1 hit", MilestoneRanksHit.Contains(1));
	TestTrue("Milestone 5 hit", MilestoneRanksHit.Contains(5));
	TestTrue("Milestone 10 hit", MilestoneRanksHit.Contains(10));
	TestTrue("Milestone 15 hit", MilestoneRanksHit.Contains(15));
	TestTrue("Milestone 20 hit", MilestoneRanksHit.Contains(20));

	// Verify all milestone abilities granted
	TestTrue("Ability at rank 1", Comp->HasGrantedMilestoneAbility(FName(TEXT("Longswords")), 1));
	TestTrue("Ability at rank 5", Comp->HasGrantedMilestoneAbility(FName(TEXT("Longswords")), 5));
	TestTrue("Ability at rank 10", Comp->HasGrantedMilestoneAbility(FName(TEXT("Longswords")), 10));
	TestTrue("Ability at rank 15", Comp->HasGrantedMilestoneAbility(FName(TEXT("Longswords")), 15));
	TestTrue("Ability at rank 20", Comp->HasGrantedMilestoneAbility(FName(TEXT("Longswords")), 20));

	// Verify all milestone tags active
	for (int32 MR : { 1, 5, 10, 15, 20 })
	{
		TestTrue(FString::Printf(TEXT("Tag for rank %d is active"), MR),
			Comp->HasMilestoneTag(FName(TEXT("Longswords")), MR));
	}

	// --- Verify can't allocate beyond max ---
	TestFalse("Can't allocate at max rank", Comp->TryAllocateSkillPoint(FName(TEXT("Longswords"))));

	return true;
}

// ===========================================================================
// Bonus: Verify PlayerState has SkillComponent
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSkills_PlayerStateHasComponent,
	"Mordecai.Skills.PlayerStateHasSkillComponent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSkills_PlayerStateHasComponent::RunTest(const FString& Parameters)
{
	const AMordecaiPlayerState* CDO = GetDefault<AMordecaiPlayerState>();
	TestNotNull("PlayerState CDO exists", CDO);

	UMordecaiSkillComponent* SkillComp = const_cast<AMordecaiPlayerState*>(CDO)->GetSkillComponent();
	TestNotNull("SkillComponent exists on PlayerState", SkillComp);

	return true;
}
