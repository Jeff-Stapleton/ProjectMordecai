// Project Mordecai

#include "MordecaiCharacter.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/MordecaiGameMode.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Mordecai/MordecaiHeroComponent.h"
#include "Mordecai/Camera/MordecaiCameraMode_Diorama.h"
#include "Camera/LyraCameraComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiCharacter)

AMordecaiCharacter::AMordecaiCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, WalkSpeed(600.0f)
	, SprintSpeedMultiplier(1.5f)
	, GroundAcceleration(2048.0f)
	, GroundDeceleration(2048.0f)
	, bIsSprinting(false)
{
	// Hero component: binds camera mode from PawnData and initializes input
	// Uses MordecaiHeroComponent to set DefaultInputMappings with IMC_Mordecai
	HeroComponent = CreateDefaultSubobject<UMordecaiHeroComponent>(TEXT("HeroComponent"));

	// Arrow component for facing direction visualization (AC-2.1.5)
	FacingArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("FacingArrow"));
	FacingArrowComponent->SetupAttachment(RootComponent);
	FacingArrowComponent->ArrowColor = FColor::Green;
	FacingArrowComponent->ArrowSize = 1.5f;
	FacingArrowComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));

	// Placeholder visual mesh — cylinder body + sphere head (replace with skeletal mesh when art is ready)
	PlaceholderBodyComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
	PlaceholderBodyComponent->SetupAttachment(RootComponent);
	PlaceholderBodyComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PlaceholderHeadComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderHead"));
	PlaceholderHeadComponent->SetupAttachment(RootComponent);
	PlaceholderHeadComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	if (CylinderFinder.Succeeded())
	{
		PlaceholderBodyComponent->SetStaticMesh(CylinderFinder.Object);
		// Engine cylinder is 100 units radius, 100 units half-height by default
		// Scale to match capsule: ~35 radius, ~70 half-height (body portion below head)
		PlaceholderBodyComponent->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.4f));
		PlaceholderBodyComponent->SetRelativeLocation(FVector(0.f, 0.f, -20.f));
	}

	if (SphereFinder.Succeeded())
	{
		PlaceholderHeadComponent->SetStaticMesh(SphereFinder.Object);
		// Sphere head on top of body
		PlaceholderHeadComponent->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
		PlaceholderHeadComponent->SetRelativeLocation(FVector(0.f, 0.f, 65.f));
	}

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

	// Override camera delegate to always use diorama camera (permanent, no asset dependency)
	if (ULyraCameraComponent* CameraComp = FindComponentByClass<ULyraCameraComponent>())
	{
		CameraComp->DetermineCameraModeDelegate.BindUObject(this, &AMordecaiCharacter::DetermineDioramaCameraMode);
	}
}

TSubclassOf<ULyraCameraMode> AMordecaiCharacter::DetermineDioramaCameraMode() const
{
	return UMordecaiCameraMode_Diorama::StaticClass();
}

void AMordecaiCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	BindToASC();
}

void AMordecaiCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UE_LOG(LogTemp, Warning, TEXT("MORDECAI: SetupPlayerInputComponent called. PlayerInputComponent=%s, Controller=%s"),
		PlayerInputComponent ? *PlayerInputComponent->GetClass()->GetName() : TEXT("NULL"),
		GetController() ? *GetController()->GetClass()->GetName() : TEXT("NULL"));

	APlayerController* PC = Cast<APlayerController>(GetController());

	// Ensure IMC_Mordecai is added and movement input is bound.
	if (HeroComponent && PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("MORDECAI: Calling EnsureMordecaiInputBound"));
		HeroComponent->EnsureMordecaiInputBound(PC);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MORDECAI: Cannot bind input! HeroComponent=%s, PC=%s"),
			HeroComponent ? TEXT("valid") : TEXT("NULL"),
			PC ? TEXT("valid") : TEXT("NULL"));
	}
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
