// Project Mordecai — authoritative player posture regeneration (US-123)

#include "Mordecai/Combat/MordecaiPostureRuntimeComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiPostureSystem.h"
#include "Mordecai/MordecaiGameplayTags.h"

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
	BoundASC = ASC;
}

void UMordecaiPostureRuntimeComponent::UnbindFromAbilitySystem()
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetPostureAttribute())
			.Remove(PostureChangedHandle);
	}
	PostureChangedHandle.Reset();
	BoundASC.Reset();
}

void UMordecaiPostureRuntimeComponent::OnPostureChanged(const FOnAttributeChangeData& Data)
{
	if (PostureSystem && Data.NewValue < Data.OldValue - KINDA_SMALL_NUMBER)
	{
		PostureSystem->NotifyPostureDamageApplied(GetRuntimeSeconds());
	}
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
