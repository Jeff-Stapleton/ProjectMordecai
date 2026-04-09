// Project Mordecai — Control Debuff Tests: Silenced & Rooted (US-017)

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
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Silenced.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Rooted.h"
#include "Mordecai/Magic/MordecaiGA_SpellBase.h"

// ---------------------------------------------------------------------------
// Helper: Test environment with ASC + Attribute Set (same pattern as US-016)
// ---------------------------------------------------------------------------
struct FControlDebuffTestEnv
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
// SILENCED TESTS
// ===========================================================================

// ---------------------------------------------------------------------------
// AC-017.1: Silenced applies tag with correct duration
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Silenced_AppliesTag,
	"Mordecai.StatusEffect.Silenced.AppliesTag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Silenced_AppliesTag::RunTest(const FString& Parameters)
{
	FControlDebuffTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify GE CDO configuration
	const UMordecaiGE_Silenced* CDO = GetDefault<UMordecaiGE_Silenced>();
	TestNotNull("Silenced GE CDO exists", CDO);
	TestEqual("StatusTag is Silenced", CDO->StatusTag, FGameplayTag(MordecaiGameplayTags::Status_Silenced));
	TestEqual("Default duration is 5.0s", CDO->SilencedDurationSec, 5.0f);

	// Apply Silenced
	FActiveGameplayEffectHandle Handle = Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Silenced::StaticClass(), nullptr);
	TestTrue("Silenced applied successfully (handle valid)", Handle.IsValid());
	TestTrue("Silenced tag is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Silenced));

	// Remove and verify cleared
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Silenced);
	TestFalse("Silenced tag removed", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Silenced));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-017.3: Silenced pauses spell point regen (SpellPointsRegenMultiplier -> 0)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Silenced_PausesSpellRegen,
	"Mordecai.StatusEffect.Silenced.PausesSpellRegen",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Silenced_PausesSpellRegen::RunTest(const FString& Parameters)
{
	FControlDebuffTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify initial SpellPointsRegenMultiplier is 1.0
	const float InitialMult = Env.AttrSet->GetSpellPointsRegenMultiplier();
	TestTrue("Initial SpellPointsRegenMultiplier is 1.0",
		FMath::IsNearlyEqual(InitialMult, 1.0f));

	// Apply Silenced
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Silenced::StaticClass(), nullptr);

	// Verify SpellPointsRegenMultiplier is 0
	const float SilencedMult = Env.AttrSet->GetSpellPointsRegenMultiplier();
	TestTrue(
		FString::Printf(TEXT("SpellPointsRegenMultiplier is 0 while Silenced (actual=%.2f)"), SilencedMult),
		FMath::IsNearlyEqual(SilencedMult, 0.0f, 0.05f));

	// Remove and verify restored
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Silenced);
	const float RestoredMult = Env.AttrSet->GetSpellPointsRegenMultiplier();
	TestTrue(
		FString::Printf(TEXT("SpellPointsRegenMultiplier restored to 1.0 (actual=%.2f)"), RestoredMult),
		FMath::IsNearlyEqual(RestoredMult, 1.0f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-017.5: Silenced blocked by immunity
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Silenced_BlockedByImmunity,
	"Mordecai.StatusEffect.Silenced.BlockedByImmunity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Silenced_BlockedByImmunity::RunTest(const FString& Parameters)
{
	FControlDebuffTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Grant immunity
	Env.ASC->AddLooseGameplayTag(MordecaiGameplayTags::Immunity_Silenced);

	// Attempt to apply Silenced
	FActiveGameplayEffectHandle Handle = Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Silenced::StaticClass(), nullptr);
	TestFalse("Silenced blocked by immunity (handle invalid)", Handle.IsValid());
	TestFalse("Silenced tag not present", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Silenced));

	// Verify attribute unchanged
	const float Mult = Env.AttrSet->GetSpellPointsRegenMultiplier();
	TestTrue("SpellPointsRegenMultiplier unchanged at 1.0",
		FMath::IsNearlyEqual(Mult, 1.0f));

	Env.Teardown();
	return true;
}

// ===========================================================================
// ROOTED TESTS
// ===========================================================================

// ---------------------------------------------------------------------------
// AC-017.6: Rooted applies tag with correct duration
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Rooted_AppliesTag,
	"Mordecai.StatusEffect.Rooted.AppliesTag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Rooted_AppliesTag::RunTest(const FString& Parameters)
{
	FControlDebuffTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify GE CDO configuration
	const UMordecaiGE_Rooted* CDO = GetDefault<UMordecaiGE_Rooted>();
	TestNotNull("Rooted GE CDO exists", CDO);
	TestEqual("StatusTag is Rooted", CDO->StatusTag, FGameplayTag(MordecaiGameplayTags::Status_Rooted));
	TestEqual("Default duration is 3.0s", CDO->RootedDurationSec, 3.0f);

	// Apply Rooted
	FActiveGameplayEffectHandle Handle = Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Rooted::StaticClass(), nullptr);
	TestTrue("Rooted applied successfully (handle valid)", Handle.IsValid());
	TestTrue("Rooted tag is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Rooted));

	// Remove and verify cleared
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Rooted);
	TestFalse("Rooted tag removed", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Rooted));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-017.7: Rooted blocks movement (MoveSpeedMultiplier -> 0)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Rooted_BlocksMovement,
	"Mordecai.StatusEffect.Rooted.BlocksMovement",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Rooted_BlocksMovement::RunTest(const FString& Parameters)
{
	FControlDebuffTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify initial MoveSpeedMultiplier is 1.0
	const float InitialMult = Env.AttrSet->GetMoveSpeedMultiplier();
	TestTrue("Initial MoveSpeedMultiplier is 1.0",
		FMath::IsNearlyEqual(InitialMult, 1.0f));

	// Apply Rooted
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Rooted::StaticClass(), nullptr);

	// Verify MoveSpeedMultiplier is 0
	const float RootedMult = Env.AttrSet->GetMoveSpeedMultiplier();
	TestTrue(
		FString::Printf(TEXT("MoveSpeedMultiplier is 0 while Rooted (actual=%.2f)"), RootedMult),
		FMath::IsNearlyEqual(RootedMult, 0.0f, 0.05f));

	// Remove and verify restored
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Rooted);
	const float RestoredMult = Env.AttrSet->GetMoveSpeedMultiplier();
	TestTrue(
		FString::Printf(TEXT("MoveSpeedMultiplier restored to 1.0 (actual=%.2f)"), RestoredMult),
		FMath::IsNearlyEqual(RestoredMult, 1.0f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-017.8: Rooted allows attack, block, and spellcasting
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Rooted_AllowsAttackAndBlock,
	"Mordecai.StatusEffect.Rooted.AllowsAttackAndBlock",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Rooted_AllowsAttackAndBlock::RunTest(const FString& Parameters)
{
	FControlDebuffTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Apply Rooted
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Rooted::StaticClass(), nullptr);
	TestTrue("Rooted is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Rooted));

	// Verify Rooted only blocks movement, not combat actions:
	// 1. Status_Rooted is NOT in SpellBase's ActivationBlockedTags (spells still castable)
	const UMordecaiGA_SpellBase* SpellCDO = GetDefault<UMordecaiGA_SpellBase>();
	TestFalse("SpellBase does NOT block on Status_Rooted",
		SpellCDO->HasActivationBlockedTag(MordecaiGameplayTags::Status_Rooted));

	// 2. No PostureBroken or KnockedDown tags set by Rooted
	TestFalse("No PostureBroken from Rooted",
		Env.ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_PostureBroken));
	TestFalse("No KnockedDown from Rooted",
		Env.ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_KnockedDown));

	// 3. Combat-related attributes are unchanged
	TestTrue("OutgoingPostureDamageMultiplier unchanged at 1.0",
		FMath::IsNearlyEqual(Env.AttrSet->GetOutgoingPostureDamageMultiplier(), 1.0f));
	TestTrue("IncomingDamageMultiplier unchanged at 1.0",
		FMath::IsNearlyEqual(Env.AttrSet->GetIncomingDamageMultiplier(), 1.0f));

	// 4. SpellPointsRegenMultiplier is NOT affected by Rooted (only by Silenced)
	TestTrue("SpellPointsRegenMultiplier unchanged at 1.0",
		FMath::IsNearlyEqual(Env.AttrSet->GetSpellPointsRegenMultiplier(), 1.0f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-017.9: Break-free removes Root and consumes stamina
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Rooted_BreakFreeConsumesStamina,
	"Mordecai.StatusEffect.Rooted.BreakFreeConsumesStamina",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Rooted_BreakFreeConsumesStamina::RunTest(const FString& Parameters)
{
	FControlDebuffTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Verify initial stamina
	const float InitialStamina = Env.AttrSet->GetStamina();
	TestTrue("Initial Stamina is 100.0",
		FMath::IsNearlyEqual(InitialStamina, 100.0f));

	// Apply Rooted
	Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Rooted::StaticClass(), nullptr);
	TestTrue("Rooted is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Rooted));

	// Send BreakFree event
	FGameplayEventData EventData;
	EventData.Instigator = Env.TestActor;
	Env.ASC->HandleGameplayEvent(MordecaiGameplayTags::Event_BreakFree, &EventData);

	// Verify Root removed
	TestFalse("Root removed after break-free", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Rooted));

	// Verify stamina consumed (default cost: 25.0)
	const UMordecaiGE_Rooted* CDO = GetDefault<UMordecaiGE_Rooted>();
	const float ExpectedStamina = InitialStamina - CDO->RootedBreakFreeStaminaCost;
	const float ActualStamina = Env.AttrSet->GetStamina();
	TestTrue(
		FString::Printf(TEXT("Stamina reduced to %.1f (actual=%.1f)"), ExpectedStamina, ActualStamina),
		FMath::IsNearlyEqual(ActualStamina, ExpectedStamina, 1.0f));

	// Verify MoveSpeedMultiplier restored
	const float RestoredMult = Env.AttrSet->GetMoveSpeedMultiplier();
	TestTrue(
		FString::Printf(TEXT("MoveSpeedMultiplier restored to 1.0 (actual=%.2f)"), RestoredMult),
		FMath::IsNearlyEqual(RestoredMult, 1.0f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-017.10: Rooted refreshes duration on re-application
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Rooted_RefreshesDurationOnReapply,
	"Mordecai.StatusEffect.Rooted.RefreshesDurationOnReapply",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Rooted_RefreshesDurationOnReapply::RunTest(const FString& Parameters)
{
	FControlDebuffTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Apply Rooted
	FActiveGameplayEffectHandle Handle1 = Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Rooted::StaticClass(), nullptr);
	TestTrue("First Rooted applied", Handle1.IsValid());
	TestTrue("Rooted is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Rooted));

	// Re-apply Rooted (should refresh duration, not stack)
	FActiveGameplayEffectHandle Handle2 = Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Rooted::StaticClass(), nullptr);
	TestTrue("Second Rooted applied (refreshed)", Handle2.IsValid());
	TestTrue("Rooted still active after refresh", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Rooted));

	// Verify stacking policy: AggregateBySource with refresh
	const UMordecaiGE_Rooted* CDO = GetDefault<UMordecaiGE_Rooted>();
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	TestEqual("StackDurationRefreshPolicy is RefreshOnSuccessfulApplication",
		static_cast<int32>(CDO->StackDurationRefreshPolicy),
		static_cast<int32>(EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication));
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	// MoveSpeedMultiplier should still be 0 (not double-stacked)
	const float Mult = Env.AttrSet->GetMoveSpeedMultiplier();
	TestTrue(
		FString::Printf(TEXT("MoveSpeedMultiplier still 0 after refresh (actual=%.2f)"), Mult),
		FMath::IsNearlyEqual(Mult, 0.0f, 0.05f));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// AC-017.11: Rooted blocked by immunity
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_Rooted_BlockedByImmunity,
	"Mordecai.StatusEffect.Rooted.BlockedByImmunity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_Rooted_BlockedByImmunity::RunTest(const FString& Parameters)
{
	FControlDebuffTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Grant immunity
	Env.ASC->AddLooseGameplayTag(MordecaiGameplayTags::Immunity_Rooted);

	// Attempt to apply Rooted
	FActiveGameplayEffectHandle Handle = Env.StatusComp->ApplyStatusEffect(UMordecaiGE_Rooted::StaticClass(), nullptr);
	TestFalse("Rooted blocked by immunity (handle invalid)", Handle.IsValid());
	TestFalse("Rooted tag not present", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Rooted));

	// Verify attribute unchanged
	const float Mult = Env.AttrSet->GetMoveSpeedMultiplier();
	TestTrue("MoveSpeedMultiplier unchanged at 1.0",
		FMath::IsNearlyEqual(Mult, 1.0f));

	Env.Teardown();
	return true;
}
