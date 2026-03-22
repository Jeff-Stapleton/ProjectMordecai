// Project Mordecai — Dodge Gameplay Ability (US-005)

#include "Mordecai/Combat/MordecaiGA_Dodge.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiGA_Dodge::UMordecaiGA_Dodge(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// Prevent dodge while already dodging
	ActivationBlockedTags.AddTag(MordecaiGameplayTags::State_Dodging);

	// AC-005.10: Block dodge during cooldown
	ActivationBlockedTags.AddTag(MordecaiGameplayTags::State_DodgeCooldown);

	// AC-005.11: Block dodge during posture broken (staggered)
	ActivationBlockedTags.AddTag(MordecaiGameplayTags::State_PostureBroken);

	// AC-007.8: Block dodge during knocked down
	ActivationBlockedTags.AddTag(MordecaiGameplayTags::State_KnockedDown);

	// AC-005.12: Dodge is NOT blocked by Rooted
	// (intentionally omitted from ActivationBlockedTags)
}

// ---------------------------------------------------------------------------
// Direction Computation (AC-005.4)
// ---------------------------------------------------------------------------

FVector UMordecaiGA_Dodge::ComputeDodgeDirection(const FVector& MovementInput, const FVector& CharacterForward)
{
	// If there's meaningful movement input, dodge in that direction
	if (MovementInput.SizeSquared2D() > KINDA_SMALL_NUMBER)
	{
		FVector Dir = MovementInput;
		Dir.Z = 0.f;
		return Dir.GetSafeNormal();
	}

	// No input: dodge backward (opposite of character facing)
	FVector Backward = -CharacterForward;
	Backward.Z = 0.f;
	return Backward.GetSafeNormal();
}

// ---------------------------------------------------------------------------
// Impulse Computation (AC-005.5)
// ---------------------------------------------------------------------------

FVector UMordecaiGA_Dodge::ComputeImpulseVelocity(const FVector& Direction) const
{
	if (DodgeDurationMs <= 0.f) return FVector::ZeroVector;

	float Speed = DodgeDistanceCm / (DodgeDurationMs / 1000.f);
	return Direction.GetSafeNormal() * Speed;
}

// ---------------------------------------------------------------------------
// Phase Management
// ---------------------------------------------------------------------------

void UMordecaiGA_Dodge::TransitionToDodgePhase(EMordecaiDodgePhase NewPhase)
{
	CurrentDodgePhase = NewPhase;
}

void UMordecaiGA_Dodge::EndPerfectDodgeWindow()
{
	bPerfectDodgeActive = false;
	RemovePerfectDodgeTag();
}

// ---------------------------------------------------------------------------
// Tag Accessors
// ---------------------------------------------------------------------------

bool UMordecaiGA_Dodge::HasActivationBlockedTag(const FGameplayTag& Tag) const
{
	return ActivationBlockedTags.HasTag(Tag);
}

// ---------------------------------------------------------------------------
// Ability Activation (AC-005.1, AC-005.2)
// ---------------------------------------------------------------------------

void UMordecaiGA_Dodge::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// AC-005.6: Consume stamina on activation (even if stamina is at 0)
	ApplyStaminaCost();

	// AC-005.4: Compute dodge direction from movement input at activation time
	FVector MoveInput = FVector::ZeroVector;
	FVector Forward = FVector::ForwardVector;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (ACharacter* Character = Cast<ACharacter>(AvatarActor))
	{
		Forward = Character->GetActorForwardVector();
		if (UCharacterMovementComponent* CMC = Character->GetCharacterMovement())
		{
			MoveInput = CMC->GetLastInputVector();
		}
	}

	// AC-005.13: Direction is locked at activation — no steering mid-dodge
	DodgeDirection = ComputeDodgeDirection(MoveInput, Forward);

	// AC-005.2: Apply Dodging tag for i-frames
	TransitionToDodgePhase(EMordecaiDodgePhase::Active);
	ApplyDodgingTag();

	// AC-005.8: Apply PerfectDodge tag for the timing window
	bPerfectDodgeActive = true;
	ApplyPerfectDodgeTag();

	// AC-005.5: Apply movement impulse in dodge direction
	ApplyDodgeMovement();

	if (UWorld* World = GetWorld())
	{
		// Timer: end perfect dodge window after PerfectDodgeWindowMs
		if (PerfectDodgeWindowMs > 0.f)
		{
			World->GetTimerManager().SetTimer(
				PerfectDodgeTimerHandle,
				this, &UMordecaiGA_Dodge::OnPerfectDodgeWindowEnd,
				PerfectDodgeWindowMs / 1000.f,
				false);
		}

		// Timer: end dodge active phase after DodgeDurationMs
		World->GetTimerManager().SetTimer(
			DodgeTimerHandle,
			this, &UMordecaiGA_Dodge::OnDodgeComplete,
			DodgeDurationMs / 1000.f,
			false);
	}
}

void UMordecaiGA_Dodge::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// Clear all timers
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DodgeTimerHandle);
		World->GetTimerManager().ClearTimer(PerfectDodgeTimerHandle);
		World->GetTimerManager().ClearTimer(CooldownTimerHandle);
	}

	// Clean up tags based on current phase
	if (CurrentDodgePhase == EMordecaiDodgePhase::Active)
	{
		RemoveDodgingTag();
		StopDodgeMovement();
	}
	if (bPerfectDodgeActive)
	{
		RemovePerfectDodgeTag();
		bPerfectDodgeActive = false;
	}
	if (CurrentDodgePhase == EMordecaiDodgePhase::Cooldown)
	{
		RemoveDodgeCooldownTag();
	}

	CurrentDodgePhase = EMordecaiDodgePhase::None;
	DodgeDirection = FVector::ZeroVector;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------
// Phase Callbacks
// ---------------------------------------------------------------------------

void UMordecaiGA_Dodge::OnDodgeComplete()
{
	// End dodge active phase: remove i-frames, stop movement
	RemoveDodgingTag();
	StopDodgeMovement();

	if (bPerfectDodgeActive)
	{
		EndPerfectDodgeWindow();
	}

	// AC-005.10: Start cooldown period
	TransitionToDodgePhase(EMordecaiDodgePhase::Cooldown);
	ApplyDodgeCooldownTag();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			CooldownTimerHandle,
			this, &UMordecaiGA_Dodge::OnCooldownComplete,
			DodgeCooldownMs / 1000.f,
			false);
	}
}

void UMordecaiGA_Dodge::OnPerfectDodgeWindowEnd()
{
	EndPerfectDodgeWindow();
}

void UMordecaiGA_Dodge::OnCooldownComplete()
{
	RemoveDodgeCooldownTag();
	TransitionToDodgePhase(EMordecaiDodgePhase::None);

	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

// ---------------------------------------------------------------------------
// Stamina (AC-005.6)
// ---------------------------------------------------------------------------

void UMordecaiGA_Dodge::ApplyStaminaCost()
{
	if (StaminaCost <= 0.f) return;
	if (!CurrentActorInfo) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	// Consume stamina — dodge always fires regardless of current stamina.
	// Per stamina tier rules, stamina may go to 0 (clamped by attribute set).
	ASC->ApplyModToAttribute(UMordecaiAttributeSet::GetStaminaAttribute(), EGameplayModOp::Additive, -StaminaCost);
}

// ---------------------------------------------------------------------------
// Movement (AC-005.5, AC-005.13)
// ---------------------------------------------------------------------------

void UMordecaiGA_Dodge::ApplyDodgeMovement()
{
	if (!CurrentActorInfo) return;
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	ACharacter* Character = Cast<ACharacter>(AvatarActor);
	if (!Character) return;

	FVector Velocity = ComputeImpulseVelocity(DodgeDirection);

	// Use LaunchCharacter to apply dodge velocity.
	// bXYOverride=true locks to dodge direction (AC-005.13: no steering).
	Character->LaunchCharacter(Velocity, true, false);
}

void UMordecaiGA_Dodge::StopDodgeMovement()
{
	if (!CurrentActorInfo) return;
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	ACharacter* Character = Cast<ACharacter>(AvatarActor);
	if (!Character) return;

	if (UCharacterMovementComponent* CMC = Character->GetCharacterMovement())
	{
		// Zero out velocity to stop dodge momentum
		CMC->StopMovementImmediately();
	}
}

// ---------------------------------------------------------------------------
// Tag Management
// ---------------------------------------------------------------------------

void UMordecaiGA_Dodge::ApplyDodgingTag()
{
	if (!CurrentActorInfo) return;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(MordecaiGameplayTags::State_Dodging);
	}
}

void UMordecaiGA_Dodge::RemoveDodgingTag()
{
	if (!CurrentActorInfo) return;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::State_Dodging);
	}
}

void UMordecaiGA_Dodge::ApplyPerfectDodgeTag()
{
	if (!CurrentActorInfo) return;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(MordecaiGameplayTags::State_PerfectDodge);
	}
}

void UMordecaiGA_Dodge::RemovePerfectDodgeTag()
{
	if (!CurrentActorInfo) return;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::State_PerfectDodge);
	}
}

void UMordecaiGA_Dodge::ApplyDodgeCooldownTag()
{
	if (!CurrentActorInfo) return;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(MordecaiGameplayTags::State_DodgeCooldown);
	}
}

void UMordecaiGA_Dodge::RemoveDodgeCooldownTag()
{
	if (!CurrentActorInfo) return;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::State_DodgeCooldown);
	}
}
