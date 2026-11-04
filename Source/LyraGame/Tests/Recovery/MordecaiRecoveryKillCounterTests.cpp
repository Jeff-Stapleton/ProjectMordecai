// Project Mordecai — Recovery: the HUD kill counter counts the player's kills and resets when the player dies (US-086,
// issue #6, N1)
//
// The counter waited for Mordecai.Event.EnemyKill and Mordecai.Event.PlayerDeath tag counts on the player ASC that
// nothing ever changed, so it read "Kills: 0" forever while AMordecaiGameMode counted kills on its own. The game mode's
// count is now the one count: the HUD counter, the death screen and the agent bridge's player.kills all read it.
// Jeff's rules (Specs/design_decisions_log.md, 2026-11-04): only a kill the player causes counts, credited from the
// killing blow's effect context, and not while that player is dead; the player's death ends the life (the death screen
// shows its total, the live count goes back to 0); the respawn does not reset it again; a manual arena reset does.
//
// The world has an authority AMordecaiGameMode, the player kills with its real light attack (granted by the
// controller, pressed through Lyra's input path), and the HUD is the real combat HUD bound the way
// AMordecaiPlayerController::CreateCombatHUD binds it. The tests read what the agent bridge reads: the TextBlocks.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilitySpec.h"
#include "TimerManager.h"
#include "Mordecai/MordecaiGameMode.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/MordecaiPlayerController.h"
#include "Mordecai/UI/MordecaiCombatHUDWidget.h"
#include "Mordecai/UI/MordecaiDeathScreenComponent.h"
#include "Mordecai/UI/MordecaiDeathScreenWidget.h"
#include "Mordecai/UI/MordecaiKillCounterWidget.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"
#include "Tests/Recovery/MordecaiRecoveryTestLayout.h"

namespace MordecaiRecoveryKillCounterTest
{
	using namespace MordecaiRecoveryTest;

	/** One test frame: the 3 s respawn timer is 60 of them. */
	static constexpr float FrameSeconds = 0.05f;

	/** MakeStubProfile's light swing (250 cm, 120 degrees, 0.1 s phases), lethal to a 100-HP enemy in one hit. */
	inline UMordecaiAttackProfileDataAsset* MakeLethalProfile()
	{
		UMordecaiAttackProfileDataAsset* Profile = MakeStubProfile();
		Profile->DamageProfile.BasePower = 1000.f;
		Profile->PostureDamageScalar = 0.f;
		return Profile;
	}

	/** The widget's non-empty TextBlock texts in tree order, as one string: "You Died | 3 | Kills: 1". */
	inline FString DrawnTexts(const UUserWidget* Widget)
	{
		TArray<FString> Texts;
		if (Widget && Widget->WidgetTree)
		{
			Widget->WidgetTree->ForEachWidget([&Texts](UWidget* Child)
			{
				if (const UTextBlock* Text = Cast<UTextBlock>(Child))
				{
					const FString String = Text->GetText().ToString();
					if (!String.IsEmpty())
					{
						Texts.Add(String);
					}
				}
			});
		}
		return FString::Join(Texts, TEXT(" | "));
	}

	/**
	 * A controlled player in a world with an authority AMordecaiGameMode, on the ground, with the light attack the
	 * controller grants (one lethal profile, no spells), the real combat HUD bound to the PlayerState ASC, and the death
	 * screen component pushing to a test layout's UI.Layer.Game. World time starts at 10 s and advances every frame.
	 */
	struct FKillCounterTestEnv : FRecoveryControlledTestEnv
	{
		AMordecaiPlayerController* GrantPC = nullptr;
		UMordecaiCombatHUDWidget* HUD = nullptr;
		UMordecaiKillCounterWidget* Counter = nullptr;
		UCommonActivatableWidgetContainerBase* GameLayer = nullptr;
		UMordecaiDeathScreenComponent* DeathScreen = nullptr;

		bool SetupKillCounter()
		{
			if (!SetupWithController(/*bWithGameMode=*/ true) || !GetGameMode())
			{
				return false;
			}
			World->TimeSeconds = 10.0;
			Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);

			// The light attack as OnPossess grants it (AMordecaiPlayerController::GrantCombatAbilities)
			GrantPC = NewObject<AMordecaiPlayerController>();
			GrantPC->PlayerState = PlayerState;
			GrantPC->DefaultSpellAbilities.Reset();
			GrantPC->MeleeAttackProfiles.Reset();
			GrantPC->MeleeAttackProfiles.Add(MakeLethalProfile());
			GrantPC->GrantCombatAbilities();

			// The HUD as CreateCombatHUD makes and binds it; its kill counter is the one BuildDefaultLayout adds
			HUD = MakeWidget<UMordecaiCombatHUDWidget>();
			HUD->BindToASC(GetPlayerASC());
			if (HUD->WidgetTree)
			{
				HUD->WidgetTree->ForEachWidget([this](UWidget* Child)
				{
					if (UMordecaiKillCounterWidget* Found = Cast<UMordecaiKillCounterWidget>(Child))
					{
						Counter = Found;
					}
				});
			}

			UMordecaiRecoveryTestLayout* Layout = MakeWidget<UMordecaiRecoveryTestLayout>();
			GameLayer = Layout->AddTestLayer(FGameplayTag::RequestGameplayTag(TEXT("UI.Layer.Game")));
			DeathScreen = NewObject<UMordecaiDeathScreenComponent>(PC);
			DeathScreen->RegisterComponent();
			DeathScreen->SetLayoutOverride(Layout);
			DeathScreen->BindToPlayerState();

			const FGameplayAbilitySpec* Melee = GetPlayerASC()->FindAbilitySpecFromClass(UMordecaiGA_MeleeAttack::StaticClass());
			return Melee && Counter && DeathScreen->IsBoundToPlayerState();
		}

		/** The HUD counter's text, as the bridge reads it. */
		FString CounterText() const
		{
			const UTextBlock* Text = Counter && Counter->WidgetTree
				? Counter->WidgetTree->FindWidget<UTextBlock>(TEXT("KillCountText")) : nullptr;
			return Text ? Text->GetText().ToString() : FString(TEXT("<no KillCountText>"));
		}

		/** The one death screen on the Game layer, as text, or "" when there is none. */
		FString DeathScreenText() const
		{
			const TArray<UCommonActivatableWidget*> Widgets = GameLayer ? GameLayer->GetWidgetList() : TArray<UCommonActivatableWidget*>();
			return Widgets.Num() == 1 ? DrawnTexts(Cast<UMordecaiDeathScreenWidget>(Widgets[0])) : FString();
		}

		/** An enemy (100 HP, AI-possessed) on the ground; possessed pawns are Falling in this floorless world. */
		AMordecaiEnemyCharacter* SpawnTarget(const FVector& Location) const
		{
			AMordecaiEnemyCharacter* Enemy = SpawnEnemy(Location, { MakeStubProfile() });
			if (Enemy)
			{
				Enemy->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			}
			return Enemy;
		}

		/** Advance timers and world time one frame at a time (the timer manager ticks once per frame). */
		void TickFrames(int32 Frames) const
		{
			for (int32 Frame = 0; Frame < Frames; ++Frame)
			{
				++GFrameCounter;
				World->TimeSeconds += FrameSeconds;
				World->GetTimerManager().Tick(FrameSeconds);
			}
		}

		/** LMB: press and release through Lyra's input path, then the whole swing (its hit lands as the windup ends). */
		bool LightAttack() const
		{
			ULyraAbilitySystemComponent* ASC = PlayerState->GetLyraAbilitySystemComponent();
			ASC->AbilityInputTagPressed(MordecaiGameplayTags::Input_LightAttack);
			ASC->ProcessAbilityInput(0.f, false);
			ASC->AbilityInputTagReleased(MordecaiGameplayTags::Input_LightAttack);
			ASC->ProcessAbilityInput(0.f, false);
			const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(UMordecaiGA_MeleeAttack::StaticClass());
			if (!Spec || !Spec->IsActive())
			{
				return false;
			}
			for (int32 Frame = 0; Frame < 100 && Spec->IsActive(); ++Frame)
			{
				TickFrames(1);
			}
			return !Spec->IsActive();
		}

		/** The game mode's respawn timer runs out (RespawnDelaySeconds, 3 s). */
		bool WaitForRespawn() const
		{
			for (int32 Frame = 0; Frame < 200 && Player->IsDead(); ++Frame)
			{
				TickFrames(1);
			}
			return !Player->IsDead();
		}

		void TeardownKillCounter()
		{
			if (DeathScreen)
			{
				DeathScreen->Unbind();
			}
			if (GrantPC)
			{
				GrantPC->PlayerState = nullptr;
			}
			GrantPC = nullptr;
			HUD = nullptr;
			Counter = nullptr;
			GameLayer = nullptr;
			DeathScreen = nullptr;
			TeardownWithController();
		}
	};
}

// ---------------------------------------------------------------------------
// 1. Mordecai.Recovery.KillCounter.IncrementsOnPlayerKill (AC-086.3, AC-063.1)
// The player's light attack kills an enemy: the game mode counts it and the HUD counter shows the same number. One
// swing that kills two more counts both.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_KillCounter_IncrementsOnPlayerKill,
	"Mordecai.Recovery.KillCounter.IncrementsOnPlayerKill",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_KillCounter_IncrementsOnPlayerKill::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryKillCounterTest;

	FKillCounterTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownKillCounter(); };
	if (!TestTrue("player with a lethal light attack and a bound HUD, in a world with an auth game mode", Env.SetupKillCounter()))
	{
		return true;
	}
	AMordecaiGameMode* GameMode = Env.GetGameMode();
	TestTrue("the HUD's BindToASC bound the counter to the world's game mode", Env.Counter->GetBoundGameMode() == GameMode);
	TestEqualSensitive("no kills yet", Env.CounterText(), FString(TEXT("Kills: 0")));

	AMordecaiEnemyCharacter* First = Env.SpawnTarget(FVector(100.0, 0.0, 0.0));
	if (!TestNotNull("enemy in front (+X)", First) || !TestTrue("the light attack swings", Env.LightAttack())
		|| !TestTrue("the player's hit killed it", First->IsDead()))
	{
		return true;
	}
	TestEqual("the game mode counted it (player.kills)", GameMode->GetEnemyKillCount(), 1);
	TestEqualSensitive("the HUD counter shows it", Env.CounterText(), FString(TEXT("Kills: 1")));
	TestEqual("GetKillCount", Env.Counter->GetKillCount(), 1);

	AMordecaiEnemyCharacter* Second = Env.SpawnTarget(FVector(150.0, 60.0, 0.0));
	AMordecaiEnemyCharacter* Third = Env.SpawnTarget(FVector(150.0, -60.0, 0.0));
	if (!TestTrue("the second swing", Env.LightAttack())
		|| !TestTrue("it killed both", Second && Third && Second->IsDead() && Third->IsDead()))
	{
		return true;
	}
	TestEqual("the game mode counted three", GameMode->GetEnemyKillCount(), 3);
	TestEqualSensitive("one source of truth: the HUD shows the game mode's count", Env.CounterText(),
		UMordecaiKillCounterWidget::FormatKillCount(GameMode->GetEnemyKillCount()));
	TestEqualSensitive("Kills: 3", Env.CounterText(), FString(TEXT("Kills: 3")));
	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.Recovery.KillCounter.ResetsOnPlayerDeath (AC-086.3, AC-063.2, AC-062.2, AC-053.8)
// The player's death ends the life: the death screen shows that life's kills, the HUD and the game mode go back to 0,
// and stay there through the respawn. The respawn revives the registered enemy, whose next death counts again. Three
// lives (1 kill, 3 kills, none), so a death handled only once, a session total or a stale snapshot fails.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_KillCounter_ResetsOnPlayerDeath,
	"Mordecai.Recovery.KillCounter.ResetsOnPlayerDeath",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_KillCounter_ResetsOnPlayerDeath::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryKillCounterTest;

	FKillCounterTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownKillCounter(); };
	if (!TestTrue("player with a lethal light attack and a bound HUD, in a world with an auth game mode", Env.SetupKillCounter()))
	{
		return true;
	}
	AMordecaiGameMode* GameMode = Env.GetGameMode();

	// Life 1: one kill. The enemy is registered for the arena reset, as its BeginPlay does in the game.
	AMordecaiEnemyCharacter* First = Env.SpawnTarget(FVector(100.0, 0.0, 0.0));
	if (!TestNotNull("an enemy in front (+X)", First))
	{
		return true;
	}
	GameMode->RegisterEnemy(First, First->GetActorTransform());
	if (!TestTrue("life 1: the player's swing kills it", Env.LightAttack() && First->IsDead())
		|| !TestEqualSensitive("life 1: Kills: 1 before dying", Env.CounterText(), FString(TEXT("Kills: 1"))))
	{
		return true;
	}

	// The real death path: a Health GE on the PlayerState ASC -> HandlePlayerDeath
	Env.ApplyPlayerHealthDelta(-1000.f);
	if (!TestTrue("life 1: the player is dead", Env.Player->IsDead()))
	{
		return true;
	}
	TestEqualSensitive("life 1: the death screen shows the life's kills", Env.DeathScreenText(), FString(TEXT("You Died | 3 | Kills: 1")));
	TestEqualSensitive("life 1: the HUD counter is back to 0", Env.CounterText(), FString(TEXT("Kills: 0")));
	TestEqual("life 1: so is the game mode's count (player.kills)", GameMode->GetEnemyKillCount(), 0);

	if (!TestTrue("life 1: the game mode respawns the player", Env.WaitForRespawn()))
	{
		return true;
	}
	TestEqualSensitive("life 1: no death screen after the respawn", Env.DeathScreenText(), FString());
	TestEqualSensitive("life 1: still Kills: 0 after the respawn", Env.CounterText(), FString(TEXT("Kills: 0")));
	TestFalse("life 1: the respawn's arena reset revived the enemy", First->IsDead());

	// Life 2: one swing kills the revived enemy and two more. The death screen shows this life's 3, not the session's 4.
	AMordecaiEnemyCharacter* Second = Env.SpawnTarget(FVector(150.0, 60.0, 0.0));
	AMordecaiEnemyCharacter* Third = Env.SpawnTarget(FVector(150.0, -60.0, 0.0));
	if (!TestTrue("life 2: one swing kills the revived enemy and two more",
			Second && Third && Env.LightAttack() && First->IsDead() && Second->IsDead() && Third->IsDead())
		|| !TestEqualSensitive("life 2: Kills: 3 before dying", Env.CounterText(), FString(TEXT("Kills: 3"))))
	{
		return true;
	}
	Env.ApplyPlayerHealthDelta(-1000.f);
	TestEqualSensitive("life 2: the death screen shows this life's kills", Env.DeathScreenText(), FString(TEXT("You Died | 3 | Kills: 3")));
	TestEqualSensitive("life 2: the HUD counter is back to 0", Env.CounterText(), FString(TEXT("Kills: 0")));
	TestEqual("life 2: so is the game mode's count", GameMode->GetEnemyKillCount(), 0);
	TestTrue("life 2: the game mode respawns the player", Env.WaitForRespawn());
	TestEqualSensitive("life 2: still Kills: 0 after the respawn", Env.CounterText(), FString(TEXT("Kills: 0")));

	// Life 3: no kills. The death screen shows 0, not the last life's 3.
	Env.ApplyPlayerHealthDelta(-1000.f);
	TestEqualSensitive("life 3: the death screen shows no kills", Env.DeathScreenText(), FString(TEXT("You Died | 3 | Kills: 0")));
	TestTrue("life 3: the game mode respawns the player", Env.WaitForRespawn());
	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.Recovery.KillCounter.DeathWindowKillsScoreNothing (AC-086.3; Jeff, 2026-11-04)
// Damage the player made that lands while the player is dead (a projectile or DoT from the life that just ended)
// scores for no one. After the respawn the player's kills count again, and a manual arena reset clears the count.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_KillCounter_DeathWindowKillsScoreNothing,
	"Mordecai.Recovery.KillCounter.DeathWindowKillsScoreNothing",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_KillCounter_DeathWindowKillsScoreNothing::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryKillCounterTest;

	FKillCounterTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownKillCounter(); };
	if (!TestTrue("player with a lethal light attack and a bound HUD, in a world with an auth game mode", Env.SetupKillCounter()))
	{
		return true;
	}
	AMordecaiGameMode* GameMode = Env.GetGameMode();
	AMordecaiEnemyCharacter* DuringDeath = Env.SpawnTarget(FVector(500.0, 0.0, 0.0));
	AMordecaiEnemyCharacter* AfterRespawn = Env.SpawnTarget(FVector(500.0, 400.0, 0.0));
	if (!TestTrue("two enemies out of reach", DuringDeath && AfterRespawn))
	{
		return true;
	}

	Env.ApplyPlayerHealthDelta(-1000.f);
	if (!TestTrue("the player is dead", Env.Player->IsDead()))
	{
		return true;
	}

	// While dead: damage the player made (its PlayerState ASC's effect context) kills an enemy
	FRecoveryTestEnv::ApplyHealthDelta(Env.GetPlayerASC(), DuringDeath, -1000.f);
	if (!TestTrue("the player's damage killed an enemy during the death", DuringDeath->IsDead()))
	{
		return true;
	}
	TestEqual("it scores for no one", GameMode->GetEnemyKillCount(), 0);
	TestEqualSensitive("the HUD counter stays at 0", Env.CounterText(), FString(TEXT("Kills: 0")));
	TestEqualSensitive("the death screen still shows the life that ended", Env.DeathScreenText(), FString(TEXT("You Died | 3 | Kills: 0")));

	if (!TestTrue("the game mode respawns the player", Env.WaitForRespawn()))
	{
		return true;
	}
	TestEqual("still 0 after the respawn", GameMode->GetEnemyKillCount(), 0);

	FRecoveryTestEnv::ApplyHealthDelta(Env.GetPlayerASC(), AfterRespawn, -1000.f);
	TestTrue("alive again, the player kills an enemy", AfterRespawn->IsDead());
	TestEqual("that one counts", GameMode->GetEnemyKillCount(), 1);
	TestEqualSensitive("the HUD shows it", Env.CounterText(), FString(TEXT("Kills: 1")));

	GameMode->ResetArena();
	TestEqual("a manual arena reset clears the count", GameMode->GetEnemyKillCount(), 0);
	TestEqualSensitive("and the HUD counter", Env.CounterText(), FString(TEXT("Kills: 0")));
	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.Recovery.KillCounter.OnlyPlayerKillsCount (AC-086.3; Jeff, 2026-11-04)
// An enemy that dies to anything but the player scores nothing: its own effect, another enemy's, a change with no
// effect context (the environment), a death called directly. A dead enemy counts once.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_KillCounter_OnlyPlayerKillsCount,
	"Mordecai.Recovery.KillCounter.OnlyPlayerKillsCount",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_KillCounter_OnlyPlayerKillsCount::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryKillCounterTest;

	FKillCounterTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownKillCounter(); };
	if (!TestTrue("player with a bound HUD, in a world with an auth game mode", Env.SetupKillCounter()))
	{
		return true;
	}
	AMordecaiGameMode* GameMode = Env.GetGameMode();
	AMordecaiEnemyCharacter* SelfKilled = Env.SpawnTarget(FVector(1000.0, 0.0, 0.0));
	AMordecaiEnemyCharacter* EnemyKilled = Env.SpawnTarget(FVector(1000.0, 400.0, 0.0));
	AMordecaiEnemyCharacter* EnvironmentKilled = Env.SpawnTarget(FVector(1000.0, 800.0, 0.0));
	AMordecaiEnemyCharacter* DirectlyKilled = Env.SpawnTarget(FVector(1000.0, 1200.0, 0.0));
	AMordecaiEnemyCharacter* PlayerKilled = Env.SpawnTarget(FVector(1000.0, 1600.0, 0.0));
	if (!TestTrue("five enemies", SelfKilled && EnemyKilled && EnvironmentKilled && DirectlyKilled && PlayerKilled))
	{
		return true;
	}

	FRecoveryTestEnv::ApplyHealthDelta(SelfKilled->GetAbilitySystemComponent(), SelfKilled, -1000.f);
	TestTrue("its own effect kills it", SelfKilled->IsDead());
	FRecoveryTestEnv::ApplyHealthDelta(SelfKilled->GetAbilitySystemComponent(), EnemyKilled, -1000.f);
	TestTrue("another enemy's effect kills it", EnemyKilled->IsDead());
	EnvironmentKilled->GetAbilitySystemComponent()->SetNumericAttributeBase(UMordecaiAttributeSet::GetHealthAttribute(), 0.f);
	TestTrue("a change with no effect context kills it", EnvironmentKilled->IsDead());
	DirectlyKilled->EnterDeathState();
	TestTrue("a direct death call kills it", DirectlyKilled->IsDead());
	TestEqual("none of them counts", GameMode->GetEnemyKillCount(), 0);
	TestEqualSensitive("the HUD counter stays at 0", Env.CounterText(), FString(TEXT("Kills: 0")));

	FRecoveryTestEnv::ApplyHealthDelta(Env.GetPlayerASC(), PlayerKilled, -1000.f);
	TestTrue("the player's effect kills it", PlayerKilled->IsDead());
	TestEqual("that one counts", GameMode->GetEnemyKillCount(), 1);
	FRecoveryTestEnv::ApplyHealthDelta(Env.GetPlayerASC(), PlayerKilled, -1000.f);
	PlayerKilled->EnterDeathState(Env.PlayerState);
	TestEqual("a dead enemy counts once", GameMode->GetEnemyKillCount(), 1);
	TestEqualSensitive("the HUD counter shows 1", Env.CounterText(), FString(TEXT("Kills: 1")));
	return true;
}
