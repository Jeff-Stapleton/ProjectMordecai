// Project Mordecai — authoritative player stamina runtime (US-122 / issue #143)

#include "Mordecai/Combat/MordecaiStaminaRuntimeComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiStaminaSystem.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiStaminaRuntimeComponent)

UMordecaiStaminaRuntimeComponent::UMordecaiStaminaRuntimeComponent(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UMordecaiStaminaRuntimeComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		SetComponentTickEnabled(false);
		return;
	}

	StaminaSystem = NewObject<UMordecaiStaminaSystem>(this);
	BindToAbilitySystem();
}

void UMordecaiStaminaRuntimeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromAbilitySystem();
	StaminaSystem = nullptr;
	Super::EndPlay(EndPlayReason);
}

void UMordecaiStaminaRuntimeComponent::BindToAbilitySystem()
{
	UAbilitySystemComponent* ASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (!ASC || BoundASC.Get() == ASC)
	{
		return;
	}

	UnbindFromAbilitySystem();
	StaminaChangedHandle =
		ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetStaminaAttribute())
		.AddUObject(this, &ThisClass::OnStaminaChanged);
	MaxStaminaChangedHandle =
		ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetMaxStaminaAttribute())
		.AddUObject(this, &ThisClass::OnMaxStaminaChanged);
	BoundASC = ASC;
	UpdateTierTags();
}

void UMordecaiStaminaRuntimeComponent::UnbindFromAbilitySystem()
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetStaminaAttribute())
			.Remove(StaminaChangedHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetMaxStaminaAttribute())
			.Remove(MaxStaminaChangedHandle);
	}
	StaminaChangedHandle.Reset();
	MaxStaminaChangedHandle.Reset();
	BoundASC.Reset();
	ResetSprintTracking();
}

void UMordecaiStaminaRuntimeComponent::ResetSprintTracking()
{
	StationarySprintSeconds = 0.f;
	SprintToggleSeconds = 0.f;
	bSprintMovedSinceToggle = false;
	bWasSprinting = false;
}

double UMordecaiStaminaRuntimeComponent::GetRuntimeSeconds() const
{
	const UWorld* World = GetWorld();
	return World ? static_cast<double>(World->GetTimeSeconds()) : 0.0;
}

void UMordecaiStaminaRuntimeComponent::MarkStaminaConsumed()
{
	if (!StaminaSystem)
	{
		return;
	}

	StaminaSystem->NotifyStaminaConsumed(GetRuntimeSeconds());
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		ASC->SetLooseGameplayTagCount(
			MordecaiGameplayTags::State_StaminaRegenPaused,
			1,
			EGameplayTagReplicationState::TagOnly);
	}
}

void UMordecaiStaminaRuntimeComponent::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue < Data.OldValue - KINDA_SMALL_NUMBER)
	{
		MarkStaminaConsumed();
	}
	UpdateTierTags();
}

void UMordecaiStaminaRuntimeComponent::OnMaxStaminaChanged(const FOnAttributeChangeData& Data)
{
	UpdateTierTags();
}

void UMordecaiStaminaRuntimeComponent::UpdateTierTags()
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC || !StaminaSystem)
	{
		return;
	}

	const float Stamina = ASC->GetNumericAttribute(UMordecaiAttributeSet::GetStaminaAttribute());
	const float MaxStamina = ASC->GetNumericAttribute(UMordecaiAttributeSet::GetMaxStaminaAttribute());
	FGameplayTag DesiredTier;
	if (StaminaSystem->IsExhausted(Stamina))
	{
		DesiredTier = MordecaiGameplayTags::Stamina_Tier_Exhausted;
	}
	else
	{
		switch (StaminaSystem->GetTierForStamina(Stamina, MaxStamina))
		{
		case EMordecaiStaminaTier::Green:
			DesiredTier = MordecaiGameplayTags::Stamina_Tier_Green;
			break;
		case EMordecaiStaminaTier::Yellow:
			DesiredTier = MordecaiGameplayTags::Stamina_Tier_Yellow;
			break;
		case EMordecaiStaminaTier::Red:
		default:
			DesiredTier = MordecaiGameplayTags::Stamina_Tier_Red;
			break;
		}
	}

	// Add the new tag first so listeners never observe an untagged ASC, then remove
	// every other tier. The final authoritative state is exactly one tier tag.
	ASC->SetLooseGameplayTagCount(
		DesiredTier,
		1,
		EGameplayTagReplicationState::TagOnly);
	const FGameplayTag TierTags[] = {
		MordecaiGameplayTags::Stamina_Tier_Green.GetTag(),
		MordecaiGameplayTags::Stamina_Tier_Yellow.GetTag(),
		MordecaiGameplayTags::Stamina_Tier_Red.GetTag(),
		MordecaiGameplayTags::Stamina_Tier_Exhausted.GetTag()
	};
	for (const FGameplayTag& TierTag : TierTags)
	{
		if (TierTag != DesiredTier)
		{
			ASC->SetLooseGameplayTagCount(
				TierTag,
				0,
				EGameplayTagReplicationState::TagOnly);
		}
	}
}

bool UMordecaiStaminaRuntimeComponent::IsSprintingAvatarMoving(
	const UAbilitySystemComponent* ASC) const
{
	const AMordecaiCharacter* Character =
		ASC ? Cast<AMordecaiCharacter>(ASC->GetAvatarActor()) : nullptr;
	if (!Character)
	{
		return false;
	}

	const float MovementThreshold = FMath::Max(
		0.f,
		Character->WalkSpeed * SprintMovementThresholdRatio);
	return Character->GetVelocity().Size2D() > MovementThreshold;
}

void UMordecaiStaminaRuntimeComponent::ClearSprintToggle(UAbilitySystemComponent* ASC)
{
	if (AMordecaiCharacter* Character =
		ASC ? Cast<AMordecaiCharacter>(ASC->GetAvatarActor()) : nullptr)
	{
		Character->SetSprinting(false);
	}
	else if (ASC)
	{
		ASC->SetLooseGameplayTagCount(
			MordecaiGameplayTags::State_Sprinting,
			0,
			EGameplayTagReplicationState::TagOnly);
	}
}

void UMordecaiStaminaRuntimeComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC)
	{
		BindToAbilitySystem();
		ASC = BoundASC.Get();
	}
	if (!ASC || !StaminaSystem || DeltaTime <= 0.f)
	{
		return;
	}

	// Death owns the whole resource pause. Respawn still restores full stamina
	// through AMordecaiCharacter::HandlePlayerRespawn, as it did before US-122.
	if (ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Dead))
	{
		ResetSprintTracking();
		return;
	}

	if (ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Sprinting))
	{
		if (!bWasSprinting)
		{
			StationarySprintSeconds = 0.f;
			SprintToggleSeconds = 0.f;
			bSprintMovedSinceToggle = false;
			bWasSprinting = true;
		}
		SprintToggleSeconds += DeltaTime;

		if (IsSprintingAvatarMoving(ASC))
		{
			bSprintMovedSinceToggle = true;
			StationarySprintSeconds = 0.f;
			// Only a moving sprint spends stamina and resets the delay. Attempts
			// at the -50 floor still count, so regeneration waits until movement
			// actually stops.
			const float StaminaBeforeDrain = ASC->GetNumericAttribute(
				UMordecaiAttributeSet::GetStaminaAttribute());
			ASC->ApplyModToAttribute(
				UMordecaiAttributeSet::GetStaminaAttribute(),
				EGameplayModOp::Additive,
				-StaminaSystem->ComputeSprintDrain(DeltaTime));
			if (StaminaBeforeDrain <= -50.f + KINDA_SMALL_NUMBER)
			{
				// At the clamp floor no decrease delegate fires, so explicitly
				// preserve the moving-spend delay semantics.
				MarkStaminaConsumed();
			}
			return;
		}

		// A fresh toggle stays armed long enough for a player to press movement.
		// Once this toggle has moved, or its grace expires, the ordinary stopped
		// auto-clear begins counting.
		if (bSprintMovedSinceToggle || SprintToggleSeconds >= SprintToggleGraceSeconds)
		{
			StationarySprintSeconds += DeltaTime;
			if (StationarySprintSeconds >= SprintStationaryAutoClearSeconds)
			{
				ClearSprintToggle(ASC);
				ResetSprintTracking();
			}
		}
		// A stationary toggle neither spends nor blocks regeneration.
	}
	else
	{
		ResetSprintTracking();
	}

	const double Now = GetRuntimeSeconds();
	if (!StaminaSystem->IsRegenAllowed(Now))
	{
		return;
	}

	ASC->SetLooseGameplayTagCount(
		MordecaiGameplayTags::State_StaminaRegenPaused,
		0,
		EGameplayTagReplicationState::TagOnly);
	const float Stamina = ASC->GetNumericAttribute(UMordecaiAttributeSet::GetStaminaAttribute());
	const float MaxStamina = ASC->GetNumericAttribute(UMordecaiAttributeSet::GetMaxStaminaAttribute());
	if (Stamina >= MaxStamina)
	{
		return;
	}

	const float RegenMultiplier = FMath::Max(
		0.f,
		ASC->GetNumericAttribute(UMordecaiAttributeSet::GetStaminaRegenMultiplierAttribute()));
	const float RegenAmount = FMath::Min(
		StaminaSystem->ComputeRegenAmount(DeltaTime) * RegenMultiplier,
		MaxStamina - Stamina);
	if (RegenAmount > 0.f)
	{
		ASC->ApplyModToAttribute(
			UMordecaiAttributeSet::GetStaminaAttribute(),
			EGameplayModOp::Additive,
			RegenAmount);
	}
}
