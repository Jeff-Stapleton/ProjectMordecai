// Project Mordecai — Feat Display Widget Tests (US-068)
// All NullRHI-compatible headless tests covering unlocked feat display, tier colors,
// detail panel, locked feats, condition progress, summary counts, live updates,
// and graceful behavior without a component.

#include "Misc/AutomationTest.h"
#include "GameplayTagContainer.h"

#include "Mordecai/Feats/MordecaiFeatComponent.h"
#include "Mordecai/Feats/MordecaiFeatDataAsset.h"
#include "Mordecai/Feats/MordecaiFeatTypes.h"
#include "Mordecai/Skills/MordecaiSkillComponent.h"
#include "Mordecai/Skills/MordecaiSkillDataAsset.h"
#include "Mordecai/UI/MordecaiFeatDisplayWidget.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace MordecaiFeatDisplayTestHelpers
{
	static UMordecaiFeatDataAsset* CreateFeatAsset(
		FName FeatName,
		EMordecaiFeatTier Tier,
		const TArray<FMordecaiFeatCondition>& Conditions = {})
	{
		UMordecaiFeatDataAsset* Asset = NewObject<UMordecaiFeatDataAsset>();
		Asset->FeatName = FeatName;
		Asset->DisplayName = FText::FromString(FeatName.ToString());
		Asset->Description = FText::FromString(FString::Printf(TEXT("Description of %s"), *FeatName.ToString()));
		Asset->Tier = Tier;
		Asset->UnlockConditions = Conditions;
		return Asset;
	}

	static FMordecaiFeatCondition MakeStatCondition(FName StatName, int32 Threshold)
	{
		FMordecaiFeatCondition Cond;
		Cond.ConditionType = EMordecaiFeatConditionType::StatThreshold;
		Cond.StatName = StatName;
		Cond.ThresholdValue = Threshold;
		return Cond;
	}

	static FMordecaiFeatCondition MakeEventCountCondition(FName StatName, int32 Threshold)
	{
		FMordecaiFeatCondition Cond;
		Cond.ConditionType = EMordecaiFeatConditionType::EventCount;
		Cond.StatName = StatName;
		Cond.ThresholdValue = Threshold;
		return Cond;
	}

	static FMordecaiFeatCondition MakeSkillRankCondition(FName SkillName, int32 RequiredRank)
	{
		FMordecaiFeatCondition Cond;
		Cond.ConditionType = EMordecaiFeatConditionType::SkillRank;
		Cond.StatName = SkillName;
		Cond.ThresholdValue = RequiredRank;
		return Cond;
	}

	static FMordecaiFeatCondition MakeTagCondition(FGameplayTag Tag)
	{
		FMordecaiFeatCondition Cond;
		Cond.ConditionType = EMordecaiFeatConditionType::TagPresent;
		Cond.RequiredTag = Tag;
		return Cond;
	}

	/** Create a FeatComponent with a mix of feats and pre-unlock some. */
	static UMordecaiFeatComponent* CreateComponentWithFeats()
	{
		UMordecaiFeatComponent* Comp = NewObject<UMordecaiFeatComponent>();

		// Unlocked feats: register, set stat high enough, then increment to trigger eval
		UMordecaiFeatDataAsset* CommonFeat = CreateFeatAsset(
			FName(TEXT("IronWill")), EMordecaiFeatTier::Common,
			{ MakeStatCondition(FName(TEXT("BlockCount")), 10) });
		Comp->RegisterFeatDataAsset(CommonFeat);

		UMordecaiFeatDataAsset* RareFeat = CreateFeatAsset(
			FName(TEXT("BladeDancer")), EMordecaiFeatTier::Rare,
			{ MakeStatCondition(FName(TEXT("ComboChains")), 5) });
		Comp->RegisterFeatDataAsset(RareFeat);

		UMordecaiFeatDataAsset* LegendaryFeat = CreateFeatAsset(
			FName(TEXT("DragonSlayer")), EMordecaiFeatTier::Legendary,
			{ MakeStatCondition(FName(TEXT("DragonsKilled")), 1) });
		Comp->RegisterFeatDataAsset(LegendaryFeat);

		// Locked feats (conditions not met)
		UMordecaiFeatDataAsset* LockedFeat1 = CreateFeatAsset(
			FName(TEXT("Pyromaniac")), EMordecaiFeatTier::Rare,
			{ MakeStatCondition(FName(TEXT("EnemiesBurned")), 50) });
		Comp->RegisterFeatDataAsset(LockedFeat1);

		UMordecaiFeatDataAsset* LockedFeat2 = CreateFeatAsset(
			FName(TEXT("MasterMage")), EMordecaiFeatTier::Legendary,
			{ MakeSkillRankCondition(FName(TEXT("Fireball")), 15),
			  MakeEventCountCondition(FName(TEXT("SpellsCast")), 100) });
		Comp->RegisterFeatDataAsset(LockedFeat2);

		// Unlock the first 3 feats by meeting their conditions
		Comp->IncrementFeatStat(FName(TEXT("BlockCount")), 10);
		Comp->IncrementFeatStat(FName(TEXT("ComboChains")), 5);
		Comp->IncrementFeatStat(FName(TEXT("DragonsKilled")), 1);

		return Comp;
	}

	static UMordecaiFeatDisplayWidget* CreateWidget()
	{
		return NewObject<UMordecaiFeatDisplayWidget>(GetTransientPackage());
	}
}

using namespace MordecaiFeatDisplayTestHelpers;

// ===========================================================================
// 1. Mordecai.UI.FeatDisplay.ShowsUnlockedFeats (AC-068.3)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_FeatDisplay_ShowsUnlockedFeats,
	"Mordecai.UI.FeatDisplay.ShowsUnlockedFeats",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_FeatDisplay_ShowsUnlockedFeats::RunTest(const FString& Parameters)
{
	UMordecaiFeatComponent* Comp = CreateComponentWithFeats();
	UMordecaiFeatDisplayWidget* Widget = CreateWidget();

	Widget->BindToFeatComponent(Comp);

	TArray<FMordecaiFeatDisplayRow> Rows = Widget->GetUnlockedFeatRows();
	TestEqual("3 unlocked feats", Rows.Num(), 3);

	// Check that all 3 expected feats are present with correct tiers
	TMap<FName, EMordecaiFeatTier> Found;
	for (const FMordecaiFeatDisplayRow& Row : Rows)
	{
		TestTrue("Row is unlocked", Row.bUnlocked);
		TestFalse("Row has display name", Row.DisplayName.IsEmpty());
		Found.Add(Row.FeatName, Row.Tier);
	}

	TestTrue("IronWill present", Found.Contains(FName(TEXT("IronWill"))));
	TestTrue("BladeDancer present", Found.Contains(FName(TEXT("BladeDancer"))));
	TestTrue("DragonSlayer present", Found.Contains(FName(TEXT("DragonSlayer"))));

	TestEqual("IronWill is Common", Found[FName(TEXT("IronWill"))], EMordecaiFeatTier::Common);
	TestEqual("BladeDancer is Rare", Found[FName(TEXT("BladeDancer"))], EMordecaiFeatTier::Rare);
	TestEqual("DragonSlayer is Legendary", Found[FName(TEXT("DragonSlayer"))], EMordecaiFeatTier::Legendary);

	return true;
}

// ===========================================================================
// 2. Mordecai.UI.FeatDisplay.TierColorCoding (AC-068.4)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_FeatDisplay_TierColorCoding,
	"Mordecai.UI.FeatDisplay.TierColorCoding",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_FeatDisplay_TierColorCoding::RunTest(const FString& Parameters)
{
	UMordecaiFeatDisplayWidget* Widget = CreateWidget();

	// Verify default tier colors match spec
	const FLinearColor CommonColor = Widget->GetTierColor(EMordecaiFeatTier::Common);
	const FLinearColor RareColor = Widget->GetTierColor(EMordecaiFeatTier::Rare);
	const FLinearColor LegendaryColor = Widget->GetTierColor(EMordecaiFeatTier::Legendary);

	// Common: White/Gray (0.8, 0.8, 0.8)
	TestTrue("Common R ~0.8", FMath::IsNearlyEqual(CommonColor.R, 0.8f, 0.01f));
	TestTrue("Common G ~0.8", FMath::IsNearlyEqual(CommonColor.G, 0.8f, 0.01f));
	TestTrue("Common B ~0.8", FMath::IsNearlyEqual(CommonColor.B, 0.8f, 0.01f));

	// Rare: Blue (0.3, 0.5, 1.0)
	TestTrue("Rare R ~0.3", FMath::IsNearlyEqual(RareColor.R, 0.3f, 0.01f));
	TestTrue("Rare G ~0.5", FMath::IsNearlyEqual(RareColor.G, 0.5f, 0.01f));
	TestTrue("Rare B ~1.0", FMath::IsNearlyEqual(RareColor.B, 1.0f, 0.01f));

	// Legendary: Gold (1.0, 0.75, 0.0)
	TestTrue("Legendary R ~1.0", FMath::IsNearlyEqual(LegendaryColor.R, 1.0f, 0.01f));
	TestTrue("Legendary G ~0.75", FMath::IsNearlyEqual(LegendaryColor.G, 0.75f, 0.01f));
	TestTrue("Legendary B ~0.0", FMath::IsNearlyEqual(LegendaryColor.B, 0.0f, 0.01f));

	return true;
}

// ===========================================================================
// 3. Mordecai.UI.FeatDisplay.DetailPanelShowsDescription (AC-068.5)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_FeatDisplay_DetailPanelShowsDescription,
	"Mordecai.UI.FeatDisplay.DetailPanelShowsDescription",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_FeatDisplay_DetailPanelShowsDescription::RunTest(const FString& Parameters)
{
	UMordecaiFeatComponent* Comp = CreateComponentWithFeats();
	UMordecaiFeatDisplayWidget* Widget = CreateWidget();

	Widget->BindToFeatComponent(Comp);
	Widget->SetSelectedFeat(FName(TEXT("IronWill")));

	FMordecaiFeatDisplayRow Detail = Widget->GetSelectedFeatDetail();
	TestEqual("Selected feat name matches", Detail.FeatName, FName(TEXT("IronWill")));
	TestFalse("Full description non-empty", Detail.Description.IsEmpty());
	TestEqual("Detail tier is Common", Detail.Tier, EMordecaiFeatTier::Common);
	TestTrue("Detail is unlocked", Detail.bUnlocked);

	// Applied record should be populated (IronWill was unlocked)
	// Even though test feats have no real GEs, the counts come from the applied record
	// (which was populated during unlock even if effect classes were null)
	// The record should exist and have been queried.
	// Granted effects/abilities/drawbacks counts come from the DataAsset arrays
	// For our test feats, these arrays are empty, so counts are 0
	TestEqual("Applied record effects count", Detail.AppliedRecord.GrantedEffectsCount, 0);
	TestEqual("Applied record abilities count", Detail.AppliedRecord.GrantedAbilitiesCount, 0);
	TestEqual("Applied record drawback count", Detail.AppliedRecord.DrawbackEffectsCount, 0);

	return true;
}

// ===========================================================================
// 4. Mordecai.UI.FeatDisplay.ShowsLockedFeats (AC-068.6)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_FeatDisplay_ShowsLockedFeats,
	"Mordecai.UI.FeatDisplay.ShowsLockedFeats",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_FeatDisplay_ShowsLockedFeats::RunTest(const FString& Parameters)
{
	UMordecaiFeatComponent* Comp = CreateComponentWithFeats();
	UMordecaiFeatDisplayWidget* Widget = CreateWidget();

	Widget->BindToFeatComponent(Comp);

	TArray<FMordecaiFeatDisplayRow> LockedRows = Widget->GetLockedFeatRows();
	TestEqual("2 locked feats", LockedRows.Num(), 2);

	TMap<FName, const FMordecaiFeatDisplayRow*> Found;
	for (const FMordecaiFeatDisplayRow& Row : LockedRows)
	{
		TestFalse("Row is locked", Row.bUnlocked);
		TestFalse("Locked row has display name", Row.DisplayName.IsEmpty());
		// Description should be hidden (teaser text)
		TestTrue("Locked description is teaser", Row.Description.ToString().Contains(TEXT("???")));
		Found.Add(Row.FeatName, &Row);
	}

	TestTrue("Pyromaniac present", Found.Contains(FName(TEXT("Pyromaniac"))));
	TestTrue("MasterMage present", Found.Contains(FName(TEXT("MasterMage"))));

	return true;
}

// ===========================================================================
// 5. Mordecai.UI.FeatDisplay.LockedFeatShowsProgress (AC-068.7)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_FeatDisplay_LockedFeatShowsProgress,
	"Mordecai.UI.FeatDisplay.LockedFeatShowsProgress",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_FeatDisplay_LockedFeatShowsProgress::RunTest(const FString& Parameters)
{
	UMordecaiFeatComponent* Comp = NewObject<UMordecaiFeatComponent>();

	// Set up a skill component for SkillRank condition testing
	UMordecaiSkillComponent* SkillComp = NewObject<UMordecaiSkillComponent>();
	UMordecaiSkillDataAsset* FireballSkill = NewObject<UMordecaiSkillDataAsset>();
	FireballSkill->SkillName = FName(TEXT("Fireball"));
	FireballSkill->Category = EMordecaiSkillCategory::Magic;
	SkillComp->RegisterSkillDataAsset(FireballSkill);
	SkillComp->SetSkillRank(FName(TEXT("Fireball")), 8);
	Comp->SetSkillComponentOverride(SkillComp);

	// Register a locked feat with multiple conditions
	UMordecaiFeatDataAsset* Feat = CreateFeatAsset(
		FName(TEXT("MasterMage")), EMordecaiFeatTier::Legendary,
		{ MakeSkillRankCondition(FName(TEXT("Fireball")), 15),
		  MakeEventCountCondition(FName(TEXT("SpellsCast")), 100) });
	Comp->RegisterFeatDataAsset(Feat);

	// Partially progress the event count
	Comp->IncrementFeatStat(FName(TEXT("SpellsCast")), 42);

	UMordecaiFeatDisplayWidget* Widget = CreateWidget();
	Widget->BindToFeatComponent(Comp);

	TArray<FMordecaiFeatDisplayRow> LockedRows = Widget->GetLockedFeatRows();
	TestEqual("1 locked feat", LockedRows.Num(), 1);

	const FMordecaiFeatDisplayRow& Row = LockedRows[0];
	TestEqual("2 conditions", Row.ConditionProgress.Num(), 2);

	// Find the SkillRank condition
	bool bFoundSkillRank = false;
	bool bFoundEventCount = false;
	for (const FMordecaiFeatConditionProgress& Prog : Row.ConditionProgress)
	{
		if (Prog.ConditionType == EMordecaiFeatConditionType::SkillRank)
		{
			TestEqual("SkillRank current=8", Prog.CurrentValue, 8);
			TestEqual("SkillRank threshold=15", Prog.ThresholdValue, 15);
			TestFalse("SkillRank not met", Prog.bMet);
			bFoundSkillRank = true;
		}
		else if (Prog.ConditionType == EMordecaiFeatConditionType::EventCount)
		{
			TestEqual("EventCount current=42", Prog.CurrentValue, 42);
			TestEqual("EventCount threshold=100", Prog.ThresholdValue, 100);
			TestFalse("EventCount not met", Prog.bMet);
			bFoundEventCount = true;
		}
	}
	TestTrue("SkillRank condition found", bFoundSkillRank);
	TestTrue("EventCount condition found", bFoundEventCount);

	return true;
}

// ===========================================================================
// 6. Mordecai.UI.FeatDisplay.SummaryCountsCorrect (AC-068.8)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_FeatDisplay_SummaryCountsCorrect,
	"Mordecai.UI.FeatDisplay.SummaryCountsCorrect",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_FeatDisplay_SummaryCountsCorrect::RunTest(const FString& Parameters)
{
	UMordecaiFeatComponent* Comp = CreateComponentWithFeats();
	UMordecaiFeatDisplayWidget* Widget = CreateWidget();

	Widget->BindToFeatComponent(Comp);

	FMordecaiFeatDisplaySummary Summary = Widget->GetSummary();
	// CreateComponentWithFeats unlocks: IronWill(Common), BladeDancer(Rare), DragonSlayer(Legendary)
	TestEqual("Total unlocked = 3", Summary.TotalUnlocked, 3);
	TestEqual("Common count = 1", Summary.CommonCount, 1);
	TestEqual("Rare count = 1", Summary.RareCount, 1);
	TestEqual("Legendary count = 1", Summary.LegendaryCount, 1);

	return true;
}

// ===========================================================================
// 7. Mordecai.UI.FeatDisplay.LiveUnlockMovesToUnlocked (AC-068.9)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_FeatDisplay_LiveUnlockMovesToUnlocked,
	"Mordecai.UI.FeatDisplay.LiveUnlockMovesToUnlocked",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_FeatDisplay_LiveUnlockMovesToUnlocked::RunTest(const FString& Parameters)
{
	UMordecaiFeatComponent* Comp = CreateComponentWithFeats();
	UMordecaiFeatDisplayWidget* Widget = CreateWidget();

	Widget->BindToFeatComponent(Comp);

	// Before: 3 unlocked, 2 locked
	TestEqual("3 unlocked before", Widget->GetUnlockedFeatRows().Num(), 3);
	TestEqual("2 locked before", Widget->GetLockedFeatRows().Num(), 2);
	TestEqual("Summary total=3 before", Widget->GetSummary().TotalUnlocked, 3);

	// Unlock Pyromaniac by meeting its stat condition
	Comp->IncrementFeatStat(FName(TEXT("EnemiesBurned")), 50);

	// After: Pyromaniac moves from locked to unlocked
	TestEqual("4 unlocked after", Widget->GetUnlockedFeatRows().Num(), 4);
	TestEqual("1 locked after", Widget->GetLockedFeatRows().Num(), 1);

	FMordecaiFeatDisplaySummary Summary = Widget->GetSummary();
	TestEqual("Summary total=4 after", Summary.TotalUnlocked, 4);
	TestEqual("Rare count=2 after", Summary.RareCount, 2);

	// Verify the widget observed the unlock event
	TestEqual("Last unlocked feat name", Widget->GetLastUnlockedFeatName(), FName(TEXT("Pyromaniac")));

	return true;
}

// ===========================================================================
// 8. Mordecai.UI.FeatDisplay.GracefulWithoutComponent (AC-068.2)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_FeatDisplay_GracefulWithoutComponent,
	"Mordecai.UI.FeatDisplay.GracefulWithoutComponent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_FeatDisplay_GracefulWithoutComponent::RunTest(const FString& Parameters)
{
	UMordecaiFeatDisplayWidget* Widget = CreateWidget();

	// No component bound
	TestFalse("Not bound by default", Widget->IsBoundToComponent());
	TestEqual("0 unlocked feats without component", Widget->GetUnlockedFeatRows().Num(), 0);
	TestEqual("0 locked feats without component", Widget->GetLockedFeatRows().Num(), 0);

	FMordecaiFeatDisplaySummary Summary = Widget->GetSummary();
	TestEqual("Summary total=0", Summary.TotalUnlocked, 0);
	TestEqual("Summary common=0", Summary.CommonCount, 0);
	TestEqual("Summary rare=0", Summary.RareCount, 0);
	TestEqual("Summary legendary=0", Summary.LegendaryCount, 0);

	// Selected feat detail returns empty row
	Widget->SetSelectedFeat(FName(TEXT("Anything")));
	FMordecaiFeatDisplayRow Detail = Widget->GetSelectedFeatDetail();
	TestTrue("Detail feat name is None", Detail.FeatName.IsNone());

	// Nullptr bind is a no-op (no crash)
	Widget->BindToFeatComponent(nullptr);
	TestFalse("Still unbound after nullptr", Widget->IsBoundToComponent());

	// Tab id / display name constants (AC-068.1)
	TestEqual("Tab id is 'feats'", UMordecaiFeatDisplayWidget::GetFeatsTabId(), FName(TEXT("feats")));
	TestFalse("Tab display name non-empty", UMordecaiFeatDisplayWidget::GetFeatsTabDisplayName().IsEmpty());

	return true;
}
