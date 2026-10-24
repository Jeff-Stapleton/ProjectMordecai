// Project Mordecai — Test environment for the player's combat ability grant (issue #40, US-083)

#pragma once

#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameModes/LyraGameState.h"
#include "GameplayAbilitySpec.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiPlayerController.h"
#include "Mordecai/MordecaiPlayerState.h"

namespace MordecaiCombatGrantTest
{
	/**
	 * A game world with a Lyra game state (Lyra player states require one), a Mordecai player state (it owns
	 * the ASC, as in the game) and a Mordecai player controller using it, so the controller's real grant path
	 * runs. The controller is not spawned: it needs no camera or cheat manager, and it still has authority.
	 */
	struct FMordecaiCombatGrantTestEnv
	{
		UWorld* World = nullptr;
		AMordecaiPlayerState* PlayerState = nullptr;
		AMordecaiPlayerController* PC = nullptr;
		AMordecaiCharacter* Avatar = nullptr;

		bool Setup()
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

			if (!World->SpawnActor<ALyraGameState>(ALyraGameState::StaticClass()))
			{
				return false;
			}
			PlayerState = World->SpawnActor<AMordecaiPlayerState>(AMordecaiPlayerState::StaticClass());
			if (!PlayerState)
			{
				return false;
			}
			PC = NewObject<AMordecaiPlayerController>();
			PC->PlayerState = PlayerState;
			return true;
		}

		/** The player state's ASC, the one the controller grants into. */
		ULyraAbilitySystemComponent* GetASC() const
		{
			return Cast<ULyraAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerState));
		}

		FGameplayAbilitySpec* FindSpec(TSubclassOf<UGameplayAbility> AbilityClass) const
		{
			const ULyraAbilitySystemComponent* ASC = GetASC();
			return ASC ? ASC->FindAbilitySpecFromClass(AbilityClass) : nullptr;
		}

		/** Grant through the controller with its native combat defaults. No spells: their grant loads DA_Spell_* content. */
		void GrantCombatOnly()
		{
			PC->DefaultSpellAbilities.Reset();
			PC->GrantCombatAbilities();
		}

		/** Grant through the controller with only the melee ability and these profiles; returns the melee spec. */
		FGameplayAbilitySpec* GrantMeleeOnly(const TArray<UMordecaiAttackProfileDataAsset*>& Profiles)
		{
			PC->DefaultCombatAbilities.Reset();
			PC->DefaultCombatAbilities.Add(UMordecaiGA_MeleeAttack::StaticClass());
			PC->MeleeAttackProfiles.Reset();
			for (UMordecaiAttackProfileDataAsset* Profile : Profiles)
			{
				PC->MeleeAttackProfiles.Add(Profile);
			}
			GrantCombatOnly();
			return FindSpec(UMordecaiGA_MeleeAttack::StaticClass());
		}

		/**
		 * Give the ASC a pawn avatar, as ULyraPawnExtensionComponent does in the game, so its abilities can activate.
		 * The character is not spawned; nothing ticks.
		 */
		AMordecaiCharacter* AttachAvatar()
		{
			Avatar = NewObject<AMordecaiCharacter>();
			if (ULyraAbilitySystemComponent* ASC = GetASC())
			{
				ASC->InitAbilityActorInfo(PlayerState, Avatar);
			}
			return Avatar;
		}

		/** Press and release go through Lyra's input path, as the hero component's bindings do in the game. */
		void PressInput(const FGameplayTag& InputTag) const
		{
			ULyraAbilitySystemComponent* ASC = GetASC();
			ASC->AbilityInputTagPressed(InputTag);
			ASC->ProcessAbilityInput(0.f, false);
		}

		void ReleaseInput(const FGameplayTag& InputTag) const
		{
			ULyraAbilitySystemComponent* ASC = GetASC();
			ASC->AbilityInputTagReleased(InputTag);
			ASC->ProcessAbilityInput(0.f, false);
		}

		void Teardown()
		{
			if (PC)
			{
				PC->PlayerState = nullptr;
			}
			if (World && GEngine)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
			World = nullptr;
			PlayerState = nullptr;
			PC = nullptr;
			Avatar = nullptr;
		}
	};
}
