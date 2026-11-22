// Project Mordecai — Parry Gameplay Ability (US-006)

#include "Mordecai/Combat/MordecaiGA_Parry.h"
#include "Mordecai/Combat/MordecaiGA_Block.h"
#include "Mordecai/Combat/MordecaiPostureSystem.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"
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

	// US-083: no parry from a dead player
	ActivationBlockedTags.AddTag(MordecaiGameplayTags::State_Dead);

	// AC-006.17/AC-006.18: Parry is NOT blocked by Blocking tag.
	// Parry cancels an active block (ActivateAbility), and Block cannot activate while Parrying.
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

bool UMordecaiGA_Parry::ReceiveParriedHit(
	UAbilitySystemComponent* AttackerASC,
	const bool bApplyParriedState)
{
	if (CurrentParryPhase != EMordecaiParryPhase::Active)
	{
		return false;
	}

	if (!CurrentActorInfo)
	{
		return false;
	}
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor || !AvatarActor->HasAuthority())
	{
		return false;
	}

	bParrySuccessful = true;

	// Apply Parried tag to attacker (stagger)
	if (AttackerASC)
	{
		if (bApplyParriedState)
		{
			ApplyParriedTagToAttacker(AttackerASC);
		}

		ApplyParryPostureDamage(AttackerASC);
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

void UMordecaiGA_Parry::ApplyParryPostureDamage(UAbilitySystemComponent* AttackerASC)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* DefenderASC = GetAbilitySystemComponentFromActorInfo();
	if (!AvatarActor || !AvatarActor->HasAuthority() || !AttackerASC || !DefenderASC)
	{
		return;
	}

	const float ResolvedDamage = UMordecaiPostureSystem::ComputePostureDamageFromAttributes(
		DefenderASC,
		AttackerASC,
		ParryPostureDamage);
	AttackerASC->ApplyModToAttribute(
		UMordecaiAttributeSet::GetPostureAttribute(),
		EGameplayModOp::Additive,
		-ResolvedDamage);
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

bool UMordecaiGA_Parry::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(
		Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	return !UMordecaiGA_MeleeAttack::FindActiveMelee(
		ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr);
}

void UMordecaiGA_Parry::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// AC-006.18: the parry drops an active guard (the player presses attack while blocking)
	CancelActiveBlock();

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
// Block (AC-006.17, AC-006.18)
// ---------------------------------------------------------------------------

void UMordecaiGA_Parry::CancelActiveBlock()
{
	if (!CurrentActorInfo) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	TArray<FGameplayAbilitySpecHandle> ActiveBlocks;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.IsActive() && Spec.Ability && Spec.Ability->IsA<UMordecaiGA_Block>())
		{
			ActiveBlocks.Add(Spec.Handle);
		}
	}
	for (const FGameplayAbilitySpecHandle& BlockHandle : ActiveBlocks)
	{
		ASC->CancelAbilityHandle(BlockHandle);
	}
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
	if (AMordecaiEnemyCharacter* Enemy =
		Cast<AMordecaiEnemyCharacter>(AttackerASC->GetAvatarActor()))
	{
		Enemy->EnterParriedState();
		return;
	}

	AttackerASC->SetLooseGameplayTagCount(
		MordecaiGameplayTags::State_Parried,
		1,
		EGameplayTagReplicationState::TagOnly);
	ParriedAttackerASC = AttackerASC;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ParriedAttackerTimerHandle);
		World->GetTimerManager().SetTimer(
			ParriedAttackerTimerHandle,
			this,
			&ThisClass::RemoveParriedTagFromAttacker,
			GetDefault<UMordecaiPostureSystem>()->GetParriedDurationSeconds(),
			false);
	}
}

void UMordecaiGA_Parry::RemoveParriedTagFromAttacker()
{
	if (UAbilitySystemComponent* AttackerASC = ParriedAttackerASC.Get())
	{
		AttackerASC->SetLooseGameplayTagCount(
			MordecaiGameplayTags::State_Parried,
			0,
			EGameplayTagReplicationState::TagOnly);
	}
	ParriedAttackerASC.Reset();
}

void UMordecaiGA_Parry::ApplyRiposteTag()
{
	if (!CurrentActorInfo) return;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->SetLooseGameplayTagCount(
			MordecaiGameplayTags::State_Riposte,
			1,
			EGameplayTagReplicationState::TagOnly);
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(RiposteTimerHandle);
			const TWeakObjectPtr<UAbilitySystemComponent> WeakASC = ASC;
			World->GetTimerManager().SetTimer(
				RiposteTimerHandle,
				FTimerDelegate::CreateLambda([WeakASC]()
				{
					if (UAbilitySystemComponent* RiposteASC = WeakASC.Get())
					{
						RiposteASC->SetLooseGameplayTagCount(
							MordecaiGameplayTags::State_Riposte,
							0,
							EGameplayTagReplicationState::TagOnly);
					}
				}),
				GetDefault<UMordecaiPostureSystem>()->GetRiposteWindowSeconds(),
				false);
		}
	}
}
