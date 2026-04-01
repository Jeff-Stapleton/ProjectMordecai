// Project Mordecai — Spell Base Gameplay Ability (US-019)

#include "Mordecai/Magic/MordecaiGA_SpellBase.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeScaling.h"
#include "Mordecai/Skills/MordecaiSkillComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiGA_SpellBase::UMordecaiGA_SpellBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// AC-019.12: Tag this ability as a spell
	SetAssetTags(FGameplayTagContainer(MordecaiGameplayTags::Ability_Spell));

	// AC-019.13: Cannot cast while silenced
	ActivationBlockedTags.AddTag(MordecaiGameplayTags::Status_Silenced);

	// Block during vulnerable states (consistent with melee)
	ActivationBlockedTags.AddTag(MordecaiGameplayTags::State_PostureBroken);
	ActivationBlockedTags.AddTag(MordecaiGameplayTags::State_KnockedDown);

	// Apply casting state tag while active
	ActivationOwnedTags.AddTag(MordecaiGameplayTags::State_Casting);
}

// ---------------------------------------------------------------------------
// Public Getters (testable API)
// ---------------------------------------------------------------------------

float UMordecaiGA_SpellBase::GetEffectiveSPCost(bool bUpcast) const
{
	if (!SpellData) return 0.f;

	float Cost = static_cast<float>(SpellData->SpellPointCost);
	if (bUpcast)
	{
		Cost *= SpellData->UpcastCostMultiplier;
	}
	return Cost;
}

bool UMordecaiGA_SpellBase::CheckSpellPointCost(float CurrentSP, bool bUpcast) const
{
	return CurrentSP >= GetEffectiveSPCost(bUpcast);
}

float UMordecaiGA_SpellBase::GetCooldownDuration() const
{
	if (!SpellData) return 0.f;
	return SpellData->CooldownDuration;
}

FGameplayTag UMordecaiGA_SpellBase::GetCooldownTag() const
{
	if (!SpellData) return FGameplayTag();
	return SpellData->CooldownTag;
}

float UMordecaiGA_SpellBase::GetWindupDuration() const
{
	if (!SpellData) return 0.f;
	return SpellData->WindupTime;
}

float UMordecaiGA_SpellBase::GetCastDuration() const
{
	if (!SpellData) return 0.f;
	return SpellData->CastTime;
}

float UMordecaiGA_SpellBase::GetRecoveryDuration() const
{
	if (!SpellData) return 0.f;
	return SpellData->RecoveryTime;
}

FGameplayTag UMordecaiGA_SpellBase::GetMovementPolicyTag() const
{
	if (!SpellData) return FGameplayTag();

	switch (SpellData->MovementPolicy)
	{
	case EMordecaiMovementPolicy::SlowWhileCasting:
		return MordecaiGameplayTags::State_CastingSlow;
	case EMordecaiMovementPolicy::RootWhileCasting:
		return MordecaiGameplayTags::State_CastingRooted;
	case EMordecaiMovementPolicy::FreeMove:
	default:
		return FGameplayTag();
	}
}

bool UMordecaiGA_SpellBase::IsInterruptible() const
{
	if (!SpellData) return false;
	return SpellData->bInterruptible;
}

bool UMordecaiGA_SpellBase::ShouldInterruptInCurrentPhase() const
{
	if (!IsInterruptible()) return false;

	return CurrentPhase == EMordecaiCastingPhase::Windup ||
	       CurrentPhase == EMordecaiCastingPhase::Cast;
}

float UMordecaiGA_SpellBase::ComputeSpellPower(float AttributeScalingBonus, int32 SkillRank, bool bUpcast) const
{
	if (!SpellData) return 0.f;

	// AC-019.9: SpellPower = BasePower * (1.0 + Σ(ScalingStat.Coefficient × EffectiveModForStat))
	float Power = SpellData->BasePower * (1.0f + AttributeScalingBonus);

	// AC-019.10: Rank multiplier = 1.0 + (Rank × 0.05)
	float RankMultiplier = 1.0f + (static_cast<float>(SkillRank) * 0.05f);
	Power *= RankMultiplier;

	// AC-019.11: Upcast power multiplier
	if (bUpcast)
	{
		Power *= SpellData->UpcastPowerMultiplier;
	}

	return Power;
}

void UMordecaiGA_SpellBase::TransitionToPhase(EMordecaiCastingPhase NewPhase)
{
	CurrentPhase = NewPhase;
}

bool UMordecaiGA_SpellBase::HasActivationBlockedTag(const FGameplayTag& Tag) const
{
	return ActivationBlockedTags.HasTag(Tag);
}

// ---------------------------------------------------------------------------
// CanActivateAbility (AC-019.3, AC-019.5, AC-019.13)
// ---------------------------------------------------------------------------

bool UMordecaiGA_SpellBase::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (!SpellData)
	{
		return false;
	}

	// AC-019.3: Check SP cost
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		float CurrentSP = ASC->GetNumericAttribute(UMordecaiAttributeSet::GetSpellPointsAttribute());
		if (!CheckSpellPointCost(CurrentSP, bIsUpcast))
		{
			return false;
		}

		// AC-019.5: Check cooldown tag
		FGameplayTag CdTag = GetCooldownTag();
		if (CdTag.IsValid() && ASC->HasMatchingGameplayTag(CdTag))
		{
			return false;
		}
	}

	return true;
}

// ---------------------------------------------------------------------------
// ActivateAbility (AC-019.4, AC-019.6, AC-019.7, AC-019.8)
// ---------------------------------------------------------------------------

void UMordecaiGA_SpellBase::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!SpellData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// AC-019.4: Deduct SP on activation (cost committed)
	ApplySpellPointCost();

	// AC-019.7: Apply movement policy tag during casting
	ApplyMovementPolicyTag();

	// AC-019.8: Register for damage events if interruptible
	if (SpellData->bInterruptible)
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			FGameplayEventMulticastDelegate& Delegate = ASC->GenericGameplayEventCallbacks.FindOrAdd(
				MordecaiGameplayTags::Event_DamageTaken);
			DamageEventDelegateHandle = Delegate.AddUObject(
				this, &UMordecaiGA_SpellBase::OnDamageTakenCallback);
		}
	}

	bCooldownApplied = false;

	// AC-019.6: Start Windup phase
	TransitionToPhase(EMordecaiCastingPhase::Windup);

	if (UWorld* World = GetWorld())
	{
		float Duration = GetWindupDuration();
		if (Duration > 0.f)
		{
			World->GetTimerManager().SetTimer(
				PhaseTimerHandle,
				this, &UMordecaiGA_SpellBase::HandleWindupComplete,
				Duration,
				false);
		}
		else
		{
			HandleWindupComplete();
		}
	}
}

// ---------------------------------------------------------------------------
// EndAbility
// ---------------------------------------------------------------------------

void UMordecaiGA_SpellBase::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// Clear phase timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PhaseTimerHandle);
	}

	// Remove movement policy tag
	RemoveMovementPolicyTag();

	// Unregister damage event callback
	if (DamageEventDelegateHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->GenericGameplayEventCallbacks.FindOrAdd(
				MordecaiGameplayTags::Event_DamageTaken).Remove(DamageEventDelegateHandle);
		}
		DamageEventDelegateHandle.Reset();
	}

	CurrentPhase = EMordecaiCastingPhase::None;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------
// Phase Callbacks (AC-019.6)
// ---------------------------------------------------------------------------

void UMordecaiGA_SpellBase::HandleWindupComplete()
{
	// AC-019.6: Windup → Cast
	OnSpellWindupComplete();

	TransitionToPhase(EMordecaiCastingPhase::Cast);

	if (UWorld* World = GetWorld())
	{
		float Duration = GetCastDuration();
		if (Duration > 0.f)
		{
			World->GetTimerManager().SetTimer(
				PhaseTimerHandle,
				this, &UMordecaiGA_SpellBase::HandleCastComplete,
				Duration,
				false);
		}
		else
		{
			HandleCastComplete();
		}
	}
}

void UMordecaiGA_SpellBase::HandleCastComplete()
{
	// AC-019.6: Cast phase fires — subclass delivers spell effect
	OnSpellCast();

	// AC-019.12: Broadcast spell cast event
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		FGameplayEventData EventData;
		EventData.Instigator = GetAvatarActorFromActorInfo();
		ASC->HandleGameplayEvent(MordecaiGameplayTags::Event_SpellCast, &EventData);
	}

	// AC-019.5: Apply cooldown after successful cast (not on interrupt per AC-019.8)
	ApplySpellCooldown();

	// AC-019.7: Remove movement policy tag on transition to Recovery
	RemoveMovementPolicyTag();

	// AC-019.6: Cast → Recovery
	TransitionToPhase(EMordecaiCastingPhase::Recovery);

	if (UWorld* World = GetWorld())
	{
		float Duration = GetRecoveryDuration();
		if (Duration > 0.f)
		{
			World->GetTimerManager().SetTimer(
				PhaseTimerHandle,
				this, &UMordecaiGA_SpellBase::HandleRecoveryComplete,
				Duration,
				false);
		}
		else
		{
			HandleRecoveryComplete();
		}
	}
}

void UMordecaiGA_SpellBase::HandleRecoveryComplete()
{
	OnSpellRecoveryComplete();

	TransitionToPhase(EMordecaiCastingPhase::None);

	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

// ---------------------------------------------------------------------------
// Virtual Hooks (AC-019.6) — Default implementations are no-ops
// ---------------------------------------------------------------------------

void UMordecaiGA_SpellBase::OnSpellWindupComplete()
{
}

void UMordecaiGA_SpellBase::OnSpellCast()
{
}

void UMordecaiGA_SpellBase::OnSpellRecoveryComplete()
{
}

// ---------------------------------------------------------------------------
// SP Cost Application (AC-019.4)
// ---------------------------------------------------------------------------

void UMordecaiGA_SpellBase::ApplySpellPointCost()
{
	float Cost = GetEffectiveSPCost(bIsUpcast);
	if (Cost <= 0.f) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	// AC-019.4: Deduct SP via instant GameplayEffect
	UGameplayEffect* SPGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_MordecaiSpellPointCost"));
	SPGE->DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& Mod = SPGE->Modifiers.AddDefaulted_GetRef();
	Mod.Attribute = UMordecaiAttributeSet::GetSpellPointsAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-Cost));

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpec Spec(SPGE, Context, 1.0f);
	ASC->ApplyGameplayEffectSpecToSelf(Spec);
}

// ---------------------------------------------------------------------------
// Cooldown Application (AC-019.5)
// ---------------------------------------------------------------------------

void UMordecaiGA_SpellBase::ApplySpellCooldown()
{
	if (bCooldownApplied) return;

	float Duration = GetCooldownDuration();
	FGameplayTag CdTag = GetCooldownTag();
	if (Duration <= 0.f || !CdTag.IsValid()) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	// AC-019.5: Duration GE that grants the cooldown tag
	UGameplayEffect* CooldownGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_MordecaiSpellCooldown"));
	CooldownGE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	CooldownGE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));

	// Use the modern component API to grant the cooldown tag to the target
	UTargetTagsGameplayEffectComponent& TargetTagsComp = CooldownGE->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagChanges;
	TagChanges.AddTag(CdTag);
	TargetTagsComp.SetAndApplyTargetTagChanges(TagChanges);

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpec Spec(CooldownGE, Context, 1.0f);
	ASC->ApplyGameplayEffectSpecToSelf(Spec);

	bCooldownApplied = true;
}

// ---------------------------------------------------------------------------
// Movement Policy (AC-019.7)
// ---------------------------------------------------------------------------

void UMordecaiGA_SpellBase::ApplyMovementPolicyTag()
{
	FGameplayTag PolicyTag = GetMovementPolicyTag();
	if (!PolicyTag.IsValid()) return;

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(PolicyTag);
		bMovementPolicyTagApplied = true;
	}
}

void UMordecaiGA_SpellBase::RemoveMovementPolicyTag()
{
	if (!bMovementPolicyTagApplied) return;

	FGameplayTag PolicyTag = GetMovementPolicyTag();
	if (!PolicyTag.IsValid()) return;

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(PolicyTag);
	}
	bMovementPolicyTagApplied = false;
}

// ---------------------------------------------------------------------------
// Interruption (AC-019.8)
// ---------------------------------------------------------------------------

void UMordecaiGA_SpellBase::OnDamageTakenCallback(const FGameplayEventData* Payload)
{
	if (!ShouldInterruptInCurrentPhase()) return;

	// AC-019.8: Cancel ability on damage during Windup or Cast.
	// SP is already consumed. Cooldown is NOT applied (bCooldownApplied remains false).
	if (IsActive())
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
	}
}

// ---------------------------------------------------------------------------
// Internal Helpers
// ---------------------------------------------------------------------------

float UMordecaiGA_SpellBase::ComputeAttributeScalingSumFromASC() const
{
	if (!SpellData) return 0.f;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return 0.f;

	float Sum = 0.f;
	for (const FMordecaiSpellScalingStat& Entry : SpellData->ScalingStats)
	{
		if (Entry.Stat.IsValid())
		{
			float RawValue = ASC->GetNumericAttribute(Entry.Stat);
			float EffMod = FMordecaiAttributeScaling::CalculateEffectiveMod(RawValue);
			Sum += Entry.Coefficient * EffMod;
		}
	}
	return Sum;
}

int32 UMordecaiGA_SpellBase::GetSkillRankFromComponent() const
{
	if (!SpellData) return 0;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor) return 0;

	// Skill component lives on PlayerState — check the pawn's controller's PlayerState
	if (APawn* Pawn = Cast<APawn>(AvatarActor))
	{
		if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
		{
			if (APlayerState* PS = PC->PlayerState)
			{
				if (UMordecaiSkillComponent* SkillComp = PS->FindComponentByClass<UMordecaiSkillComponent>())
				{
					return SkillComp->GetSkillRank(SpellData->SpellId);
				}
			}
		}
	}

	// Fallback: check the avatar actor directly
	if (UMordecaiSkillComponent* SkillComp = AvatarActor->FindComponentByClass<UMordecaiSkillComponent>())
	{
		return SkillComp->GetSkillRank(SpellData->SpellId);
	}

	return 0;
}
