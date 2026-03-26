// Project Mordecai — Speed & Timing Impairment Status Effect Tests (US-015)

#include "Misc/AutomationTest.h"
#include "GameplayTagsManager.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectTypes.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectComponent.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Frostbitten.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Frozen.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Shocked.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_MicroStunned.h"

// ---------------------------------------------------------------------------
// Helper: Test environment with ASC + Attribute Set
// ---------------------------------------------------------------------------
struct FSpeedTimingTestEnv
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
// FROSTBITTEN TESTS
// ===========================================================================

// ---------------------------------------------------------------------------
// AC-015.1, AC-015.2: Frostbitten applies tag and reduces move speed
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Frostbitten_AppliesTagAndReducesSpeed,
	"Mordecai.StatusEffect.Frostbitten.AppliesTagAndReducesSpeed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Frostbitten_AppliesTagAndReducesSpeed::RunTest(const FString& Parameters)
{
	FSpeedTimingTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify GE CDO configuration
	const UMordecaiGE_Frostbitten* CDO = GetDefault<UMordecaiGE_Frostbitten>();
	TestNotNull("Frostbitten GE CDO exists", CDO);
	TestEqual("StatusTag is Frostbitten", CDO->StatusTag, FGameplayTag(MordecaiGameplayTags::Status_Frostbitten));
	TestEqual("Default duration is 6.0s", CDO->FrostbittenDurationSec, 6.0f);

	// Record initial MoveSpeedMultiplier
	const float InitialSpeed = Env.AttrSet->GetMoveSpeedMultiplier();
	TestTrue("Initial MoveSpeedMultiplier is 1.0", FMath::IsNearlyEqual(InitialSpeed, 1.0f));

	// Apply Frostbitten
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Frostbitten::StaticClass(), nullptr);
	TestTrue("Frostbitten tag is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Frostbitten));

	// Verify MoveSpeedMultiplier reduced by 25%
	const float FrostbittenSpeed = Env.AttrSet->GetMoveSpeedMultiplier();
	TestTrue(
		FString::Printf(TEXT("MoveSpeedMultiplier reduced to ~0.75 (actual=%.2f)"), FrostbittenSpeed),
		FMath::IsNearlyEqual(FrostbittenSpeed, 0.75f, 0.05f));

	// Remove and verify restored
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
	FSpeedTimingTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	const float InitialAttackSpeed = Env.AttrSet->GetAttackSpeedMultiplier();

	// Apply Frostbitten
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Frostbitten::StaticClass(), nullptr);

	// Verify AttackSpeedMultiplier reduced by 20%
	const float FrostbittenAttackSpeed = Env.AttrSet->GetAttackSpeedMultiplier();
	const float ExpectedAttackSpeed = InitialAttackSpeed - 0.20f;
	TestTrue(
		FString::Printf(TEXT("AttackSpeedMultiplier reduced (initial=%.2f, frostbitten=%.2f, expected=%.2f)"),
			InitialAttackSpeed, FrostbittenAttackSpeed, ExpectedAttackSpeed),
		FMath::IsNearlyEqual(FrostbittenAttackSpeed, ExpectedAttackSpeed, 0.05f));

	// Remove and verify restored
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Frostbitten);
	Env.StatusComp->StopFrostbittenTracking();
	const float RestoredAttackSpeed = Env.AttrSet->GetAttackSpeedMultiplier();
	TestTrue(
		FString::Printf(TEXT("AttackSpeedMultiplier restored (actual=%.2f, expected=%.2f)"),
			RestoredAttackSpeed, InitialAttackSpeed),
		FMath::IsNearlyEqual(RestoredAttackSpeed, InitialAttackSpeed, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-015.4: Frostbitten increases dodge recovery time
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Frostbitten_IncreaseDodgeRecovery,
	"Mordecai.StatusEffect.Frostbitten.IncreaseDodgeRecovery",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Frostbitten_IncreaseDodgeRecovery::RunTest(const FString& Parameters)
{
	FSpeedTimingTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify initial DodgeRecoveryMultiplier is 1.0
	const float InitialRecovery = Env.AttrSet->GetDodgeRecoveryMultiplier();
	TestTrue("Initial DodgeRecoveryMultiplier is 1.0",
		FMath::IsNearlyEqual(InitialRecovery, 1.0f));

	// Apply Frostbitten
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Frostbitten::StaticClass(), nullptr);

	// Verify DodgeRecoveryMultiplier increased by 30%
	const float FrostbittenRecovery = Env.AttrSet->GetDodgeRecoveryMultiplier();
	TestTrue(
		FString::Printf(TEXT("DodgeRecoveryMultiplier increased to ~1.30 (actual=%.2f)"), FrostbittenRecovery),
		FMath::IsNearlyEqual(FrostbittenRecovery, 1.30f, 0.05f));

	// Remove and verify restored
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Frostbitten);
	Env.StatusComp->StopFrostbittenTracking();
	const float RestoredRecovery = Env.AttrSet->GetDodgeRecoveryMultiplier();
	TestTrue(
		FString::Printf(TEXT("DodgeRecoveryMultiplier restored to 1.0 (actual=%.2f)"), RestoredRecovery),
		FMath::IsNearlyEqual(RestoredRecovery, 1.0f, 0.05f));

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
	FSpeedTimingTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	const UMordecaiGE_Frostbitten* CDO = GetDefault<UMordecaiGE_Frostbitten>();
	TestEqual("Default max stacks is 5", CDO->FrostbittenMaxStacks, 5);

	// Apply Frostbitten 4 times (below max to avoid freeze trigger)
	for (int32 i = 0; i < 4; i++)
	{
		Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Frostbitten::StaticClass(), nullptr);
	}

	TestTrue("Frostbitten active after 4 stacks", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Frostbitten));
	TestEqual("Stack count is 4", Env.StatusComp->GetFrostbittenStackCount(), 4);

	// Verify stacking config
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	TestEqual("StackingType is AggregateByTarget",
		static_cast<int32>(CDO->StackingType),
		static_cast<int32>(EGameplayEffectStackingType::AggregateByTarget));
	TestEqual("StackLimitCount is 5", CDO->StackLimitCount, 5);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-015.5, AC-015.6: Freeze at max Frostbitten stacks
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Frostbitten_FreezeAtMaxStacks,
	"Mordecai.StatusEffect.Frostbitten.FreezeAtMaxStacks",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Frostbitten_FreezeAtMaxStacks::RunTest(const FString& Parameters)
{
	FSpeedTimingTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Apply Frostbitten 5 times (max stacks)
	for (int32 i = 0; i < 5; i++)
	{
		Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Frostbitten::StaticClass(), nullptr);
	}

	// Frozen should be triggered and Frostbitten cleared
	TestTrue("Frozen tag active after max stacks",
		Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Frozen));
	TestFalse("Frostbitten stacks cleared after freeze",
		Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Frostbitten));

	// Verify Frozen GE configuration
	const UMordecaiGE_Frozen* FrozenCDO = GetDefault<UMordecaiGE_Frozen>();
	TestEqual("Frozen StatusTag", FrozenCDO->StatusTag, FGameplayTag(MordecaiGameplayTags::Status_Frozen));
	TestEqual("Frozen duration is 2.0s", FrozenCDO->FrozenDurationSec, 2.0f);

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-015.6: Frozen blocks all actions (ability activation blocked)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Frostbitten_FrozenBlocksAllActions,
	"Mordecai.StatusEffect.Frostbitten.FrozenBlocksAllActions",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Frostbitten_FrozenBlocksAllActions::RunTest(const FString& Parameters)
{
	FSpeedTimingTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Create a test ability with Frozen in its ActivationBlockedTags
	UGameplayAbility* TestAbility = NewObject<UGameplayAbility>(GetTransientPackage());
	TestAbility->ActivationBlockedTags.AddTag(MordecaiGameplayTags::Status_Frozen);

	// Give the ability to the ASC
	FGameplayAbilitySpec AbilitySpec(TestAbility, 1, INDEX_NONE, nullptr);
	FGameplayAbilitySpecHandle Handle = Env.ASC->GiveAbility(AbilitySpec);
	TestTrue("Test ability granted", Handle.IsValid());

	// Apply Frozen
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Frozen::StaticClass(), nullptr);
	TestTrue("Frozen tag active", Env.ASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_Frozen));

	// Attempt to activate ability — should be blocked by Frozen tag
	bool bActivated = Env.ASC->TryActivateAbility(Handle);
	TestFalse("Ability blocked by Frozen tag", bActivated);

	// Remove Frozen and verify ability can activate
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Frozen);
	TestFalse("Frozen tag removed", Env.ASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_Frozen));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-015.6: Frozen auto-expires after FrozenDurationSec
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Frostbitten_FrozenAutoExpires,
	"Mordecai.StatusEffect.Frostbitten.FrozenAutoExpires",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Frostbitten_FrozenAutoExpires::RunTest(const FString& Parameters)
{
	FSpeedTimingTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify Frozen duration policy
	const UMordecaiGE_Frozen* CDO = GetDefault<UMordecaiGE_Frozen>();
	TestEqual("DurationPolicy is HasDuration",
		static_cast<int32>(CDO->DurationPolicy),
		static_cast<int32>(EGameplayEffectDurationType::HasDuration));
	TestEqual("Frozen duration is 2.0s", CDO->FrozenDurationSec, 2.0f);

	// Apply Frozen
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Frozen::StaticClass(), nullptr);
	TestTrue("Frozen active after application", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Frozen));

	// Tick past the Frozen duration
	Env.World->Tick(LEVELTICK_All, 2.5f);

	// Frozen should have expired
	TestFalse("Frozen expired after duration", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Frozen));

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
	FSpeedTimingTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Grant immunity
	Env.ASC->AddLooseGameplayTag(MordecaiGameplayTags::Immunity_Frostbitten);

	// Attempt to apply Frostbitten
	FActiveGameplayEffectHandle Handle = Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Frostbitten::StaticClass(), nullptr);
	TestFalse("Frostbitten blocked by immunity (handle invalid)", Handle.IsValid());
	TestFalse("Frostbitten tag not present", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Frostbitten));

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
	FSpeedTimingTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify GE CDO configuration
	const UMordecaiGE_Shocked* CDO = GetDefault<UMordecaiGE_Shocked>();
	TestNotNull("Shocked GE CDO exists", CDO);
	TestEqual("StatusTag is Shocked", CDO->StatusTag, FGameplayTag(MordecaiGameplayTags::Status_Shocked));
	TestEqual("Default duration is 5.0s", CDO->ShockedDurationSec, 5.0f);
	TestEqual("Default max stacks is 3", CDO->ShockedMaxStacks, 3);

	// Apply Shocked twice
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Shocked::StaticClass(), nullptr);
	TestTrue("Shocked tag active after first apply", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Shocked));

	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Shocked::StaticClass(), nullptr);
	TestTrue("Shocked tag still active after second apply", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Shocked));

	// Verify stack count is 2
	TestEqual("Shocked stack count is 2", Env.StatusComp->GetShockedStackCount(), 2);

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-015.11: Micro-stun on hit while Shocked
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Shocked_MicroStunOnHit,
	"Mordecai.StatusEffect.Shocked.MicroStunOnHit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Shocked_MicroStunOnHit::RunTest(const FString& Parameters)
{
	FSpeedTimingTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify CDO configuration
	const UMordecaiGE_Shocked* CDO = GetDefault<UMordecaiGE_Shocked>();
	TestTrue("Default micro-stun chance is 0.15 per stack",
		FMath::IsNearlyEqual(CDO->ShockedMicroStunChancePerStack, 0.15f));

	// Verify MicroStunned GE configuration
	const UMordecaiGE_MicroStunned* MicroStunCDO = GetDefault<UMordecaiGE_MicroStunned>();
	TestEqual("MicroStunned StatusTag", MicroStunCDO->StatusTag, FGameplayTag(MordecaiGameplayTags::Status_MicroStunned));
	TestTrue("MicroStunned duration is ~0.3s",
		FMath::IsNearlyEqual(MicroStunCDO->MicroStunDurationSec, 0.3f));

	// Test with 100% chance — should always trigger
	bool bStunned = UMordecaiGE_Shocked::TryMicroStun(Env.ASC, 1.0f);
	TestTrue("Micro-stun triggered with 100% chance", bStunned);
	TestTrue("MicroStunned tag active",
		Env.ASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_MicroStunned));

	// Test with 0% chance — should never trigger
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_MicroStunned);
	bool bNotStunned = UMordecaiGE_Shocked::TryMicroStun(Env.ASC, 0.0f);
	TestFalse("Micro-stun NOT triggered with 0% chance", bNotStunned);

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
	FSpeedTimingTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	const UMordecaiGE_Shocked* CDO = GetDefault<UMordecaiGE_Shocked>();
	TestTrue("Default block cost multiplier per stack is 0.20",
		FMath::IsNearlyEqual(CDO->ShockedBlockStaminaCostMultiplierPerStack, 0.20f));

	// Verify initial BlockStaminaCostMultiplier
	const float InitialCost = Env.AttrSet->GetBlockStaminaCostMultiplier();
	TestTrue("Initial BlockStaminaCostMultiplier is 1.0",
		FMath::IsNearlyEqual(InitialCost, 1.0f));

	// Apply Shocked twice (2 stacks)
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Shocked::StaticClass(), nullptr);
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Shocked::StaticClass(), nullptr);

	// Expected: 1.0 + (0.20 * 2 stacks) = 1.40
	const float ShockedCost = Env.AttrSet->GetBlockStaminaCostMultiplier();
	TestTrue(
		FString::Printf(TEXT("BlockStaminaCostMultiplier increased to ~1.40 with 2 stacks (actual=%.2f)"), ShockedCost),
		FMath::IsNearlyEqual(ShockedCost, 1.40f, 0.05f));

	// Remove Shocked and verify restored
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Shocked);
	Env.StatusComp->StopShockedTracking();
	const float RestoredCost = Env.AttrSet->GetBlockStaminaCostMultiplier();
	TestTrue(
		FString::Printf(TEXT("BlockStaminaCostMultiplier restored to 1.0 (actual=%.2f)"), RestoredCost),
		FMath::IsNearlyEqual(RestoredCost, 1.0f, 0.05f));

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
	FSpeedTimingTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify CDO default
	const UMordecaiGE_Shocked* CDO = GetDefault<UMordecaiGE_Shocked>();
	TestTrue("Default cast interrupt chance is 0.25",
		FMath::IsNearlyEqual(CDO->ShockedCastInterruptChance, 0.25f));

	// Add Casting tag to target
	Env.ASC->AddLooseGameplayTag(MordecaiGameplayTags::State_Casting);
	TestTrue("Target has Casting tag", Env.ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Casting));

	// Test with 100% chance — should always interrupt
	bool bInterrupted = UMordecaiGE_Shocked::TryShockedCastInterrupt(Env.ASC, 1.0f);
	TestTrue("Cast interrupted with 100% chance", bInterrupted);

	// Test with 0% chance — should never interrupt
	bool bNotInterrupted = UMordecaiGE_Shocked::TryShockedCastInterrupt(Env.ASC, 0.0f);
	TestFalse("Cast NOT interrupted with 0% chance", bNotInterrupted);

	// Test without Casting tag — should not interrupt even at 100%
	Env.ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::State_Casting);
	bool bNoEffect = UMordecaiGE_Shocked::TryShockedCastInterrupt(Env.ASC, 1.0f);
	TestFalse("No interrupt without Casting tag", bNoEffect);

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
	FSpeedTimingTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Grant immunity
	Env.ASC->AddLooseGameplayTag(MordecaiGameplayTags::Immunity_Shocked);

	// Attempt to apply Shocked
	FActiveGameplayEffectHandle Handle = Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Shocked::StaticClass(), nullptr);
	TestFalse("Shocked blocked by immunity (handle invalid)", Handle.IsValid());
	TestFalse("Shocked tag not present", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Shocked));

	Env.Teardown();
	return true;
}
