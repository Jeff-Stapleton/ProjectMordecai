// Project Mordecai — Melee Attack Gameplay Ability (US-004)

#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/Combat/MordecaiHitDetectionSubsystem.h"
#include "Mordecai/Combat/MordecaiHitDetectionTypes.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "TimerManager.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiGA_MeleeAttack::UMordecaiGA_MeleeAttack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// AC-004.14: Prevent concurrent activation (cannot double-swing)
	ActivationOwnedTags.AddTag(MordecaiGameplayTags::State_Attacking);
	ActivationBlockedTags.AddTag(MordecaiGameplayTags::State_Attacking);

	// AC-007.8: Cannot attack during posture broken or knocked down
	ActivationBlockedTags.AddTag(MordecaiGameplayTags::State_PostureBroken);
	ActivationBlockedTags.AddTag(MordecaiGameplayTags::State_KnockedDown);
}

// ---------------------------------------------------------------------------
// Configuration Getters
// ---------------------------------------------------------------------------

const UMordecaiAttackProfileDataAsset* UMordecaiGA_MeleeAttack::GetActiveProfile() const
{
	if (AttackProfiles.IsValidIndex(CurrentComboIndex))
	{
		return AttackProfiles[CurrentComboIndex];
	}
	return nullptr;
}

float UMordecaiGA_MeleeAttack::GetPhaseDurationSeconds(EMordecaiAttackPhase Phase) const
{
	const UMordecaiAttackProfileDataAsset* Profile = GetActiveProfile();
	if (!Profile) return 0.f;

	switch (Phase)
	{
	case EMordecaiAttackPhase::Windup:   return Profile->WindupTimeMs / 1000.f;
	case EMordecaiAttackPhase::Active:   return Profile->ActiveTimeMs / 1000.f;
	case EMordecaiAttackPhase::Recovery: return Profile->RecoveryTimeMs / 1000.f;
	default: return 0.f;
	}
}

// ---------------------------------------------------------------------------
// Damage Computation (AC-004.5)
// ---------------------------------------------------------------------------

float UMordecaiGA_MeleeAttack::ComputeHealthDamage() const
{
	const UMordecaiAttackProfileDataAsset* Profile = GetActiveProfile();
	if (!Profile) return 0.f;

	// Damage = BasePower x SkillModifier x AttributeScaling x CriticalModifier x StatusModifier
	// TODO(DECISION): SkillModifier, AttributeScaling, CriticalModifier, StatusModifier
	//                 all stubbed at 1.0 until Epic 3 (Attributes) and Epic 4 (Status Effects)
	return -Profile->DamageProfile.BasePower;
}

float UMordecaiGA_MeleeAttack::ComputePostureDamage() const
{
	const UMordecaiAttackProfileDataAsset* Profile = GetActiveProfile();
	if (!Profile) return 0.f;

	return -(Profile->DamageProfile.BasePower * Profile->PostureDamageScalar);
}

// ---------------------------------------------------------------------------
// Stamina (AC-004.7)
// ---------------------------------------------------------------------------

float UMordecaiGA_MeleeAttack::GetStaminaCost() const
{
	const UMordecaiAttackProfileDataAsset* Profile = GetActiveProfile();
	if (!Profile) return 0.f;
	return Profile->StaminaCost;
}

// ---------------------------------------------------------------------------
// Rooting (AC-004.8)
// ---------------------------------------------------------------------------

bool UMordecaiGA_MeleeAttack::ShouldBeRootedDuringPhase(EMordecaiAttackPhase Phase) const
{
	const UMordecaiAttackProfileDataAsset* Profile = GetActiveProfile();
	if (!Profile) return false;

	switch (Profile->RootedDuring)
	{
	case EMordecaiRootedMode::None:
		return false;
	case EMordecaiRootedMode::Windup:
		return Phase == EMordecaiAttackPhase::Windup;
	case EMordecaiRootedMode::Active:
		return Phase == EMordecaiAttackPhase::Active;
	case EMordecaiRootedMode::Full:
		return Phase == EMordecaiAttackPhase::Windup ||
		       Phase == EMordecaiAttackPhase::Active ||
		       Phase == EMordecaiAttackPhase::Recovery;
	default:
		return false;
	}
}

// ---------------------------------------------------------------------------
// Cancel (AC-004.12, AC-004.13)
// ---------------------------------------------------------------------------

bool UMordecaiGA_MeleeAttack::CanCancelIntoDodge() const
{
	const UMordecaiAttackProfileDataAsset* Profile = GetActiveProfile();
	if (!Profile) return false;
	return Profile->CancelableIntoDodge && CurrentPhase == EMordecaiAttackPhase::Recovery;
}

bool UMordecaiGA_MeleeAttack::CanCancelIntoBlock() const
{
	const UMordecaiAttackProfileDataAsset* Profile = GetActiveProfile();
	if (!Profile) return false;
	return Profile->CancelableIntoBlock && CurrentPhase == EMordecaiAttackPhase::Recovery;
}

// ---------------------------------------------------------------------------
// Combo (AC-004.9, AC-004.10)
// ---------------------------------------------------------------------------

bool UMordecaiGA_MeleeAttack::AdvanceCombo()
{
	if (CurrentComboIndex + 1 < AttackProfiles.Num())
	{
		CurrentComboIndex++;
		return true;
	}
	return false;
}

void UMordecaiGA_MeleeAttack::ResetCombo()
{
	CurrentComboIndex = 0;
	bComboInputReceived = false;
}

void UMordecaiGA_MeleeAttack::NotifyComboInput()
{
	if (CurrentPhase == EMordecaiAttackPhase::Recovery)
	{
		bComboInputReceived = true;
	}
}

void UMordecaiGA_MeleeAttack::NotifyDifferentInput()
{
	ResetCombo();
}

// ---------------------------------------------------------------------------
// Damage Tag Mapping (AC-004.6)
// ---------------------------------------------------------------------------

FGameplayTag UMordecaiGA_MeleeAttack::GetDamageTagForType(EMordecaiDamageType DamageType)
{
	switch (DamageType)
	{
	case EMordecaiDamageType::Physical:  return MordecaiGameplayTags::Damage_Physical;
	case EMordecaiDamageType::Slash:     return MordecaiGameplayTags::Damage_Physical_Slash;
	case EMordecaiDamageType::Pierce:    return MordecaiGameplayTags::Damage_Physical_Pierce;
	case EMordecaiDamageType::Blunt:     return MordecaiGameplayTags::Damage_Physical_Blunt;
	case EMordecaiDamageType::Fire:      return MordecaiGameplayTags::Damage_Fire;
	case EMordecaiDamageType::Frost:     return MordecaiGameplayTags::Damage_Frost;
	case EMordecaiDamageType::Lightning: return MordecaiGameplayTags::Damage_Lightning;
	case EMordecaiDamageType::Poison:    return MordecaiGameplayTags::Damage_Poison;
	case EMordecaiDamageType::Corrosive: return MordecaiGameplayTags::Damage_Corrosive;
	case EMordecaiDamageType::Arcane:    return MordecaiGameplayTags::Damage_Arcane;
	case EMordecaiDamageType::Shadow:    return MordecaiGameplayTags::Damage_Shadow;
	case EMordecaiDamageType::Radiant:   return MordecaiGameplayTags::Damage_Radiant;
	default:                             return FGameplayTag();
	}
}

// ---------------------------------------------------------------------------
// Phase Management
// ---------------------------------------------------------------------------

void UMordecaiGA_MeleeAttack::TransitionToPhase(EMordecaiAttackPhase NewPhase)
{
	CurrentPhase = NewPhase;
	UpdateRooting();
}

// ---------------------------------------------------------------------------
// Tag Accessors (AC-004.14 testability)
// ---------------------------------------------------------------------------

bool UMordecaiGA_MeleeAttack::HasActivationOwnedTag(const FGameplayTag& Tag) const
{
	return ActivationOwnedTags.HasTag(Tag);
}

bool UMordecaiGA_MeleeAttack::HasActivationBlockedTag(const FGameplayTag& Tag) const
{
	return ActivationBlockedTags.HasTag(Tag);
}

// ---------------------------------------------------------------------------
// Ability Activation (AC-004.1, AC-004.3)
// ---------------------------------------------------------------------------

void UMordecaiGA_MeleeAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const UMordecaiAttackProfileDataAsset* Profile = GetActiveProfile();
	if (!Profile)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// AC-004.7: Consume stamina on activation
	ApplyStaminaCost();

	// AC-004.11: Apply OnUse payloads on activation
	ApplyOnUsePayloads();

	// AC-004.3: Start Windup phase
	TransitionToPhase(EMordecaiAttackPhase::Windup);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PhaseTimerHandle,
			this, &UMordecaiGA_MeleeAttack::OnWindupComplete,
			GetPhaseDurationSeconds(EMordecaiAttackPhase::Windup),
			false);
	}
}

void UMordecaiGA_MeleeAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// Clear all timers
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PhaseTimerHandle);
		World->GetTimerManager().ClearTimer(ComboWindowTimerHandle);
	}

	// Remove rooting if active
	if (bIsRooted)
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::State_Rooted);
		}
		bIsRooted = false;
	}

	CurrentPhase = EMordecaiAttackPhase::None;
	bComboInputReceived = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------
// Phase Callbacks
// ---------------------------------------------------------------------------

void UMordecaiGA_MeleeAttack::OnWindupComplete()
{
	// AC-004.3: Transition Windup -> Active
	TransitionToPhase(EMordecaiAttackPhase::Active);

	// AC-004.4: Hit detection fires once during Active
	PerformHitDetectionAndApplyDamage();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PhaseTimerHandle,
			this, &UMordecaiGA_MeleeAttack::OnActiveComplete,
			GetPhaseDurationSeconds(EMordecaiAttackPhase::Active),
			false);
	}
}

void UMordecaiGA_MeleeAttack::OnActiveComplete()
{
	// AC-004.3: Transition Active -> Recovery
	TransitionToPhase(EMordecaiAttackPhase::Recovery);

	const UMordecaiAttackProfileDataAsset* Profile = GetActiveProfile();
	if (!Profile) return;

	if (UWorld* World = GetWorld())
	{
		// Recovery phase timer
		World->GetTimerManager().SetTimer(
			PhaseTimerHandle,
			this, &UMordecaiGA_MeleeAttack::OnRecoveryComplete,
			GetPhaseDurationSeconds(EMordecaiAttackPhase::Recovery),
			false);

		// AC-004.10: Combo window timeout
		if (Profile->InputSlot == EMordecaiInputSlot::Light)
		{
			World->GetTimerManager().SetTimer(
				ComboWindowTimerHandle,
				this, &UMordecaiGA_MeleeAttack::OnComboWindowTimeout,
				ComboWindowTimeoutMs / 1000.f,
				false);
		}
	}
}

void UMordecaiGA_MeleeAttack::OnRecoveryComplete()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ComboWindowTimerHandle);
	}

	// AC-004.9: If combo input was received, advance and start next step
	if (bComboInputReceived && AdvanceCombo())
	{
		bComboInputReceived = false;
		const UMordecaiAttackProfileDataAsset* Profile = GetActiveProfile();
		if (Profile)
		{
			ApplyStaminaCost();
			TransitionToPhase(EMordecaiAttackPhase::Windup);
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(
					PhaseTimerHandle,
					this, &UMordecaiGA_MeleeAttack::OnWindupComplete,
					GetPhaseDurationSeconds(EMordecaiAttackPhase::Windup),
					false);
			}
			return;
		}
	}

	// No combo continuation — end ability
	TransitionToPhase(EMordecaiAttackPhase::None);
	ResetCombo();

	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UMordecaiGA_MeleeAttack::OnComboWindowTimeout()
{
	// AC-004.10: Combo resets after timeout
	bComboInputReceived = false;
}

// ---------------------------------------------------------------------------
// Hit Detection & Damage (AC-004.4, AC-004.5)
// ---------------------------------------------------------------------------

void UMordecaiGA_MeleeAttack::PerformHitDetectionAndApplyDamage()
{
	const UMordecaiAttackProfileDataAsset* Profile = GetActiveProfile();
	if (!Profile) return;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor) return;

	UWorld* World = AvatarActor->GetWorld();
	if (!World) return;

	UMordecaiHitDetectionSubsystem* HitDetection = World->GetSubsystem<UMordecaiHitDetectionSubsystem>();
	if (!HitDetection) return;

	const FVector Origin = AvatarActor->GetActorLocation();
	const FVector Forward = AvatarActor->GetActorForwardVector();

	TArray<FMordecaiHitResult> HitResults = HitDetection->PerformMeleeHitDetection(
		Profile, Origin, Forward, AvatarActor);

	for (const FMordecaiHitResult& Hit : HitResults)
	{
		if (Hit.HitActor.IsValid())
		{
			ApplyDamageToTarget(Hit.HitActor.Get(), Hit.HitLocation);
		}
	}
}

void UMordecaiGA_MeleeAttack::ApplyDamageToTarget(AActor* TargetActor, const FVector& /*HitLocation*/)
{
	if (!TargetActor) return;

	const UMordecaiAttackProfileDataAsset* Profile = GetActiveProfile();
	if (!Profile) return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
	if (!TargetASC) return;

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC) return;

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(this);

	// AC-004.5: Health damage via instant GE
	{
		UGameplayEffect* HealthGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_MordecaiMeleeHealthDamage"));
		HealthGE->DurationPolicy = EGameplayEffectDurationType::Instant;

		FGameplayModifierInfo& Mod = HealthGE->Modifiers.AddDefaulted_GetRef();
		Mod.Attribute = UMordecaiAttributeSet::GetHealthAttribute();
		Mod.ModifierOp = EGameplayModOp::Additive;
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(ComputeHealthDamage()));

		FGameplayEffectSpec HealthSpec(HealthGE, Context, 1.0f);

		// AC-004.6: Tag damage with correct Mordecai.Damage.* tag
		FGameplayTag DamageTag = GetDamageTagForType(Profile->DamageProfile.DamageType);
		if (DamageTag.IsValid())
		{
			HealthSpec.AddDynamicAssetTag(DamageTag);
		}

		TargetASC->ApplyGameplayEffectSpecToSelf(HealthSpec);
	}

	// AC-004.5: Posture damage via instant GE
	{
		UGameplayEffect* PostureGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_MordecaiMeleePostureDamage"));
		PostureGE->DurationPolicy = EGameplayEffectDurationType::Instant;

		FGameplayModifierInfo& Mod = PostureGE->Modifiers.AddDefaulted_GetRef();
		Mod.Attribute = UMordecaiAttributeSet::GetPostureAttribute();
		Mod.ModifierOp = EGameplayModOp::Additive;
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(ComputePostureDamage()));

		FGameplayEffectSpec PostureSpec(PostureGE, Context, 1.0f);

		FGameplayTag DamageTag = GetDamageTagForType(Profile->DamageProfile.DamageType);
		if (DamageTag.IsValid())
		{
			PostureSpec.AddDynamicAssetTag(DamageTag);
		}

		TargetASC->ApplyGameplayEffectSpecToSelf(PostureSpec);
	}

	// AC-004.11: Apply OnHit payloads
	ApplyOnHitPayloads(TargetASC);
}

// ---------------------------------------------------------------------------
// Stamina Cost Application (AC-004.7)
// ---------------------------------------------------------------------------

void UMordecaiGA_MeleeAttack::ApplyStaminaCost()
{
	float Cost = GetStaminaCost();
	if (Cost <= 0.f) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	// Direct attribute modification for stamina consumption on activation
	ASC->ApplyModToAttribute(UMordecaiAttributeSet::GetStaminaAttribute(), EGameplayModOp::Additive, -Cost);
}

// ---------------------------------------------------------------------------
// Payload Application (AC-004.11)
// ---------------------------------------------------------------------------

void UMordecaiGA_MeleeAttack::ApplyOnUsePayloads()
{
	const UMordecaiAttackProfileDataAsset* Profile = GetActiveProfile();
	if (!Profile) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);

	for (const TSubclassOf<UGameplayEffect>& PayloadClass : Profile->OnUsePayload)
	{
		if (PayloadClass)
		{
			FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(PayloadClass, 1.0f, Context);
			if (Spec.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
			}
		}
	}
}

void UMordecaiGA_MeleeAttack::ApplyOnHitPayloads(UAbilitySystemComponent* TargetASC)
{
	const UMordecaiAttackProfileDataAsset* Profile = GetActiveProfile();
	if (!Profile || !TargetASC) return;

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC) return;

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(this);

	for (const TSubclassOf<UGameplayEffect>& PayloadClass : Profile->OnHitPayload)
	{
		if (PayloadClass)
		{
			FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(PayloadClass, 1.0f, Context);
			if (Spec.IsValid())
			{
				TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Rooting (AC-004.8)
// ---------------------------------------------------------------------------

void UMordecaiGA_MeleeAttack::UpdateRooting()
{
	bool bShouldBeRooted = ShouldBeRootedDuringPhase(CurrentPhase);

	if (bShouldBeRooted && !bIsRooted)
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->AddLooseGameplayTag(MordecaiGameplayTags::State_Rooted);
		}
		bIsRooted = true;
	}
	else if (!bShouldBeRooted && bIsRooted)
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::State_Rooted);
		}
		bIsRooted = false;
	}
}
