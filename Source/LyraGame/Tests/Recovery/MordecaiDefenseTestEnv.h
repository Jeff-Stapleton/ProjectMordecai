// Project Mordecai — shared behavioral combat fixture for defense/posture runtime tests.

#pragma once

#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilitySpec.h"
#include "TimerManager.h"
#include "UObject/SoftObjectPath.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiGA_Block.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/MordecaiPlayerController.h"
#include "Mordecai/UI/MordecaiDamagePopComponent.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"

namespace MordecaiRecoveryDefenseTest
{
	/** One test frame. Small enough that 250 ms and 1000 ms state windows span several frames. */
	static constexpr float FrameSeconds = 0.05f;

	static const TCHAR* const TestAttackerClassPath =
		TEXT("/MordecaiCore/Enemies/BP_MordecaiEnemy_TestAttacker.BP_MordecaiEnemy_TestAttacker_C");

	inline void SetFalling(ACharacter* Character, bool bFalling)
	{
		if (UCharacterMovementComponent* CMC = Character ? Character->GetCharacterMovement() : nullptr)
		{
			CMC->SetMovementMode(bFalling ? MOVE_Falling : MOVE_Walking);
		}
	}

	inline float GetAttribute(const AActor* Actor, const FGameplayAttribute& Attribute)
	{
		const UAbilitySystemComponent* ASC =
			UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
		return ASC ? ASC->GetNumericAttribute(Attribute) : -1.f;
	}

	inline UMordecaiGA_MeleeAttack* GetMelee(AMordecaiEnemyCharacter* Enemy)
	{
		UAbilitySystemComponent* ASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
		FGameplayAbilitySpec* Spec =
			ASC ? ASC->FindAbilitySpecFromClass(UMordecaiGA_MeleeAttack::StaticClass()) : nullptr;
		return Spec ? Cast<UMordecaiGA_MeleeAttack>(Spec->GetPrimaryInstance()) : nullptr;
	}

	/**
	 * A real player/PlayerState ASC with granted combat abilities, a local damage-pop receiver,
	 * and helpers that advance timers frame-by-frame in an authority game world.
	 */
	struct FDefenseTestEnv : MordecaiRecoveryTest::FRecoveryTestEnv
	{
		AMordecaiPlayerController* GrantPC = nullptr;
		UMordecaiDamagePopComponent* Pops = nullptr;

		bool SetupDefender(
			const FVector& PlayerLocation,
			const TArray<UMordecaiAttackProfileDataAsset*>& PlayerProfiles = {})
		{
			if (!Setup(PlayerLocation))
			{
				return false;
			}
			World->TimeSeconds = 10.0;
			SetFalling(Player, false);

			GrantPC = NewObject<AMordecaiPlayerController>();
			GrantPC->PlayerState = PlayerState;
			GrantPC->DefaultSpellAbilities.Reset();
			GrantPC->MeleeAttackProfiles.Reset();
			for (UMordecaiAttackProfileDataAsset* Profile : PlayerProfiles)
			{
				GrantPC->MeleeAttackProfiles.Add(Profile);
			}
			GrantPC->GrantCombatAbilities();

			APlayerController* PopPC =
				World->SpawnActor<APlayerController>(APlayerController::StaticClass());
			if (!PopPC)
			{
				return false;
			}
			Pops = NewObject<UMordecaiDamagePopComponent>(PopPC);
			Pops->RegisterComponent();
			return World->GetFirstPlayerController() == PopPC
				&& FindSpec(UMordecaiGA_Block::StaticClass()) != nullptr;
		}

		AMordecaiEnemyCharacter* SpawnAttacker(
			UMordecaiAttackProfileDataAsset* Profile) const
		{
			AMordecaiEnemyCharacter* Enemy = SpawnEnemy(FVector::ZeroVector, { Profile });
			SetFalling(Enemy, false);
			PlaceAttackerAtDefenseAngle(Enemy, 0.f);
			return Enemy;
		}

		AMordecaiEnemyCharacter* SpawnTestAttacker() const
		{
			UClass* AttackerClass =
				FSoftClassPath(TestAttackerClassPath).TryLoadClass<AMordecaiEnemyCharacter>();
			AMordecaiEnemyCharacter* Enemy =
				AttackerClass ? SpawnEnemyOfClass(AttackerClass, FVector::ZeroVector) : nullptr;
			SetFalling(Enemy, false);
			PlaceAttackerAtDefenseAngle(Enemy, 0.f);
			return Enemy;
		}

		ULyraAbilitySystemComponent* GetLyraASC() const
		{
			return Cast<ULyraAbilitySystemComponent>(GetPlayerASC());
		}

		FGameplayAbilitySpec* FindSpec(TSubclassOf<UGameplayAbility> AbilityClass) const
		{
			UAbilitySystemComponent* ASC = GetPlayerASC();
			return ASC ? ASC->FindAbilitySpecFromClass(AbilityClass) : nullptr;
		}

		template <typename AbilityT>
		AbilityT* FindInstance() const
		{
			FGameplayAbilitySpec* Spec = FindSpec(AbilityT::StaticClass());
			return Spec ? Cast<AbilityT>(Spec->GetPrimaryInstance()) : nullptr;
		}

		bool IsActive(TSubclassOf<UGameplayAbility> AbilityClass) const
		{
			const FGameplayAbilitySpec* Spec = FindSpec(AbilityClass);
			return Spec && Spec->IsActive();
		}

		void Press(const FGameplayTag& InputTag) const
		{
			ULyraAbilitySystemComponent* ASC = GetLyraASC();
			ASC->AbilityInputTagPressed(InputTag);
			ASC->ProcessAbilityInput(0.f, false);
		}

		void Release(const FGameplayTag& InputTag) const
		{
			ULyraAbilitySystemComponent* ASC = GetLyraASC();
			ASC->AbilityInputTagReleased(InputTag);
			ASC->ProcessAbilityInput(0.f, false);
		}

		float GetPlayer(const FGameplayAttribute& Attribute) const
		{
			return GetAttribute(Player, Attribute);
		}

		bool PlayerHas(const FGameplayTag& Tag) const
		{
			const UAbilitySystemComponent* ASC = GetPlayerASC();
			return ASC && ASC->HasMatchingGameplayTag(Tag);
		}

		void TickFrames(int32 Frames) const
		{
			for (int32 Frame = 0; Frame < Frames; ++Frame)
			{
				++GFrameCounter;
				World->TimeSeconds += FrameSeconds;
				World->GetTimerManager().Tick(FrameSeconds);
			}
		}

		bool LandSwing(AMordecaiEnemyCharacter* Enemy) const
		{
			UAbilitySystemComponent* EnemyASC =
				Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
			const UMordecaiGA_MeleeAttack* Melee = GetMelee(Enemy);
			if (!EnemyASC || !Melee
				|| !EnemyASC->TryActivateAbilityByClass(UMordecaiGA_MeleeAttack::StaticClass()))
			{
				return false;
			}
			for (int32 Frame = 0;
				Frame < 200 && Melee->GetCurrentPhase() == EMordecaiAttackPhase::Windup;
				++Frame)
			{
				TickFrames(1);
			}
			return Melee->GetCurrentPhase() == EMordecaiAttackPhase::Active;
		}

		void FinishSwing(AMordecaiEnemyCharacter* Enemy) const
		{
			UAbilitySystemComponent* EnemyASC =
				Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
			const FGameplayAbilitySpec* Spec = EnemyASC
				? EnemyASC->FindAbilitySpecFromClass(UMordecaiGA_MeleeAttack::StaticClass())
				: nullptr;
			for (int32 Frame = 0; Frame < 200 && Spec && Spec->IsActive(); ++Frame)
			{
				TickFrames(1);
			}
		}

		void PlaceAttackerAtDefenseAngle(
			AMordecaiEnemyCharacter* Enemy,
			float DefenseAngleDegrees) const
		{
			if (!Enemy || !Player)
			{
				return;
			}
			const FVector TowardSource =
				FRotator(0.f, DefenseAngleDegrees, 0.f).RotateVector(FVector::ForwardVector);
			Enemy->SetActorLocation(
				Player->GetActorLocation() + TowardSource * 100.f,
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
			Enemy->SetActorRotation((-TowardSource).Rotation());
		}

		int32 PopCount() const
		{
			return Pops ? Pops->GetActivePopCount() : -1;
		}

		int32 LastPopAmount() const
		{
			return Pops && Pops->GetActivePopCount() > 0
				? Pops->GetActivePops().Last().DamageAmount
				: -1;
		}

		bool LastPopIsCritical() const
		{
			return Pops && Pops->GetActivePopCount() > 0
				&& Pops->GetActivePops().Last().bIsCritical;
		}

		void TeardownDefender()
		{
			if (GrantPC)
			{
				GrantPC->PlayerState = nullptr;
			}
			GrantPC = nullptr;
			Pops = nullptr;
			Teardown();
		}
	};
}
