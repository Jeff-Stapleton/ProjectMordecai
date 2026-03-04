// Copyright Project Mordecai. All Rights Reserved.

#pragma once

#include "Character/LyraHeroComponent.h"

#include "MordecaiHeroComponent.generated.h"

/**
 * UMordecaiHeroComponent
 *
 * Extends ULyraHeroComponent to implement twin-stick controls for the diorama camera:
 * - Left Stick / WASD: Moves character in world space (fixed camera means up = always north)
 * - Right Stick / Mouse: Rotates character facing direction (independent of movement)
 * - Character movement and facing are fully decoupled
 * - Sprint toggle support
 */
UCLASS(Blueprintable, Meta = (BlueprintSpawnableComponent))
class UMordecaiHeroComponent : public ULyraHeroComponent
{
	GENERATED_BODY()

public:
	UMordecaiHeroComponent(const FObjectInitializer& ObjectInitializer);

protected:
	//~ ULyraHeroComponent interface
	virtual void InitializePlayerInput(UInputComponent* PlayerInputComponent) override;
	//~ End ULyraHeroComponent interface

	/** Twin-stick move: world-space movement, not camera-relative. */
	void Input_MordecaiMove(const FInputActionValue& InputActionValue);

	/** Twin-stick aim via mouse: rotates character facing toward cursor world position. */
	void Input_MordecaiLookMouse(const FInputActionValue& InputActionValue);

	/** Twin-stick aim via stick: rotates character facing in stick direction. */
	void Input_MordecaiLookStick(const FInputActionValue& InputActionValue);

	/** Sprint toggle. */
	void Input_MordecaiSprint(const FInputActionValue& InputActionValue);

protected:
	/** Whether the character is currently sprinting. */
	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Movement")
	bool bIsSprinting;

	/** Walk speed multiplier when sprinting. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Movement", meta = (ClampMin = "1.0"))
	float SprintSpeedMultiplier;

	/** Base walk speed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Movement", meta = (ClampMin = "0.0"))
	float BaseWalkSpeed;

	/** Fixed yaw for world-space movement (should match camera YawAngle). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Movement")
	float FixedWorldYaw;
};
