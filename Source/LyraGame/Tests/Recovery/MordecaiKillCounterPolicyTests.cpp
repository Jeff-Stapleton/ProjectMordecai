// Project Mordecai — kill attribution and life-boundary policy regressions (US-086, issue #6)

#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "Mordecai/AbilitySystem/MordecaiAbilitySystemComponent.h"
#include "Mordecai/Combat/MordecaiGA_Block.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiProjectile.h"
#include "Mordecai/Magic/MordecaiGA_Fireball.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/MordecaiGameMode.h"
#include "Mordecai/MordecaiPlayerController.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Burning.h"
#include "Mordecai/UI/MordecaiDeathScreenComponent.h"
#include "Mordecai/UI/MordecaiDeathScreenWidget.h"
#include "Mordecai/UI/MordecaiKillCounterWidget.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"
#include "Tests/Recovery/MordecaiRecoveryTestLayout.h"

namespace MordecaiKillCounterPolicyTest
{
	using namespace MordecaiRecoveryTest;

	void ApplyHealthEffect(
		AMordecaiEnemyCharacter* Enemy,
		const FGameplayEffectContextHandle& Context,
		EGameplayModOp::Type Operation,
		float Magnitude)
	{
		UAbilitySystemComponent* TargetASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
		if (!TargetASC)
		{
			return;
		}

		UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
		Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
		FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = UMordecaiAttributeSet::GetHealthAttribute();
		Modifier.ModifierOp = Operation;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Magnitude));
		TargetASC->ApplyGameplayEffectSpecToSelf(FGameplayEffectSpec(Effect, Context, 1.f));
	}

	void ApplyPlayerDamage(
		const FRecoveryTestEnv& Env,
		AMordecaiEnemyCharacter* Enemy,
		float Magnitude = -1000.f)
	{
		ApplyHealthEffect(
			Enemy,
			Env.GetPlayerASC()->MakeEffectContext(),
			EGameplayModOp::Additive,
			Magnitude);
	}

	void ConfigureLethalProjectile(AMordecaiProjectile* Projectile)
	{
		FMordecaiDamageProfile Damage;
		Damage.BasePower = 1000.f;
		Projectile->InitFromSpec(FMordecaiProjectileSpec(), Damage);
	}

	FActiveGameplayEffectHandle ApplyBurning(
		UAbilitySystemComponent* SourceASC,
		AMordecaiEnemyCharacter* Target)
	{
		if (!SourceASC || !Target)
		{
			return FActiveGameplayEffectHandle();
		}
		FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(
			UMordecaiGE_Burning::StaticClass(), 1.f, SourceASC->MakeEffectContext());
		return Spec.IsValid()
			? Target->GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*Spec.Data)
			: FActiveGameplayEffectHandle();
	}

	void ExecuteBurningUntilDead(
		AMordecaiEnemyCharacter* Target,
		const FActiveGameplayEffectHandle Handle)
	{
		for (int32 Tick = 0; Tick < 5 && Target && !Target->IsDead(); ++Tick)
		{
			if (UMordecaiAbilitySystemComponent* ASC =
				Cast<UMordecaiAbilitySystemComponent>(Target->GetAbilitySystemComponent()))
			{
				ASC->ExecutePeriodicEffectForTesting(Handle);
			}
		}
	}

	FString DrawnText(const UUserWidget* Widget, const FName Name)
	{
		const UTextBlock* Text = Widget && Widget->WidgetTree
			? Widget->WidgetTree->FindWidget<UTextBlock>(Name)
			: nullptr;
		return Text ? Text->GetText().ToString() : FString();
	}

	UMordecaiAttackProfileDataAsset* MakeLethalMeleeProfile()
	{
		UMordecaiAttackProfileDataAsset* Profile = MakeStubProfile();
		Profile->DamageProfile.BasePower = 1000.f;
		Profile->JumpAvoidable = false;
		return Profile;
	}

	FGameplayAbilitySpecHandle GrantPlayerMelee(
		FRecoveryTestEnv& Env,
		UMordecaiAttackProfileDataAsset* Profile,
		AMordecaiPlayerController*& OutGranter)
	{
		OutGranter = NewObject<AMordecaiPlayerController>();
		OutGranter->PlayerState = Env.PlayerState;
		OutGranter->DefaultSpellAbilities.Reset();
		OutGranter->MeleeAttackProfiles.Reset();
		OutGranter->MeleeAttackProfiles = { Profile };
		OutGranter->GrantCombatAbilities();
		const FGameplayAbilitySpec* Spec =
			Env.GetPlayerASC()->FindAbilitySpecFromClass(UMordecaiGA_MeleeAttack::StaticClass());
		return Spec ? Spec->Handle : FGameplayAbilitySpecHandle();
	}

	void TickTimers(UWorld* World, int32 Frames)
	{
		for (int32 Frame = 0; Frame < Frames; ++Frame)
		{
			++GFrameCounter;
			World->TimeSeconds += 0.05f;
			World->GetTimerManager().Tick(0.05f);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_KillCounter_OldProjectileScoresAfterRespawn,
	"Mordecai.Recovery.KillCounter.OldProjectileScoresAfterRespawn",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_KillCounter_OldProjectileScoresAfterRespawn::RunTest(const FString& Parameters)
{
	using namespace MordecaiKillCounterPolicyTest;

	FRecoveryTestEnv Env;
	if (!TestTrue("production-style world set up", Env.Setup(FVector::ZeroVector)))
	{
		Env.Teardown();
		return true;
	}
	AMordecaiGameMode* GameMode = Env.InstallGameMode();
	GameMode->RespawnDelaySeconds = 30.f;

	AMordecaiProjectile* Projectile = Env.World->SpawnActor<AMordecaiProjectile>(
		AMordecaiProjectile::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator,
		FRecoveryTestEnv::AlwaysSpawn());
	ConfigureLethalProjectile(Projectile);
	Projectile->SetProjectileInstigator(Env.Player);

	Env.ApplyPlayerHealthDelta(-1000.f);
	TestTrue("the player died", Env.Player->IsDead());
	GameMode->ExecuteRespawnForTesting();
	TestFalse("the player respawned", Env.Player->IsDead());

	AMordecaiEnemyCharacter* OldProjectileTarget = Env.SpawnEnemy(FVector(500.f, 0.f, 100.f), {});
	Projectile->SimulateOverlapForTesting(OldProjectileTarget);
	TestTrue("the old-life projectile killed after respawn", OldProjectileTarget->IsDead());
	TestEqual("Jeff's policy credits the old-life projectile to the new life",
		GameMode->GetEnemyKillCount(), 1);

	AMordecaiEnemyCharacter* FreshTarget = Env.SpawnEnemy(FVector(700.f, 0.f, 100.f), {});
	ApplyPlayerDamage(Env, FreshTarget);
	TestTrue("fresh post-respawn damage kills", FreshTarget->IsDead());
	TestEqual("a fresh post-respawn kill increments after the delayed kill",
		GameMode->GetEnemyKillCount(), 2);

	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_KillCounter_BurningScoresDuringDeathWindow,
	"Mordecai.Recovery.KillCounter.BurningScoresDuringDeathWindow",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_KillCounter_BurningScoresDuringDeathWindow::RunTest(const FString& Parameters)
{
	using namespace MordecaiKillCounterPolicyTest;

	FRecoveryControlledTestEnv Env;
	if (!TestTrue("controlled production-style world set up", Env.SetupWithController()))
	{
		Env.TeardownWithController();
		return true;
	}
	AMordecaiGameMode* GameMode = Env.InstallGameMode();
	GameMode->RespawnDelaySeconds = 30.f;

	UMordecaiRecoveryTestLayout* Layout = Env.MakeWidget<UMordecaiRecoveryTestLayout>();
	Layout->AddTestLayer(FGameplayTag::RequestGameplayTag(TEXT("UI.Layer.Game")));
	UMordecaiDeathScreenComponent* DeathComponent = NewObject<UMordecaiDeathScreenComponent>(Env.PC);
	DeathComponent->RegisterComponent();
	DeathComponent->SetLayoutOverride(Layout);
	DeathComponent->BindToPlayerState();

	UMordecaiKillCounterWidget* Counter = Env.MakeWidget<UMordecaiKillCounterWidget>();
	Counter->BindToASC(Env.GetPlayerASC());

	AMordecaiEnemyCharacter* CompletedLifeKill = Env.SpawnEnemy(FVector(500.f, 0.f, 100.f), {});
	CompletedLifeKill->EnterDeathState(Env.PlayerState);
	TestEqual("the completed life starts with one kill", GameMode->GetEnemyKillCount(), 1);

	AMordecaiEnemyCharacter* BurningTarget = Env.SpawnEnemy(FVector(700.f, 0.f, 100.f), {});
	BurningTarget->GetMordecaiAttributeSet()->InitHealth(7.f);
	const FActiveGameplayEffectHandle BurningHandle = ApplyBurning(Env.GetPlayerASC(), BurningTarget);
	TestTrue("player Burning applied", BurningHandle.IsValid());

	Env.ApplyPlayerHealthDelta(-1000.f);
	UMordecaiDeathScreenWidget* DeathScreen = DeathComponent->GetDeathScreenWidget();
	TestNotNull("death screen was shown", DeathScreen);
	TestEqualSensitive("death screen snapshots the completed life",
		DrawnText(DeathScreen, TEXT("KillCountText")), FString(TEXT("Kills: 1")));
	TestEqual("HUD resets at death", Counter->GetKillCount(), 0);

	ExecuteBurningUntilDead(BurningTarget, BurningHandle);
	TestTrue("the player-applied Burning kills during the death window", BurningTarget->IsDead());
	TestEqual("the death-window kill starts the new-life count", GameMode->GetEnemyKillCount(), 1);
	TestEqualSensitive("the completed-life death screen remains unchanged",
		DrawnText(DeathScreen, TEXT("KillCountText")), FString(TEXT("Kills: 1")));

	GameMode->ExecuteRespawnForTesting();
	TestFalse("the player respawned", Env.Player->IsDead());
	TestEqual("ExecuteRespawn preserves the death-window kill", GameMode->GetEnemyKillCount(), 1);
	TestEqual("the HUD shows the death-window kill after respawn", Counter->GetKillCount(), 1);

	DeathComponent->Unbind();
	Env.TeardownWithController();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_KillCounter_ResetArenaRevivedEnemyCountsAgain,
	"Mordecai.Recovery.KillCounter.ResetArenaRevivedEnemyCountsAgain",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_KillCounter_ResetArenaRevivedEnemyCountsAgain::RunTest(const FString& Parameters)
{
	using namespace MordecaiKillCounterPolicyTest;

	FRecoveryTestEnv Env;
	if (!TestTrue("production-style world set up", Env.Setup(FVector::ZeroVector)))
	{
		Env.Teardown();
		return true;
	}
	AMordecaiGameMode* GameMode = Env.InstallGameMode();
	AMordecaiEnemyCharacter* Enemy = Env.SpawnEnemy(FVector(500.f, 0.f, 100.f), {});
	GameMode->RegisterEnemy(Enemy, Enemy->GetActorTransform());

	ApplyPlayerDamage(Env, Enemy);
	TestEqual("first death counts", GameMode->GetEnemyKillCount(), 1);
	GameMode->ResetArena();
	TestEqual("manual ResetArena resets the count", GameMode->GetEnemyKillCount(), 0);
	TestFalse("manual ResetArena revives the enemy", Enemy->IsDead());

	ApplyPlayerDamage(Env, Enemy);
	TestTrue("the revived enemy dies again", Enemy->IsDead());
	TestEqual("clearing the dedupe lets the same revived enemy count again",
		GameMode->GetEnemyKillCount(), 1);

	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_KillCounter_ResetArenaKeepsOutstandingProjectileValid,
	"Mordecai.Recovery.KillCounter.ResetArenaKeepsOutstandingProjectileValid",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_KillCounter_ResetArenaKeepsOutstandingProjectileValid::RunTest(const FString& Parameters)
{
	using namespace MordecaiKillCounterPolicyTest;

	FRecoveryTestEnv Env;
	if (!TestTrue("production-style world set up", Env.Setup(FVector::ZeroVector)))
	{
		Env.Teardown();
		return true;
	}
	AMordecaiGameMode* GameMode = Env.InstallGameMode();
	AMordecaiProjectile* Projectile = Env.World->SpawnActor<AMordecaiProjectile>(
		AMordecaiProjectile::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator,
		FRecoveryTestEnv::AlwaysSpawn());
	ConfigureLethalProjectile(Projectile);
	Projectile->SetProjectileInstigator(Env.Player);

	GameMode->ResetArena();
	AMordecaiEnemyCharacter* Target = Env.SpawnEnemy(FVector(500.f, 0.f, 100.f), {});
	Projectile->SimulateOverlapForTesting(Target);
	TestTrue("the projectile launched before the manual reset still kills", Target->IsDead());
	TestEqual("manual ResetArena does not invalidate an outstanding player projectile",
		GameMode->GetEnemyKillCount(), 1);

	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_KillCounter_PlayerBurningKeepsCreditAfterEnemyRefresh,
	"Mordecai.Recovery.KillCounter.PlayerBurningKeepsCreditAfterEnemyRefresh",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_KillCounter_PlayerBurningKeepsCreditAfterEnemyRefresh::RunTest(const FString& Parameters)
{
	using namespace MordecaiKillCounterPolicyTest;

	FRecoveryTestEnv Env;
	if (!TestTrue("production-style world set up", Env.Setup(FVector::ZeroVector)))
	{
		Env.Teardown();
		return true;
	}
	AMordecaiGameMode* GameMode = Env.InstallGameMode();
	AMordecaiEnemyCharacter* Refresher = Env.SpawnEnemy(FVector(900.f, 0.f, 100.f), {});
	AMordecaiEnemyCharacter* Target = Env.SpawnEnemy(FVector(500.f, 0.f, 100.f), {});
	Target->GetMordecaiAttributeSet()->InitHealth(8.f);

	TestTrue("player Burning applied first", ApplyBurning(Env.GetPlayerASC(), Target).IsValid());
	const FActiveGameplayEffectHandle BurningHandle =
		ApplyBurning(Refresher->GetAbilitySystemComponent(), Target);
	TestTrue("enemy refresh applied", BurningHandle.IsValid());
	ExecuteBurningUntilDead(Target, BurningHandle);

	TestTrue("refreshed Burning killed the target", Target->IsDead());
	TestEqual("the original player applier keeps the credit", GameMode->GetEnemyKillCount(), 1);

	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_KillCounter_EnemyBurningKeepsCreditAfterPlayerRefresh,
	"Mordecai.Recovery.KillCounter.EnemyBurningKeepsCreditAfterPlayerRefresh",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_KillCounter_EnemyBurningKeepsCreditAfterPlayerRefresh::RunTest(const FString& Parameters)
{
	using namespace MordecaiKillCounterPolicyTest;

	FRecoveryTestEnv Env;
	if (!TestTrue("production-style world set up", Env.Setup(FVector::ZeroVector)))
	{
		Env.Teardown();
		return true;
	}
	AMordecaiGameMode* GameMode = Env.InstallGameMode();
	AMordecaiEnemyCharacter* OriginalApplier = Env.SpawnEnemy(FVector(900.f, 0.f, 100.f), {});
	AMordecaiEnemyCharacter* Target = Env.SpawnEnemy(FVector(500.f, 0.f, 100.f), {});
	Target->GetMordecaiAttributeSet()->InitHealth(8.f);

	TestTrue("enemy Burning applied first",
		ApplyBurning(OriginalApplier->GetAbilitySystemComponent(), Target).IsValid());
	const FActiveGameplayEffectHandle BurningHandle = ApplyBurning(Env.GetPlayerASC(), Target);
	TestTrue("player refresh applied", BurningHandle.IsValid());
	ExecuteBurningUntilDead(Target, BurningHandle);

	TestTrue("refreshed Burning killed the target", Target->IsDead());
	TestEqual("the original enemy applier earns no player credit", GameMode->GetEnemyKillCount(), 0);

	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_KillCounter_PlayerOverrideZeroCounts,
	"Mordecai.Recovery.KillCounter.PlayerOverrideZeroCounts",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_KillCounter_PlayerOverrideZeroCounts::RunTest(const FString& Parameters)
{
	using namespace MordecaiKillCounterPolicyTest;

	FRecoveryTestEnv Env;
	if (!TestTrue("production-style world set up", Env.Setup(FVector::ZeroVector)))
	{
		Env.Teardown();
		return true;
	}
	AMordecaiGameMode* GameMode = Env.InstallGameMode();
	AMordecaiEnemyCharacter* Target = Env.SpawnEnemy(FVector(500.f, 0.f, 100.f), {});
	ApplyHealthEffect(
		Target, Env.GetPlayerASC()->MakeEffectContext(), EGameplayModOp::Override, 0.f);

	TestTrue("Override Health=0 killed the target", Target->IsDead());
	TestEqual("player-context Override Health=0 counts", GameMode->GetEnemyKillCount(), 1);

	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_KillCounter_EmptyOverrideClearsPlayerPoke,
	"Mordecai.Recovery.KillCounter.EmptyOverrideClearsPlayerPoke",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_KillCounter_EmptyOverrideClearsPlayerPoke::RunTest(const FString& Parameters)
{
	using namespace MordecaiKillCounterPolicyTest;

	FRecoveryTestEnv Env;
	if (!TestTrue("production-style world set up", Env.Setup(FVector::ZeroVector)))
	{
		Env.Teardown();
		return true;
	}
	AMordecaiGameMode* GameMode = Env.InstallGameMode();
	AMordecaiEnemyCharacter* Target = Env.SpawnEnemy(FVector(500.f, 0.f, 100.f), {});
	ApplyPlayerDamage(Env, Target, -1.f);
	ApplyHealthEffect(Target, FGameplayEffectContextHandle(), EGameplayModOp::Override, 0.f);

	TestTrue("empty-context Override Health=0 killed the target", Target->IsDead());
	TestEqual("the empty lethal override does not inherit the player poke",
		GameMode->GetEnemyKillCount(), 0);

	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_KillCounter_DirectEnvironmentalLethalClearsPlayerPoke,
	"Mordecai.Recovery.KillCounter.DirectEnvironmentalLethalClearsPlayerPoke",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_KillCounter_DirectEnvironmentalLethalClearsPlayerPoke::RunTest(const FString& Parameters)
{
	using namespace MordecaiKillCounterPolicyTest;

	FRecoveryTestEnv Env;
	if (!TestTrue("production-style world set up", Env.Setup(FVector::ZeroVector)))
	{
		Env.Teardown();
		return true;
	}
	AMordecaiGameMode* GameMode = Env.InstallGameMode();
	AMordecaiEnemyCharacter* Target = Env.SpawnEnemy(FVector(500.f, 0.f, 100.f), {});
	ApplyPlayerDamage(Env, Target, -1.f);
	Target->GetAbilitySystemComponent()->ApplyModToAttribute(
		UMordecaiAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, -1000.f);

	TestTrue("direct environmental attribute mod killed the target", Target->IsDead());
	TestEqual("the direct lethal mod does not inherit the player poke",
		GameMode->GetEnemyKillCount(), 0);

	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_KillCounter_BlockingEnemyCreditsPlayerMelee,
	"Mordecai.Recovery.KillCounter.BlockingEnemyCreditsPlayerMelee",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_KillCounter_BlockingEnemyCreditsPlayerMelee::RunTest(const FString& Parameters)
{
	using namespace MordecaiKillCounterPolicyTest;

	FRecoveryTestEnv Env;
	if (!TestTrue("production-style world set up", Env.Setup(FVector::ZeroVector)))
	{
		Env.Teardown();
		return true;
	}
	AMordecaiGameMode* GameMode = Env.InstallGameMode();
	Env.Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	AMordecaiEnemyCharacter* Target = Env.SpawnEnemy(FVector(100.f, 0.f, 0.f), {});
	Target->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
	const FGameplayAbilitySpecHandle BlockHandle = TargetASC->GiveAbility(
		FGameplayAbilitySpec(UMordecaiGA_Block::StaticClass(), 1));
	TestTrue("enemy block activates", TargetASC->TryActivateAbility(BlockHandle));

	AMordecaiPlayerController* Granter = nullptr;
	const FGameplayAbilitySpecHandle MeleeHandle =
		GrantPlayerMelee(Env, MakeLethalMeleeProfile(), Granter);
	TestTrue("the player's real melee ability activates",
		Env.GetPlayerASC()->TryActivateAbility(MeleeHandle));
	TickTimers(Env.World, 20);

	TestTrue("the mitigated real melee hit killed the blocking enemy", Target->IsDead());
	TestEqual("the blocked health path preserves the player source", GameMode->GetEnemyKillCount(), 1);
	if (Granter)
	{
		Granter->PlayerState = nullptr;
	}

	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_KillCounter_ActualMeleeDeliveryCounts,
	"Mordecai.Recovery.KillCounter.ActualMeleeDeliveryCounts",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_KillCounter_ActualMeleeDeliveryCounts::RunTest(const FString& Parameters)
{
	using namespace MordecaiKillCounterPolicyTest;

	FRecoveryTestEnv Env;
	if (!TestTrue("production-style world set up", Env.Setup(FVector::ZeroVector)))
	{
		Env.Teardown();
		return true;
	}
	AMordecaiGameMode* GameMode = Env.InstallGameMode();
	Env.Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	AMordecaiEnemyCharacter* Target = Env.SpawnEnemy(FVector(100.f, 0.f, 0.f), {});
	Target->GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	AMordecaiPlayerController* Granter = nullptr;
	const FGameplayAbilitySpecHandle MeleeHandle =
		GrantPlayerMelee(Env, MakeLethalMeleeProfile(), Granter);
	TestTrue("the player's real melee ability activates",
		Env.GetPlayerASC()->TryActivateAbility(MeleeHandle));
	TickTimers(Env.World, 20);

	TestTrue("the real melee ability killed its query target", Target->IsDead());
	TestEqual("the real melee delivery counts exactly once", GameMode->GetEnemyKillCount(), 1);
	if (Granter)
	{
		Granter->PlayerState = nullptr;
	}

	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_KillCounter_ActualProjectileDeliveryCounts,
	"Mordecai.Recovery.KillCounter.ActualProjectileDeliveryCounts",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_KillCounter_ActualProjectileDeliveryCounts::RunTest(const FString& Parameters)
{
	using namespace MordecaiKillCounterPolicyTest;

	FRecoveryTestEnv Env;
	if (!TestTrue("production-style world set up", Env.Setup(FVector::ZeroVector)))
	{
		Env.Teardown();
		return true;
	}
	AMordecaiGameMode* GameMode = Env.InstallGameMode();
	AMordecaiEnemyCharacter* Target = Env.SpawnEnemy(FVector(500.f, 0.f, 100.f), {});
	AMordecaiProjectile* Projectile = Env.World->SpawnActor<AMordecaiProjectile>(
		AMordecaiProjectile::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator,
		FRecoveryTestEnv::AlwaysSpawn());
	FMordecaiDamageProfile Damage;
	Damage.BasePower = 1000.f;
	Projectile->InitFromSpec(FMordecaiProjectileSpec(), Damage);
	Projectile->SetProjectileInstigator(Env.Player);
	Projectile->SimulateOverlapForTesting(Target);

	TestTrue("the spawned MordecaiProjectile killed its target", Target->IsDead());
	TestEqual("the real projectile delivery counts exactly once", GameMode->GetEnemyKillCount(), 1);

	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_KillCounter_ActualFireballCastCounts,
	"Mordecai.Recovery.KillCounter.ActualFireballCastCounts",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_KillCounter_ActualFireballCastCounts::RunTest(const FString& Parameters)
{
	using namespace MordecaiKillCounterPolicyTest;

	FRecoveryTestEnv Env;
	if (!TestTrue("production-style world set up", Env.Setup(FVector::ZeroVector)))
	{
		Env.Teardown();
		return true;
	}
	AMordecaiGameMode* GameMode = Env.InstallGameMode();
	AMordecaiEnemyCharacter* Target = Env.SpawnEnemy(FVector(500.f, 0.f, 100.f), {});

	UMordecaiSpellDataAsset* Data = NewObject<UMordecaiSpellDataAsset>();
	Data->SpellId = TEXT("Fireball");
	Data->BasePower = 1000.f;
	Data->Range = 1000.f;
	Data->ProjectileSpeed = 1000.f;
	Data->ProjectileLifetime = 3.f;
	Data->ProjectileCollisionRadius = 20.f;
	Data->SpellPointCost = 0;
	Data->WindupTime = 0.f;
	Data->CastTime = 0.f;
	Data->RecoveryTime = 0.f;

	UAbilitySystemComponent* PlayerASC = Env.GetPlayerASC();
	const FGameplayAbilitySpecHandle FireballHandle = PlayerASC->GiveAbility(
		FGameplayAbilitySpec(UMordecaiGA_Fireball::StaticClass(), 1, INDEX_NONE, Data));
	TestTrue("the actual Fireball ability casts", PlayerASC->TryActivateAbility(FireballHandle));

	AMordecaiProjectile* FireballProjectile = nullptr;
	for (TActorIterator<AMordecaiProjectile> It(Env.World); It; ++It)
	{
		FireballProjectile = *It;
	}
	TestNotNull("the Fireball cast spawned a MordecaiProjectile", FireballProjectile);
	if (FireballProjectile)
	{
		FireballProjectile->SimulateOverlapForTesting(Target);
	}

	TestTrue("the cast Fireball projectile killed its target", Target->IsDead());
	TestEqual("the actual Fireball delivery counts exactly once", GameMode->GetEnemyKillCount(), 1);

	Env.Teardown();
	return true;
}
