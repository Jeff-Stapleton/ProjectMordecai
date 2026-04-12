// Project Mordecai — Drenched & Focused Status Effect Tests (US-018)

#include "Misc/AutomationTest.h"
#include "GameplayTagsManager.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectTypes.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectComponent.h"
#include "Mordecai/StatusEffects/MordecaiPerfectActionTracker.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Drenched.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Focused.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Burning.h"

// ---------------------------------------------------------------------------
// Helper: Test environment with ASC + Attribute Set + StatusComp + Tracker
// ---------------------------------------------------------------------------
struct FDrenchedFocusedTestEnv
{
	UWorld* World = nullptr;
	AActor* TestActor = nullptr;
	UAbilitySystemComponent* ASC = nullptr;
	UMordecaiStatusEffectComponent* StatusComp = nullptr;
	UMordecaiAttributeSet* AttrSet = nullptr;
	UMordecaiPerfectActionTracker* Tracker = nullptr;

	bool Setup()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false);
		if (!World || !GEngine)
		{
			return false;
		}

		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);

		FActorSpawnParameters SpawnParams;
		TestActor = World->SpawnActor<AActor>(AActor::StaticClass(), SpawnParams);
		if (!TestActor)
		{
			Teardown();
			return false;
		}

		// Create and register ASC
		ASC = NewObject<UAbilitySystemComponent>(TestActor, TEXT("TestASC"));
		ASC->RegisterComponent();

		// Create and register attribute set
		AttrSet = NewObject<UMordecaiAttributeSet>(TestActor, TEXT("TestAttrSet"));
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		ASC->GetSpawnedAttributes_Mutable().AddUnique(AttrSet);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		ASC->InitAbilityActorInfo(TestActor, TestActor);

		// Create status effect component with ASC override
		StatusComp = NewObject<UMordecaiStatusEffectComponent>(TestActor, TEXT("TestStatusComp"));
		StatusComp->SetAbilitySystemComponentOverride(ASC);
		StatusComp->RegisterComponent();

		// Create PerfectActionTracker with ASC override
		Tracker = NewObject<UMordecaiPerfectActionTracker>(TestActor, TEXT("TestTracker"));
		Tracker->SetAbilitySystemComponentOverride(ASC);
		Tracker->RegisterComponent();
		Tracker->StartTracking();

		return true;
	}

	void Teardown()
	{
		if (Tracker)
		{
			Tracker->StopTracking();
		}
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
		World = nullptr;
		TestActor = nullptr;
		ASC = nullptr;
		StatusComp = nullptr;
		AttrSet = nullptr;
		Tracker = nullptr;
	}
};

// ===========================================================================
// DRENCHED TESTS
// ===========================================================================

// ---------------------------------------------------------------------------
// AC-018.1: Drenched applies tag with configurable duration
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Drenched_AppliesTag,
	"Mordecai.StatusEffect.Drenched.AppliesTag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Drenched_AppliesTag::RunTest(const FString& Parameters)
{
	FDrenchedFocusedTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify CDO configuration
	const UMordecaiGE_Drenched* CDO = GetDefault<UMordecaiGE_Drenched>();
	TestNotNull("Drenched GE CDO exists", CDO);
	TestEqual("StatusTag is Drenched", CDO->StatusTag, FGameplayTag(MordecaiGameplayTags::Status_Drenched));
	TestEqual("Default duration is 15.0s", CDO->DrenchedDurationSec, 15.0f);
	TestEqual("DurationPolicy is HasDuration",
		static_cast<int32>(CDO->DurationPolicy),
		static_cast<int32>(EGameplayEffectDurationType::HasDuration));

	// Apply and verify tag
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Drenched::StaticClass(), nullptr);
	TestTrue("Drenched tag is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Drenched));

	// Remove and verify
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Drenched);
	TestFalse("Drenched tag removed", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Drenched));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-018.2: Drenched reduces fire damage taken
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Drenched_ReducesFireDamage,
	"Mordecai.StatusEffect.Drenched.ReducesFireDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Drenched_ReducesFireDamage::RunTest(const FString& Parameters)
{
	FDrenchedFocusedTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Baseline: FireDamageReceivedMultiplier should be 1.0
	const float BaseMultiplier = Env.ASC->GetNumericAttribute(UMordecaiAttributeSet::GetFireDamageReceivedMultiplierAttribute());
	TestEqual("Baseline fire damage multiplier is 1.0", BaseMultiplier, 1.0f);

	// Apply Drenched
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Drenched::StaticClass(), nullptr);

	// Verify fire damage reduction: 1.0 - 0.25 = 0.75
	const float DrenchedMultiplier = Env.ASC->GetNumericAttribute(UMordecaiAttributeSet::GetFireDamageReceivedMultiplierAttribute());
	TestEqual("Fire damage multiplier reduced to 0.75", DrenchedMultiplier, 0.75f);

	// Remove Drenched — multiplier should return to 1.0
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Drenched);
	const float RestoredMultiplier = Env.ASC->GetNumericAttribute(UMordecaiAttributeSet::GetFireDamageReceivedMultiplierAttribute());
	TestEqual("Fire damage multiplier restored to 1.0", RestoredMultiplier, 1.0f);

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-018.3: Drenched amplifies Shocked (multiplier approach)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Drenched_AmplifiesShocked,
	"Mordecai.StatusEffect.Drenched.AmplifiesShocked",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Drenched_AmplifiesShocked::RunTest(const FString& Parameters)
{
	FDrenchedFocusedTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Without Drenched: Shocked multiplier should be 1.0
	const float NormalMultiplier = UMordecaiGE_Drenched::GetShockedChanceMultiplier(Env.ASC);
	TestEqual("Shocked multiplier without Drenched is 1.0", NormalMultiplier, 1.0f);

	// Apply Drenched
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Drenched::StaticClass(), nullptr);

	// With Drenched: Shocked multiplier should be 1.50
	const float DrenchedMultiplier = UMordecaiGE_Drenched::GetShockedChanceMultiplier(Env.ASC);
	TestEqual("Shocked multiplier with Drenched is 1.50", DrenchedMultiplier, 1.50f);

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-018.4: Drenched amplifies Frostbitten (bonus stacks)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Drenched_AmplifiesFrostbitten,
	"Mordecai.StatusEffect.Drenched.AmplifiesFrostbitten",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Drenched_AmplifiesFrostbitten::RunTest(const FString& Parameters)
{
	FDrenchedFocusedTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Without Drenched: bonus frost stacks should be 0
	const int32 NormalBonusStacks = UMordecaiGE_Drenched::GetFrostBonusStacks(Env.ASC);
	TestEqual("Frost bonus stacks without Drenched is 0", NormalBonusStacks, 0);

	// Apply Drenched
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Drenched::StaticClass(), nullptr);

	// With Drenched: bonus frost stacks should be 1
	const int32 DrenchedBonusStacks = UMordecaiGE_Drenched::GetFrostBonusStacks(Env.ASC);
	TestEqual("Frost bonus stacks with Drenched is 1", DrenchedBonusStacks, 1);

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-018.5: Drenched douses Burning (applying Drenched removes Burning)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Drenched_DousesBurning,
	"Mordecai.StatusEffect.Drenched.DousesBurning",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Drenched_DousesBurning::RunTest(const FString& Parameters)
{
	FDrenchedFocusedTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Apply Burning first
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Burning::StaticClass(), nullptr);
	TestTrue("Burning is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));

	// Apply Drenched — should douse Burning
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Drenched::StaticClass(), nullptr);
	TestTrue("Drenched is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Drenched));
	TestFalse("Burning was doused (removed)", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-018.6: Fire on Drenched target removes Drenched, skips Burning
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Drenched_FireRemovesDrenched,
	"Mordecai.StatusEffect.Drenched.FireRemovesDrenched",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Drenched_FireRemovesDrenched::RunTest(const FString& Parameters)
{
	FDrenchedFocusedTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Apply Drenched first
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Drenched::StaticClass(), nullptr);
	TestTrue("Drenched is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Drenched));

	// Apply Burning — should remove Drenched and NOT apply Burning
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Burning::StaticClass(), nullptr);
	TestFalse("Drenched was removed by fire", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Drenched));
	TestFalse("Burning was NOT applied (neutralized by water)", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-018.7: Drenched blocked by immunity
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Drenched_BlockedByImmunity,
	"Mordecai.StatusEffect.Drenched.BlockedByImmunity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Drenched_BlockedByImmunity::RunTest(const FString& Parameters)
{
	FDrenchedFocusedTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Grant immunity tag
	Env.ASC->AddLooseGameplayTag(MordecaiGameplayTags::Immunity_Drenched);

	// Attempt to apply Drenched — should be blocked
	FActiveGameplayEffectHandle Handle = Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Drenched::StaticClass(), nullptr);
	TestFalse("Drenched application blocked by immunity", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Drenched));

	Env.Teardown();
	return true;
}

// ===========================================================================
// FOCUSED TESTS
// ===========================================================================

// ---------------------------------------------------------------------------
// AC-018.9: Focused granted after perfect action streak
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Focused_GrantedAfterPerfectStreak,
	"Mordecai.StatusEffect.Focused.GrantedAfterPerfectStreak",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Focused_GrantedAfterPerfectStreak::RunTest(const FString& Parameters)
{
	FDrenchedFocusedTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify threshold default
	TestEqual("Default streak threshold is 3", Env.Tracker->FocusedStreakThreshold, 3);

	// Send 2 perfect actions — not enough yet
	Env.Tracker->SimulatePerfectAction();
	Env.Tracker->SimulatePerfectAction();
	TestEqual("Streak is 2", Env.Tracker->GetCurrentStreak(), 2);
	TestFalse("Focused NOT active at 2", Env.Tracker->IsFocused());

	// Send 3rd perfect action — should trigger Focused
	Env.Tracker->SimulatePerfectAction();
	TestTrue("Focused active after reaching threshold", Env.Tracker->IsFocused());

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-018.10: Focused forgives stamina tier penalties
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Focused_ForgivesStaminaPenalties,
	"Mordecai.StatusEffect.Focused.ForgivesStaminaPenalties",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Focused_ForgivesStaminaPenalties::RunTest(const FString& Parameters)
{
	FDrenchedFocusedTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Baseline: StaminaTierPenaltyMultiplier should be 1.0
	const float BasePenalty = Env.ASC->GetNumericAttribute(UMordecaiAttributeSet::GetStaminaTierPenaltyMultiplierAttribute());
	TestEqual("Baseline stamina tier penalty multiplier is 1.0", BasePenalty, 1.0f);

	// Apply Focused via streak
	Env.Tracker->SimulatePerfectAction();
	Env.Tracker->SimulatePerfectAction();
	Env.Tracker->SimulatePerfectAction();
	TestTrue("Focused is active", Env.Tracker->IsFocused());

	// Verify penalty multiplier is 0 (forgiven)
	const float FocusedPenalty = Env.ASC->GetNumericAttribute(UMordecaiAttributeSet::GetStaminaTierPenaltyMultiplierAttribute());
	TestEqual("Stamina tier penalty multiplier is 0.0 (forgiven)", FocusedPenalty, 0.0f);

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-018.11: Focused boosts outgoing posture damage
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Focused_BoostsPostureDamage,
	"Mordecai.StatusEffect.Focused.BoostsPostureDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Focused_BoostsPostureDamage::RunTest(const FString& Parameters)
{
	FDrenchedFocusedTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Baseline
	const float BasePosture = Env.ASC->GetNumericAttribute(UMordecaiAttributeSet::GetOutgoingPostureDamageMultiplierAttribute());
	TestEqual("Baseline outgoing posture damage multiplier is 1.0", BasePosture, 1.0f);

	// Apply Focused
	Env.Tracker->SimulatePerfectAction();
	Env.Tracker->SimulatePerfectAction();
	Env.Tracker->SimulatePerfectAction();
	TestTrue("Focused is active", Env.Tracker->IsFocused());

	// Verify posture damage bonus: 1.0 + 0.20 = 1.20
	const float FocusedPosture = Env.ASC->GetNumericAttribute(UMordecaiAttributeSet::GetOutgoingPostureDamageMultiplierAttribute());
	TestEqual("Outgoing posture damage multiplier is 1.20", FocusedPosture, 1.20f);

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-018.12: Focused breaks on damage taken
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Focused_BreaksOnDamageTaken,
	"Mordecai.StatusEffect.Focused.BreaksOnDamageTaken",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Focused_BreaksOnDamageTaken::RunTest(const FString& Parameters)
{
	FDrenchedFocusedTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Get Focused
	Env.Tracker->SimulatePerfectAction();
	Env.Tracker->SimulatePerfectAction();
	Env.Tracker->SimulatePerfectAction();
	TestTrue("Focused is active", Env.Tracker->IsFocused());

	// Take damage — should break Focused
	Env.Tracker->SimulateDamageTaken();
	TestFalse("Focused broken on damage", Env.Tracker->IsFocused());

	// Streak should also be reset
	TestEqual("Streak reset to 0", Env.Tracker->GetCurrentStreak(), 0);

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-018.14: Counter resets on hit (before threshold reached)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Focused_CounterResetsOnHit,
	"Mordecai.StatusEffect.Focused.CounterResetsOnHit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Focused_CounterResetsOnHit::RunTest(const FString& Parameters)
{
	FDrenchedFocusedTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Build up streak partway
	Env.Tracker->SimulatePerfectAction();
	Env.Tracker->SimulatePerfectAction();
	TestEqual("Streak is 2", Env.Tracker->GetCurrentStreak(), 2);

	// Take damage before reaching threshold
	Env.Tracker->SimulateDamageTaken();
	TestEqual("Streak reset to 0 on hit", Env.Tracker->GetCurrentStreak(), 0);
	TestFalse("Focused not active", Env.Tracker->IsFocused());

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-018.14: Counter resets on non-perfect action
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Focused_CounterResetsOnNonPerfectAction,
	"Mordecai.StatusEffect.Focused.CounterResetsOnNonPerfectAction",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Focused_CounterResetsOnNonPerfectAction::RunTest(const FString& Parameters)
{
	FDrenchedFocusedTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Build up streak partway
	Env.Tracker->SimulatePerfectAction();
	Env.Tracker->SimulatePerfectAction();
	TestEqual("Streak is 2", Env.Tracker->GetCurrentStreak(), 2);

	// Non-perfect action resets counter
	Env.Tracker->SimulateNonPerfectAction();
	TestEqual("Streak reset to 0 on non-perfect action", Env.Tracker->GetCurrentStreak(), 0);
	TestFalse("Focused not active", Env.Tracker->IsFocused());

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-018.8: Focused persists until broken (infinite duration)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Focused_PersistUntilBroken,
	"Mordecai.StatusEffect.Focused.PersistUntilBroken",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Focused_PersistUntilBroken::RunTest(const FString& Parameters)
{
	FDrenchedFocusedTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify CDO has infinite duration
	const UMordecaiGE_Focused* CDO = GetDefault<UMordecaiGE_Focused>();
	TestNotNull("Focused GE CDO exists", CDO);
	TestEqual("Duration is Infinite",
		static_cast<int32>(CDO->DurationPolicy),
		static_cast<int32>(EGameplayEffectDurationType::Infinite));

	// Apply Focused
	Env.Tracker->SimulatePerfectAction();
	Env.Tracker->SimulatePerfectAction();
	Env.Tracker->SimulatePerfectAction();
	TestTrue("Focused is active", Env.Tracker->IsFocused());

	// Tick world time significantly — Focused should persist (infinite duration)
	if (Env.World)
	{
		// Simulate passage of time — Focused should NOT expire
		Env.World->Tick(LEVELTICK_All, 30.0f);
	}
	TestTrue("Focused still active after 30s (infinite duration)", Env.Tracker->IsFocused());

	Env.Teardown();
	return true;
}
