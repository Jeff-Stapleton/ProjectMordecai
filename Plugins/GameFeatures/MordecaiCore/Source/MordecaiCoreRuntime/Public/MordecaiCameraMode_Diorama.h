// Copyright Project Mordecai. All Rights Reserved.

#pragma once

#include "Camera/LyraCameraMode.h"

#include "MordecaiCameraMode_Diorama.generated.h"

/**
 * UMordecaiCameraMode_Diorama
 *
 * Fixed-orientation diorama camera that follows the player character.
 * Camera does NOT rotate from player input - it maintains a fixed pitch/yaw
 * and smoothly follows the character with positional lag.
 */
UCLASS(Abstract, Blueprintable)
class UMordecaiCameraMode_Diorama : public ULyraCameraMode
{
	GENERATED_BODY()

public:
	UMordecaiCameraMode_Diorama();

protected:
	//~ ULyraCameraMode interface
	virtual void UpdateView(float DeltaTime) override;
	//~ End ULyraCameraMode interface

protected:
	/** Fixed pitch angle of the camera in degrees (e.g., -50 means looking down at 50 deg). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Diorama", meta = (ClampMin = "-89.0", ClampMax = "0.0"))
	float PitchAngle;

	/** Fixed yaw angle of the camera in degrees. 0 = default north-facing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Diorama")
	float YawAngle;

	/** Distance from the camera to the follow target. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Diorama", meta = (ClampMin = "100.0"))
	float Distance;

	/** Speed at which camera follows the character (higher = less lag). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Diorama", meta = (ClampMin = "0.0"))
	float FollowSpeed;

private:
	/** Smoothed camera location for positional lag. */
	FVector SmoothedPivotLocation;

	/** Whether SmoothedPivotLocation has been initialized. */
	bool bSmoothedLocationInitialized;
};
