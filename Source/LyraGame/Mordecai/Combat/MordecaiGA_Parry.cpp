// Project Mordecai — Parry Gameplay Ability (US-006)

#include "Mordecai/Combat/MordecaiGA_Parry.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "TimerManager.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiGA_Parry::UMordecaiGA_Parry(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// AC-006.16: Cannot parry while PostureBroken or GuardBroken
	ActivationBlockedTags.AddTag(MordecaiGameplayTags::State_PostureBroken);
	ActivationBlockedTags.AddTag(MordecaiGameplayTags::State_GuardBroken);

	// AC-007.8: Cannot parry while knocked down
	ActivationBlockedTags.AddTag(MordecaiGameplayTags::State_KnockedDown);

	// Cannot parry while already in whiff recovery
	ActivationBlockedTags.AddTag(MordecaiGameplayTags::State_ParryWhiff);

	// AC-006.17/AC-006.18: Parry is NOT blocked by Blocking tag.
	// Parry cancels an active block (handled by the input system / controller).
	// (intentionally omitted from ActivationBlockedTags)
}

// ---------------------------------------------------------------------------
// Phase Management
// ---------------------------------------------------------------------------

void UMordecaiGA_Parry::TransitionToParryPhase(EMordecaiParryPhase NewPhase)
{
	CurrentParryPhase = NewPhase;
}

// ---------------------------------------------------------------------------
// Receive Parried Hit (AC-006.13)
// ---------------------------------------------------------------------------

bool UMordecaiGA_Parry::ReceiveParriedHit(UAbilitySystemComponent* AttackerASC)
{
	if (CurrentParryPhase != EMordecaiParryPhase::Active) return false;

	bParrySuccessful = true;

	// Apply Parried tag to attacker (stagger)
	if (AttackerASC)
	{
		ApplyParriedTagToAttacker(AttackerASC);

		// Apply posture damage to attacker
		AttackerASC->ApplyModToAttribute(
			UMordecaiAttributeSet::GetPostureAttribute(),
			EGameplayModOp::Additive,
			-ParryPostureDamage);
	}

	// Apply Riposte tag to self (punish window)
	ApplyRiposteTag();

	// End parry window (successful — no whiff)
	RemoveParryingTag();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ParryWindowTimerHandle);
	}

	// Transition to None — parry resolved successfully
	TransitionToParryPhase(EMordecaiParryPhase::None);

	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}

	return true;
}

// ---------------------------------------------------------------------------
// Tag Accessors
// ---------------------------------------------------------------------------

bool UMordecaiGA_Parry::HasActivationBlockedTag(const FGameplayTag& Tag) const
{
	return ActivationBlockedTags.HasTag(Tag);
}

// ---------------------------------------------------------------------------
// Ability Activation (AC-006.11)
// ---------------------------------------------------------------------------

void UMordecaiGA_Parry::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bParrySuccessful = false;

	// AC-006.15: Consume stamina on activation
	ApplyStaminaCost();

	// Enter active parry window
	TransitionToParryPhase(EMordecaiParryPhase::Active);
	ApplyParryingTag();

	// Timer: parry window expires
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ParryWindowTimerHandle,
			this, &UMordecaiGA_Parry::OnParryWindowExpired,
			ParryWindowMs / 1000.f,
			false);
	}
}

void UMordecaiGA_Parry::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// Clear timers
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ParryWindowTimerHandle);
		World->GetTimerManager().ClearTimer(WhiffTimerHandle);
	}

	// Clean up tags
	if (CurrentParryPhase == EMordecaiParryPhase::Active)
	{
		RemoveParryingTag();
	}
	if (CurrentParryPhase == EMordecaiParryPhase::Whiff)
	{
		RemoveParryWhiffTag();
	}

	CurrentParryPhase = EMordecaiParryPhase::None;
	bParrySuccessful = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------
// Phase Callbacks
// ---------------------------------------------------------------------------

void UMordecaiGA_Parry::OnParryWindowExpired()
{
	if (bParrySuccessful) return; // Already resolved

	// AC-006.14: Failed parry — apply whiff penalty
	RemoveParryingTag();
	TransitionToParryPhase(EMordecaiParryPhase::Whiff);
	ApplyParryWhiffTag();

	// Timer: whiff recovery ends
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			WhiffTimerHandle,
			this, &UMordecaiGA_Parry::OnWhiffComplete,
			ParryWhiffDurationMs / 1000.f,
			false);
	}
}

void UMordecaiGA_Parry::OnWhiffComplete()
{
	RemoveParryWhiffTag();
	TransitionToParryPhase(EMordecaiParryPhase::None);

	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

// ---------------------------------------------------------------------------
// Stamina (AC-006.15)
// ---------------------------------------------------------------------------

void UMordecaiGA_Parry::ApplyStaminaCost()
{
	if (ParryStaminaCost <= 0.f) return;
	if (!CurrentActorInfo) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	ASC->ApplyModToAttribute(UMordecaiAttributeSet::GetStaminaAttribute(), EGameplayModOp::Additive, -ParryStaminaCost);
}

// ---------------------------------------------------------------------------
// Tag Management
// ---------------------------------------------------------------------------

void UMordecaiGA_Parry::ApplyParryingTag()
{
	if (!CurrentActorInfo) return;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(MordecaiGameplayTags::State_Parrying);
	}
}

void UMordecaiGA_Parry::RemoveParryingTag()
{
	if (!CurrentActorInfo) return;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::State_Parrying);
	}
}

void UMordecaiGA_Parry::ApplyParryWhiffTag()
{
	if (!CurrentActorInfo) return;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(MordecaiGameplayTags::State_ParryWhiff);
	}
}

void UMordecaiGA_Parry::RemoveParryWhiffTag()
{
	if (!CurrentActorInfo) return;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::State_ParryWhiff);
	}
}

void UMordecaiGA_Parry::ApplyParriedTagToAttacker(UAbilitySystemComponent* AttackerASC)
{
	if (!AttackerASC) return;
	AttackerASC->AddLooseGameplayTag(MordecaiGameplayTags::State_Parried);
}

void UMordecaiGA_Parry::ApplyRiposteTag()
{
	if (!CurrentActorInfo) return;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(MordecaiGameplayTags::State_Riposte);
	}
}

void UMordecaiGA_Parry::RemoveRiposteTag()
{
	if (!CurrentActorInfo) return;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::State_Riposte);
	}
}
