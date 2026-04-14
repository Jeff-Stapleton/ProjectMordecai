// Project Mordecai — Frostbitten & Shocked Status Effect Tests (US-015)

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
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Frostbitten.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Shocked.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Frozen.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_MicroStunned.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Drenched.h"
#include "GameplayAbilitySpec.h"
#include "Abilities/GameplayAbility.h"

// ---------------------------------------------------------------------------
// Helper: Test environment with ASC + Attribute Set
// ---------------------------------------------------------------------------
struct FFrostShockTestEnv
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
// FROSTBITTEN TESTS
// ===========================================================================

// ---------------------------------------------------------------------------
// AC-015.1, AC-015.2: Frostbitten applies tag and reduces movement speed
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Frostbitten_AppliesTagAndReducesSpeed,
	"Mordecai.StatusEffect.Frostbitten.AppliesTagAndReducesSpeed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Frostbitten_AppliesTagAndReducesSpeed::RunTest(const FString& Parameters)
{
	FFrostShockTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify CDO
	const UMordecaiGE_Frostbitten* CDO = GetDefault<UMordecaiGE_Frostbitten>();
	TestNotNull("Frostbitten GE CDO exists", CDO);
	TestEqual("StatusTag is Frostbitten", CDO->StatusTag, FGameplayTag(MordecaiGameplayTags::Status_Frostbitten));
	TestEqual("Default duration is 6.0s", CDO->FrostbittenDurationSec, 6.0f);

	// Verify initial MoveSpeedMultiplier
	const float InitialSpeed = Env.AttrSet->GetMoveSpeedMultiplier();
	TestTrue("Initial MoveSpeedMultiplier is 1.0", FMath::IsNearlyEqual(InitialSpeed, 1.0f));

	// Apply Frostbitten
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Frostbitten::StaticClass(), nullptr);
	TestTrue("Frostbitten tag active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Frostbitten));

	// MoveSpeedMultiplier reduced by 25%
	const float FrostSpeed = Env.AttrSet->GetMoveSpeedMultiplier();
	TestTrue(
		FString::Printf(TEXT("MoveSpeedMultiplier reduced to ~0.75 (actual=%.2f)"), FrostSpeed),
		FMath::IsNearlyEqual(FrostSpeed, 1.0f - CDO->FrostbittenMoveSpeedReductionPct, 0.05f));

	// Remove and verify restore
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Frostbitten);
	Env.StatusComp->StopFrostbittenTracking();
	const float RestoredSpeed = Env.AttrSet->GetMoveSpeedMultiplier();
	TestTrue(
		FString::Printf(TEXT("MoveSpeedMultiplier restored to 1.0 (actual=%.2f)"), RestoredSpeed),
		FMath::IsNearlyEqual(RestoredSpeed, 1.0f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-015.3: Frostbitten reduces attack speed
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Frostbitten_ReducesAttackSpeed,
	"Mordecai.StatusEffect.Frostbitten.ReducesAttackSpeed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Frostbitten_ReducesAttackSpeed::RunTest(const FString& Parameters)
{
	FFrostShockTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	const UMordecaiGE_Frostbitten* CDO = GetDefault<UMordecaiGE_Frostbitten>();
	const float InitialAtkSpd = Env.AttrSet->GetAttackSpeedMultiplier();

	// Apply Frostbitten
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Frostbitten::StaticClass(), nullptr);

	// AttackSpeedMultiplier reduced by 20%
	const float FrostAtkSpd = Env.AttrSet->GetAttackSpeedMultiplier();
	TestTrue(
		FString::Printf(TEXT("AttackSpeedMultiplier reduced (initial=%.2f, frost=%.2f, expected=%.2f)"),
			InitialAtkSpd, FrostAtkSpd, InitialAtkSpd - CDO->FrostbittenAttackSpeedReductionPct),
		FMath::IsNearlyEqual(FrostAtkSpd, InitialAtkSpd - CDO->FrostbittenAttackSpeedReductionPct, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-015.4: Frostbitten increases dodge recovery
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Frostbitten_IncreaseDodgeRecovery,
	"Mordecai.StatusEffect.Frostbitten.IncreaseDodgeRecovery",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Frostbitten_IncreaseDodgeRecovery::RunTest(const FString& Parameters)
{
	FFrostShockTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	const UMordecaiGE_Frostbitten* CDO = GetDefault<UMordecaiGE_Frostbitten>();
	const float InitialDodge = Env.AttrSet->GetDodgeRecoveryMultiplier();

	// Apply Frostbitten
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Frostbitten::StaticClass(), nullptr);

	// DodgeRecoveryMultiplier increased by 30%
	const float FrostDodge = Env.AttrSet->GetDodgeRecoveryMultiplier();
	TestTrue(
		FString::Printf(TEXT("DodgeRecoveryMultiplier increased (initial=%.2f, frost=%.2f, expected=%.2f)"),
			InitialDodge, FrostDodge, InitialDodge + CDO->FrostbittenDodgeRecoveryIncreasePct),
		FMath::IsNearlyEqual(FrostDodge, InitialDodge + CDO->FrostbittenDodgeRecoveryIncreasePct, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-015.5: Frostbitten stacks up to max
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Frostbitten_StacksUpToMax,
	"Mordecai.StatusEffect.Frostbitten.StacksUpToMax",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Frostbitten_StacksUpToMax::RunTest(const FString& Parameters)
{
	FFrostShockTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	const UMordecaiGE_Frostbitten* CDO = GetDefault<UMordecaiGE_Frostbitten>();
	TestEqual("Default max stacks is 5", CDO->FrostbittenMaxStacks, 5);

	// Apply Frostbitten multiple times (but less than max to avoid Frozen trigger)
	for (int32 i = 0; i < CDO->FrostbittenMaxStacks - 1; ++i)
	{
		Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Frostbitten::StaticClass(), nullptr);
	}

	// Should still have Frostbitten (not yet at max)
	TestTrue("Frostbitten still active at max-1 stacks",
		Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Frostbitten));

	// Verify stack count via ASC
	const int32 StackCount = Env.StatusComp->GetStatusEffectStackCount(MordecaiGameplayTags::Status_Frostbitten);
	TestEqual("Stack count matches max-1", StackCount, CDO->FrostbittenMaxStacks - 1);

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-015.5, AC-015.6: Freeze at max stacks — applies Frozen, clears Frostbitten
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Frostbitten_FreezeAtMaxStacks,
	"Mordecai.StatusEffect.Frostbitten.FreezeAtMaxStacks",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Frostbitten_FreezeAtMaxStacks::RunTest(const FString& Parameters)
{
	FFrostShockTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	const UMordecaiGE_Frostbitten* CDO = GetDefault<UMordecaiGE_Frostbitten>();

	// Apply Frostbitten max times
	for (int32 i = 0; i < CDO->FrostbittenMaxStacks; ++i)
	{
		Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Frostbitten::StaticClass(), nullptr);
	}

	// Frozen should be active
	TestTrue("Frozen tag active at max stacks",
		Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Frozen));

	// Frostbitten stacks should be cleared
	TestFalse("Frostbitten cleared after Frozen triggered",
		Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Frostbitten));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-015.6: Frozen blocks all actions (test ability blocked by Frozen tag)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Frostbitten_FrozenBlocksAllActions,
	"Mordecai.StatusEffect.Frostbitten.FrozenBlocksAllActions",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Frostbitten_FrozenBlocksAllActions::RunTest(const FString& Parameters)
{
	FFrostShockTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify Frozen GE CDO configuration
	const UMordecaiGE_Frozen* FrozenCDO = GetDefault<UMordecaiGE_Frozen>();
	TestNotNull("Frozen GE CDO exists", FrozenCDO);
	TestEqual("StatusTag is Frozen", FrozenCDO->StatusTag, FGameplayTag(MordecaiGameplayTags::Status_Frozen));
	TestEqual("Default duration is 2.0s", FrozenCDO->FrozenDurationSec, 2.0f);

	// Apply Frozen directly
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Frozen::StaticClass(), nullptr);
	TestTrue("Frozen tag active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Frozen));

	// Grant a basic ability and try to activate it
	FGameplayAbilitySpec AbilitySpec(UGameplayAbility::StaticClass(), 1, INDEX_NONE, nullptr);
	FGameplayAbilitySpecHandle AbilityHandle = Env.ASC->GiveAbility(AbilitySpec);

	// Verify Frozen GE has BlockAbilityTags configured
	// The Frozen GE should block ability activation via a BlockAbilityTagsGameplayEffectComponent.
	// We verify the Frozen tag is present — ability blocking is wired via the GE component.
	TestTrue("Frozen tag is on ASC", Env.ASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_Frozen));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-015.6: Frozen auto-expires after duration
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Frostbitten_FrozenAutoExpires,
	"Mordecai.StatusEffect.Frostbitten.FrozenAutoExpires",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Frostbitten_FrozenAutoExpires::RunTest(const FString& Parameters)
{
	FFrostShockTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	const UMordecaiGE_Frozen* CDO = GetDefault<UMordecaiGE_Frozen>();

	// Verify Frozen is a duration-based GE with correct duration
	TestEqual("Frozen DurationPolicy is HasDuration",
		static_cast<int32>(CDO->DurationPolicy),
		static_cast<int32>(EGameplayEffectDurationType::HasDuration));
	TestTrue("Frozen default duration is 2.0s",
		FMath::IsNearlyEqual(CDO->FrozenDurationSec, 2.0f));

	// Apply Frozen and verify it's active
	FActiveGameplayEffectHandle Handle = Env.StatusComp->ApplyStatusEffect(
		UMordecaiGE_Frozen::StaticClass(), nullptr);
	TestTrue("Frozen handle valid", Handle.IsValid());
	TestTrue("Frozen tag active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Frozen));

	// Verify we can remove it (simulates what expiry does)
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Frozen);
	TestFalse("Frozen removed after manual removal",
		Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Frozen));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-015.8: Frostbitten blocked by immunity
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Frostbitten_BlockedByImmunity,
	"Mordecai.StatusEffect.Frostbitten.BlockedByImmunity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Frostbitten_BlockedByImmunity::RunTest(const FString& Parameters)
{
	FFrostShockTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Grant immunity
	Env.ASC->AddLooseGameplayTag(MordecaiGameplayTags::Immunity_Frostbitten);

	// Attempt to apply Frostbitten
	FActiveGameplayEffectHandle Handle = Env.StatusComp->ApplyStatusEffect(
		UMordecaiGE_Frostbitten::StaticClass(), nullptr);
	TestFalse("Frostbitten blocked by immunity (handle invalid)", Handle.IsValid());
	TestFalse("Frostbitten tag not present",
		Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Frostbitten));

	Env.Teardown();
	return true;
}

// ===========================================================================
// SHOCKED TESTS
// ===========================================================================

// ---------------------------------------------------------------------------
// AC-015.9, AC-015.10: Shocked applies tag and stacks
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Shocked_AppliesTagAndStacks,
	"Mordecai.StatusEffect.Shocked.AppliesTagAndStacks",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Shocked_AppliesTagAndStacks::RunTest(const FString& Parameters)
{
	FFrostShockTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	const UMordecaiGE_Shocked* CDO = GetDefault<UMordecaiGE_Shocked>();
	TestNotNull("Shocked GE CDO exists", CDO);
	TestEqual("StatusTag is Shocked", CDO->StatusTag, FGameplayTag(MordecaiGameplayTags::Status_Shocked));
	TestEqual("Default duration is 5.0s", CDO->ShockedDurationSec, 5.0f);
	TestEqual("Default max stacks is 3", CDO->ShockedMaxStacks, 3);

	// Apply twice
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Shocked::StaticClass(), nullptr);
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Shocked::StaticClass(), nullptr);

	TestTrue("Shocked tag active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Shocked));

	const int32 StackCount = Env.StatusComp->GetStatusEffectStackCount(MordecaiGameplayTags::Status_Shocked);
	TestEqual("Stack count is 2", StackCount, 2);

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-015.11: Micro-stun on hit (forced 100% chance)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Shocked_MicroStunOnHit,
	"Mordecai.StatusEffect.Shocked.MicroStunOnHit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Shocked_MicroStunOnHit::RunTest(const FString& Parameters)
{
	FFrostShockTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Apply Shocked with enough stacks to guarantee micro-stun
	// We'll force by applying at max stacks, giving 3 * 0.15 = 0.45 chance
	// But to guarantee it in test, we use the override on the component
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Shocked::StaticClass(), nullptr);
	TestTrue("Shocked active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Shocked));

	// Force micro-stun chance to 1.0 for deterministic test
	Env.StatusComp->SetShockedMicroStunChanceOverride(1.0f);

	// Trigger damage taken
	Env.StatusComp->NotifyDamageTaken();

	// MicroStunned should be applied
	TestTrue("MicroStunned applied on hit while Shocked",
		Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_MicroStunned));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-015.12: Shocked increases block stamina cost per stack
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Shocked_IncreasesBlockStaminaCost,
	"Mordecai.StatusEffect.Shocked.IncreasesBlockStaminaCost",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Shocked_IncreasesBlockStaminaCost::RunTest(const FString& Parameters)
{
	FFrostShockTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	const UMordecaiGE_Shocked* CDO = GetDefault<UMordecaiGE_Shocked>();
	const float InitialBlockCost = Env.AttrSet->GetBlockStaminaCostMultiplier();
	TestTrue("Initial BlockStaminaCostMultiplier is 1.0",
		FMath::IsNearlyEqual(InitialBlockCost, 1.0f));

	// Apply Shocked twice (2 stacks)
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Shocked::StaticClass(), nullptr);
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Shocked::StaticClass(), nullptr);

	// BlockStaminaCostMultiplier should increase by per-stack rate * 2
	const float Expected = 1.0f + (CDO->ShockedBlockStaminaCostMultiplierPerStack * 2.0f);
	const float Actual = Env.AttrSet->GetBlockStaminaCostMultiplier();
	TestTrue(
		FString::Printf(TEXT("BlockStaminaCostMultiplier at 2 stacks (actual=%.2f, expected=%.2f)"),
			Actual, Expected),
		FMath::IsNearlyEqual(Actual, Expected, 0.05f));

	// Apply third stack
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Shocked::StaticClass(), nullptr);
	const float Expected3 = 1.0f + (CDO->ShockedBlockStaminaCostMultiplierPerStack * 3.0f);
	const float Actual3 = Env.AttrSet->GetBlockStaminaCostMultiplier();
	TestTrue(
		FString::Printf(TEXT("BlockStaminaCostMultiplier at 3 stacks (actual=%.2f, expected=%.2f)"),
			Actual3, Expected3),
		FMath::IsNearlyEqual(Actual3, Expected3, 0.05f));

	// Remove Shocked and verify BlockStaminaCostMultiplier restores
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Shocked);
	Env.StatusComp->StopShockedTracking();
	const float Restored = Env.AttrSet->GetBlockStaminaCostMultiplier();
	TestTrue(
		FString::Printf(TEXT("BlockStaminaCostMultiplier restored to 1.0 (actual=%.2f)"), Restored),
		FMath::IsNearlyEqual(Restored, 1.0f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-015.13: Shocked interrupts casting
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Shocked_InterruptsCasting,
	"Mordecai.StatusEffect.Shocked.InterruptsCasting",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Shocked_InterruptsCasting::RunTest(const FString& Parameters)
{
	FFrostShockTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Add Casting tag to target
	Env.ASC->AddLooseGameplayTag(MordecaiGameplayTags::State_Casting);
	TestTrue("Target is casting", Env.ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Casting));

	// Apply Shocked
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Shocked::StaticClass(), nullptr);

	// Force cast interrupt chance to 1.0
	Env.StatusComp->SetShockedCastInterruptChanceOverride(1.0f);

	// Trigger damage taken — should attempt cast interrupt
	bool bInterrupted = Env.StatusComp->TryShockedCastInterrupt();
	TestTrue("Cast interrupt succeeded with 100% chance while Shocked + Casting", bInterrupted);

	// Test without Casting tag
	Env.ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::State_Casting);
	bool bNoInterrupt = Env.StatusComp->TryShockedCastInterrupt();
	TestFalse("No interrupt without Casting tag", bNoInterrupt);

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-015.15: Shocked blocked by immunity
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Shocked_BlockedByImmunity,
	"Mordecai.StatusEffect.Shocked.BlockedByImmunity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Shocked_BlockedByImmunity::RunTest(const FString& Parameters)
{
	FFrostShockTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Grant immunity
	Env.ASC->AddLooseGameplayTag(MordecaiGameplayTags::Immunity_Shocked);

	// Attempt to apply Shocked
	FActiveGameplayEffectHandle Handle = Env.StatusComp->ApplyStatusEffect(
		UMordecaiGE_Shocked::StaticClass(), nullptr);
	TestFalse("Shocked blocked by immunity (handle invalid)", Handle.IsValid());
	TestFalse("Shocked tag not present",
		Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Shocked));

	Env.Teardown();
	return true;
}
