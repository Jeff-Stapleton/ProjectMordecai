// Project Mordecai — Combat Modifier Debuff Tests (US-016)

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
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Weakened.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Brittle.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Exposed.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Corroded.h"

// ---------------------------------------------------------------------------
// Helper: Test environment with ASC + Attribute Set (same pattern as US-014)
// ---------------------------------------------------------------------------
struct FCombatModifierTestEnv
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
// WEAKENED TESTS
// ===========================================================================

// ---------------------------------------------------------------------------
// AC-016.1 + AC-016.2: Weakened applies tag and reduces OutgoingPostureDamageMultiplier
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Weakened_AppliesTagAndModifier,
	"Mordecai.StatusEffect.Weakened.AppliesTagAndModifier",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Weakened_AppliesTagAndModifier::RunTest(const FString& Parameters)
{
	FCombatModifierTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify GE CDO configuration
	const UMordecaiGE_Weakened* CDO = GetDefault<UMordecaiGE_Weakened>();
	TestNotNull("Weakened GE CDO exists", CDO);
	TestEqual("StatusTag is Weakened", CDO->StatusTag, FGameplayTag(MordecaiGameplayTags::Status_Weakened));
	TestEqual("Default duration is 8.0s", CDO->WeakenedDurationSec, 8.0f);
	TestTrue("Default posture damage reduction is 0.40",
		FMath::IsNearlyEqual(CDO->WeakenedPostureDamageReductionPct, 0.40f));

	// Verify initial attribute is 1.0
	const float InitialMult = Env.AttrSet->GetOutgoingPostureDamageMultiplier();
	TestTrue("Initial OutgoingPostureDamageMultiplier is 1.0",
		FMath::IsNearlyEqual(InitialMult, 1.0f));

	// Apply Weakened
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Weakened::StaticClass(), nullptr);
	TestTrue("Weakened tag is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Weakened));

	// Verify OutgoingPostureDamageMultiplier reduced by 40%
	// Additive: 1.0 + (-0.40) = 0.60
	const float WeakenedMult = Env.AttrSet->GetOutgoingPostureDamageMultiplier();
	TestTrue(
		FString::Printf(TEXT("OutgoingPostureDamageMultiplier reduced to ~0.60 (actual=%.2f)"), WeakenedMult),
		FMath::IsNearlyEqual(WeakenedMult, 0.60f, 0.05f));

	// Remove and verify restored
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Weakened);
	const float RestoredMult = Env.AttrSet->GetOutgoingPostureDamageMultiplier();
	TestTrue(
		FString::Printf(TEXT("OutgoingPostureDamageMultiplier restored to 1.0 (actual=%.2f)"), RestoredMult),
		FMath::IsNearlyEqual(RestoredMult, 1.0f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-016.2: Weakened reduces outgoing posture damage (attribute readback)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Weakened_ReducesOutgoingPostureDamage,
	"Mordecai.StatusEffect.Weakened.ReducesOutgoingPostureDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Weakened_ReducesOutgoingPostureDamage::RunTest(const FString& Parameters)
{
	FCombatModifierTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Apply Weakened
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Weakened::StaticClass(), nullptr);

	// Simulate posture damage output calculation:
	// BasePostureDamage * OutgoingPostureDamageMultiplier
	const float BasePostureDamage = 50.0f;
	const float Multiplier = Env.AttrSet->GetOutgoingPostureDamageMultiplier();
	const float EffectiveDamage = BasePostureDamage * Multiplier;
	const float ExpectedDamage = BasePostureDamage * 0.60f; // 40% reduction

	TestTrue(
		FString::Printf(TEXT("Effective posture damage is ~30.0 (actual=%.1f, expected=%.1f)"),
			EffectiveDamage, ExpectedDamage),
		FMath::IsNearlyEqual(EffectiveDamage, ExpectedDamage, 1.0f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-016.3: Heavy attack bypass configurable exists
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Weakened_HeavyBypassesPartially,
	"Mordecai.StatusEffect.Weakened.HeavyBypassesPartially",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Weakened_HeavyBypassesPartially::RunTest(const FString& Parameters)
{
	const UMordecaiGE_Weakened* CDO = GetDefault<UMordecaiGE_Weakened>();
	TestNotNull("Weakened GE CDO exists", CDO);

	// Verify the heavy bypass property exists with correct default
	TestTrue("WeakenedHeavyBypassPct defaults to 0.50",
		FMath::IsNearlyEqual(CDO->WeakenedHeavyBypassPct, 0.50f));

	// Verify Attack.Heavy tag is registered
	FGameplayTag HeavyTag = FGameplayTag::RequestGameplayTag(FName("Mordecai.Attack.Heavy"), false);
	TestTrue("Mordecai.Attack.Heavy tag is registered", HeavyTag.IsValid());

	return true;
}

// ---------------------------------------------------------------------------
// AC-016.4: Weakened blocked by immunity
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Weakened_BlockedByImmunity,
	"Mordecai.StatusEffect.Weakened.BlockedByImmunity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Weakened_BlockedByImmunity::RunTest(const FString& Parameters)
{
	FCombatModifierTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Grant immunity
	Env.ASC->AddLooseGameplayTag(MordecaiGameplayTags::Immunity_Weakened);

	// Attempt to apply Weakened
	FActiveGameplayEffectHandle Handle = Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Weakened::StaticClass(), nullptr);
	TestFalse("Weakened blocked by immunity (handle invalid)", Handle.IsValid());
	TestFalse("Weakened tag not present", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Weakened));

	// Verify attribute unchanged
	const float Mult = Env.AttrSet->GetOutgoingPostureDamageMultiplier();
	TestTrue("OutgoingPostureDamageMultiplier unchanged at 1.0",
		FMath::IsNearlyEqual(Mult, 1.0f));

	Env.Teardown();
	return true;
}

// ===========================================================================
// BRITTLE TESTS
// ===========================================================================

// ---------------------------------------------------------------------------
// AC-016.5 + AC-016.6: Brittle applies tag and increases IncomingPostureDamageMultiplier
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Brittle_AppliesTagAndModifier,
	"Mordecai.StatusEffect.Brittle.AppliesTagAndModifier",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Brittle_AppliesTagAndModifier::RunTest(const FString& Parameters)
{
	FCombatModifierTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify GE CDO configuration
	const UMordecaiGE_Brittle* CDO = GetDefault<UMordecaiGE_Brittle>();
	TestNotNull("Brittle GE CDO exists", CDO);
	TestEqual("StatusTag is Brittle", CDO->StatusTag, FGameplayTag(MordecaiGameplayTags::Status_Brittle));
	TestEqual("Default duration is 7.0s", CDO->BrittleDurationSec, 7.0f);
	TestTrue("Default posture damage increase is 0.35",
		FMath::IsNearlyEqual(CDO->BrittlePostureDamageIncreasePct, 0.35f));

	// Verify initial attribute is 1.0
	const float InitialMult = Env.AttrSet->GetIncomingPostureDamageMultiplier();
	TestTrue("Initial IncomingPostureDamageMultiplier is 1.0",
		FMath::IsNearlyEqual(InitialMult, 1.0f));

	// Apply Brittle
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Brittle::StaticClass(), nullptr);
	TestTrue("Brittle tag is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Brittle));

	// Verify IncomingPostureDamageMultiplier increased by 35%
	// Additive: 1.0 + 0.35 = 1.35
	const float BrittleMult = Env.AttrSet->GetIncomingPostureDamageMultiplier();
	TestTrue(
		FString::Printf(TEXT("IncomingPostureDamageMultiplier increased to ~1.35 (actual=%.2f)"), BrittleMult),
		FMath::IsNearlyEqual(BrittleMult, 1.35f, 0.05f));

	// Remove and verify restored
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Brittle);
	const float RestoredMult = Env.AttrSet->GetIncomingPostureDamageMultiplier();
	TestTrue(
		FString::Printf(TEXT("IncomingPostureDamageMultiplier restored to 1.0 (actual=%.2f)"), RestoredMult),
		FMath::IsNearlyEqual(RestoredMult, 1.0f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-016.7: Brittle lowers MaxPosture
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Brittle_LowersMaxPosture,
	"Mordecai.StatusEffect.Brittle.LowersMaxPosture",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Brittle_LowersMaxPosture::RunTest(const FString& Parameters)
{
	FCombatModifierTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	const UMordecaiGE_Brittle* CDO = GetDefault<UMordecaiGE_Brittle>();
	TestTrue("Default MaxPosture reduction is 0.20",
		FMath::IsNearlyEqual(CDO->BrittleMaxPostureReductionPct, 0.20f));

	// Record initial MaxPosture
	const float InitialMaxPosture = Env.AttrSet->GetMaxPosture();
	TestTrue("Initial MaxPosture is 100.0",
		FMath::IsNearlyEqual(InitialMaxPosture, 100.0f));

	// Apply Brittle
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Brittle::StaticClass(), nullptr);

	// Verify MaxPosture reduced by 20%
	// Multiply op: 100.0 * 0.80 = 80.0
	const float BrittleMaxPosture = Env.AttrSet->GetMaxPosture();
	TestTrue(
		FString::Printf(TEXT("MaxPosture reduced to ~80.0 (actual=%.1f)"), BrittleMaxPosture),
		FMath::IsNearlyEqual(BrittleMaxPosture, 80.0f, 2.0f));

	// Remove and verify restored
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Brittle);
	const float RestoredMaxPosture = Env.AttrSet->GetMaxPosture();
	TestTrue(
		FString::Printf(TEXT("MaxPosture restored to ~100.0 (actual=%.1f)"), RestoredMaxPosture),
		FMath::IsNearlyEqual(RestoredMaxPosture, InitialMaxPosture, 2.0f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-016.9: Brittle blocked by immunity
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Brittle_BlockedByImmunity,
	"Mordecai.StatusEffect.Brittle.BlockedByImmunity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Brittle_BlockedByImmunity::RunTest(const FString& Parameters)
{
	FCombatModifierTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Grant immunity
	Env.ASC->AddLooseGameplayTag(MordecaiGameplayTags::Immunity_Brittle);

	// Attempt to apply Brittle
	FActiveGameplayEffectHandle Handle = Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Brittle::StaticClass(), nullptr);
	TestFalse("Brittle blocked by immunity (handle invalid)", Handle.IsValid());
	TestFalse("Brittle tag not present", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Brittle));

	Env.Teardown();
	return true;
}

// ===========================================================================
// EXPOSED TESTS
// ===========================================================================

// ---------------------------------------------------------------------------
// AC-016.10 + AC-016.11: Exposed applies tag and bonus damage multiplier
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Exposed_AppliesTagAndBonusDamage,
	"Mordecai.StatusEffect.Exposed.AppliesTagAndBonusDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Exposed_AppliesTagAndBonusDamage::RunTest(const FString& Parameters)
{
	FCombatModifierTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify GE CDO configuration
	const UMordecaiGE_Exposed* CDO = GetDefault<UMordecaiGE_Exposed>();
	TestNotNull("Exposed GE CDO exists", CDO);
	TestEqual("StatusTag is Exposed", CDO->StatusTag, FGameplayTag(MordecaiGameplayTags::Status_Exposed));
	TestEqual("Default duration is 5.0s", CDO->ExposedDurationSec, 5.0f);
	TestTrue("Default bonus damage is 0.30",
		FMath::IsNearlyEqual(CDO->ExposedBonusDamagePct, 0.30f));

	// Verify initial attribute is 1.0
	const float InitialMult = Env.AttrSet->GetIncomingDamageMultiplier();
	TestTrue("Initial IncomingDamageMultiplier is 1.0",
		FMath::IsNearlyEqual(InitialMult, 1.0f));

	// Apply Exposed
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Exposed::StaticClass(), nullptr);
	TestTrue("Exposed tag is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Exposed));

	// Verify IncomingDamageMultiplier increased by 30%
	// Additive: 1.0 + 0.30 = 1.30
	const float ExposedMult = Env.AttrSet->GetIncomingDamageMultiplier();
	TestTrue(
		FString::Printf(TEXT("IncomingDamageMultiplier increased to ~1.30 (actual=%.2f)"), ExposedMult),
		FMath::IsNearlyEqual(ExposedMult, 1.30f, 0.05f));

	// Remove and verify restored
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Exposed);
	const float RestoredMult = Env.AttrSet->GetIncomingDamageMultiplier();
	TestTrue(
		FString::Printf(TEXT("IncomingDamageMultiplier restored to 1.0 (actual=%.2f)"), RestoredMult),
		FMath::IsNearlyEqual(RestoredMult, 1.0f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-016.12: Exposed consumed on first hit
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Exposed_ConsumedOnFirstHit,
	"Mordecai.StatusEffect.Exposed.ConsumedOnFirstHit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Exposed_ConsumedOnFirstHit::RunTest(const FString& Parameters)
{
	FCombatModifierTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Apply Exposed
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Exposed::StaticClass(), nullptr);
	TestTrue("Exposed is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Exposed));

	// Verify bonus damage is active before hit
	const float MultBeforeHit = Env.AttrSet->GetIncomingDamageMultiplier();
	TestTrue("IncomingDamageMultiplier is ~1.30 before hit",
		FMath::IsNearlyEqual(MultBeforeHit, 1.30f, 0.05f));

	// Simulate damage taken — Exposed should be consumed
	Env.StatusComp->NotifyDamageTaken();

	// Exposed should be removed after the hit
	TestFalse("Exposed removed after first hit", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Exposed));

	// IncomingDamageMultiplier should be restored
	const float MultAfterHit = Env.AttrSet->GetIncomingDamageMultiplier();
	TestTrue(
		FString::Printf(TEXT("IncomingDamageMultiplier restored to 1.0 after hit (actual=%.2f)"), MultAfterHit),
		FMath::IsNearlyEqual(MultAfterHit, 1.0f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-016.10: Exposed has correct duration configuration (expires after timer if no hit)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Exposed_ExpiresOnDurationIfNoHit,
	"Mordecai.StatusEffect.Exposed.ExpiresOnDurationIfNoHit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Exposed_ExpiresOnDurationIfNoHit::RunTest(const FString& Parameters)
{
	// Verify duration configuration — GAS handles the actual timer-based expiry
	const UMordecaiGE_Exposed* CDO = GetDefault<UMordecaiGE_Exposed>();
	TestNotNull("Exposed GE CDO exists", CDO);

	TestEqual("DurationPolicy is HasDuration",
		static_cast<int32>(CDO->DurationPolicy),
		static_cast<int32>(EGameplayEffectDurationType::HasDuration));

	TestEqual("Default duration is 5.0s", CDO->ExposedDurationSec, 5.0f);

	// Verify stacking policy refreshes duration on re-application
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	TestEqual("StackDurationRefreshPolicy is RefreshOnSuccessfulApplication",
		static_cast<int32>(CDO->StackDurationRefreshPolicy),
		static_cast<int32>(EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication));
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	return true;
}

// ---------------------------------------------------------------------------
// AC-016.14: Exposed blocked by immunity
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Exposed_BlockedByImmunity,
	"Mordecai.StatusEffect.Exposed.BlockedByImmunity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Exposed_BlockedByImmunity::RunTest(const FString& Parameters)
{
	FCombatModifierTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Grant immunity
	Env.ASC->AddLooseGameplayTag(MordecaiGameplayTags::Immunity_Exposed);

	// Attempt to apply Exposed
	FActiveGameplayEffectHandle Handle = Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Exposed::StaticClass(), nullptr);
	TestFalse("Exposed blocked by immunity (handle invalid)", Handle.IsValid());
	TestFalse("Exposed tag not present", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Exposed));

	Env.Teardown();
	return true;
}

// ===========================================================================
// CORRODED TESTS
// ===========================================================================

// ---------------------------------------------------------------------------
// AC-016.15 + AC-016.16: Corroded reduces BlockStabilityMultiplier
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Corroded_ReducesBlockStability,
	"Mordecai.StatusEffect.Corroded.ReducesBlockStability",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Corroded_ReducesBlockStability::RunTest(const FString& Parameters)
{
	FCombatModifierTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify GE CDO configuration
	const UMordecaiGE_Corroded* CDO = GetDefault<UMordecaiGE_Corroded>();
	TestNotNull("Corroded GE CDO exists", CDO);
	TestEqual("StatusTag is Corroded", CDO->StatusTag, FGameplayTag(MordecaiGameplayTags::Status_Corroded));
	TestEqual("Default duration is 10.0s", CDO->CorrodedDurationSec, 10.0f);
	TestTrue("Default block stability reduction is 0.30",
		FMath::IsNearlyEqual(CDO->CorrodedBlockStabilityReductionPct, 0.30f));

	// Verify initial attribute is 1.0
	const float InitialMult = Env.AttrSet->GetBlockStabilityMultiplier();
	TestTrue("Initial BlockStabilityMultiplier is 1.0",
		FMath::IsNearlyEqual(InitialMult, 1.0f));

	// Apply Corroded
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Corroded::StaticClass(), nullptr);
	TestTrue("Corroded tag is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Corroded));

	// Verify BlockStabilityMultiplier reduced by 30%
	// Additive: 1.0 + (-0.30) = 0.70
	const float CorrodedMult = Env.AttrSet->GetBlockStabilityMultiplier();
	TestTrue(
		FString::Printf(TEXT("BlockStabilityMultiplier reduced to ~0.70 (actual=%.2f)"), CorrodedMult),
		FMath::IsNearlyEqual(CorrodedMult, 0.70f, 0.05f));

	// Remove and verify restored
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Corroded);
	const float RestoredMult = Env.AttrSet->GetBlockStabilityMultiplier();
	TestTrue(
		FString::Printf(TEXT("BlockStabilityMultiplier restored to 1.0 (actual=%.2f)"), RestoredMult),
		FMath::IsNearlyEqual(RestoredMult, 1.0f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-016.17: Corroded reduces ArmorEfficiencyMultiplier
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Corroded_ReducesArmorEfficiency,
	"Mordecai.StatusEffect.Corroded.ReducesArmorEfficiency",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Corroded_ReducesArmorEfficiency::RunTest(const FString& Parameters)
{
	FCombatModifierTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	const UMordecaiGE_Corroded* CDO = GetDefault<UMordecaiGE_Corroded>();
	TestTrue("Default armor reduction is 0.25",
		FMath::IsNearlyEqual(CDO->CorrodedArmorReductionPct, 0.25f));

	// Verify initial attribute is 1.0
	const float InitialMult = Env.AttrSet->GetArmorEfficiencyMultiplier();
	TestTrue("Initial ArmorEfficiencyMultiplier is 1.0",
		FMath::IsNearlyEqual(InitialMult, 1.0f));

	// Apply Corroded
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Corroded::StaticClass(), nullptr);

	// Verify ArmorEfficiencyMultiplier reduced by 25%
	// Additive: 1.0 + (-0.25) = 0.75
	const float CorrodedMult = Env.AttrSet->GetArmorEfficiencyMultiplier();
	TestTrue(
		FString::Printf(TEXT("ArmorEfficiencyMultiplier reduced to ~0.75 (actual=%.2f)"), CorrodedMult),
		FMath::IsNearlyEqual(CorrodedMult, 0.75f, 0.05f));

	// Remove and verify restored
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Corroded);
	const float RestoredMult = Env.AttrSet->GetArmorEfficiencyMultiplier();
	TestTrue(
		FString::Printf(TEXT("ArmorEfficiencyMultiplier restored to 1.0 (actual=%.2f)"), RestoredMult),
		FMath::IsNearlyEqual(RestoredMult, 1.0f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-016.19: Corroded blocked by immunity
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Corroded_BlockedByImmunity,
	"Mordecai.StatusEffect.Corroded.BlockedByImmunity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Corroded_BlockedByImmunity::RunTest(const FString& Parameters)
{
	FCombatModifierTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Grant immunity
	Env.ASC->AddLooseGameplayTag(MordecaiGameplayTags::Immunity_Corroded);

	// Attempt to apply Corroded
	FActiveGameplayEffectHandle Handle = Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Corroded::StaticClass(), nullptr);
	TestFalse("Corroded blocked by immunity (handle invalid)", Handle.IsValid());
	TestFalse("Corroded tag not present", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Corroded));

	Env.Teardown();
	return true;
}
