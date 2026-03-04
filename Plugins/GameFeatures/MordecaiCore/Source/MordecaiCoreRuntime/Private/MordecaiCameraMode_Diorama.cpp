// Copyright Project Mordecai. All Rights Reserved.

#include "MordecaiCameraMode_Diorama.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiCameraMode_Diorama)

UMordecaiCameraMode_Diorama::UMordecaiCameraMode_Diorama()
{
	PitchAngle = -50.0f;
	YawAngle = 0.0f;
	Distance = 1800.0f;
	FollowSpeed = 8.0f;
	FieldOfView = 60.0f;

	SmoothedPivotLocation = FVector::ZeroVector;
	bSmoothedLocationInitialized = false;
}

void UMordecaiCameraMode_Diorama::UpdateView(float DeltaTime)
{
	// Get the character's location as the follow target.
	const FVector TargetLocation = GetPivotLocation();

	// Initialize smoothed location on first frame or when interpolation is reset.
	if (!bSmoothedLocationInitialized || bResetInterpolation)
	{
		SmoothedPivotLocation = TargetLocation;
		bSmoothedLocationInitialized = true;
	}
	else
	{
		// Smooth follow with positional lag.
		SmoothedPivotLocation = FMath::VInterpTo(SmoothedPivotLocation, TargetLocation, DeltaTime, FollowSpeed);
	}

	// Fixed camera rotation - does NOT read from controller/player input.
	const FRotator CameraRotation(PitchAngle, YawAngle, 0.0f);

	// Place camera at distance behind the smoothed pivot, along the inverse of the camera's forward vector.
	const FVector CameraOffset = -CameraRotation.Vector() * Distance;
	const FVector CameraLocation = SmoothedPivotLocation + CameraOffset;

	// Write to the view output.
	View.Location = CameraLocation;
	View.Rotation = CameraRotation;
	View.ControlRotation = CameraRotation;
	View.FieldOfView = FieldOfView;
}
