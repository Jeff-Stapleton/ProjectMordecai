// Project Mordecai — Perception & Mental Debuff Tests (US-059)

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
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Blinded.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Fear.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Cursed.h"

// ---------------------------------------------------------------------------
// Helper: Test environment with ASC + Attribute Set (same pattern as US-016)
// ---------------------------------------------------------------------------
struct FPerceptionMentalTestEnv
{
	UWorld* World = nullptr;
	AActor* TestActor = nullptr;
	UAbilitySystemComponent* ASC = nullptr;
	UMordecaiStatusEffectComponent* StatusComp = nullptr;
	UMordecaiAttributeSet* AttrSet = nullptr;

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

		ASC = NewObject<UAbilitySystemComponent>(TestActor, TEXT("TestASC"));
		ASC->RegisterComponent();

		AttrSet = NewObject<UMordecaiAttributeSet>(TestActor, TEXT("TestAttrSet"));
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		ASC->GetSpawnedAttributes_Mutable().AddUnique(AttrSet);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		ASC->InitAbilityActorInfo(TestActor, TestActor);

		StatusComp = NewObject<UMordecaiStatusEffectComponent>(TestActor, TEXT("TestStatusComp"));
		StatusComp->SetAbilitySystemComponentOverride(ASC);
		StatusComp->RegisterComponent();

		return true;
	}

	void Teardown()
	{
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
	}
};

// ===========================================================================
// BLINDED TESTS
// ===========================================================================

// ---------------------------------------------------------------------------
// AC-059.2: Blinded reduces RangedAccuracyMultiplier
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Blinded_ReducesAccuracy,
	"Mordecai.StatusEffect.Blinded.ReducesAccuracy",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Blinded_ReducesAccuracy::RunTest(const FString& Parameters)
{
	FPerceptionMentalTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify GE CDO configuration
	const UMordecaiGE_Blinded* CDO = GetDefault<UMordecaiGE_Blinded>();
	TestNotNull("Blinded GE CDO exists", CDO);
	TestEqual("StatusTag is Blinded", CDO->StatusTag, FGameplayTag(MordecaiGameplayTags::Status_Blinded));
	TestEqual("Default duration is 4.0s", CDO->BlindedDurationSec, 4.0f);
	TestTrue("Default accuracy reduction is 0.40",
		FMath::IsNearlyEqual(CDO->BlindedAccuracyReductionPct, 0.40f));

	// Verify initial attribute is 1.0
	const float InitialMult = Env.AttrSet->GetRangedAccuracyMultiplier();
	TestTrue("Initial RangedAccuracyMultiplier is 1.0",
		FMath::IsNearlyEqual(InitialMult, 1.0f));

	// Apply Blinded
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Blinded::StaticClass(), nullptr);
	TestTrue("Blinded tag is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Blinded));

	// Verify RangedAccuracyMultiplier reduced by 40%
	// Additive: 1.0 + (-0.40) = 0.60
	const float BlindedMult = Env.AttrSet->GetRangedAccuracyMultiplier();
	TestTrue(
		FString::Printf(TEXT("RangedAccuracyMultiplier reduced to ~0.60 (actual=%.2f)"), BlindedMult),
		FMath::IsNearlyEqual(BlindedMult, 0.60f, 0.05f));

	// Remove and verify restored
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Blinded);
	const float RestoredMult = Env.AttrSet->GetRangedAccuracyMultiplier();
	TestTrue(
		FString::Printf(TEXT("RangedAccuracyMultiplier restored to 1.0 (actual=%.2f)"), RestoredMult),
		FMath::IsNearlyEqual(RestoredMult, 1.0f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-059.3: Blinded reduces AimAssistMultiplier
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Blinded_ReducesAimAssist,
	"Mordecai.StatusEffect.Blinded.ReducesAimAssist",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Blinded_ReducesAimAssist::RunTest(const FString& Parameters)
{
	FPerceptionMentalTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	const UMordecaiGE_Blinded* CDO = GetDefault<UMordecaiGE_Blinded>();
	TestTrue("Default aim assist reduction is 0.50",
		FMath::IsNearlyEqual(CDO->BlindedAimAssistReductionPct, 0.50f));

	// Verify initial attribute is 1.0
	const float InitialMult = Env.AttrSet->GetAimAssistMultiplier();
	TestTrue("Initial AimAssistMultiplier is 1.0",
		FMath::IsNearlyEqual(InitialMult, 1.0f));

	// Apply Blinded
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Blinded::StaticClass(), nullptr);

	// Verify AimAssistMultiplier reduced by 50%
	// Additive: 1.0 + (-0.50) = 0.50
	const float BlindedMult = Env.AttrSet->GetAimAssistMultiplier();
	TestTrue(
		FString::Printf(TEXT("AimAssistMultiplier reduced to ~0.50 (actual=%.2f)"), BlindedMult),
		FMath::IsNearlyEqual(BlindedMult, 0.50f, 0.05f));

	// Remove and verify restored
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Blinded);
	const float RestoredMult = Env.AttrSet->GetAimAssistMultiplier();
	TestTrue(
		FString::Printf(TEXT("AimAssistMultiplier restored to 1.0 (actual=%.2f)"), RestoredMult),
		FMath::IsNearlyEqual(RestoredMult, 1.0f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-059.4: Blinded reduces StealthDetectionMultiplier
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Blinded_ReducesDetection,
	"Mordecai.StatusEffect.Blinded.ReducesDetection",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Blinded_ReducesDetection::RunTest(const FString& Parameters)
{
	FPerceptionMentalTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	const UMordecaiGE_Blinded* CDO = GetDefault<UMordecaiGE_Blinded>();
	TestTrue("Default detection reduction is 0.60",
		FMath::IsNearlyEqual(CDO->BlindedDetectionReductionPct, 0.60f));

	// Verify initial attribute is 1.0
	const float InitialMult = Env.AttrSet->GetStealthDetectionMultiplier();
	TestTrue("Initial StealthDetectionMultiplier is 1.0",
		FMath::IsNearlyEqual(InitialMult, 1.0f));

	// Apply Blinded
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Blinded::StaticClass(), nullptr);

	// Verify StealthDetectionMultiplier reduced by 60%
	// Additive: 1.0 + (-0.60) = 0.40
	const float BlindedMult = Env.AttrSet->GetStealthDetectionMultiplier();
	TestTrue(
		FString::Printf(TEXT("StealthDetectionMultiplier reduced to ~0.40 (actual=%.2f)"), BlindedMult),
		FMath::IsNearlyEqual(BlindedMult, 0.40f, 0.05f));

	// Remove and verify restored
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Blinded);
	const float RestoredMult = Env.AttrSet->GetStealthDetectionMultiplier();
	TestTrue(
		FString::Printf(TEXT("StealthDetectionMultiplier restored to 1.0 (actual=%.2f)"), RestoredMult),
		FMath::IsNearlyEqual(RestoredMult, 1.0f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-059.6: Blinded blocked by immunity
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Blinded_BlockedByImmunity,
	"Mordecai.StatusEffect.Blinded.BlockedByImmunity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Blinded_BlockedByImmunity::RunTest(const FString& Parameters)
{
	FPerceptionMentalTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Grant immunity
	Env.ASC->AddLooseGameplayTag(MordecaiGameplayTags::Immunity_Blinded);

	// Attempt to apply Blinded
	FActiveGameplayEffectHandle Handle = Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Blinded::StaticClass(), nullptr);
	TestFalse("Blinded blocked by immunity (handle invalid)", Handle.IsValid());
	TestFalse("Blinded tag not present", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Blinded));

	// Verify attributes unchanged
	const float AccuracyMult = Env.AttrSet->GetRangedAccuracyMultiplier();
	TestTrue("RangedAccuracyMultiplier unchanged at 1.0",
		FMath::IsNearlyEqual(AccuracyMult, 1.0f));

	Env.Teardown();
	return true;
}

// ===========================================================================
// FEAR TESTS
// ===========================================================================

// ---------------------------------------------------------------------------
// AC-059.8: Fear amplifies stamina tier penalties
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Fear_AmplifiesStaminaPenalties,
	"Mordecai.StatusEffect.Fear.AmplifiesStaminaPenalties",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Fear_AmplifiesStaminaPenalties::RunTest(const FString& Parameters)
{
	FPerceptionMentalTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify GE CDO configuration
	const UMordecaiGE_Fear* CDO = GetDefault<UMordecaiGE_Fear>();
	TestNotNull("Fear GE CDO exists", CDO);
	TestEqual("StatusTag is Fear", CDO->StatusTag, FGameplayTag(MordecaiGameplayTags::Status_Fear));
	TestEqual("Default duration is 5.0s", CDO->FearDurationSec, 5.0f);
	TestTrue("Default stamina tier penalty multiplier is 1.50",
		FMath::IsNearlyEqual(CDO->FearStaminaTierPenaltyMultiplier, 1.50f));

	// Verify initial attribute is 1.0
	const float InitialMult = Env.AttrSet->GetStaminaTierPenaltyMultiplier();
	TestTrue("Initial StaminaTierPenaltyMultiplier is 1.0",
		FMath::IsNearlyEqual(InitialMult, 1.0f));

	// Apply Fear
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Fear::StaticClass(), nullptr);
	TestTrue("Fear tag is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Fear));

	// Verify StaminaTierPenaltyMultiplier amplified by 50%
	// Additive: 1.0 + 0.50 = 1.50
	const float FearMult = Env.AttrSet->GetStaminaTierPenaltyMultiplier();
	TestTrue(
		FString::Printf(TEXT("StaminaTierPenaltyMultiplier amplified to ~1.50 (actual=%.2f)"), FearMult),
		FMath::IsNearlyEqual(FearMult, 1.50f, 0.05f));

	// Remove and verify restored
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Fear);
	const float RestoredMult = Env.AttrSet->GetStaminaTierPenaltyMultiplier();
	TestTrue(
		FString::Printf(TEXT("StaminaTierPenaltyMultiplier restored to 1.0 (actual=%.2f)"), RestoredMult),
		FMath::IsNearlyEqual(RestoredMult, 1.0f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-059.9: Fear reduces guard stability
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Fear_ReducesGuardStability,
	"Mordecai.StatusEffect.Fear.ReducesGuardStability",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Fear_ReducesGuardStability::RunTest(const FString& Parameters)
{
	FPerceptionMentalTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	const UMordecaiGE_Fear* CDO = GetDefault<UMordecaiGE_Fear>();
	TestTrue("Default guard stability reduction is 0.25",
		FMath::IsNearlyEqual(CDO->FearGuardStabilityReductionPct, 0.25f));

	// Verify initial attribute is 1.0
	const float InitialMult = Env.AttrSet->GetBlockStabilityMultiplier();
	TestTrue("Initial BlockStabilityMultiplier is 1.0",
		FMath::IsNearlyEqual(InitialMult, 1.0f));

	// Apply Fear
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Fear::StaticClass(), nullptr);

	// Verify BlockStabilityMultiplier reduced by 25%
	// Additive: 1.0 + (-0.25) = 0.75
	const float FearMult = Env.AttrSet->GetBlockStabilityMultiplier();
	TestTrue(
		FString::Printf(TEXT("BlockStabilityMultiplier reduced to ~0.75 (actual=%.2f)"), FearMult),
		FMath::IsNearlyEqual(FearMult, 0.75f, 0.05f));

	// Remove and verify restored
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Fear);
	const float RestoredMult = Env.AttrSet->GetBlockStabilityMultiplier();
	TestTrue(
		FString::Printf(TEXT("BlockStabilityMultiplier restored to 1.0 (actual=%.2f)"), RestoredMult),
		FMath::IsNearlyEqual(RestoredMult, 1.0f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-059.10: Fear applies aim drift
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Fear_AppliesAimDrift,
	"Mordecai.StatusEffect.Fear.AppliesAimDrift",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Fear_AppliesAimDrift::RunTest(const FString& Parameters)
{
	FPerceptionMentalTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	const UMordecaiGE_Fear* CDO = GetDefault<UMordecaiGE_Fear>();
	TestTrue("Default aim drift is 0.15",
		FMath::IsNearlyEqual(CDO->FearAimDriftPct, 0.15f));

	// Verify initial attribute is 1.0
	const float InitialMult = Env.AttrSet->GetRangedAccuracyMultiplier();
	TestTrue("Initial RangedAccuracyMultiplier is 1.0",
		FMath::IsNearlyEqual(InitialMult, 1.0f));

	// Apply Fear
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Fear::StaticClass(), nullptr);

	// Verify RangedAccuracyMultiplier reduced by 15% (aim drift)
	// Additive: 1.0 + (-0.15) = 0.85
	const float FearMult = Env.AttrSet->GetRangedAccuracyMultiplier();
	TestTrue(
		FString::Printf(TEXT("RangedAccuracyMultiplier reduced to ~0.85 (actual=%.2f)"), FearMult),
		FMath::IsNearlyEqual(FearMult, 0.85f, 0.05f));

	// Remove and verify restored
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Fear);
	const float RestoredMult = Env.AttrSet->GetRangedAccuracyMultiplier();
	TestTrue(
		FString::Printf(TEXT("RangedAccuracyMultiplier restored to 1.0 (actual=%.2f)"), RestoredMult),
		FMath::IsNearlyEqual(RestoredMult, 1.0f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-059.12: Fear blocked by immunity
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Fear_BlockedByImmunity,
	"Mordecai.StatusEffect.Fear.BlockedByImmunity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Fear_BlockedByImmunity::RunTest(const FString& Parameters)
{
	FPerceptionMentalTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Grant immunity
	Env.ASC->AddLooseGameplayTag(MordecaiGameplayTags::Immunity_Fear);

	// Attempt to apply Fear
	FActiveGameplayEffectHandle Handle = Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Fear::StaticClass(), nullptr);
	TestFalse("Fear blocked by immunity (handle invalid)", Handle.IsValid());
	TestFalse("Fear tag not present", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Fear));

	// Verify attributes unchanged
	const float StaminaMult = Env.AttrSet->GetStaminaTierPenaltyMultiplier();
	TestTrue("StaminaTierPenaltyMultiplier unchanged at 1.0",
		FMath::IsNearlyEqual(StaminaMult, 1.0f));

	Env.Teardown();
	return true;
}

// ===========================================================================
// CURSED TESTS
// ===========================================================================

// ---------------------------------------------------------------------------
// AC-059.14: Cursed reduces spell point regen
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Cursed_ReducesSpellRegen,
	"Mordecai.StatusEffect.Cursed.ReducesSpellRegen",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Cursed_ReducesSpellRegen::RunTest(const FString& Parameters)
{
	FPerceptionMentalTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify GE CDO configuration
	const UMordecaiGE_Cursed* CDO = GetDefault<UMordecaiGE_Cursed>();
	TestNotNull("Cursed GE CDO exists", CDO);
	TestEqual("StatusTag is Cursed", CDO->StatusTag, FGameplayTag(MordecaiGameplayTags::Status_Cursed));
	TestEqual("Default duration is 10.0s", CDO->CursedDurationSec, 10.0f);
	TestTrue("Default spell regen reduction is 0.50",
		FMath::IsNearlyEqual(CDO->CursedSpellRegenReductionPct, 0.50f));

	// Verify initial attribute is 1.0
	const float InitialMult = Env.AttrSet->GetSpellPointsRegenMultiplier();
	TestTrue("Initial SpellPointsRegenMultiplier is 1.0",
		FMath::IsNearlyEqual(InitialMult, 1.0f));

	// Apply Cursed
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Cursed::StaticClass(), nullptr);
	TestTrue("Cursed tag is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Cursed));

	// Verify SpellPointsRegenMultiplier reduced by 50%
	// Additive: 1.0 + (-0.50) = 0.50
	const float CursedMult = Env.AttrSet->GetSpellPointsRegenMultiplier();
	TestTrue(
		FString::Printf(TEXT("SpellPointsRegenMultiplier reduced to ~0.50 (actual=%.2f)"), CursedMult),
		FMath::IsNearlyEqual(CursedMult, 0.50f, 0.05f));

	// Remove and verify restored
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Cursed);
	const float RestoredMult = Env.AttrSet->GetSpellPointsRegenMultiplier();
	TestTrue(
		FString::Printf(TEXT("SpellPointsRegenMultiplier restored to 1.0 (actual=%.2f)"), RestoredMult),
		FMath::IsNearlyEqual(RestoredMult, 1.0f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-059.15: Cursed reduces healing received
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Cursed_ReducesHealing,
	"Mordecai.StatusEffect.Cursed.ReducesHealing",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Cursed_ReducesHealing::RunTest(const FString& Parameters)
{
	FPerceptionMentalTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	const UMordecaiGE_Cursed* CDO = GetDefault<UMordecaiGE_Cursed>();
	TestTrue("Default heal reduction is 0.30",
		FMath::IsNearlyEqual(CDO->CursedHealReductionPct, 0.30f));

	// Verify initial attribute is 1.0
	const float InitialMult = Env.AttrSet->GetHealingReceivedMultiplier();
	TestTrue("Initial HealingReceivedMultiplier is 1.0",
		FMath::IsNearlyEqual(InitialMult, 1.0f));

	// Apply Cursed
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Cursed::StaticClass(), nullptr);

	// Verify HealingReceivedMultiplier reduced by 30%
	// Additive: 1.0 + (-0.30) = 0.70
	const float CursedMult = Env.AttrSet->GetHealingReceivedMultiplier();
	TestTrue(
		FString::Printf(TEXT("HealingReceivedMultiplier reduced to ~0.70 (actual=%.2f)"), CursedMult),
		FMath::IsNearlyEqual(CursedMult, 0.70f, 0.05f));

	// Remove and verify restored
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Cursed);
	const float RestoredMult = Env.AttrSet->GetHealingReceivedMultiplier();
	TestTrue(
		FString::Printf(TEXT("HealingReceivedMultiplier restored to 1.0 (actual=%.2f)"), RestoredMult),
		FMath::IsNearlyEqual(RestoredMult, 1.0f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-059.17: Cursed blocked by immunity
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Cursed_BlockedByImmunity,
	"Mordecai.StatusEffect.Cursed.BlockedByImmunity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Cursed_BlockedByImmunity::RunTest(const FString& Parameters)
{
	FPerceptionMentalTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Grant immunity
	Env.ASC->AddLooseGameplayTag(MordecaiGameplayTags::Immunity_Cursed);

	// Attempt to apply Cursed
	FActiveGameplayEffectHandle Handle = Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Cursed::StaticClass(), nullptr);
	TestFalse("Cursed blocked by immunity (handle invalid)", Handle.IsValid());
	TestFalse("Cursed tag not present", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Cursed));

	// Verify attributes unchanged
	const float SpellRegenMult = Env.AttrSet->GetSpellPointsRegenMultiplier();
	TestTrue("SpellPointsRegenMultiplier unchanged at 1.0",
		FMath::IsNearlyEqual(SpellRegenMult, 1.0f));
	const float HealMult = Env.AttrSet->GetHealingReceivedMultiplier();
	TestTrue("HealingReceivedMultiplier unchanged at 1.0",
		FMath::IsNearlyEqual(HealMult, 1.0f));

	Env.Teardown();
	return true;
}
