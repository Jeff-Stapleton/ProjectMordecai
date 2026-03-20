// Project Mordecai

#include "MordecaiCameraMode_Diorama.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiCameraMode_Diorama)

UMordecaiCameraMode_Diorama::UMordecaiCameraMode_Diorama()
	: PitchAngle(-50.0f)
	, Distance(1800.0f)
	, FollowSpeed(8.0f)
	, SmoothedPivotLocation(FVector::ZeroVector)
	, bFirstUpdate(true)
{
	// Fixed diorama camera — no pitch range for player input (AC-1.2.2, AC-1.2.4)
	FieldOfView = 60.0f;
	ViewPitchMin = PitchAngle;
	ViewPitchMax = PitchAngle;
	BlendTime = 0.5f;
}

void UMordecaiCameraMode_Diorama::UpdateView(float DeltaTime)
{
	AActor* TargetActor = GetTargetActor();
	if (!TargetActor)
	{
		return;
	}

	const FVector TargetLocation = TargetActor->GetActorLocation();

	// Smooth follow (AC-1.2.1)
	if (bFirstUpdate)
	{
		SmoothedPivotLocation = TargetLocation;
		bFirstUpdate = false;
	}
	else
	{
		SmoothedPivotLocation = FMath::VInterpTo(SmoothedPivotLocation, TargetLocation, DeltaTime, FollowSpeed);
	}

	// Fixed rotation — camera NEVER rotates (AC-1.2.2)
	const FRotator FixedRotation(PitchAngle, 0.0f, 0.0f);

	// Calculate camera position offset from target using fixed pitch and distance
	const FVector Offset = FixedRotation.Vector() * -Distance;

	View.Location = SmoothedPivotLocation + Offset;
	View.Rotation = FixedRotation;
	View.ControlRotation = FixedRotation;
	View.FieldOfView = FieldOfView;
}
