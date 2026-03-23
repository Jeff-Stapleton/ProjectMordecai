// Project Mordecai — Feat System Tests (US-012)

#include "Misc/AutomationTest.h"
#include "Mordecai/Feats/MordecaiFeatTypes.h"
#include "Mordecai/Feats/MordecaiFeatDataAsset.h"
#include "Mordecai/Feats/MordecaiFeatComponent.h"
#include "Mordecai/Skills/MordecaiSkillComponent.h"
#include "Mordecai/Skills/MordecaiSkillDataAsset.h"
#include "Mordecai/Skills/MordecaiSkillTypes.h"
#include "Mordecai/MordecaiPlayerState.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"

// ---------------------------------------------------------------------------
// Helper: Create a test feat data asset
// ---------------------------------------------------------------------------
static UMordecaiFeatDataAsset* CreateTestFeatAsset(
	FName FeatName,
	EMordecaiFeatTier Tier,
	int32 NumGrantedEffects = 0,
	int32 NumDrawbackEffects = 0,
	int32 NumGrantedAbilities = 0)
{
	UMordecaiFeatDataAsset* Asset = NewObject<UMordecaiFeatDataAsset>();
	Asset->FeatName = FeatName;
	Asset->DisplayName = FText::FromString(FeatName.ToString());
	Asset->Description = FText::FromString(FString::Printf(TEXT("Test feat: %s"), *FeatName.ToString()));
	Asset->Tier = Tier;

	for (int32 i = 0; i < NumGrantedEffects; ++i)
	{
		Asset->GrantedEffects.Add(UGameplayEffect::StaticClass());
	}

	for (int32 i = 0; i < NumDrawbackEffects; ++i)
	{
		Asset->DrawbackEffects.Add(UGameplayEffect::StaticClass());
	}

	for (int32 i = 0; i < NumGrantedAbilities; ++i)
	{
		Asset->GrantedAbilities.Add(UGameplayAbility::StaticClass());
	}

	return Asset;
}

// ---------------------------------------------------------------------------
// Helper: Create a test skill data asset (for SkillRank condition tests)
// ---------------------------------------------------------------------------
static UMordecaiSkillDataAsset* CreateTestSkillDataAsset(FName SkillName)
{
	UMordecaiSkillDataAsset* Asset = NewObject<UMordecaiSkillDataAsset>();
	Asset->SkillName = SkillName;
	Asset->Category = EMordecaiSkillCategory::Weapon;
	Asset->RankDescriptions.Add(1, FText::FromString(TEXT("Rank 1")));
	return Asset;
}

// ===========================================================================
// AC-012.1: UMordecaiFeatDataAsset has required fields
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFeats_DataAssetFields,
	"Mordecai.Feats.DataAssetHasRequiredFields",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFeats_DataAssetFields::RunTest(const FString& Parameters)
{
	UMordecaiFeatDataAsset* Asset = NewObject<UMordecaiFeatDataAsset>();
	TestNotNull("DataAsset created", Asset);

	// Set all required fields and verify they store correctly
	Asset->FeatName = FName(TEXT("Pyromaniac"));
	Asset->DisplayName = FText::FromString(TEXT("Pyromaniac"));
	Asset->Description = FText::FromString(TEXT("Burn them all"));
	Asset->Tier = EMordecaiFeatTier::Common;
	Asset->GrantedEffects.Add(UGameplayEffect::StaticClass());
	Asset->GrantedAbilities.Add(UGameplayAbility::StaticClass());
	Asset->DrawbackEffects.Add(UGameplayEffect::StaticClass());
	Asset->FeatTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Event.FeatUnlocked")), false);

	TestEqual("FeatName stored", Asset->FeatName, FName(TEXT("Pyromaniac")));
	TestFalse("DisplayName not empty", Asset->DisplayName.IsEmpty());
	TestFalse("Description not empty", Asset->Description.IsEmpty());
	TestEqual("Tier is Common", Asset->Tier, EMordecaiFeatTier::Common);
	TestEqual("GrantedEffects has 1 entry", Asset->GrantedEffects.Num(), 1);
	TestEqual("GrantedAbilities has 1 entry", Asset->GrantedAbilities.Num(), 1);
	TestEqual("DrawbackEffects has 1 entry", Asset->DrawbackEffects.Num(), 1);
	TestTrue("FeatTag is valid", Asset->FeatTag.IsValid());

	return true;
}

// ===========================================================================
// AC-012.2: EMordecaiFeatTier enum defines three tiers
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFeats_TierEnum,
	"Mordecai.Feats.TierEnumMatchesDesign",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFeats_TierEnum::RunTest(const FString& Parameters)
{
	TestEqual("Common value", static_cast<uint8>(EMordecaiFeatTier::Common), 0);
	TestEqual("Rare value", static_cast<uint8>(EMordecaiFeatTier::Rare), 1);
	TestEqual("Legendary value", static_cast<uint8>(EMordecaiFeatTier::Legendary), 2);

	// Verify all three are distinct
	TestTrue("Common != Rare", EMordecaiFeatTier::Common != EMordecaiFeatTier::Rare);
	TestTrue("Rare != Legendary", EMordecaiFeatTier::Rare != EMordecaiFeatTier::Legendary);
	TestTrue("Common != Legendary", EMordecaiFeatTier::Common != EMordecaiFeatTier::Legendary);

	return true;
}

// ===========================================================================
// AC-012.3: Feat data assets are queryable at runtime
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFeats_DataAssetQueryable,
	"Mordecai.Feats.DataAssetQueryable",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFeats_DataAssetQueryable::RunTest(const FString& Parameters)
{
	UMordecaiFeatDataAsset* Asset = CreateTestFeatAsset(
		FName(TEXT("IronWill")), EMordecaiFeatTier::Rare, 2, 1, 1);

	// Query tier
	TestEqual("Tier is Rare", Asset->Tier, EMordecaiFeatTier::Rare);

	// Query effects
	TestEqual("2 granted effects", Asset->GrantedEffects.Num(), 2);
	TestEqual("1 drawback effect", Asset->DrawbackEffects.Num(), 1);
	TestEqual("1 granted ability", Asset->GrantedAbilities.Num(), 1);

	// Query description
	TestFalse("Description not empty", Asset->Description.IsEmpty());

	// Query primary asset ID
	FPrimaryAssetId Id = Asset->GetPrimaryAssetId();
	TestEqual("PrimaryAssetType", Id.PrimaryAssetType.GetName(), FName(TEXT("MordecaiFeat")));
	TestEqual("PrimaryAssetName", Id.PrimaryAssetName, FName(TEXT("IronWill")));

	return true;
}

// ===========================================================================
// AC-012.4: FMordecaiFeatCondition struct defines condition types
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFeats_ConditionStructTypes,
	"Mordecai.Feats.ConditionStructDefinesTypes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFeats_ConditionStructTypes::RunTest(const FString& Parameters)
{
	// Verify all 4 condition types exist and are distinct
	TestEqual("StatThreshold value", static_cast<uint8>(EMordecaiFeatConditionType::StatThreshold), 0);
	TestEqual("EventCount value", static_cast<uint8>(EMordecaiFeatConditionType::EventCount), 1);
	TestEqual("TagPresent value", static_cast<uint8>(EMordecaiFeatConditionType::TagPresent), 2);
	TestEqual("SkillRank value", static_cast<uint8>(EMordecaiFeatConditionType::SkillRank), 3);

	// Verify condition struct fields
	FMordecaiFeatCondition Cond;
	Cond.ConditionType = EMordecaiFeatConditionType::StatThreshold;
	Cond.StatName = FName(TEXT("EnemiesBurned"));
	Cond.ThresholdValue = 6;
	TestEqual("ConditionType stored", Cond.ConditionType, EMordecaiFeatConditionType::StatThreshold);
	TestEqual("StatName stored", Cond.StatName, FName(TEXT("EnemiesBurned")));
	TestEqual("ThresholdValue stored", Cond.ThresholdValue, 6);

	// TagPresent condition
	FMordecaiFeatCondition TagCond;
	TagCond.ConditionType = EMordecaiFeatConditionType::TagPresent;
	TagCond.RequiredTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.State.Blocking")), false);
	TestEqual("TagPresent type stored", TagCond.ConditionType, EMordecaiFeatConditionType::TagPresent);

	return true;
}

// ===========================================================================
// AC-012.5: All conditions must be met (AND logic)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFeats_AllConditionsMustBeMet,
	"Mordecai.Feats.AllConditionsMustBeMet",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFeats_AllConditionsMustBeMet::RunTest(const FString& Parameters)
{
	UMordecaiFeatComponent* Comp = NewObject<UMordecaiFeatComponent>();

	// Create a feat with 2 conditions (both must pass)
	UMordecaiFeatDataAsset* Asset = CreateTestFeatAsset(FName(TEXT("MultiCond")), EMordecaiFeatTier::Common, 1);

	FMordecaiFeatCondition Cond1;
	Cond1.ConditionType = EMordecaiFeatConditionType::StatThreshold;
	Cond1.StatName = FName(TEXT("StatA"));
	Cond1.ThresholdValue = 5;

	FMordecaiFeatCondition Cond2;
	Cond2.ConditionType = EMordecaiFeatConditionType::EventCount;
	Cond2.StatName = FName(TEXT("StatB"));
	Cond2.ThresholdValue = 10;

	Asset->UnlockConditions.Add(Cond1);
	Asset->UnlockConditions.Add(Cond2);

	Comp->RegisterFeatDataAsset(Asset);

	// Meet only first condition
	Comp->IncrementFeatStat(FName(TEXT("StatA")), 5);
	TestFalse("Not unlocked with only 1 condition met", Comp->HasFeat(FName(TEXT("MultiCond"))));

	// Meet second condition — now both are met
	Comp->IncrementFeatStat(FName(TEXT("StatB")), 10);
	TestTrue("Unlocked when both conditions met", Comp->HasFeat(FName(TEXT("MultiCond"))));

	return true;
}

// ===========================================================================
// AC-012.6: StatThreshold condition evaluates correctly
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFeats_StatThreshold,
	"Mordecai.Feats.StatThresholdConditionEvaluates",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFeats_StatThreshold::RunTest(const FString& Parameters)
{
	UMordecaiFeatComponent* Comp = NewObject<UMordecaiFeatComponent>();

	UMordecaiFeatDataAsset* Asset = CreateTestFeatAsset(FName(TEXT("Threshold")), EMordecaiFeatTier::Common, 1);
	FMordecaiFeatCondition Cond;
	Cond.ConditionType = EMordecaiFeatConditionType::StatThreshold;
	Cond.StatName = FName(TEXT("EnemiesBurnedSimultaneously"));
	Cond.ThresholdValue = 6;
	Asset->UnlockConditions.Add(Cond);
	Comp->RegisterFeatDataAsset(Asset);

	// Below threshold
	Comp->IncrementFeatStat(FName(TEXT("EnemiesBurnedSimultaneously")), 3);
	TestFalse("Not unlocked below threshold", Comp->HasFeat(FName(TEXT("Threshold"))));
	TestEqual("Stat value is 3", Comp->GetFeatStatValue(FName(TEXT("EnemiesBurnedSimultaneously"))), 3);

	// At threshold
	Comp->IncrementFeatStat(FName(TEXT("EnemiesBurnedSimultaneously")), 3);
	TestTrue("Unlocked at threshold", Comp->HasFeat(FName(TEXT("Threshold"))));
	TestEqual("Stat value is 6", Comp->GetFeatStatValue(FName(TEXT("EnemiesBurnedSimultaneously"))), 6);

	return true;
}

// ===========================================================================
// AC-012.7: EventCount condition evaluates correctly
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFeats_EventCount,
	"Mordecai.Feats.EventCountConditionEvaluates",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFeats_EventCount::RunTest(const FString& Parameters)
{
	UMordecaiFeatComponent* Comp = NewObject<UMordecaiFeatComponent>();

	UMordecaiFeatDataAsset* Asset = CreateTestFeatAsset(FName(TEXT("ParryMaster")), EMordecaiFeatTier::Common, 1);
	FMordecaiFeatCondition Cond;
	Cond.ConditionType = EMordecaiFeatConditionType::EventCount;
	Cond.StatName = FName(TEXT("TotalPerfectParries"));
	Cond.ThresholdValue = 50;
	Asset->UnlockConditions.Add(Cond);
	Comp->RegisterFeatDataAsset(Asset);

	// Accumulate over multiple increments
	Comp->IncrementFeatStat(FName(TEXT("TotalPerfectParries")), 20);
	TestFalse("Not unlocked at 20", Comp->HasFeat(FName(TEXT("ParryMaster"))));

	Comp->IncrementFeatStat(FName(TEXT("TotalPerfectParries")), 20);
	TestFalse("Not unlocked at 40", Comp->HasFeat(FName(TEXT("ParryMaster"))));

	Comp->IncrementFeatStat(FName(TEXT("TotalPerfectParries")), 10);
	TestTrue("Unlocked at 50", Comp->HasFeat(FName(TEXT("ParryMaster"))));
	TestEqual("Stat value is 50", Comp->GetFeatStatValue(FName(TEXT("TotalPerfectParries"))), 50);

	return true;
}

// ===========================================================================
// AC-012.8: TagPresent condition evaluates correctly
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFeats_TagPresent,
	"Mordecai.Feats.TagPresentConditionEvaluates",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFeats_TagPresent::RunTest(const FString& Parameters)
{
	UMordecaiFeatComponent* Comp = NewObject<UMordecaiFeatComponent>();

	// Use a native tag that definitely exists
	FGameplayTag BlockingTag = MordecaiGameplayTags::State_Blocking;
	TestTrue("Blocking tag is valid", BlockingTag.IsValid());

	UMordecaiFeatDataAsset* Asset = CreateTestFeatAsset(FName(TEXT("Guardian")), EMordecaiFeatTier::Common, 1);
	FMordecaiFeatCondition TagCond;
	TagCond.ConditionType = EMordecaiFeatConditionType::TagPresent;
	TagCond.RequiredTag = BlockingTag;
	Asset->UnlockConditions.Add(TagCond);

	// Also need a stat condition to trigger evaluation
	FMordecaiFeatCondition StatCond;
	StatCond.ConditionType = EMordecaiFeatConditionType::StatThreshold;
	StatCond.StatName = FName(TEXT("BlockCount"));
	StatCond.ThresholdValue = 1;
	Asset->UnlockConditions.Add(StatCond);

	Comp->RegisterFeatDataAsset(Asset);

	// Meet stat condition but NOT tag condition
	Comp->IncrementFeatStat(FName(TEXT("BlockCount")), 1);
	TestFalse("Not unlocked without tag", Comp->HasFeat(FName(TEXT("Guardian"))));

	// Add the tag — triggers re-evaluation, now both conditions met
	Comp->NotifyTagChanged(BlockingTag, true);
	TestTrue("Unlocked when tag present and stat met", Comp->HasFeat(FName(TEXT("Guardian"))));

	return true;
}

// ===========================================================================
// AC-012.9: SkillRank condition evaluates correctly
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFeats_SkillRank,
	"Mordecai.Feats.SkillRankConditionEvaluates",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFeats_SkillRank::RunTest(const FString& Parameters)
{
	UMordecaiFeatComponent* FeatComp = NewObject<UMordecaiFeatComponent>();
	UMordecaiSkillComponent* SkillComp = NewObject<UMordecaiSkillComponent>();

	// Wire up skill component via override
	FeatComp->SetSkillComponentOverride(SkillComp);

	// Register a skill
	UMordecaiSkillDataAsset* SkillAsset = CreateTestSkillDataAsset(FName(TEXT("Longswords")));
	SkillComp->RegisterSkillDataAsset(SkillAsset);

	// Create feat with SkillRank condition
	UMordecaiFeatDataAsset* FeatAsset = CreateTestFeatAsset(FName(TEXT("BladeSpecialist")), EMordecaiFeatTier::Common, 1);
	FMordecaiFeatCondition Cond;
	Cond.ConditionType = EMordecaiFeatConditionType::SkillRank;
	Cond.StatName = FName(TEXT("Longswords")); // Skill name
	Cond.ThresholdValue = 10;
	FeatAsset->UnlockConditions.Add(Cond);

	FeatComp->RegisterFeatDataAsset(FeatAsset);

	// Skill at rank 5 — not enough
	SkillComp->SetSkillRank(FName(TEXT("Longswords")), 5);
	FeatComp->NotifySkillRankChanged(FName(TEXT("Longswords")), 5);
	TestFalse("Not unlocked at skill rank 5", FeatComp->HasFeat(FName(TEXT("BladeSpecialist"))));

	// Skill at rank 10 — meets threshold
	SkillComp->SetSkillRank(FName(TEXT("Longswords")), 10);
	FeatComp->NotifySkillRankChanged(FName(TEXT("Longswords")), 10);
	TestTrue("Unlocked at skill rank 10", FeatComp->HasFeat(FName(TEXT("BladeSpecialist"))));

	return true;
}

// ===========================================================================
// AC-012.10: FeatComponent tracks unlocked feats and stats
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFeats_ComponentTracking,
	"Mordecai.Feats.ComponentTracksUnlockedFeats",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFeats_ComponentTracking::RunTest(const FString& Parameters)
{
	UMordecaiFeatComponent* Comp = NewObject<UMordecaiFeatComponent>();

	// Initially empty
	TestEqual("No feats unlocked initially", Comp->GetUnlockedFeats().Num(), 0);
	TestEqual("Untracked stat returns 0", Comp->GetFeatStatValue(FName(TEXT("NonExistent"))), 0);

	// Track a stat
	Comp->IncrementFeatStat(FName(TEXT("TestStat")), 5);
	TestEqual("Stat tracked correctly", Comp->GetFeatStatValue(FName(TEXT("TestStat"))), 5);

	// Increment again
	Comp->IncrementFeatStat(FName(TEXT("TestStat")), 3);
	TestEqual("Stat accumulated", Comp->GetFeatStatValue(FName(TEXT("TestStat"))), 8);

	// Register and unlock a feat
	UMordecaiFeatDataAsset* Asset = CreateTestFeatAsset(FName(TEXT("TestFeat")), EMordecaiFeatTier::Common, 1);
	FMordecaiFeatCondition Cond;
	Cond.ConditionType = EMordecaiFeatConditionType::StatThreshold;
	Cond.StatName = FName(TEXT("TestStat"));
	Cond.ThresholdValue = 5;
	Asset->UnlockConditions.Add(Cond);
	Comp->RegisterFeatDataAsset(Asset);

	// Stat already meets threshold — trigger re-evaluation via another increment
	Comp->IncrementFeatStat(FName(TEXT("TestStat")), 1);
	TestTrue("Feat unlocked", Comp->HasFeat(FName(TEXT("TestFeat"))));
	TestEqual("1 feat unlocked", Comp->GetUnlockedFeats().Num(), 1);
	TestTrue("Unlocked feats contains TestFeat", Comp->GetUnlockedFeats().Contains(FName(TEXT("TestFeat"))));

	return true;
}

// ===========================================================================
// AC-012.11: IncrementFeatStat triggers condition evaluation
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFeats_IncrementTriggersEval,
	"Mordecai.Feats.IncrementStatTriggersEvaluation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFeats_IncrementTriggersEval::RunTest(const FString& Parameters)
{
	UMordecaiFeatComponent* Comp = NewObject<UMordecaiFeatComponent>();

	UMordecaiFeatDataAsset* Asset = CreateTestFeatAsset(FName(TEXT("QuickDraw")), EMordecaiFeatTier::Common, 1);
	FMordecaiFeatCondition Cond;
	Cond.ConditionType = EMordecaiFeatConditionType::StatThreshold;
	Cond.StatName = FName(TEXT("FastKills"));
	Cond.ThresholdValue = 3;
	Asset->UnlockConditions.Add(Cond);
	Comp->RegisterFeatDataAsset(Asset);

	// Not yet met
	TestFalse("Not unlocked initially", Comp->HasFeat(FName(TEXT("QuickDraw"))));

	// Increment below threshold
	Comp->IncrementFeatStat(FName(TEXT("FastKills")), 2);
	TestFalse("Not unlocked at 2", Comp->HasFeat(FName(TEXT("QuickDraw"))));

	// Increment to meet threshold — should auto-evaluate and unlock
	Comp->IncrementFeatStat(FName(TEXT("FastKills")), 1);
	TestTrue("Auto-unlocked at 3", Comp->HasFeat(FName(TEXT("QuickDraw"))));

	return true;
}

// ===========================================================================
// AC-012.12: Auto-unlock applies effects to ASC
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFeats_AutoUnlockEffects,
	"Mordecai.Feats.AutoUnlockAppliesEffects",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFeats_AutoUnlockEffects::RunTest(const FString& Parameters)
{
	UMordecaiFeatComponent* Comp = NewObject<UMordecaiFeatComponent>();

	// Create a feat with effects and abilities
	UMordecaiFeatDataAsset* Asset = CreateTestFeatAsset(
		FName(TEXT("PowerUp")), EMordecaiFeatTier::Rare, 2, 1, 1);

	FMordecaiFeatCondition Cond;
	Cond.ConditionType = EMordecaiFeatConditionType::StatThreshold;
	Cond.StatName = FName(TEXT("Trigger"));
	Cond.ThresholdValue = 1;
	Asset->UnlockConditions.Add(Cond);
	Comp->RegisterFeatDataAsset(Asset);

	// Trigger unlock
	Comp->IncrementFeatStat(FName(TEXT("Trigger")), 1);
	TestTrue("Feat unlocked", Comp->HasFeat(FName(TEXT("PowerUp"))));

	// Verify applied record tracks what was applied
	FMordecaiFeatAppliedRecord Record = Comp->GetFeatAppliedRecord(FName(TEXT("PowerUp")));
	TestEqual("2 granted effects applied", Record.GrantedEffectsCount, 2);
	TestEqual("1 granted ability applied", Record.GrantedAbilitiesCount, 1);
	TestEqual("1 drawback effect applied (Rare tier)", Record.DrawbackEffectsCount, 1);

	return true;
}

// ===========================================================================
// AC-012.13: HasFeat returns whether a feat has been unlocked
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFeats_HasFeatQuery,
	"Mordecai.Feats.HasFeatQueryWorks",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFeats_HasFeatQuery::RunTest(const FString& Parameters)
{
	UMordecaiFeatComponent* Comp = NewObject<UMordecaiFeatComponent>();

	// Unknown feat
	TestFalse("Unknown feat returns false", Comp->HasFeat(FName(TEXT("DoesNotExist"))));

	// Registered but not unlocked
	UMordecaiFeatDataAsset* Asset = CreateTestFeatAsset(FName(TEXT("Registered")), EMordecaiFeatTier::Common, 1);
	FMordecaiFeatCondition Cond;
	Cond.ConditionType = EMordecaiFeatConditionType::StatThreshold;
	Cond.StatName = FName(TEXT("X"));
	Cond.ThresholdValue = 100;
	Asset->UnlockConditions.Add(Cond);
	Comp->RegisterFeatDataAsset(Asset);
	TestFalse("Registered but not unlocked returns false", Comp->HasFeat(FName(TEXT("Registered"))));

	// Unlock it
	Comp->IncrementFeatStat(FName(TEXT("X")), 100);
	TestTrue("After unlock returns true", Comp->HasFeat(FName(TEXT("Registered"))));

	return true;
}

// ===========================================================================
// AC-012.14: Unlocked feats are replicated
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFeats_Replicated,
	"Mordecai.Feats.UnlockedFeatsReplicated",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFeats_Replicated::RunTest(const FString& Parameters)
{
	// Verify that ReplicatedUnlockedFeats has CPF_Net flag
	FProperty* Prop = UMordecaiFeatComponent::StaticClass()->FindPropertyByName(TEXT("ReplicatedUnlockedFeats"));
	TestNotNull("ReplicatedUnlockedFeats property exists", Prop);
	if (Prop)
	{
		TestTrue("ReplicatedUnlockedFeats has CPF_Net flag", (Prop->PropertyFlags & CPF_Net) != 0);
	}

	return true;
}

// ===========================================================================
// AC-012.15: Duplicate unlock attempts are no-ops
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFeats_DuplicateNoOp,
	"Mordecai.Feats.DuplicateUnlockIsNoOp",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFeats_DuplicateNoOp::RunTest(const FString& Parameters)
{
	UMordecaiFeatComponent* Comp = NewObject<UMordecaiFeatComponent>();

	UMordecaiFeatDataAsset* Asset = CreateTestFeatAsset(FName(TEXT("OnceOnly")), EMordecaiFeatTier::Common, 1);
	FMordecaiFeatCondition Cond;
	Cond.ConditionType = EMordecaiFeatConditionType::StatThreshold;
	Cond.StatName = FName(TEXT("Counter"));
	Cond.ThresholdValue = 1;
	Asset->UnlockConditions.Add(Cond);
	Comp->RegisterFeatDataAsset(Asset);

	// Track unlock events
	int32 UnlockCount = 0;
	Comp->OnFeatUnlocked.AddLambda([&UnlockCount](FName Name)
	{
		UnlockCount++;
	});

	// First unlock
	Comp->IncrementFeatStat(FName(TEXT("Counter")), 1);
	TestTrue("Feat unlocked", Comp->HasFeat(FName(TEXT("OnceOnly"))));
	TestEqual("Unlock event fired once", UnlockCount, 1);

	// Increment again — should not re-unlock
	Comp->IncrementFeatStat(FName(TEXT("Counter")), 1);
	TestEqual("Unlock event still fired only once", UnlockCount, 1);
	TestEqual("Still only 1 feat unlocked", Comp->GetUnlockedFeats().Num(), 1);

	return true;
}

// ===========================================================================
// AC-012.16: Common feats apply GrantedEffects only (no drawback)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFeats_CommonNoDrawback,
	"Mordecai.Feats.CommonFeatHasNoDrawback",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFeats_CommonNoDrawback::RunTest(const FString& Parameters)
{
	UMordecaiFeatComponent* Comp = NewObject<UMordecaiFeatComponent>();

	// Common feat with both granted and drawback effects defined
	UMordecaiFeatDataAsset* Asset = CreateTestFeatAsset(
		FName(TEXT("CommonFeat")), EMordecaiFeatTier::Common, 2, 1, 0);

	FMordecaiFeatCondition Cond;
	Cond.ConditionType = EMordecaiFeatConditionType::StatThreshold;
	Cond.StatName = FName(TEXT("Trigger"));
	Cond.ThresholdValue = 1;
	Asset->UnlockConditions.Add(Cond);
	Comp->RegisterFeatDataAsset(Asset);

	Comp->IncrementFeatStat(FName(TEXT("Trigger")), 1);
	TestTrue("Feat unlocked", Comp->HasFeat(FName(TEXT("CommonFeat"))));

	FMordecaiFeatAppliedRecord Record = Comp->GetFeatAppliedRecord(FName(TEXT("CommonFeat")));
	TestEqual("Granted effects applied", Record.GrantedEffectsCount, 2);
	TestEqual("Drawback effects NOT applied (Common)", Record.DrawbackEffectsCount, 0);

	return true;
}

// ===========================================================================
// AC-012.17: Rare feats apply both GrantedEffects and DrawbackEffects
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFeats_RareDrawback,
	"Mordecai.Feats.RareFeatAppliesDrawback",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFeats_RareDrawback::RunTest(const FString& Parameters)
{
	UMordecaiFeatComponent* Comp = NewObject<UMordecaiFeatComponent>();

	UMordecaiFeatDataAsset* Asset = CreateTestFeatAsset(
		FName(TEXT("RareFeat")), EMordecaiFeatTier::Rare, 2, 1, 0);

	FMordecaiFeatCondition Cond;
	Cond.ConditionType = EMordecaiFeatConditionType::StatThreshold;
	Cond.StatName = FName(TEXT("Trigger"));
	Cond.ThresholdValue = 1;
	Asset->UnlockConditions.Add(Cond);
	Comp->RegisterFeatDataAsset(Asset);

	Comp->IncrementFeatStat(FName(TEXT("Trigger")), 1);
	TestTrue("Feat unlocked", Comp->HasFeat(FName(TEXT("RareFeat"))));

	FMordecaiFeatAppliedRecord Record = Comp->GetFeatAppliedRecord(FName(TEXT("RareFeat")));
	TestEqual("Granted effects applied", Record.GrantedEffectsCount, 2);
	TestEqual("Drawback effects applied (Rare)", Record.DrawbackEffectsCount, 1);

	return true;
}

// ===========================================================================
// AC-012.18: Legendary feats apply both GrantedEffects and DrawbackEffects
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFeats_LegendaryDrawback,
	"Mordecai.Feats.LegendaryFeatAppliesDrawback",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFeats_LegendaryDrawback::RunTest(const FString& Parameters)
{
	UMordecaiFeatComponent* Comp = NewObject<UMordecaiFeatComponent>();

	UMordecaiFeatDataAsset* Asset = CreateTestFeatAsset(
		FName(TEXT("LegendaryFeat")), EMordecaiFeatTier::Legendary, 3, 2, 1);

	FMordecaiFeatCondition Cond;
	Cond.ConditionType = EMordecaiFeatConditionType::StatThreshold;
	Cond.StatName = FName(TEXT("Trigger"));
	Cond.ThresholdValue = 1;
	Asset->UnlockConditions.Add(Cond);
	Comp->RegisterFeatDataAsset(Asset);

	Comp->IncrementFeatStat(FName(TEXT("Trigger")), 1);
	TestTrue("Feat unlocked", Comp->HasFeat(FName(TEXT("LegendaryFeat"))));

	FMordecaiFeatAppliedRecord Record = Comp->GetFeatAppliedRecord(FName(TEXT("LegendaryFeat")));
	TestEqual("Granted effects applied", Record.GrantedEffectsCount, 3);
	TestEqual("Granted abilities applied", Record.GrantedAbilitiesCount, 1);
	TestEqual("Drawback effects applied (Legendary)", Record.DrawbackEffectsCount, 2);

	return true;
}

// ===========================================================================
// AC-012.19: Feat tag granted on unlock
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFeats_FeatTagGranted,
	"Mordecai.Feats.FeatTagGrantedOnUnlock",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFeats_FeatTagGranted::RunTest(const FString& Parameters)
{
	UMordecaiFeatComponent* Comp = NewObject<UMordecaiFeatComponent>();

	// Use a native tag that's guaranteed to exist for testing
	FGameplayTag FeatTag = MordecaiGameplayTags::Event_FeatUnlocked;
	TestTrue("Feat tag is valid", FeatTag.IsValid());

	UMordecaiFeatDataAsset* Asset = CreateTestFeatAsset(FName(TEXT("TaggedFeat")), EMordecaiFeatTier::Common, 1);
	Asset->FeatTag = FeatTag;

	FMordecaiFeatCondition Cond;
	Cond.ConditionType = EMordecaiFeatConditionType::StatThreshold;
	Cond.StatName = FName(TEXT("Trigger"));
	Cond.ThresholdValue = 1;
	Asset->UnlockConditions.Add(Cond);
	Comp->RegisterFeatDataAsset(Asset);

	// Before unlock — no feat tag
	TestFalse("Feat tag not granted before unlock", Comp->HasGrantedFeatTag(FeatTag));

	// Unlock
	Comp->IncrementFeatStat(FName(TEXT("Trigger")), 1);
	TestTrue("Feat unlocked", Comp->HasFeat(FName(TEXT("TaggedFeat"))));
	TestTrue("Feat tag granted after unlock", Comp->HasGrantedFeatTag(FeatTag));

	return true;
}

// ===========================================================================
// AC-012.20: FeatUnlocked event fires
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFeats_UnlockEventFires,
	"Mordecai.Feats.FeatUnlockedEventFires",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFeats_UnlockEventFires::RunTest(const FString& Parameters)
{
	UMordecaiFeatComponent* Comp = NewObject<UMordecaiFeatComponent>();

	TArray<FName> UnlockedNames;
	Comp->OnFeatUnlocked.AddLambda([&UnlockedNames](FName Name)
	{
		UnlockedNames.Add(Name);
	});

	// Register two feats
	UMordecaiFeatDataAsset* FeatA = CreateTestFeatAsset(FName(TEXT("FeatA")), EMordecaiFeatTier::Common, 1);
	FMordecaiFeatCondition CondA;
	CondA.ConditionType = EMordecaiFeatConditionType::StatThreshold;
	CondA.StatName = FName(TEXT("StatA"));
	CondA.ThresholdValue = 1;
	FeatA->UnlockConditions.Add(CondA);
	Comp->RegisterFeatDataAsset(FeatA);

	UMordecaiFeatDataAsset* FeatB = CreateTestFeatAsset(FName(TEXT("FeatB")), EMordecaiFeatTier::Rare, 1);
	FMordecaiFeatCondition CondB;
	CondB.ConditionType = EMordecaiFeatConditionType::StatThreshold;
	CondB.StatName = FName(TEXT("StatB"));
	CondB.ThresholdValue = 1;
	FeatB->UnlockConditions.Add(CondB);
	Comp->RegisterFeatDataAsset(FeatB);

	// Unlock first feat
	Comp->IncrementFeatStat(FName(TEXT("StatA")), 1);
	TestEqual("1 event fired", UnlockedNames.Num(), 1);
	TestEqual("Event has correct name", UnlockedNames[0], FName(TEXT("FeatA")));

	// Unlock second feat
	Comp->IncrementFeatStat(FName(TEXT("StatB")), 1);
	TestEqual("2 events fired total", UnlockedNames.Num(), 2);
	TestEqual("Second event has correct name", UnlockedNames[1], FName(TEXT("FeatB")));

	return true;
}

// ===========================================================================
// AC-012.21: Pyromaniac sample feat pipeline
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFeats_PyromaniacPipeline,
	"Mordecai.Feats.PyromaniacSamplePipeline",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFeats_PyromaniacPipeline::RunTest(const FString& Parameters)
{
	UMordecaiFeatComponent* Comp = NewObject<UMordecaiFeatComponent>();

	// --- Define Pyromaniac feat ---
	UMordecaiFeatDataAsset* Pyromaniac = NewObject<UMordecaiFeatDataAsset>();
	Pyromaniac->FeatName = FName(TEXT("Pyromaniac"));
	Pyromaniac->DisplayName = FText::FromString(TEXT("Pyromaniac"));
	Pyromaniac->Description = FText::FromString(TEXT("Burn 6 enemies simultaneously to unlock."));
	Pyromaniac->Tier = EMordecaiFeatTier::Common;
	Pyromaniac->GrantedEffects.Add(UGameplayEffect::StaticClass()); // Placeholder fire damage boost
	Pyromaniac->FeatTag = MordecaiGameplayTags::Event_FeatUnlocked; // Using existing tag for test

	// Unlock condition: burn 6 enemies simultaneously
	FMordecaiFeatCondition BurnCond;
	BurnCond.ConditionType = EMordecaiFeatConditionType::StatThreshold;
	BurnCond.StatName = FName(TEXT("EnemiesBurnedSimultaneously"));
	BurnCond.ThresholdValue = 6;
	Pyromaniac->UnlockConditions.Add(BurnCond);

	Comp->RegisterFeatDataAsset(Pyromaniac);

	// Track unlock events
	FName UnlockedFeatName;
	Comp->OnFeatUnlocked.AddLambda([&UnlockedFeatName](FName Name)
	{
		UnlockedFeatName = Name;
	});

	// --- Simulate gameplay: burn 3 enemies (not enough) ---
	Comp->IncrementFeatStat(FName(TEXT("EnemiesBurnedSimultaneously")), 3);
	TestFalse("Not unlocked at 3 burns", Comp->HasFeat(FName(TEXT("Pyromaniac"))));
	TestEqual("Stat is 3", Comp->GetFeatStatValue(FName(TEXT("EnemiesBurnedSimultaneously"))), 3);

	// --- Simulate gameplay: burn 3 more (total 6, meets threshold) ---
	Comp->IncrementFeatStat(FName(TEXT("EnemiesBurnedSimultaneously")), 3);
	TestTrue("Pyromaniac unlocked at 6 burns", Comp->HasFeat(FName(TEXT("Pyromaniac"))));
	TestEqual("Stat is 6", Comp->GetFeatStatValue(FName(TEXT("EnemiesBurnedSimultaneously"))), 6);

	// Verify event fired with correct name
	TestEqual("Event has Pyromaniac name", UnlockedFeatName, FName(TEXT("Pyromaniac")));

	// Verify granted effects applied (Common: no drawback)
	FMordecaiFeatAppliedRecord Record = Comp->GetFeatAppliedRecord(FName(TEXT("Pyromaniac")));
	TestEqual("1 granted effect applied", Record.GrantedEffectsCount, 1);
	TestEqual("No drawback (Common tier)", Record.DrawbackEffectsCount, 0);

	// Verify feat tag granted
	TestTrue("Feat tag granted", Comp->HasGrantedFeatTag(Pyromaniac->FeatTag));

	// Verify duplicate-safe
	int32 PreCount = Comp->GetUnlockedFeats().Num();
	Comp->IncrementFeatStat(FName(TEXT("EnemiesBurnedSimultaneously")), 10);
	TestEqual("Still 1 feat after duplicate trigger", Comp->GetUnlockedFeats().Num(), PreCount);

	return true;
}
