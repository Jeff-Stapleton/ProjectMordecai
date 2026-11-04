// Project Mordecai — Test world for the Epic R recovery tests: players and enemies as the game builds them (US-084)

#pragma once

#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Blueprint/UserWidget.h"
#include "Character/LyraPawnData.h"
#include "Character/LyraPawnExtensionComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameModes/LyraGameState.h"
#include "GameplayEffect.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Enemy/MordecaiEnemyAIController.h"
#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiGameMode.h"
#include "Mordecai/MordecaiPlayerState.h"

namespace MordecaiRecoveryTest
{
	/** A quick arc sweep in front of the attacker: 120 degrees, 250 cm, 10 slash damage, 0.1 s per phase. */
	inline UMordecaiAttackProfileDataAsset* MakeStubProfile()
	{
		UMordecaiAttackProfileDataAsset* Profile = NewObject<UMordecaiAttackProfileDataAsset>();
		Profile->InputSlot = EMordecaiInputSlot::Light;
		Profile->WindupTimeMs = 100.f;
		Profile->ActiveTimeMs = 100.f;
		Profile->RecoveryTimeMs = 100.f;
		Profile->HitShapeType = EMordecaiHitShapeType::ArcSector;
		Profile->HitShapeParams.Radius = 250.f;
		Profile->HitShapeParams.Angle = 120.f;
		Profile->DamageProfile.DamageType = EMordecaiDamageType::Slash;
		Profile->DamageProfile.BasePower = 10.f;
		return Profile;
	}

	/**
	 * A game world with the player as the game builds it: a Mordecai player state owning the ASC (Lyra pattern)
	 * and a Mordecai character whose pawn extension attaches that ASC after the pawn exists (in the game,
	 * ULyraHeroComponent does it from SetupPlayerInputComponent). No Lyra teams are created, as in the Mordecai
	 * experiences, so every player and enemy has FGenericTeamId::NoTeam. The world never begins play and nothing
	 * ticks unless a test does it.
	 *
	 * With bWithGameMode, the world has an authority AMordecaiGameMode, as a map load gives it: enemy deaths reach its
	 * kill count, the player's death starts its respawn timer, and GetAuthGameMode finds it. It never runs InitGame, so
	 * no experience loads.
	 */
	struct FRecoveryTestEnv
	{
		UWorld* World = nullptr;
		AMordecaiPlayerState* PlayerState = nullptr;
		AMordecaiCharacter* Player = nullptr;

		bool Setup(const FVector& PlayerLocation, bool bWithGameMode = false)
		{
			if (!GEngine)
			{
				return false;
			}
			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (!World)
			{
				return false;
			}
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());

			if (bWithGameMode)
			{
				// Spawned as UGameInstance::CreateGameModeForURL spawns it; its PreInitializeComponents spawns its game
				// state. SetGameMode needs a game instance, which this world has none of: CopyGameState makes it the
				// authority. Actors are already initialized, so InitGame never runs and no experience loads.
				FActorSpawnParameters Params = AlwaysSpawn();
				Params.ObjectFlags |= RF_Transient;
				AMordecaiGameMode* GameMode = World->SpawnActor<AMordecaiGameMode>(AMordecaiGameMode::StaticClass(), Params);
				if (!GameMode || !GameMode->GameState)
				{
					return false;
				}
				World->CopyGameState(GameMode, GameMode->GameState.Get());
			}
			else if (!World->SpawnActor<ALyraGameState>(ALyraGameState::StaticClass()))
			{
				return false;
			}
			return SpawnPlayer(PlayerLocation, PlayerState, Player);
		}

		/** The authority game mode (bWithGameMode), or null. */
		AMordecaiGameMode* GetGameMode() const
		{
			return World ? World->GetAuthGameMode<AMordecaiGameMode>() : nullptr;
		}

		/** Another player with its own player state ASC, built the same way (for co-op checks). */
		bool SpawnPlayer(const FVector& Location, AMordecaiPlayerState*& OutPlayerState, AMordecaiCharacter*& OutPlayer) const
		{
			OutPlayerState = World->SpawnActor<AMordecaiPlayerState>(AMordecaiPlayerState::StaticClass());
			OutPlayer = World->SpawnActor<AMordecaiCharacter>(AMordecaiCharacter::StaticClass(), Location, FRotator::ZeroRotator, AlwaysSpawn());
			if (!OutPlayerState || !OutPlayer)
			{
				return false;
			}

			ULyraPawnExtensionComponent* PawnExt = ULyraPawnExtensionComponent::FindPawnExtensionComponent(OutPlayer);
			if (!PawnExt || !OutPlayerState->GetLyraAbilitySystemComponent())
			{
				return false;
			}
			PawnExt->SetPawnData(NewObject<ULyraPawnData>(GetTransientPackage()));
			PawnExt->InitializeAbilitySystem(OutPlayerState->GetLyraAbilitySystemComponent(), OutPlayerState);
			return true;
		}

		/** An enemy spawned like a placed one (its AI controller possesses it) with the melee ability and these profiles. */
		AMordecaiEnemyCharacter* SpawnEnemy(const FVector& Location, const TArray<UMordecaiAttackProfileDataAsset*>& Profiles) const
		{
			AMordecaiEnemyCharacter* Enemy = World->SpawnActorDeferred<AMordecaiEnemyCharacter>(
				AMordecaiEnemyCharacter::StaticClass(), FTransform(FRotator::ZeroRotator, Location), nullptr, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!Enemy)
			{
				return nullptr;
			}
			Enemy->DefaultAbilities.Add(UMordecaiGA_MeleeAttack::StaticClass());
			for (UMordecaiAttackProfileDataAsset* Profile : Profiles)
			{
				Enemy->EnemyAttackProfiles.Add(Profile);
			}
			Enemy->FinishSpawning(FTransform(FRotator::ZeroRotator, Location));

			if (!Enemy->GetController())
			{
				if (AMordecaiEnemyAIController* AI = World->SpawnActor<AMordecaiEnemyAIController>())
				{
					AI->Possess(Enemy);
				}
			}
			Enemy->InitializeAbilitySystem();
			return Enemy;
		}

		/** An enemy Blueprint spawned like a placed one, with the abilities, profiles and AI its class defaults give it. */
		AMordecaiEnemyCharacter* SpawnEnemyOfClass(UClass* EnemyClass, const FVector& Location) const
		{
			AMordecaiEnemyCharacter* Enemy = World->SpawnActor<AMordecaiEnemyCharacter>(
				EnemyClass, Location, FRotator::ZeroRotator, AlwaysSpawn());
			if (!Enemy)
			{
				return nullptr;
			}
			if (!Enemy->GetController())
			{
				UClass* AIClass = Enemy->AIControllerClass ? Enemy->AIControllerClass.Get() : AMordecaiEnemyAIController::StaticClass();
				if (AController* AI = World->SpawnActor<AController>(AIClass))
				{
					AI->Possess(Enemy);
				}
			}
			Enemy->InitializeAbilitySystem();
			return Enemy;
		}

		/**
		 * Begin the enemy's play the way the world would (the test world never begins play). Lyra's pawn extension
		 * needs the game instance's UGameFrameworkComponentManager in BeginPlay and this world has no game instance;
		 * enemies own their ASC and do not use it, so it is unregistered first (BeginPlay skips unregistered components).
		 */
		static void BeginEnemyPlay(AMordecaiEnemyCharacter* Enemy)
		{
			if (ULyraPawnExtensionComponent* PawnExt = ULyraPawnExtensionComponent::FindPawnExtensionComponent(Enemy))
			{
				PawnExt->UnregisterComponent();
			}
			Enemy->DispatchBeginPlay();
		}

		UAbilitySystemComponent* GetPlayerASC() const
		{
			return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Player);
		}

		float GetPlayerHealth() const
		{
			const UAbilitySystemComponent* ASC = GetPlayerASC();
			return ASC ? ASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute()) : -1.f;
		}

		/** Change the player's Mordecai Health with an instant GE, the path melee and projectile damage take. */
		void ApplyPlayerHealthDelta(float Delta) const
		{
			ApplyHealthDelta(GetPlayerASC(), Player, Delta);
		}

		/**
		 * Change Target's Mordecai Health with an instant GE made by Source, the way melee, projectile and spell damage
		 * apply it: the effect context is Source's (instigator = its owner actor, effect causer = its avatar).
		 */
		static void ApplyHealthDelta(UAbilitySystemComponent* Source, AActor* Target, float Delta)
		{
			UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
			if (!Source || !TargetASC)
			{
				return;
			}
			UGameplayEffect* GE = NewObject<UGameplayEffect>(GetTransientPackage());
			GE->DurationPolicy = EGameplayEffectDurationType::Instant;
			FGameplayModifierInfo& Mod = GE->Modifiers.AddDefaulted_GetRef();
			Mod.Attribute = UMordecaiAttributeSet::GetHealthAttribute();
			Mod.ModifierOp = EGameplayModOp::Additive;
			Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Delta));
			Source->ApplyGameplayEffectSpecToTarget(FGameplayEffectSpec(GE, Source->MakeEffectContext(), 1.f), TargetASC);
		}

		void Teardown()
		{
			if (World && GEngine)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
			World = nullptr;
			PlayerState = nullptr;
			Player = nullptr;
		}

		static FActorSpawnParameters AlwaysSpawn()
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			return Params;
		}
	};

	/**
	 * FRecoveryTestEnv's player (pawn with inventory + ledger, player state with the ASC) controlled by a spawned
	 * player controller with a ULocalPlayer, so user widgets get a valid player context and GetOwningPlayer()
	 * resolves the pawn and player state the way the game's widgets do. The ULocalPlayer is a plain one, not a
	 * UCommonLocalPlayer: the controller has no primary game layout.
	 */
	struct FRecoveryControlledTestEnv : FRecoveryTestEnv
	{
		APlayerController* PC = nullptr;
		ULocalPlayer* LocalPlayer = nullptr;

		bool SetupWithController(bool bWithGameMode = false)
		{
			if (!Setup(FVector::ZeroVector, bWithGameMode))
			{
				return false;
			}
			PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass());
			if (!PC)
			{
				return false;
			}
			if (PC->PlayerState && PC->PlayerState != PlayerState)
			{
				// With an auth game mode the controller spawned its own player state (AController::InitPlayerState):
				// the test player's replaces it below
				PC->PlayerState->Destroy();
			}
			LocalPlayer = NewObject<ULocalPlayer>(GEngine, ULocalPlayer::StaticClass());
			PC->Player = LocalPlayer;
			LocalPlayer->PlayerController = PC;
			PC->PlayerState = PlayerState;
			PC->SetPawn(Player);
			return FLocalPlayerContext(LocalPlayer, World).IsValid() && PC->GetPawn() == Player;
		}

		/** CreateWidget's path for a native widget: player context, then Initialize (runs NativeOnInitialized). */
		template <typename WidgetT>
		WidgetT* MakeWidget() const
		{
			WidgetT* Widget = NewObject<WidgetT>(World);
			Widget->SetPlayerContext(FLocalPlayerContext(LocalPlayer, World));
			Widget->Initialize();
			return Widget;
		}

		void TeardownWithController()
		{
			if (PC)
			{
				PC->Player = nullptr;
				PC->PlayerState = nullptr;
			}
			if (LocalPlayer)
			{
				LocalPlayer->PlayerController = nullptr;
			}
			PC = nullptr;
			LocalPlayer = nullptr;
			Teardown();
		}
	};
}
