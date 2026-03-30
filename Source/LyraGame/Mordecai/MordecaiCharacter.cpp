// Project Mordecai

#include "MordecaiCharacter.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/MordecaiGameMode.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiCharacter)

AMordecaiCharacter::AMordecaiCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, WalkSpeed(600.0f)
	, SprintSpeedMultiplier(1.5f)
	, GroundAcceleration(2048.0f)
	, GroundDeceleration(2048.0f)
	, bIsSprinting(false)
{
	// Arrow component for facing direction visualization (AC-2.1.5)
	FacingArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("FacingArrow"));
	FacingArrowComponent->SetupAttachment(RootComponent);
	FacingArrowComponent->ArrowColor = FColor::Green;
	FacingArrowComponent->ArrowSize = 1.5f;
	FacingArrowComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));

	// Twin-stick: character rotation independent of movement (AC-2.1.2)
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// Configure CharacterMovementComponent for twin-stick (AC-2.1.1, AC-2.1.6)
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->bOrientRotationToMovement = false;
		CMC->bUseControllerDesiredRotation = true;
		CMC->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
		CMC->MaxWalkSpeed = WalkSpeed;
		CMC->MaxAcceleration = GroundAcceleration;
		CMC->BrakingDecelerationWalking = GroundDeceleration;
		CMC->GravityScale = 1.75f;
		CMC->JumpZVelocity = 600.0f;
		CMC->AirControl = 0.2f;
	}
}

void AMordecaiCharacter::SetSprinting(bool bNewSprinting)
{
	bIsSprinting = bNewSprinting;

	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->MaxWalkSpeed = bIsSprinting ? (WalkSpeed * SprintSpeedMultiplier) : WalkSpeed;
	}
}

// ---------------------------------------------------------------------------
// Death / Respawn (US-053)
// ---------------------------------------------------------------------------

void AMordecaiCharacter::BeginPlay()
{
	Super::BeginPlay();
	BindToASC();
}

void AMordecaiCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	BindToASC();
}

void AMordecaiCharacter::BindToASC()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(
			UMordecaiAttributeSet::GetHealthAttribute()).AddUObject(
				this, &AMordecaiCharacter::OnHealthChanged);
	}
}

void AMordecaiCharacter::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue <= 0.f && !bDeathHandled)
	{
		HandlePlayerDeath();
	}
}

bool AMordecaiCharacter::IsDead() const
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		return ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Dead);
	}
	return bDeathHandled;
}

void AMordecaiCharacter::HandlePlayerDeath()
{
	if (bDeathHandled)
	{
		return;
	}
	bDeathHandled = true;

	// AC-053.1: Apply State_Dead tag and broadcast Event_PlayerDeath
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->AddLooseGameplayTag(MordecaiGameplayTags::State_Dead);

		// AC-053.3: Broadcast Event_PlayerDeath gameplay event
		FGameplayEventData EventData;
		EventData.EventTag = MordecaiGameplayTags::Event_PlayerDeath;
		EventData.Instigator = this;
		EventData.Target = this;
		ASC->HandleGameplayEvent(MordecaiGameplayTags::Event_PlayerDeath, &EventData);
	}

	// AC-053.2: Disable movement
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->StopMovementImmediately();
		CMC->SetMovementMode(MOVE_None);
	}

	// AC-053.2: Disable input on controller
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
		DisableInput(PC);
	}

	// Notify game mode for respawn flow (AC-053.4)
	if (UWorld* World = GetWorld())
	{
		if (AMordecaiGameMode* GM = Cast<AMordecaiGameMode>(World->GetAuthGameMode()))
		{
			GM->NotifyPlayerDeath(this);
		}
	}
}

void AMordecaiCharacter::HandlePlayerRespawn()
{
	bDeathHandled = false;

	// AC-053.5: Remove State_Dead tag and restore attributes
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::State_Dead);

		const UMordecaiAttributeSet* AttrSet = ASC->GetSet<UMordecaiAttributeSet>();
		if (AttrSet)
		{
			ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetHealthAttribute(), AttrSet->GetMaxHealth());
			ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetStaminaAttribute(), AttrSet->GetMaxStamina());
			ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetPostureAttribute(), AttrSet->GetMaxPosture());
		}
	}

	// AC-053.6: Re-enable movement
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->SetMovementMode(MOVE_Walking);
	}

	// AC-053.6: Re-enable input
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
		EnableInput(PC);
	}
}
