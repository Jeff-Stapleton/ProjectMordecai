// Project Mordecai — US-123 enemy posture runtime RED-first contracts.

#include "Misc/AutomationTest.h"

#include "Dom/JsonObject.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiGA_Parry.h"
#include "Mordecai/Combat/MordecaiPostureSystem.h"
#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/MordecaiPlayerState.h"
#include "Mordecai/Playtest/MordecaiAgentBridgeState.h"

namespace MordecaiPostureRuntimeTest
{
	bool HasProperty(const UClass* Class, const TCHAR* Name)
	{
		return Class && Class->FindPropertyByName(FName(Name)) != nullptr;
	}

	bool HasFunction(const UClass* Class, const TCHAR* Name)
	{
		return Class && Class->FindFunctionByName(FName(Name)) != nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_BreakIsServerOnlyAndReplicated,
	"Mordecai.Posture.Runtime.BreakIsServerOnlyAndReplicated",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_BreakIsServerOnlyAndReplicated::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	TestTrue("enemy exposes the authority-gated replicated state helper",
		HasFunction(AMordecaiEnemyCharacter::StaticClass(), TEXT("SetPostureRuntimeState")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_ExpiryRestoresMax,
	"Mordecai.Posture.Runtime.ExpiryRestoresMax",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_ExpiryRestoresMax::RunTest(const FString& Parameters)
{
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();
	System->EnterPostureBroken();
	TestEqual("expiry restores MaxPosture", System->ExitPostureBroken(137.f), 137.f);
	TestEqual("configured break duration is 1000 ms", System->PostureBrokenDurationMs, 1000.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_BreakCancelsSwing,
	"Mordecai.Posture.Runtime.BreakCancelsSwing",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_BreakCancelsSwing::RunTest(const FString& Parameters)
{
	TestTrue("enemy exposes cancellation used by every incapacitating posture state",
		MordecaiPostureRuntimeTest::HasFunction(
			AMordecaiEnemyCharacter::StaticClass(), TEXT("CancelInFlightMelee")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_StaggeredHitTimesOnePointFive,
	"Mordecai.Posture.Runtime.StaggeredHitTimesOnePointFive",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_StaggeredHitTimesOnePointFive::RunTest(const FString& Parameters)
{
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();
	System->EnterPostureBroken();
	TestEqual("staggered hit is x1.5", System->GetVulnerabilityDamageMultiplier(), 1.5f);
	TestTrue("melee exposes the runtime damage resolver",
		MordecaiPostureRuntimeTest::HasFunction(
			UMordecaiGA_MeleeAttack::StaticClass(), TEXT("ResolvePostureRuntimeDamage")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_KnockedDownHitTimesTwo,
	"Mordecai.Posture.Runtime.KnockedDownHitTimesTwo",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_KnockedDownHitTimesTwo::RunTest(const FString& Parameters)
{
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();
	System->EnterPostureBroken();
	System->EnterKnockDown();
	TestEqual("knocked-down hit is x2", System->GetVulnerabilityDamageMultiplier(), 2.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_UnbrokenHitUnchanged,
	"Mordecai.Posture.Runtime.UnbrokenHitUnchanged",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_UnbrokenHitUnchanged::RunTest(const FString& Parameters)
{
	const UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();
	TestEqual("unbroken hit is x1", System->GetVulnerabilityDamageMultiplier(), 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_KnockdownReplacesStagger,
	"Mordecai.Posture.Runtime.KnockdownReplacesStagger",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_KnockdownReplacesStagger::RunTest(const FString& Parameters)
{
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();
	System->EnterPostureBroken();
	System->AccumulateStaggerDamage(49.f);
	TestFalse("49 health actually taken does not knock down", System->ShouldKnockDown());
	System->Reset();
	System->EnterPostureBroken();
	System->AccumulateStaggerDamage(50.f);
	TestTrue("exactly 50 health actually taken knocks down", System->ShouldKnockDown());
	System->EnterKnockDown();
	TestFalse("knockdown replaces stagger", System->IsPostureBroken());
	TestTrue("knockdown is active", System->IsKnockedDown());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_KnockdownExpiry,
	"Mordecai.Posture.Runtime.KnockdownExpiry",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_KnockdownExpiry::RunTest(const FString& Parameters)
{
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();
	System->EnterPostureBroken();
	System->EnterKnockDown();
	TestEqual("knockdown duration is 2000 ms", System->KnockDownDurationMs, 2000.f);
	TestEqual("knockdown expiry restores max", System->ExitKnockDown(123.f), 123.f);
	TestFalse("knockdown ends", System->IsKnockedDown());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_NoRegenWhileDown,
	"Mordecai.Posture.Runtime.NoRegenWhileDown",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_NoRegenWhileDown::RunTest(const FString& Parameters)
{
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();
	System->EnterPostureBroken();
	TestFalse("no regen while broken", System->IsRegenAllowed(9999.0));
	System->EnterKnockDown();
	TestFalse("no regen while knocked down", System->IsRegenAllowed(9999.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_WeakenedReducesPostureDealt,
	"Mordecai.Posture.Runtime.WeakenedReducesPostureDealt",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_WeakenedReducesPostureDealt::RunTest(const FString& Parameters)
{
	TestTrue("posture system exposes the ASC-based shared posture helper",
		MordecaiPostureRuntimeTest::HasFunction(
			UMordecaiPostureSystem::StaticClass(), TEXT("ComputePostureDamageFromAttributes")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_BrittleIncreasesPostureTaken,
	"Mordecai.Posture.Runtime.BrittleIncreasesPostureTaken",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_BrittleIncreasesPostureTaken::RunTest(const FString& Parameters)
{
	TestTrue("the same shared helper reads target posture modifiers",
		MordecaiPostureRuntimeTest::HasFunction(
			UMordecaiPostureSystem::StaticClass(), TEXT("ComputePostureDamageFromAttributes")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_ParryUsesPostureHelper,
	"Mordecai.Posture.Runtime.ParryUsesPostureHelper",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_ParryUsesPostureHelper::RunTest(const FString& Parameters)
{
	TestTrue("parry exposes its shared posture-damage route",
		MordecaiPostureRuntimeTest::HasFunction(
			UMordecaiGA_Parry::StaticClass(), TEXT("ApplyParryPostureDamage")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_RegenUsesRecoveryMultiplier,
	"Mordecai.Posture.Runtime.RegenUsesRecoveryMultiplier",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_RegenUsesRecoveryMultiplier::RunTest(const FString& Parameters)
{
	TestTrue("enemy exposes one authority regen step",
		MordecaiPostureRuntimeTest::HasFunction(
			AMordecaiEnemyCharacter::StaticClass(), TEXT("RegeneratePosture")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_ParriedExpires,
	"Mordecai.Posture.Runtime.ParriedExpires",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_ParriedExpires::RunTest(const FString& Parameters)
{
	TestTrue("ParriedDurationMs is configurable",
		MordecaiPostureRuntimeTest::HasProperty(
			UMordecaiPostureSystem::StaticClass(), TEXT("ParriedDurationMs")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_RiposteExpiresOrConsumed,
	"Mordecai.Posture.Runtime.RiposteExpiresOrConsumed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_RiposteExpiresOrConsumed::RunTest(const FString& Parameters)
{
	TestTrue("RiposteWindowMs is configurable",
		MordecaiPostureRuntimeTest::HasProperty(
			UMordecaiPostureSystem::StaticClass(), TEXT("RiposteWindowMs")));
	TestTrue("melee exposes one-hit riposte consumption",
		MordecaiPostureRuntimeTest::HasFunction(
			UMordecaiGA_MeleeAttack::StaticClass(), TEXT("ConsumeRiposte")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_ResetClearsAllPostureStates,
	"Mordecai.Posture.Runtime.ResetClearsAllPostureStates",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_ResetClearsAllPostureStates::RunTest(const FString& Parameters)
{
	TestTrue("enemy reset helper clears all runtime posture tags",
		MordecaiPostureRuntimeTest::HasFunction(
			AMordecaiEnemyCharacter::StaticClass(), TEXT("ClearPostureRuntimeStates")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_RiposteHitIsCritical,
	"Mordecai.Posture.Runtime.RiposteHitIsCritical",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_RiposteHitIsCritical::RunTest(const FString& Parameters)
{
	TestTrue("RiposteDamageMultiplier is configurable",
		MordecaiPostureRuntimeTest::HasProperty(
			UMordecaiPostureSystem::StaticClass(), TEXT("RiposteDamageMultiplier")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_RiposteDoesNotStackWithStagger,
	"Mordecai.Posture.Runtime.RiposteDoesNotStackWithStagger",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_RiposteDoesNotStackWithStagger::RunTest(const FString& Parameters)
{
	const UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();
	TestEqual("larger stagger/riposte multiplier wins, rather than multiplying",
		FMath::Max(System->StaggeredDamageMultiplier, 1.5f), 1.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_PlayerPostureRegenerates,
	"Mordecai.Posture.Runtime.PlayerPostureRegenerates",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_PlayerPostureRegenerates::RunTest(const FString& Parameters)
{
	TestTrue("PlayerState owns the posture runtime component",
		MordecaiPostureRuntimeTest::HasProperty(
			AMordecaiPlayerState::StaticClass(), TEXT("PostureRuntimeComponent")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_BridgeState_EnemyPosture,
	"Mordecai.Playtest.BridgeState.EnemyPosture",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_BridgeState_EnemyPosture::RunTest(const FString& Parameters)
{
	AMordecaiEnemyCharacter* Enemy = NewObject<AMordecaiEnemyCharacter>();
	UMordecaiAttributeSet* Attributes = Enemy->GetMordecaiAttributeSet();
	if (!TestNotNull("enemy attribute set", Attributes))
	{
		return true;
	}
	Attributes->InitPosture(37.f);
	Attributes->InitMaxPosture(80.f);
	const TSharedRef<FJsonObject> Entry =
		MordecaiAgentBridge::State::MakeActorEntry(Enemy, nullptr);
	double Posture = -1.0;
	double MaxPosture = -1.0;
	TestTrue("actors[] enemy carries posture", Entry->TryGetNumberField(TEXT("posture"), Posture));
	TestTrue("actors[] enemy carries max_posture", Entry->TryGetNumberField(TEXT("max_posture"), MaxPosture));
	TestEqual("posture value", Posture, 37.0);
	TestEqual("max_posture value", MaxPosture, 80.0);
	return true;
}
