// Project Mordecai — Block Gameplay Ability (US-006)

#include "Mordecai/Combat/MordecaiGA_Block.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiGA_Block::UMordecaiGA_Block(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// AC-006.2: Apply Blocking tag while ability is active
	ActivationOwnedTags.AddTag(MordecaiGameplayTags::State_Blocking);

	// Block is blocked during guard broken (AC-006.5)
	ActivationBlockedTags.AddTag(MordecaiGameplayTags::State_GuardBroken);

	// Block is blocked during parry whiff (AC-006.14 — can't guard while whiffing)
	ActivationBlockedTags.AddTag(MordecaiGameplayTags::State_ParryWhiff);

	// Block is blocked during posture broken
	ActivationBlockedTags.AddTag(MordecaiGameplayTags::State_PostureBroken);

	// AC-007.8: Block is blocked during knocked down
	ActivationBlockedTags.AddTag(MordecaiGameplayTags::State_KnockedDown);

	// AC-006.8: Block is NOT blocked by Rooted
	// (intentionally omitted from ActivationBlockedTags)
}

// ---------------------------------------------------------------------------
// Damage Computation (AC-006.3)
// ---------------------------------------------------------------------------

float UMordecaiGA_Block::ComputeBlockedDamage(float IncomingDamage) const
{
	// FinalDamage = IncomingDamage × (1 − Mitigation)
	return IncomingDamage * (1.f - FMath::Clamp(Mitigation, 0.f, 1.f));
}

// ---------------------------------------------------------------------------
// Stamina Drain Computation (AC-006.4)
// ---------------------------------------------------------------------------

float UMordecaiGA_Block::ComputeStaminaDrain(float IncomingDamage) const
{
	// StaminaDrain = IncomingDamage × (1 − Stability)
	return IncomingDamage * (1.f - FMath::Clamp(Stability, 0.f, 1.f));
}

// ---------------------------------------------------------------------------
// Perfect Block Timing (AC-006.9)
// ---------------------------------------------------------------------------

bool UMordecaiGA_Block::IsHitWithinPerfectBlockWindow(double HitTime) const
{
	if (BlockActivationTime <= 0.0) return false;

	double ElapsedMs = (HitTime - BlockActivationTime) * 1000.0;
	return ElapsedMs >= 0.0 && ElapsedMs <= PerfectBlockWindowMs;
}

// ---------------------------------------------------------------------------
// Phase Management
// ---------------------------------------------------------------------------

void UMordecaiGA_Block::TransitionToBlockPhase(EMordecaiBlockPhase NewPhase)
{
	CurrentBlockPhase = NewPhase;
}

void UMordecaiGA_Block::EndPerfectBlockWindow()
{
	bPerfectBlockWindowActive = false;
	RemovePerfectBlockTag();
}

// ---------------------------------------------------------------------------
// Receive Blocked Hit (AC-006.3, AC-006.4, AC-006.5, AC-006.9, AC-006.10)
// ---------------------------------------------------------------------------

float UMordecaiGA_Block::ReceiveBlockedHit(float IncomingDamage, double HitTime, UAbilitySystemComponent* AttackerASC)
{
	if (CurrentBlockPhase != EMordecaiBlockPhase::Active) return IncomingDamage;

	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();

	// Check for perfect block
	bool bIsPerfectBlock = bPerfectBlockWindowActive && IsHitWithinPerfectBlockWindow(HitTime);

	if (bIsPerfectBlock)
	{
		// AC-006.10: Perfect block — no stamina drain
		ApplyPerfectBlockTag();

		// Apply posture damage to attacker
		if (AttackerASC)
		{
			AttackerASC->ApplyModToAttribute(
				UMordecaiAttributeSet::GetPostureAttribute(),
				EGameplayModOp::Additive,
				-PerfectBlockPostureDamage);
		}

		// Compute mitigated damage (still mitigated even on perfect block)
		float FinalDamage = ComputeBlockedDamage(IncomingDamage);

		// Apply health damage
		if (OwnerASC)
		{
			OwnerASC->ApplyModToAttribute(
				UMordecaiAttributeSet::GetHealthAttribute(),
				EGameplayModOp::Additive,
				-FinalDamage);
		}

		return FinalDamage;
	}

	// Normal block
	float FinalDamage = ComputeBlockedDamage(IncomingDamage);
	float StaminaDrain = ComputeStaminaDrain(IncomingDamage);

	// Apply stamina drain
	if (OwnerASC)
	{
		OwnerASC->ApplyModToAttribute(
			UMordecaiAttributeSet::GetStaminaAttribute(),
			EGameplayModOp::Additive,
			-StaminaDrain);

		// Apply health damage
		OwnerASC->ApplyModToAttribute(
			UMordecaiAttributeSet::GetHealthAttribute(),
			EGameplayModOp::Additive,
			-FinalDamage);

		// AC-006.5: Check for guard break (stamina <= 0)
		float CurrentStamina = OwnerASC->GetNumericAttribute(UMordecaiAttributeSet::GetStaminaAttribute());
		if (CurrentStamina <= 0.f)
		{
			TriggerGuardBreak();
		}
	}

	return FinalDamage;
}

// ---------------------------------------------------------------------------
// Tag Accessors
// ---------------------------------------------------------------------------

bool UMordecaiGA_Block::HasActivationBlockedTag(const FGameplayTag& Tag) const
{
	return ActivationBlockedTags.HasTag(Tag);
}

bool UMordecaiGA_Block::HasActivationOwnedTag(const FGameplayTag& Tag) const
{
	return ActivationOwnedTags.HasTag(Tag);
}

// ---------------------------------------------------------------------------
// Ability Activation (AC-006.1, AC-006.2)
// ---------------------------------------------------------------------------

void UMordecaiGA_Block::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	TransitionToBlockPhase(EMordecaiBlockPhase::Active);

	// Record activation time for perfect block window
	if (UWorld* World = GetWorld())
	{
		BlockActivationTime = World->GetTimeSeconds();
	}

	// AC-006.9: Start perfect block window
	bPerfectBlockWindowActive = true;
	ApplyPerfectBlockTag();

	// AC-006.7: Apply movement speed reduction
	ApplyBlockMoveSpeedReduction();

	if (UWorld* World = GetWorld())
	{
		// Timer: end perfect block window
		if (PerfectBlockWindowMs > 0.f)
		{
			World->GetTimerManager().SetTimer(
				PerfectBlockTimerHandle,
				this, &UMordecaiGA_Block::OnPerfectBlockWindowEnd,
				PerfectBlockWindowMs / 1000.f,
				false);
		}
	}
}

void UMordecaiGA_Block::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// Clear timers
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PerfectBlockTimerHandle);
		World->GetTimerManager().ClearTimer(GuardBrokenTimerHandle);
	}

	// Clean up based on phase
	if (CurrentBlockPhase == EMordecaiBlockPhase::Active)
	{
		RemoveBlockMoveSpeedReduction();
	}
	if (bPerfectBlockWindowActive)
	{
		RemovePerfectBlockTag();
		bPerfectBlockWindowActive = false;
	}
	if (CurrentBlockPhase == EMordecaiBlockPhase::GuardBroken)
	{
		RemoveGuardBrokenTag();
	}

	CurrentBlockPhase = EMordecaiBlockPhase::None;
	BlockActivationTime = 0.0;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------
// Phase Callbacks
// ---------------------------------------------------------------------------

void UMordecaiGA_Block::OnGuardBrokenComplete()
{
	RemoveGuardBrokenTag();
	TransitionToBlockPhase(EMordecaiBlockPhase::None);

	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UMordecaiGA_Block::OnPerfectBlockWindowEnd()
{
	EndPerfectBlockWindow();
}

// ---------------------------------------------------------------------------
// Guard Break (AC-006.5)
// ---------------------------------------------------------------------------

void UMordecaiGA_Block::TriggerGuardBreak()
{
	// Remove blocking state
	RemoveBlockMoveSpeedReduction();

	// Transition to guard broken
	TransitionToBlockPhase(EMordecaiBlockPhase::GuardBroken);
	ApplyGuardBrokenTag();

	if (bPerfectBlockWindowActive)
	{
		EndPerfectBlockWindow();
	}

	// Timer: end guard broken after duration
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			GuardBrokenTimerHandle,
			this, &UMordecaiGA_Block::OnGuardBrokenComplete,
			GuardBrokenDurationMs / 1000.f,
			false);
	}
}

// ---------------------------------------------------------------------------
// Tag Management
// ---------------------------------------------------------------------------

void UMordecaiGA_Block::ApplyBlockingTag()
{
	if (!CurrentActorInfo) return;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(MordecaiGameplayTags::State_Blocking);
	}
}

void UMordecaiGA_Block::RemoveBlockingTag()
{
	if (!CurrentActorInfo) return;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::State_Blocking);
	}
}

void UMordecaiGA_Block::ApplyPerfectBlockTag()
{
	if (!CurrentActorInfo) return;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(MordecaiGameplayTags::State_PerfectBlock);
	}
}

void UMordecaiGA_Block::RemovePerfectBlockTag()
{
	if (!CurrentActorInfo) return;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::State_PerfectBlock);
	}
}

void UMordecaiGA_Block::ApplyGuardBrokenTag()
{
	if (!CurrentActorInfo) return;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(MordecaiGameplayTags::State_GuardBroken);
	}
}

void UMordecaiGA_Block::RemoveGuardBrokenTag()
{
	if (!CurrentActorInfo) return;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::State_GuardBroken);
	}
}

// ---------------------------------------------------------------------------
// Movement (AC-006.7)
// ---------------------------------------------------------------------------

void UMordecaiGA_Block::ApplyBlockMoveSpeedReduction()
{
	if (!CurrentActorInfo) return;
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	ACharacter* Character = Cast<ACharacter>(AvatarActor);
	if (!Character) return;

	if (UCharacterMovementComponent* CMC = Character->GetCharacterMovement())
	{
		CMC->MaxWalkSpeed *= BlockMoveSpeedMultiplier;
	}
}

void UMordecaiGA_Block::RemoveBlockMoveSpeedReduction()
{
	if (!CurrentActorInfo) return;
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	ACharacter* Character = Cast<ACharacter>(AvatarActor);
	if (!Character) return;

	if (UCharacterMovementComponent* CMC = Character->GetCharacterMovement())
	{
		if (BlockMoveSpeedMultiplier > KINDA_SMALL_NUMBER)
		{
			CMC->MaxWalkSpeed /= BlockMoveSpeedMultiplier;
		}
	}
}
