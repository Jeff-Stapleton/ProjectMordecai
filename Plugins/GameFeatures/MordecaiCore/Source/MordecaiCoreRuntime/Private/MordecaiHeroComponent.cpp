// Copyright Project Mordecai. All Rights Reserved.

#include "MordecaiHeroComponent.h"

#include "EnhancedInputSubsystems.h"
#include "Character/LyraPawnExtensionComponent.h"
#include "Character/LyraPawnData.h"
#include "Input/LyraInputConfig.h"
#include "Input/LyraInputComponent.h"
#include "Camera/LyraCameraComponent.h"
#include "Player/LyraLocalPlayer.h"
#include "Player/LyraPlayerController.h"
#include "LyraGameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputMappingContext.h"
#include "Components/GameFrameworkComponentManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiHeroComponent)

UMordecaiHeroComponent::UMordecaiHeroComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsSprinting = false;
	SprintSpeedMultiplier = 1.6f;
	BaseWalkSpeed = 600.0f;
	FixedWorldYaw = 0.0f;
}

void UMordecaiHeroComponent::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const ULocalPlayer* LP = PC->GetLocalPlayer();
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	Subsystem->ClearAllMappings();

	if (const ULyraPawnExtensionComponent* PawnExtComp = ULyraPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const ULyraPawnData* PawnData = PawnExtComp->GetPawnData<ULyraPawnData>())
		{
			if (const ULyraInputConfig* InputConfig = PawnData->InputConfig)
			{
				// Add default input mapping contexts.
				for (const FInputMappingContextAndPriority& Mapping : DefaultInputMappings)
				{
					if (UInputMappingContext* IMC = Mapping.InputMapping.LoadSynchronous())
					{
						if (Mapping.bRegisterWithSettings)
						{
							if (UEnhancedInputUserSettings* Settings = Subsystem->GetUserSettings())
							{
								Settings->RegisterInputMappingContext(IMC);
							}

							FModifyContextOptions Options = {};
							Options.bIgnoreAllPressedKeysUntilRelease = false;
							Subsystem->AddMappingContext(IMC, Mapping.Priority, Options);
						}
					}
				}

				ULyraInputComponent* LyraIC = Cast<ULyraInputComponent>(PlayerInputComponent);
				if (ensureMsgf(LyraIC, TEXT("Unexpected Input Component class! Expected ULyraInputComponent.")))
				{
					LyraIC->AddInputMappings(InputConfig, Subsystem);

					// Bind ability actions (combat stubs, etc.)
					TArray<uint32> BindHandles;
					LyraIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);

					// Bind twin-stick movement: world-space move instead of camera-relative.
					LyraIC->BindNativeAction(InputConfig, LyraGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_MordecaiMove, /*bLogIfNotFound=*/ false);

					// Bind twin-stick aim: rotate character facing, NOT the camera.
					LyraIC->BindNativeAction(InputConfig, LyraGameplayTags::InputTag_Look_Mouse, ETriggerEvent::Triggered, this, &ThisClass::Input_MordecaiLookMouse, /*bLogIfNotFound=*/ false);
					LyraIC->BindNativeAction(InputConfig, LyraGameplayTags::InputTag_Look_Stick, ETriggerEvent::Triggered, this, &ThisClass::Input_MordecaiLookStick, /*bLogIfNotFound=*/ false);

					// Bind sprint (uses Crouch tag as placeholder until we have a dedicated Sprint input tag).
					// TODO(DECISION): Create a dedicated InputTag_Sprint gameplay tag or reuse an existing one.
					// For now sprint is bound via the Crouch tag slot; remap once IMC_Mordecai is created in-editor.
					LyraIC->BindNativeAction(InputConfig, LyraGameplayTags::InputTag_Crouch, ETriggerEvent::Triggered, this, &ThisClass::Input_MordecaiSprint, /*bLogIfNotFound=*/ false);
				}
			}
		}
	}

	if (ensure(!bReadyToBindInputs))
	{
		bReadyToBindInputs = true;
	}

	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APlayerController*>(PC), NAME_BindInputsNow);
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APawn*>(Pawn), NAME_BindInputsNow);
}

void UMordecaiHeroComponent::Input_MordecaiMove(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	const FVector2D Value = InputActionValue.Get<FVector2D>();

	// World-space movement: use fixed yaw (matching camera orientation) so
	// "up" on the stick always means the same world direction.
	const FRotator WorldRotation(0.0f, FixedWorldYaw, 0.0f);

	if (Value.X != 0.0f)
	{
		const FVector RightDirection = WorldRotation.RotateVector(FVector::RightVector);
		Pawn->AddMovementInput(RightDirection, Value.X);
	}

	if (Value.Y != 0.0f)
	{
		const FVector ForwardDirection = WorldRotation.RotateVector(FVector::ForwardVector);
		Pawn->AddMovementInput(ForwardDirection, Value.Y);
	}
}

void UMordecaiHeroComponent::Input_MordecaiLookMouse(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	// For mouse aim in a diorama camera, the mouse delta drives character yaw rotation.
	// A full cursor-to-world-position aim system requires a line trace from screen to ground plane,
	// which needs the player controller and camera. This is a simplified version using mouse delta.
	// TODO(DECISION): Implement full cursor-to-ground-plane aiming via line trace for mouse aim.
	const FVector2D Value = InputActionValue.Get<FVector2D>();

	if (Value.X != 0.0f)
	{
		// Rotate character facing based on mouse X delta.
		// We set the pawn's rotation directly since the camera is fixed.
		FRotator CurrentRotation = Pawn->GetActorRotation();
		CurrentRotation.Yaw += Value.X;
		Pawn->SetActorRotation(CurrentRotation);
	}
}

void UMordecaiHeroComponent::Input_MordecaiLookStick(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	const FVector2D Value = InputActionValue.Get<FVector2D>();

	// Only rotate if stick is deflected enough (dead zone should be handled by Enhanced Input).
	if (Value.SizeSquared() > 0.01f)
	{
		// Convert stick direction to world-space yaw angle.
		// Stick X = right, Stick Y = forward, relative to fixed camera orientation.
		const float TargetYaw = FMath::RadiansToDegrees(FMath::Atan2(Value.X, Value.Y)) + FixedWorldYaw;
		const FRotator TargetRotation(0.0f, TargetYaw, 0.0f);

		// Smoothly rotate toward the target facing direction.
		const FRotator CurrentRotation = Pawn->GetActorRotation();
		const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, GetWorld()->GetDeltaSeconds(), 15.0f);
		Pawn->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
	}
}

void UMordecaiHeroComponent::Input_MordecaiSprint(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	// Toggle sprint.
	bIsSprinting = !bIsSprinting;

	// Apply speed change to CharacterMovementComponent.
	if (ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		if (UCharacterMovementComponent* CMC = Character->GetCharacterMovement())
		{
			CMC->MaxWalkSpeed = bIsSprinting ? BaseWalkSpeed * SprintSpeedMultiplier : BaseWalkSpeed;
		}
	}
}
