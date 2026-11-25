// Project Mordecai

#include "MordecaiCameraMode_Diorama.h"
#include "Components/CapsuleComponent.h"
#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiCameraMode_Diorama)

namespace
{
	const FName OcclusionFadeTag(TEXT("Mordecai.OcclusionFade"));
	const FName OcclusionFadeParameter(TEXT("MordecaiOcclusionFade"));
}

UMordecaiCameraMode_Diorama::UMordecaiCameraMode_Diorama()
	: PitchAngle(-50.0f)
	, Distance(FParse::Param(FCommandLine::Get(), TEXT("MordecaiMarketingCapture"))
		? 1000.0f
		: 1800.0f)
	, FollowSpeed(8.0f)
	, OcclusionFadeDuration(0.25f)
	, SmoothedPivotLocation(FVector::ZeroVector)
	, bFirstUpdate(true)
{
	// Fixed diorama camera — no pitch range for player input (AC-1.2.2, AC-1.2.4)
	FieldOfView = 60.0f;
	ViewPitchMin = PitchAngle;
	ViewPitchMax = PitchAngle;
	BlendTime = 0.5f;
}

float UMordecaiCameraMode_Diorama::AdvanceOcclusionFade(
	float CurrentFade, bool bBlockingPlayer, float DeltaTime, float FadeDuration)
{
	const float TargetFade = bBlockingPlayer ? 0.0f : 1.0f;
	const float FadeSpeed = FadeDuration > UE_SMALL_NUMBER ? 1.0f / FadeDuration : BIG_NUMBER;
	return FMath::FInterpConstantTo(CurrentFade, TargetFade, DeltaTime, FadeSpeed);
}

bool UMordecaiCameraMode_Diorama::IsOcclusionFadeCandidate(const UPrimitiveComponent* Primitive)
{
	return Primitive
		&& Primitive->GetCollisionObjectType() == ECC_WorldStatic
		&& Primitive->ComponentHasTag(OcclusionFadeTag)
		&& Primitive->IsA<UMeshComponent>();
}

void UMordecaiCameraMode_Diorama::OnDeactivation()
{
	RestoreOccluders();
	Super::OnDeactivation();
}

void UMordecaiCameraMode_Diorama::RestoreOccluders()
{
	for (const TPair<TWeakObjectPtr<UMeshComponent>, float>& Entry : OccluderFadeProgress)
	{
		if (UMeshComponent* Mesh = Entry.Key.Get())
		{
			Mesh->SetScalarParameterValueOnMaterials(OcclusionFadeParameter, 1.0f);
		}
	}
	OccluderFadeProgress.Reset();
}

void UMordecaiCameraMode_Diorama::UpdateOcclusionFades(float DeltaTime, AActor* TargetActor)
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer || !TargetActor)
	{
		RestoreOccluders();
		return;
	}

	float HalfHeight = 0.0f;
	if (const ACharacter* Character = Cast<ACharacter>(TargetActor))
	{
		if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			HalfHeight = Capsule->GetScaledCapsuleHalfHeight() * 0.9f;
		}
	}

	const FVector TargetCenter = TargetActor->GetActorLocation();
	const FVector TraceTargets[] = {
		TargetCenter + FVector::UpVector * HalfHeight,
		TargetCenter,
		TargetCenter - FVector::UpVector * HalfHeight
	};
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MordecaiCameraOcclusion), false, TargetActor);
	TSet<UMeshComponent*> BlockingOccluders;
	for (const FVector& TraceTarget : TraceTargets)
	{
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, View.Location, TraceTarget, ECC_Visibility, QueryParams))
		{
			if (UMeshComponent* Mesh = Cast<UMeshComponent>(Hit.GetComponent());
				IsOcclusionFadeCandidate(Mesh))
			{
				BlockingOccluders.Add(Mesh);
				OccluderFadeProgress.FindOrAdd(Mesh, 1.0f);
			}
		}
	}

	for (auto It = OccluderFadeProgress.CreateIterator(); It; ++It)
	{
		UMeshComponent* Mesh = It.Key().Get();
		if (!Mesh)
		{
			It.RemoveCurrent();
			continue;
		}

		const bool bBlockingPlayer = BlockingOccluders.Contains(Mesh);
		const float Progress = AdvanceOcclusionFade(
			It.Value(), bBlockingPlayer, DeltaTime, OcclusionFadeDuration);
		It.Value() = Progress;
		const float EasedOpacity = FMath::InterpEaseInOut(0.0f, 1.0f, Progress, 2.0f);
		Mesh->SetScalarParameterValueOnMaterials(OcclusionFadeParameter, EasedOpacity);
		if (!bBlockingPlayer && FMath::IsNearlyEqual(Progress, 1.0f))
		{
			It.RemoveCurrent();
		}
	}
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

	UpdateOcclusionFades(DeltaTime, TargetActor);
}
