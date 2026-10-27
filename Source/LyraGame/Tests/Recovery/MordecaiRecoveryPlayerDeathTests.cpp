// Project Mordecai — Recovery: the player dies when Health reaches 0 (N31, found by the issue #12 playtest)
//
// The US-053 death tests call HandlePlayerDeath() directly on a character without an ASC. These drive the real
// wiring: Health on the PlayerState ASC, attached to the pawn after it exists (as ULyraHeroComponent does).

#include "Misc/AutomationTest.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"

using namespace MordecaiRecoveryTest;

namespace MordecaiRecoveryPlayerDeathTest
{
	/** Counts Mordecai.Event.PlayerDeath gameplay events sent to the ASC. */
	struct FDeathEventCounter
	{
		int32 Count = 0;
		FDelegateHandle Handle;
		UAbilitySystemComponent* ASC = nullptr;

		void Start(UAbilitySystemComponent* InASC)
		{
			ASC = InASC;
			Handle = ASC->GenericGameplayEventCallbacks.FindOrAdd(MordecaiGameplayTags::Event_PlayerDeath)
				.AddLambda([this](const FGameplayEventData*) { ++Count; });
		}

		void Stop()
		{
			if (ASC)
			{
				ASC->GenericGameplayEventCallbacks.FindOrAdd(MordecaiGameplayTags::Event_PlayerDeath).Remove(Handle);
			}
			ASC = nullptr;
		}
	};
}

using namespace MordecaiRecoveryPlayerDeathTest;

// ---------------------------------------------------------------------------
// 1. Mordecai.Recovery.Player.DiesWhenHealthReachesZero (AC-053.1, AC-053.2)
// Lethal damage on the PlayerState ASC puts the player in the death state: Dead tag, IsDead, no movement.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Player_DiesWhenHealthReachesZero,
	"Mordecai.Recovery.Player.DiesWhenHealthReachesZero",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Player_DiesWhenHealthReachesZero::RunTest(const FString& Parameters)
{
	FRecoveryTestEnv Env;
	if (!TestTrue("test world with a player set up", Env.Setup(FVector::ZeroVector)))
	{
		Env.Teardown();
		return true;
	}

	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	TestTrue("the player's ASC lives on the PlayerState", ASC && ASC->GetOwnerActor() == Env.PlayerState);
	TestFalse("player starts alive", Env.Player->IsDead());
	Env.Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	FDeathEventCounter Deaths;
	Deaths.Start(ASC);

	// Not lethal: still alive
	Env.ApplyPlayerHealthDelta(-40.f);
	TestEqual("health after a 40 hit", Env.GetPlayerHealth(), 60.f);
	TestFalse("alive at 60 health", Env.Player->IsDead());

	Env.ApplyPlayerHealthDelta(-1000.f);
	TestEqual("health clamps at 0", Env.GetPlayerHealth(), 0.f);
	TestTrue("player is dead at 0 health", Env.Player->IsDead());
	TestTrue("Mordecai.State.Dead on the PlayerState ASC", ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Dead));
	TestEqual("movement is disabled", Env.Player->GetCharacterMovement()->MovementMode.GetValue(), MOVE_None);
	TestEqual("one death event", Deaths.Count, 1);

	// More damage while dead does not die again
	Env.ApplyPlayerHealthDelta(-10.f);
	TestEqual("still one death event", Deaths.Count, 1);

	Deaths.Stop();
	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.Recovery.Player.DiesAgainAfterRespawn (AC-053.5)
// Respawn is in place on the same pawn: the Health binding must survive it, and each death fires once.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Player_DiesAgainAfterRespawn,
	"Mordecai.Recovery.Player.DiesAgainAfterRespawn",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Player_DiesAgainAfterRespawn::RunTest(const FString& Parameters)
{
	FRecoveryTestEnv Env;
	if (!TestTrue("test world with a player set up", Env.Setup(FVector::ZeroVector)))
	{
		Env.Teardown();
		return true;
	}

	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	FDeathEventCounter Deaths;
	Deaths.Start(ASC);

	Env.ApplyPlayerHealthDelta(-1000.f);
	TestTrue("dead after the first lethal hit", Env.Player->IsDead());

	Env.Player->HandlePlayerRespawn();
	TestFalse("alive after respawn", Env.Player->IsDead());
	TestEqual("full health after respawn", Env.GetPlayerHealth(), 100.f);

	Env.ApplyPlayerHealthDelta(-1000.f);
	TestTrue("dead again after the second lethal hit", Env.Player->IsDead());
	TestEqual("one death event per death", Deaths.Count, 2);

	Deaths.Stop();
	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.Recovery.Player.DeathBindingFollowsAbilitySystemInit
// The pawn's ASC can be uninitialized and initialized again (Lyra does it when the avatar changes): the player
// still dies, exactly once, so the binding is neither lost nor doubled.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Player_DeathBindingFollowsAbilitySystemInit,
	"Mordecai.Recovery.Player.DeathBindingFollowsAbilitySystemInit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Player_DeathBindingFollowsAbilitySystemInit::RunTest(const FString& Parameters)
{
	FRecoveryTestEnv Env;
	if (!TestTrue("test world with a player set up", Env.Setup(FVector::ZeroVector)))
	{
		Env.Teardown();
		return true;
	}

	ULyraPawnExtensionComponent* PawnExt = ULyraPawnExtensionComponent::FindPawnExtensionComponent(Env.Player);
	ULyraAbilitySystemComponent* ASC = Env.PlayerState->GetLyraAbilitySystemComponent();
	PawnExt->UninitializeAbilitySystem();
	TestNull("pawn has no ASC after uninit", Env.GetPlayerASC());
	PawnExt->InitializeAbilitySystem(ASC, Env.PlayerState);
	TestTrue("pawn has the PlayerState ASC again", Env.GetPlayerASC() == ASC);

	FDeathEventCounter Deaths;
	Deaths.Start(ASC);
	Env.ApplyPlayerHealthDelta(-1000.f);
	TestTrue("dead after re-init and a lethal hit", Env.Player->IsDead());
	TestEqual("exactly one death event", Deaths.Count, 1);

	Deaths.Stop();
	Env.Teardown();
	return true;
}
