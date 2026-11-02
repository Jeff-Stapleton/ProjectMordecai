// Project Mordecai — Recovery: the HUD kill counter follows the arena's authoritative count (US-086, issue #6, N1)

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiGameMode.h"
#include "Mordecai/UI/MordecaiCombatHUDWidget.h"
#include "Mordecai/UI/MordecaiKillCounterWidget.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"

namespace MordecaiRecoveryKillCounterTest
{
	FString DrawnKillText(const UMordecaiKillCounterWidget* Counter)
	{
		const UTextBlock* Text = Counter && Counter->WidgetTree
			? Counter->WidgetTree->FindWidget<UTextBlock>(TEXT("KillCountText"))
			: nullptr;
		return Text ? Text->GetText().ToString() : FString();
	}
}

// ---------------------------------------------------------------------------
// 1. Mordecai.Recovery.KillCounter.IncrementsOnPlayerKill (AC-086.3)
// The widget does not keep a second count: each game-mode notification publishes the authoritative value.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_KillCounter_IncrementsOnPlayerKill,
	"Mordecai.Recovery.KillCounter.IncrementsOnPlayerKill",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_KillCounter_IncrementsOnPlayerKill::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryTest;
	using namespace MordecaiRecoveryKillCounterTest;

	FRecoveryControlledTestEnv Env;
	if (!TestTrue("controlled production-style world set up", Env.SetupWithController()))
	{
		Env.TeardownWithController();
		return true;
	}
	AMordecaiGameMode* GameMode = Env.InstallGameMode();
	if (!TestNotNull("production game mode installed", GameMode))
	{
		Env.TeardownWithController();
		return true;
	}

	UMordecaiKillCounterWidget* Counter = Env.MakeWidget<UMordecaiKillCounterWidget>();
	Counter->BindToASC(Env.GetPlayerASC());
	TestEqual("the widget starts from the production BindToASC path", DrawnKillText(Counter), FString(TEXT("Kills: 0")));

	TArray<int32> Broadcasts;
	const FDelegateHandle Handle = GameMode->OnEnemyKillCountChanged().AddLambda(
		[&Broadcasts](int32 Count) { Broadcasts.Add(Count); });

	AMordecaiEnemyCharacter* First = Env.SpawnEnemy(FVector(500.f, 0.f, 100.f), {});
	AMordecaiEnemyCharacter* Second = Env.SpawnEnemy(FVector(700.f, 0.f, 100.f), {});
	TestNotNull("first real enemy", First);
	TestNotNull("second real enemy", Second);
	First->EnterDeathState();
	TestEqual("the game mode records the first player kill", GameMode->GetEnemyKillCount(), 1);
	TestEqual("the widget shows the game mode's first kill", Counter->GetKillCount(), GameMode->GetEnemyKillCount());
	TestEqual("the visible TextBlock shows the first kill", DrawnKillText(Counter), FString(TEXT("Kills: 1")));

	First->EnterDeathState();
	TestEqual("the same real enemy cannot increment through a second death request", GameMode->GetEnemyKillCount(), 1);

	Second->EnterDeathState();
	TestEqual("the game mode records the second player kill", GameMode->GetEnemyKillCount(), 2);
	TestEqual("the widget still shows the one source of truth", Counter->GetKillCount(), GameMode->GetEnemyKillCount());
	TestEqual("two distinct enemies in one frame publish twice", Broadcasts.Num(), 2);
	if (Broadcasts.Num() == 2)
	{
		TestEqual("the first published absolute value", Broadcasts[0], 1);
		TestEqual("the second published absolute value", Broadcasts[1], 2);
	}

	GameMode->OnEnemyKillCountChanged().Remove(Handle);
	Env.TeardownWithController();
	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.Recovery.KillCounter.ResetsOnPlayerDeath (AC-086.3 / AC-063.2)
// AMordecaiCharacter emits Mordecai.Event.PlayerDeath immediately before this game-mode notification.
// The life count and every bound counter reset together; the death screen already cached the completed life's count.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_KillCounter_ResetsOnPlayerDeath,
	"Mordecai.Recovery.KillCounter.ResetsOnPlayerDeath",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_KillCounter_ResetsOnPlayerDeath::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryTest;
	using namespace MordecaiRecoveryKillCounterTest;

	FRecoveryControlledTestEnv Env;
	if (!TestTrue("controlled production-style world set up", Env.SetupWithController()))
	{
		Env.TeardownWithController();
		return true;
	}
	AMordecaiGameMode* GameMode = Env.InstallGameMode();
	if (!TestNotNull("production game mode installed", GameMode))
	{
		Env.TeardownWithController();
		return true;
	}

	UMordecaiKillCounterWidget* Counter = Env.MakeWidget<UMordecaiKillCounterWidget>();
	Counter->BindToASC(Env.GetPlayerASC());
	AMordecaiEnemyCharacter* First = Env.SpawnEnemy(FVector(500.f, 0.f, 100.f), {});
	AMordecaiEnemyCharacter* Second = Env.SpawnEnemy(FVector(700.f, 0.f, 100.f), {});
	TestNotNull("first real enemy", First);
	TestNotNull("second real enemy", Second);
	if (First)
	{
		First->EnterDeathState();
	}
	if (Second)
	{
		Second->EnterDeathState();
	}
	TestEqual("two real enemy deaths are visible before player death", DrawnKillText(Counter), FString(TEXT("Kills: 2")));

	Env.ApplyPlayerHealthDelta(-1000.f);
	TestEqual("player death resets the authoritative life count", GameMode->GetEnemyKillCount(), 0);
	TestEqual("the widget resets from that same notification", Counter->GetKillCount(), GameMode->GetEnemyKillCount());
	TestEqual("the visible TextBlock resets too", DrawnKillText(Counter), FString(TEXT("Kills: 0")));

	Env.TeardownWithController();
	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.Recovery.KillCounter.PostDeathKillDoesNotLeak
// A delayed projectile/DoT enemy death during the respawn window belongs to no new life.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_KillCounter_PostDeathKillDoesNotLeak,
	"Mordecai.Recovery.KillCounter.PostDeathKillDoesNotLeak",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_KillCounter_PostDeathKillDoesNotLeak::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryTest;

	FRecoveryControlledTestEnv Env;
	if (!TestTrue("controlled production-style world set up", Env.SetupWithController()))
	{
		Env.TeardownWithController();
		return true;
	}
	AMordecaiGameMode* GameMode = Env.InstallGameMode();
	if (!TestNotNull("production game mode installed", GameMode))
	{
		Env.TeardownWithController();
		return true;
	}
	GameMode->RespawnDelaySeconds = 0.01f;

	AMordecaiEnemyCharacter* BeforeDeath = Env.SpawnEnemy(FVector(500.f, 0.f, 100.f), {});
	AMordecaiEnemyCharacter* DuringRespawn = Env.SpawnEnemy(FVector(700.f, 0.f, 100.f), {});
	GameMode->RegisterEnemy(BeforeDeath, BeforeDeath->GetActorTransform());
	GameMode->RegisterEnemy(DuringRespawn, DuringRespawn->GetActorTransform());
	BeforeDeath->EnterDeathState();
	TestEqual("completed life has one kill", GameMode->GetEnemyKillCount(), 1);

	Env.ApplyPlayerHealthDelta(-1000.f);
	TestTrue("player entered the respawn window", Env.Player->IsDead());
	TestEqual("death reset the current-life count", GameMode->GetEnemyKillCount(), 0);

	DuringRespawn->EnterDeathState();
	TestEqual("a delayed kill while dead is not credited to the next life", GameMode->GetEnemyKillCount(), 0);

	Env.Player->HandlePlayerRespawn();
	GameMode->ResetArena();
	TestEqual("the count is still zero after respawn", GameMode->GetEnemyKillCount(), 0);

	Env.TeardownWithController();
	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.Recovery.KillCounter.RebindsAfterHUDRebuild
// Slate destruction unbinds the child; reconstructing the same HUD must bind it again.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_KillCounter_RebindsAfterHUDRebuild,
	"Mordecai.Recovery.KillCounter.RebindsAfterHUDRebuild",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_KillCounter_RebindsAfterHUDRebuild::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryTest;
	using namespace MordecaiRecoveryKillCounterTest;

	FRecoveryControlledTestEnv Env;
	if (!TestTrue("controlled production-style world set up", Env.SetupWithController()))
	{
		Env.TeardownWithController();
		return true;
	}
	AMordecaiGameMode* GameMode = Env.InstallGameMode();
	if (!TestNotNull("production game mode installed", GameMode))
	{
		Env.TeardownWithController();
		return true;
	}

	UMordecaiCombatHUDWidget* HUD = Env.MakeWidget<UMordecaiCombatHUDWidget>();
	HUD->BindToASC(Env.GetPlayerASC());
	UMordecaiKillCounterWidget* Counter = HUD->GetKillCounter();
	TestNotNull("HUD built its kill counter", Counter);

	TSharedPtr<SWidget> FirstSlateTree = HUD->TakeWidget();
	FirstSlateTree.Reset();
	TSharedPtr<SWidget> RebuiltSlateTree = HUD->TakeWidget();
	TestTrue("the same HUD was reconstructed", RebuiltSlateTree.IsValid());

	AMordecaiEnemyCharacter* Enemy = Env.SpawnEnemy(FVector(500.f, 0.f, 100.f), {});
	TestNotNull("real enemy", Enemy);
	if (Enemy)
	{
		Enemy->EnterDeathState();
	}
	TestEqual("the rebuilt HUD receives the next authoritative value", Counter->GetKillCount(), 1);
	TestEqual("the rebuilt HUD draws the next authoritative value", DrawnKillText(Counter), FString(TEXT("Kills: 1")));

	RebuiltSlateTree.Reset();
	Env.TeardownWithController();
	return true;
}
