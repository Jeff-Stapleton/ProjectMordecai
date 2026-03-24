// Project Mordecai — DoT & Resource Denial Status Effect Tests (US-014)

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
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Burning.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Bleeding.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Poisoned.h"

// ---------------------------------------------------------------------------
// Helper: Test environment with ASC + Attribute Set
// ---------------------------------------------------------------------------
struct FDoTTestEnv
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

		// Spawn actor
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

		// Create and register attribute set on the ASC
		AttrSet = NewObject<UMordecaiAttributeSet>(TestActor, TEXT("TestAttrSet"));
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		ASC->GetSpawnedAttributes_Mutable().AddUnique(AttrSet);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		ASC->InitAbilityActorInfo(TestActor, TestActor);

		// Create status effect component with ASC override
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
// BURNING TESTS
// ===========================================================================

// ---------------------------------------------------------------------------
// AC-014.1: Burning applies tag and has configurable duration
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Burning_AppliesTagAndDuration,
	"Mordecai.StatusEffect.Burning.AppliesTagAndDuration",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Burning_AppliesTagAndDuration::RunTest(const FString& Parameters)
{
	FDoTTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify GE CDO configuration
	const UMordecaiGE_Burning* CDO = GetDefault<UMordecaiGE_Burning>();
	TestNotNull("Burning GE CDO exists", CDO);
	TestEqual("StatusTag is Burning", CDO->StatusTag, FGameplayTag(MordecaiGameplayTags::Status_Burning));
	TestEqual("Default duration is 5.0s", CDO->BurningDurationSec, 5.0f);
	TestEqual("DurationPolicy is HasDuration",
		static_cast<int32>(CDO->DurationPolicy),
		static_cast<int32>(EGameplayEffectDurationType::HasDuration));

	// Apply via StatusComp and verify tag
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Burning::StaticClass(), nullptr);
	TestTrue("Burning tag is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));

	// Remove and verify tag is gone
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Burning);
	TestFalse("Burning tag removed", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-014.2: Burning deals periodic fire damage
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Burning_DealsPeriodicDamage,
	"Mordecai.StatusEffect.Burning.DealsPeriodicDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Burning_DealsPeriodicDamage::RunTest(const FString& Parameters)
{
	FDoTTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	const UMordecaiGE_Burning* CDO = GetDefault<UMordecaiGE_Burning>();

	// Verify periodic configuration
	TestEqual("Tick interval is 1.0s", CDO->BurningTickIntervalSec, 1.0f);
	TestEqual("Damage per tick is 3.0", CDO->BurningDamagePerTick, 3.0f);
	TestTrue("Has execution calculations", CDO->Executions.Num() > 0);
	if (CDO->Executions.Num() > 0)
	{
		TestEqual("Execution class is BurningExecCalc",
			CDO->Executions[0].CalculationClass,
			TSubclassOf<UGameplayEffectExecutionCalculation>(UMordecaiBurningExecCalc::StaticClass()));
	}

	// Verify the GE has Period set (GAS handles the actual periodic ticking)
	TestTrue("Period is set on the GE", CDO->Period.GetValue() > 0.0f);

	// Apply Burning — periodic execution fires on application (bExecutePeriodicEffectOnApplication default true)
	const float HealthBefore = Env.AttrSet->GetHealth();
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Burning::StaticClass(), nullptr);

	// Tick the world to allow the initial periodic execution to process
	Env.World->Tick(LEVELTICK_All, 0.1f);

	const float HealthAfter = Env.AttrSet->GetHealth();
	const float DamageDealt = HealthBefore - HealthAfter;

	// At minimum, 1 tick of damage should have been dealt on application
	TestTrue(
		FString::Printf(TEXT("Health reduced by periodic damage (before=%.1f, after=%.1f, damage=%.1f, expected>=%.1f)"),
			HealthBefore, HealthAfter, DamageDealt, CDO->BurningDamagePerTick),
		DamageDealt >= CDO->BurningDamagePerTick - 0.1f);

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-014.3: Burning interrupts casting on chance
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Burning_InterruptsCastingOnChance,
	"Mordecai.StatusEffect.Burning.InterruptsCastingOnChance",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Burning_InterruptsCastingOnChance::RunTest(const FString& Parameters)
{
	FDoTTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Add Casting tag to target
	Env.ASC->AddLooseGameplayTag(MordecaiGameplayTags::State_Casting);
	TestTrue("Target has Casting tag", Env.ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Casting));

	// Test with 100% interrupt chance — should always interrupt
	bool bInterrupted = UMordecaiGE_Burning::TryInterruptCasting(Env.ASC, 1.0f);
	TestTrue("Cast interrupted with 100% chance", bInterrupted);

	// Test with 0% interrupt chance — should never interrupt
	bool bNotInterrupted = UMordecaiGE_Burning::TryInterruptCasting(Env.ASC, 0.0f);
	TestFalse("Cast NOT interrupted with 0% chance", bNotInterrupted);

	// Test without Casting tag — should not interrupt even at 100%
	Env.ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::State_Casting);
	bool bNoEffect = UMordecaiGE_Burning::TryInterruptCasting(Env.ASC, 1.0f);
	TestFalse("No interrupt without Casting tag", bNoEffect);

	// Verify default interrupt chance property
	const UMordecaiGE_Burning* CDO = GetDefault<UMordecaiGE_Burning>();
	TestTrue("Default interrupt chance is 0.15",
		FMath::IsNearlyEqual(CDO->BurningInterruptChance, 0.15f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-014.4: Re-applying Burning refreshes duration
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Burning_RefreshesDurationOnReapply,
	"Mordecai.StatusEffect.Burning.RefreshesDurationOnReapply",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Burning_RefreshesDurationOnReapply::RunTest(const FString& Parameters)
{
	FDoTTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Apply Burning
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Burning::StaticClass(), nullptr);
	TestTrue("Burning active after first apply", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));

	// Re-apply Burning (should refresh, not stack)
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Burning::StaticClass(), nullptr);
	TestTrue("Burning still active after re-apply", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));

	// Single removal should clear it (proves no stacking)
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Burning);
	TestFalse("Burning fully removed by single removal", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));

	// Verify stacking config
	const UMordecaiGE_Burning* CDO = GetDefault<UMordecaiGE_Burning>();
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	TestEqual("StackDurationRefreshPolicy is RefreshOnSuccessfulApplication",
		static_cast<int32>(CDO->StackDurationRefreshPolicy),
		static_cast<int32>(EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication));
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-014.5: Burning blocked by immunity
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Burning_BlockedByImmunity,
	"Mordecai.StatusEffect.Burning.BlockedByImmunity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Burning_BlockedByImmunity::RunTest(const FString& Parameters)
{
	FDoTTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Grant immunity
	Env.ASC->AddLooseGameplayTag(MordecaiGameplayTags::Immunity_Burning);

	// Attempt to apply Burning
	FActiveGameplayEffectHandle Handle = Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Burning::StaticClass(), nullptr);
	TestFalse("Burning blocked by immunity (handle invalid)", Handle.IsValid());
	TestFalse("Burning tag not present", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));

	Env.Teardown();
	return true;
}

// ===========================================================================
// BLEEDING TESTS
// ===========================================================================

// ---------------------------------------------------------------------------
// AC-014.7: Bleeding reduces healing received
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Bleeding_ReducesHealingReceived,
	"Mordecai.StatusEffect.Bleeding.ReducesHealingReceived",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Bleeding_ReducesHealingReceived::RunTest(const FString& Parameters)
{
	FDoTTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify initial HealingReceivedMultiplier is 1.0
	const float InitialMultiplier = Env.AttrSet->GetHealingReceivedMultiplier();
	TestTrue("Initial HealingReceivedMultiplier is 1.0",
		FMath::IsNearlyEqual(InitialMultiplier, 1.0f));

	// Apply Bleeding
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Bleeding::StaticClass(), nullptr);
	TestTrue("Bleeding is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Bleeding));

	// Verify HealingReceivedMultiplier is reduced by 50%
	const float BleedingMultiplier = Env.AttrSet->GetHealingReceivedMultiplier();
	TestTrue(
		FString::Printf(TEXT("HealingReceivedMultiplier reduced to ~0.50 (actual=%.2f)"), BleedingMultiplier),
		FMath::IsNearlyEqual(BleedingMultiplier, 0.50f, 0.05f));

	// Remove Bleeding and verify multiplier restored
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Bleeding);
	Env.StatusComp->StopBleedingTracking();
	const float RestoredMultiplier = Env.AttrSet->GetHealingReceivedMultiplier();
	TestTrue(
		FString::Printf(TEXT("HealingReceivedMultiplier restored to 1.0 (actual=%.2f)"), RestoredMultiplier),
		FMath::IsNearlyEqual(RestoredMultiplier, 1.0f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-014.8: Hit-refresh mechanic — damage resets Bleeding duration
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Bleeding_HitRefreshesDuration,
	"Mordecai.StatusEffect.Bleeding.HitRefreshesDuration",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Bleeding_HitRefreshesDuration::RunTest(const FString& Parameters)
{
	FDoTTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Apply Bleeding
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Bleeding::StaticClass(), nullptr);
	TestTrue("Bleeding is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Bleeding));

	// Trigger hit-refresh via NotifyDamageTaken
	Env.StatusComp->NotifyDamageTaken();

	// Bleeding should still be active (was re-applied)
	TestTrue("Bleeding still active after hit-refresh", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Bleeding));

	// HealingReceivedMultiplier should still be reduced (re-applied GE has same modifier)
	const float Multiplier = Env.AttrSet->GetHealingReceivedMultiplier();
	TestTrue(
		FString::Printf(TEXT("HealingReceivedMultiplier still reduced after refresh (actual=%.2f)"), Multiplier),
		FMath::IsNearlyEqual(Multiplier, 0.50f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-014.9: Clot mechanic — Bleeding removed after no damage for clot time
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Bleeding_ClotsAfterNoDamage,
	"Mordecai.StatusEffect.Bleeding.ClotsAfterNoDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Bleeding_ClotsAfterNoDamage::RunTest(const FString& Parameters)
{
	FDoTTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify default clot time
	const UMordecaiGE_Bleeding* CDO = GetDefault<UMordecaiGE_Bleeding>();
	TestEqual("Default clot time is 4.0s", CDO->BleedingClotTimeSec, 4.0f);

	// Apply Bleeding
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Bleeding::StaticClass(), nullptr);
	TestTrue("Bleeding is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Bleeding));

	// Force clot expiry (simulates timer firing after no damage)
	Env.StatusComp->ForceBleedingClotExpiry();

	// Bleeding should be removed
	TestFalse("Bleeding removed by clot mechanic", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Bleeding));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-014.10: Bleeding blocked by immunity
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Bleeding_BlockedByImmunity,
	"Mordecai.StatusEffect.Bleeding.BlockedByImmunity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Bleeding_BlockedByImmunity::RunTest(const FString& Parameters)
{
	FDoTTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Grant immunity
	Env.ASC->AddLooseGameplayTag(MordecaiGameplayTags::Immunity_Bleeding);

	// Attempt to apply Bleeding
	FActiveGameplayEffectHandle Handle = Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Bleeding::StaticClass(), nullptr);
	TestFalse("Bleeding blocked by immunity (handle invalid)", Handle.IsValid());
	TestFalse("Bleeding tag not present", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Bleeding));

	Env.Teardown();
	return true;
}

// ===========================================================================
// POISONED TESTS
// ===========================================================================

// ---------------------------------------------------------------------------
// AC-014.12: Poisoned reduces stamina regen
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Poisoned_ReducesStaminaRegen,
	"Mordecai.StatusEffect.Poisoned.ReducesStaminaRegen",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Poisoned_ReducesStaminaRegen::RunTest(const FString& Parameters)
{
	FDoTTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify CDO defaults
	const UMordecaiGE_Poisoned* CDO = GetDefault<UMordecaiGE_Poisoned>();
	TestEqual("Default duration is 8.0s", CDO->PoisonedDurationSec, 8.0f);
	TestTrue("Default stamina regen reduction is 0.60",
		FMath::IsNearlyEqual(CDO->PoisonedStaminaRegenReductionPct, 0.60f));

	// Record initial StaminaRegenMultiplier
	const float InitialRegen = Env.AttrSet->GetStaminaRegenMultiplier();

	// Apply Poisoned
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Poisoned::StaticClass(), nullptr);
	TestTrue("Poisoned is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Poisoned));

	// Verify StaminaRegenMultiplier reduced by 60%
	// GAS Additive: result = base + modifier, modifier = -0.60
	// So result = 1.0 + (-0.60) = 0.40
	const float PoisonedRegen = Env.AttrSet->GetStaminaRegenMultiplier();
	const float ExpectedRegen = InitialRegen - 0.60f;
	TestTrue(
		FString::Printf(TEXT("StaminaRegenMultiplier reduced (initial=%.2f, poisoned=%.2f, expected=%.2f)"),
			InitialRegen, PoisonedRegen, ExpectedRegen),
		FMath::IsNearlyEqual(PoisonedRegen, ExpectedRegen, 0.05f));

	// Remove Poisoned and verify restored
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Poisoned);
	const float RestoredRegen = Env.AttrSet->GetStaminaRegenMultiplier();
	TestTrue(
		FString::Printf(TEXT("StaminaRegenMultiplier restored (actual=%.2f, expected=%.2f)"),
			RestoredRegen, InitialRegen),
		FMath::IsNearlyEqual(RestoredRegen, InitialRegen, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-014.13: Poisoned reduces movement speed
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Poisoned_ReducesMoveSpeed,
	"Mordecai.StatusEffect.Poisoned.ReducesMoveSpeed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Poisoned_ReducesMoveSpeed::RunTest(const FString& Parameters)
{
	FDoTTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify CDO defaults
	const UMordecaiGE_Poisoned* CDO = GetDefault<UMordecaiGE_Poisoned>();
	TestTrue("Default move speed reduction is 0.15",
		FMath::IsNearlyEqual(CDO->PoisonedMoveSpeedReductionPct, 0.15f));

	// Record initial MoveSpeedMultiplier
	const float InitialSpeed = Env.AttrSet->GetMoveSpeedMultiplier();
	TestTrue("Initial MoveSpeedMultiplier is 1.0",
		FMath::IsNearlyEqual(InitialSpeed, 1.0f));

	// Apply Poisoned
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Poisoned::StaticClass(), nullptr);

	// Verify MoveSpeedMultiplier reduced by 15%
	// GAS Additive: result = base + modifier, modifier = -0.15
	// So result = 1.0 + (-0.15) = 0.85
	const float PoisonedSpeed = Env.AttrSet->GetMoveSpeedMultiplier();
	TestTrue(
		FString::Printf(TEXT("MoveSpeedMultiplier reduced to ~0.85 (actual=%.2f)"), PoisonedSpeed),
		FMath::IsNearlyEqual(PoisonedSpeed, 0.85f, 0.05f));

	// Remove Poisoned and verify restored
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Poisoned);
	const float RestoredSpeed = Env.AttrSet->GetMoveSpeedMultiplier();
	TestTrue(
		FString::Printf(TEXT("MoveSpeedMultiplier restored to 1.0 (actual=%.2f)"), RestoredSpeed),
		FMath::IsNearlyEqual(RestoredSpeed, 1.0f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-014.14: Poisoned blocked by immunity
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Poisoned_BlockedByImmunity,
	"Mordecai.StatusEffect.Poisoned.BlockedByImmunity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Poisoned_BlockedByImmunity::RunTest(const FString& Parameters)
{
	FDoTTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Grant immunity
	Env.ASC->AddLooseGameplayTag(MordecaiGameplayTags::Immunity_Poisoned);

	// Attempt to apply Poisoned
	FActiveGameplayEffectHandle Handle = Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Poisoned::StaticClass(), nullptr);
	TestFalse("Poisoned blocked by immunity (handle invalid)", Handle.IsValid());
	TestFalse("Poisoned tag not present", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Poisoned));

	Env.Teardown();
	return true;
}
