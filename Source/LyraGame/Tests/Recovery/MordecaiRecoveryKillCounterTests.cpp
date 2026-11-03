// Project Mordecai — Recovery: the HUD kill counter follows the arena's authoritative count (US-086, issue #6, N1)

#include "Misc/AutomationTest.h"
#include "GameplayEffect.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "GameFramework/WorldSettings.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiProjectile.h"
#include "Mordecai/Magic/MordecaiGA_Fireball.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiGameMode.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Burning.h"
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

	void ApplyHealthEffect(
		AMordecaiEnemyCharacter* Enemy,
		const FGameplayEffectContextHandle& Context,
		UObject* SourceObject,
		float HealthDelta = -1000.f)
	{
		UAbilitySystemComponent* TargetASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
		if (!TargetASC)
		{
			return;
		}

		FGameplayEffectContextHandle DamageContext = Context;
		if (SourceObject)
		{
			DamageContext.AddSourceObject(SourceObject);
		}
		UGameplayEffect* DamageGE = NewObject<UGameplayEffect>(
			GetTransientPackage(), NAME_None, RF_Transient);
		DamageGE->DurationPolicy = EGameplayEffectDurationType::Instant;
		FGameplayModifierInfo& Modifier = DamageGE->Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = UMordecaiAttributeSet::GetHealthAttribute();
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(HealthDelta));
		TargetASC->ApplyGameplayEffectSpecToSelf(FGameplayEffectSpec(DamageGE, DamageContext, 1.f));
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
	First->EnterDeathState(Env.PlayerState, GameMode->GetCurrentPlayerLifeId(Env.PlayerState));
	TestEqual("the game mode records the first player kill", GameMode->GetEnemyKillCount(), 1);
	TestEqual("the widget shows the game mode's first kill", Counter->GetKillCount(), GameMode->GetEnemyKillCount());
	TestEqual("the visible TextBlock shows the first kill", DrawnKillText(Counter), FString(TEXT("Kills: 1")));

	First->EnterDeathState(Env.PlayerState, GameMode->GetCurrentPlayerLifeId(Env.PlayerState));
	TestEqual("the same real enemy cannot increment through a second death request", GameMode->GetEnemyKillCount(), 1);

	Second->EnterDeathState(Env.PlayerState, GameMode->GetCurrentPlayerLifeId(Env.PlayerState));
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
		First->EnterDeathState(Env.PlayerState, GameMode->GetCurrentPlayerLifeId(Env.PlayerState));
	}
	if (Second)
	{
		Second->EnterDeathState(Env.PlayerState, GameMode->GetCurrentPlayerLifeId(Env.PlayerState));
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
	GameMode->RespawnDelaySeconds = 0.01f;
	const FGameplayEffectContextHandle OldLifeContext = Env.GetPlayerASC()->MakeEffectContext();
	AMordecaiProjectile* OldLifeProjectile = Env.World->SpawnActor<AMordecaiProjectile>(
		AMordecaiProjectile::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator,
		FRecoveryTestEnv::AlwaysSpawn());
	OldLifeProjectile->SetProjectileInstigator(Env.Player);

	AMordecaiEnemyCharacter* BeforeDeath = Env.SpawnEnemy(FVector(500.f, 0.f, 100.f), {});
	AMordecaiEnemyCharacter* DuringRespawn = Env.SpawnEnemy(FVector(700.f, 0.f, 100.f), {});
	GameMode->RegisterEnemy(BeforeDeath, BeforeDeath->GetActorTransform());
	GameMode->RegisterEnemy(DuringRespawn, DuringRespawn->GetActorTransform());
	BeforeDeath->EnterDeathState(Env.PlayerState, GameMode->GetCurrentPlayerLifeId(Env.PlayerState));
	TestEqual("completed life has one kill", GameMode->GetEnemyKillCount(), 1);

	Env.ApplyPlayerHealthDelta(-1000.f);
	TestTrue("player entered the respawn window", Env.Player->IsDead());
	TestEqual("death reset the current-life count", GameMode->GetEnemyKillCount(), 0);

	DuringRespawn->EnterDeathState(Env.PlayerState, GameMode->GetCurrentPlayerLifeId(Env.PlayerState));
	TestEqual("a delayed kill while dead is not credited to the next life", GameMode->GetEnemyKillCount(), 0);

	GameMode->ExecuteRespawnForTesting();
	TestFalse("player completed the respawn", Env.Player->IsDead());
	TestEqual("the count is still zero after respawn", GameMode->GetEnemyKillCount(), 0);

	AMordecaiEnemyCharacter* AfterRespawn = Env.SpawnEnemy(FVector(900.f, 0.f, 100.f), {});
	ApplyHealthEffect(
		AfterRespawn, OldLifeContext, NewObject<UMordecaiGA_MeleeAttack>());
	TestEqual("an old-life damage context that kills after respawn is rejected",
		GameMode->GetEnemyKillCount(), 0);

	AMordecaiEnemyCharacter* ProjectileTarget = Env.SpawnEnemy(FVector(1100.f, 0.f, 100.f), {});
	OldLifeProjectile->ApplyDamageToTargetForTesting(ProjectileTarget, 1000.f);
	TestEqual("a projectile launched in the old life cannot score after respawn",
		GameMode->GetEnemyKillCount(), 0);

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
	TestEqual("initial HUD setup performs one child bind pass", HUD->GetChildBindPassCount(), 1);
	UMordecaiKillCounterWidget* Counter = HUD->GetKillCounter();
	TestNotNull("HUD built its kill counter", Counter);

	TSharedPtr<SWidget> FirstSlateTree = HUD->TakeWidget();
	FirstSlateTree.Reset();
	TSharedPtr<SWidget> RebuiltSlateTree = HUD->TakeWidget();
	TestTrue("the same HUD was reconstructed", RebuiltSlateTree.IsValid());
	// CreateCombatHUD calls BindToASC again after AddToViewport/NativeConstruct.
	HUD->BindToASC(Env.GetPlayerASC());
	TestEqual("reconstruct rebinds once and the controller's follow-up bind is idempotent",
		HUD->GetChildBindPassCount(), 2);

	AMordecaiEnemyCharacter* Enemy = Env.SpawnEnemy(FVector(500.f, 0.f, 100.f), {});
	TestNotNull("real enemy", Enemy);
	if (Enemy)
	{
		Enemy->EnterDeathState(Env.PlayerState, GameMode->GetCurrentPlayerLifeId(Env.PlayerState));
	}
	TestEqual("the rebuilt HUD receives the next authoritative value", Counter->GetKillCount(), 1);
	TestEqual("the rebuilt HUD draws the next authoritative value", DrawnKillText(Counter), FString(TEXT("Kills: 1")));

	RebuiltSlateTree.Reset();
	Env.TeardownWithController();
	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.Recovery.KillCounter.CountsPlayerDamageSources
// The player identity in the GAS context survives melee, projectile, spell and periodic Burning delivery.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_KillCounter_CountsPlayerDamageSources,
	"Mordecai.Recovery.KillCounter.CountsPlayerDamageSources",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_KillCounter_CountsPlayerDamageSources::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryTest;
	using namespace MordecaiRecoveryKillCounterTest;

	FRecoveryTestEnv Env;
	if (!TestTrue("production-style world set up", Env.Setup(FVector::ZeroVector)))
	{
		Env.Teardown();
		return true;
	}
	AMordecaiGameMode* GameMode = Env.InstallGameMode();
	UAbilitySystemComponent* PlayerASC = Env.GetPlayerASC();
	if (!TestNotNull("production game mode installed", GameMode)
		|| !TestNotNull("player ASC", PlayerASC))
	{
		Env.Teardown();
		return true;
	}

	const FGameplayEffectContextHandle PlayerContext = PlayerASC->MakeEffectContext();

	AMordecaiEnemyCharacter* MeleeTarget = Env.SpawnEnemy(FVector(500.f, 0.f, 100.f), {});
	ApplyHealthEffect(MeleeTarget, PlayerContext, NewObject<UMordecaiGA_MeleeAttack>());
	TestEqual("lethal player melee adds exactly one", GameMode->GetEnemyKillCount(), 1);

	AMordecaiEnemyCharacter* ProjectileTarget = Env.SpawnEnemy(FVector(700.f, 0.f, 100.f), {});
	AMordecaiProjectile* Projectile = Env.World->SpawnActor<AMordecaiProjectile>(
		AMordecaiProjectile::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator,
		FRecoveryTestEnv::AlwaysSpawn());
	ApplyHealthEffect(ProjectileTarget, PlayerContext, Projectile);
	TestEqual("lethal player projectile adds exactly one", GameMode->GetEnemyKillCount(), 2);

	AMordecaiEnemyCharacter* SpellTarget = Env.SpawnEnemy(FVector(900.f, 0.f, 100.f), {});
	ApplyHealthEffect(SpellTarget, PlayerContext, NewObject<UMordecaiGA_Fireball>());
	TestEqual("lethal player spell adds exactly one", GameMode->GetEnemyKillCount(), 3);

	AMordecaiEnemyCharacter* BurningTarget = Env.SpawnEnemy(FVector(1100.f, 0.f, 100.f), {});
	BurningTarget->GetMordecaiAttributeSet()->InitHealth(3.f);
	FGameplayEffectSpecHandle BurningSpec = PlayerASC->MakeOutgoingSpec(
		UMordecaiGE_Burning::StaticClass(), 1.f, PlayerContext);
	if (TestTrue("player Burning spec created", BurningSpec.IsValid()))
	{
		BurningTarget->GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*BurningSpec.Data);
		for (int32 Tick = 0; Tick < 4 && !BurningTarget->IsDead(); ++Tick)
		{
			Env.World->Tick(LEVELTICK_All, 1.1f);
		}
	}
	TestTrue("Burning killed its target", BurningTarget->IsDead());
	TestEqual("lethal player Burning adds exactly one", GameMode->GetEnemyKillCount(), 4);

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.Recovery.KillCounter.RejectsNonPlayerDeaths
// Empty/environmental, target-self and another-enemy GAS contexts must not grant a player kill.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_KillCounter_RejectsNonPlayerDeaths,
	"Mordecai.Recovery.KillCounter.RejectsNonPlayerDeaths",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_KillCounter_RejectsNonPlayerDeaths::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryTest;
	using namespace MordecaiRecoveryKillCounterTest;

	FRecoveryTestEnv Env;
	if (!TestTrue("production-style world set up", Env.Setup(FVector::ZeroVector)))
	{
		Env.Teardown();
		return true;
	}
	AMordecaiGameMode* GameMode = Env.InstallGameMode();
	if (!TestNotNull("production game mode installed", GameMode))
	{
		Env.Teardown();
		return true;
	}

	AMordecaiEnemyCharacter* Environmental = Env.SpawnEnemy(FVector(500.f, 0.f, 100.f), {});
	ApplyHealthEffect(Environmental, FGameplayEffectContextHandle(), nullptr);
	TestEqual("environmental death grants no player kill", GameMode->GetEnemyKillCount(), 0);

	AMordecaiEnemyCharacter* Self = Env.SpawnEnemy(FVector(700.f, 0.f, 100.f), {});
	ApplyHealthEffect(Self, Self->GetAbilitySystemComponent()->MakeEffectContext(), Self);
	TestEqual("self-caused death grants no player kill", GameMode->GetEnemyKillCount(), 0);

	AMordecaiEnemyCharacter* EnemySource = Env.SpawnEnemy(FVector(900.f, 0.f, 100.f), {});
	AMordecaiEnemyCharacter* EnemyTarget = Env.SpawnEnemy(FVector(1100.f, 0.f, 100.f), {});
	ApplyHealthEffect(
		EnemyTarget, EnemySource->GetAbilitySystemComponent()->MakeEffectContext(), EnemySource);
	TestEqual("enemy friendly-fire death grants no player kill", GameMode->GetEnemyKillCount(), 0);

	AMordecaiEnemyCharacter* MixedSource = Env.SpawnEnemy(FVector(1300.f, 0.f, 100.f), {});
	ApplyHealthEffect(
		MixedSource, Env.GetPlayerASC()->MakeEffectContext(), NewObject<UMordecaiGA_MeleeAttack>(), -1.f);
	ApplyHealthEffect(MixedSource, FGameplayEffectContextHandle(), nullptr);
	TestEqual("environmental finishing damage clears an earlier player source",
		GameMode->GetEnemyKillCount(), 0);

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// 7. Mordecai.Recovery.KillCounter.CadenceFrameCapSupportsDistinctTaps
// A light tap needs one down frame and one released frame; together they must fit the story's 0.4 s start bound.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_KillCounter_CadenceFrameCapSupportsDistinctTaps,
	"Mordecai.Recovery.KillCounter.CadenceFrameCapSupportsDistinctTaps",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_KillCounter_CadenceFrameCapSupportsDistinctTaps::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryTest;

	FRecoveryTestEnv Env;
	if (!TestTrue("production-style world set up", Env.Setup(FVector::ZeroVector)))
	{
		Env.Teardown();
		return true;
	}
	AMordecaiGameMode* GameMode = Env.InstallGameMode();
	if (TestNotNull("production game mode installed", GameMode))
	{
		// This synthetic world installs its GameMode after InitializeActorsForPlay, so invoke the production hook.
		FString Error;
		GameMode->InitGame(TEXT("RecoveryTest"), FString(), Error);
	}
	const AWorldSettings* Settings = Env.World->GetWorldSettings();
	if (TestNotNull("world settings", Settings))
	{
		TestTrue("two worst-case input frames fit within 0.4 game seconds",
			Settings->MaxUndilatedFrameTime * 2.f <= 0.4f + KINDA_SMALL_NUMBER);
	}

	Env.Teardown();
	return true;
}
