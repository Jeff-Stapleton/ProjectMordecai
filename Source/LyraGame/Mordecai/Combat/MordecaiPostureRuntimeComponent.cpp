// Project Mordecai — authoritative player posture runtime (US-123, US-124)

#include "Mordecai/Combat/MordecaiPostureRuntimeComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiPostureSystem.h"
#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffectExtension.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiPostureRuntimeComponent)

UMordecaiPostureRuntimeComponent::UMordecaiPostureRuntimeComponent(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UMordecaiPostureRuntimeComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		SetComponentTickEnabled(false);
		return;
	}

	PostureSystem = NewObject<UMordecaiPostureSystem>(this);
	BindToAbilitySystem();
}

void UMordecaiPostureRuntimeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PostureBrokenTimerHandle);
		World->GetTimerManager().ClearTimer(KnockDownTimerHandle);
	}
	UnbindFromAbilitySystem();
	PostureSystem = nullptr;
	Super::EndPlay(EndPlayReason);
}

void UMordecaiPostureRuntimeComponent::BindToAbilitySystem()
{
	UAbilitySystemComponent* ASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (!ASC || BoundASC.Get() == ASC)
	{
		return;
	}

	UnbindFromAbilitySystem();
	PostureChangedHandle =
		ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetPostureAttribute())
		.AddUObject(this, &ThisClass::OnPostureChanged);
	HealthChangedHandle =
		ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetHealthAttribute())
		.AddUObject(this, &ThisClass::OnHealthChanged);
	DeadTagChangedHandle =
		ASC->RegisterGameplayTagEvent(
			MordecaiGameplayTags::State_Dead,
			EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::OnDeadTagChanged);
	BoundASC = ASC;
}

void UMordecaiPostureRuntimeComponent::UnbindFromAbilitySystem()
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetPostureAttribute())
			.Remove(PostureChangedHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetHealthAttribute())
			.Remove(HealthChangedHandle);
		ASC->RegisterGameplayTagEvent(
			MordecaiGameplayTags::State_Dead,
			EGameplayTagEventType::NewOrRemoved)
			.Remove(DeadTagChangedHandle);
	}
	PostureChangedHandle.Reset();
	HealthChangedHandle.Reset();
	DeadTagChangedHandle.Reset();
	BoundASC.Reset();
}

void UMordecaiPostureRuntimeComponent::OnPostureChanged(const FOnAttributeChangeData& Data)
{
	if (PostureSystem && Data.NewValue < Data.OldValue - KINDA_SMALL_NUMBER)
	{
		PostureSystem->NotifyPostureDamageApplied(GetRuntimeSeconds());
	}

	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (PostureSystem && ASC && Data.NewValue <= 0.f
		&& !ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Dead)
		&& !ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_PostureBroken)
		&& !ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_KnockedDown))
	{
		EnterPostureBrokenState();
	}
}

void UMordecaiPostureRuntimeComponent::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!GetOwner() || !GetOwner()->HasAuthority()
		|| !ASC || !PostureSystem || Data.NewValue >= Data.OldValue
		|| !ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_PostureBroken)
		|| !IsEnemyDamage(Data))
	{
		return;
	}

	PostureSystem->AccumulateStaggerDamage(Data.OldValue - Data.NewValue);
	if (PostureSystem->ShouldKnockDown())
	{
		EnterKnockDownState();
	}
}

void UMordecaiPostureRuntimeComponent::OnDeadTagChanged(
	const FGameplayTag Tag,
	const int32 NewCount)
{
	ClearPostureRuntimeStates();
}

void UMordecaiPostureRuntimeComponent::EnterPostureBrokenState()
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC || !PostureSystem || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	SetPostureRuntimeState(MordecaiGameplayTags::State_KnockedDown, false);
	SetPostureRuntimeState(MordecaiGameplayTags::State_PostureBroken, true);
	PostureSystem->EnterPostureBroken();
	CancelActiveAbilities();
	LockMovement();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PostureBrokenTimerHandle);
		World->GetTimerManager().SetTimer(
			PostureBrokenTimerHandle,
			this,
			&ThisClass::ExitPostureBrokenState,
			PostureSystem->GetPostureBrokenDurationSeconds(),
			false);
	}
}

void UMordecaiPostureRuntimeComponent::ExitPostureBrokenState()
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC || !PostureSystem || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	SetPostureRuntimeState(MordecaiGameplayTags::State_PostureBroken, false);
	const float MaxPosture =
		ASC->GetNumericAttribute(UMordecaiAttributeSet::GetMaxPostureAttribute());
	PostureSystem->ExitPostureBroken(MaxPosture);
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetPostureAttribute(), MaxPosture);
	UnlockMovement();
}

void UMordecaiPostureRuntimeComponent::EnterKnockDownState()
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC || !PostureSystem || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PostureBrokenTimerHandle);
	}
	SetPostureRuntimeState(MordecaiGameplayTags::State_KnockedDown, true);
	SetPostureRuntimeState(MordecaiGameplayTags::State_PostureBroken, false);
	PostureSystem->EnterKnockDown();
	CancelActiveAbilities();
	LockMovement();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(KnockDownTimerHandle);
		World->GetTimerManager().SetTimer(
			KnockDownTimerHandle,
			this,
			&ThisClass::ExitKnockDownState,
			PostureSystem->GetKnockDownDurationSeconds(),
			false);
	}
}

void UMordecaiPostureRuntimeComponent::ExitKnockDownState()
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC || !PostureSystem || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	SetPostureRuntimeState(MordecaiGameplayTags::State_KnockedDown, false);
	const float MaxPosture =
		ASC->GetNumericAttribute(UMordecaiAttributeSet::GetMaxPostureAttribute());
	PostureSystem->ExitKnockDown(MaxPosture);
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetPostureAttribute(), MaxPosture);
	UnlockMovement();
}

void UMordecaiPostureRuntimeComponent::ClearPostureRuntimeStates()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PostureBrokenTimerHandle);
		World->GetTimerManager().ClearTimer(KnockDownTimerHandle);
	}
	SetPostureRuntimeState(MordecaiGameplayTags::State_PostureBroken, false);
	SetPostureRuntimeState(MordecaiGameplayTags::State_KnockedDown, false);
	SetPostureRuntimeState(MordecaiGameplayTags::State_Parried, false);
	SetPostureRuntimeState(MordecaiGameplayTags::State_Riposte, false);
	if (PostureSystem)
	{
		PostureSystem->Reset();
	}
	UnlockMovement();
}

void UMordecaiPostureRuntimeComponent::SetPostureRuntimeState(
	const FGameplayTag StateTag,
	const bool bEnabled)
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC || !StateTag.IsValid() || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	ASC->SetLooseGameplayTagCount(
		StateTag,
		bEnabled ? 1 : 0,
		EGameplayTagReplicationState::TagOnly);
}

void UMordecaiPostureRuntimeComponent::CancelActiveAbilities()
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		ASC->CancelAllAbilities();
	}
}

void UMordecaiPostureRuntimeComponent::LockMovement()
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	AMordecaiCharacter* Character =
		ASC ? Cast<AMordecaiCharacter>(ASC->GetAvatarActor()) : nullptr;
	UCharacterMovementComponent* Movement =
		Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		return;
	}

	if (!bMovementLocked)
	{
		SavedMovementMode = static_cast<uint8>(Movement->MovementMode);
		SavedCustomMovementMode = Movement->CustomMovementMode;
		bMovementLocked = true;
	}
	Movement->StopMovementImmediately();
	Movement->SetMovementMode(MOVE_None);
}

void UMordecaiPostureRuntimeComponent::UnlockMovement()
{
	if (!bMovementLocked)
	{
		return;
	}
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC
		|| ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Dead)
		|| ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_PostureBroken)
		|| ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_KnockedDown))
	{
		return;
	}

	AMordecaiCharacter* Character = Cast<AMordecaiCharacter>(ASC->GetAvatarActor());
	if (UCharacterMovementComponent* Movement =
		Character ? Character->GetCharacterMovement() : nullptr)
	{
		const EMovementMode RestoredMode =
			static_cast<EMovementMode>(SavedMovementMode);
		Movement->SetMovementMode(
			RestoredMode == MOVE_None ? MOVE_Walking : RestoredMode,
			SavedCustomMovementMode);
	}
	bMovementLocked = false;
}

bool UMordecaiPostureRuntimeComponent::IsEnemyDamage(
	const FOnAttributeChangeData& Data) const
{
	if (!Data.GEModData)
	{
		return false;
	}
	const FGameplayEffectContextHandle& Context =
		Data.GEModData->EffectSpec.GetEffectContext();
	for (AActor* Actor : {
		Context.GetOriginalInstigator(),
		Context.GetEffectCauser() })
	{
		if (Cast<AMordecaiEnemyCharacter>(Actor))
		{
			return true;
		}
		if (Actor && Cast<AMordecaiEnemyCharacter>(Actor->GetInstigator()))
		{
			return true;
		}
	}
	return false;
}

double UMordecaiPostureRuntimeComponent::GetRuntimeSeconds() const
{
	const UWorld* World = GetWorld();
	return World ? static_cast<double>(World->GetTimeSeconds()) : 0.0;
}

void UMordecaiPostureRuntimeComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC)
	{
		BindToAbilitySystem();
		ASC = BoundASC.Get();
	}
	if (!ASC || !PostureSystem || DeltaTime <= 0.f
		|| ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Dead)
		|| ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_PostureBroken)
		|| ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_KnockedDown)
		|| !PostureSystem->IsRegenAllowed(GetRuntimeSeconds()))
	{
		return;
	}

	const float Posture =
		ASC->GetNumericAttribute(UMordecaiAttributeSet::GetPostureAttribute());
	const float MaxPosture =
		ASC->GetNumericAttribute(UMordecaiAttributeSet::GetMaxPostureAttribute());
	if (Posture >= MaxPosture)
	{
		return;
	}

	const float RecoveryMultiplier = FMath::Max(
		0.f,
		ASC->GetNumericAttribute(
			UMordecaiAttributeSet::GetPostureRecoveryMultiplierAttribute()));
	const float RegenAmount = FMath::Min(
		PostureSystem->ComputeRegenAmount(DeltaTime) * RecoveryMultiplier,
		MaxPosture - Posture);
	if (RegenAmount > 0.f)
	{
		ASC->ApplyModToAttribute(
			UMordecaiAttributeSet::GetPostureAttribute(),
			EGameplayModOp::Additive,
			RegenAmount);
	}
}
