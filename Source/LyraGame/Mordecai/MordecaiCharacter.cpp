// Project Mordecai

#include "MordecaiCharacter.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

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
