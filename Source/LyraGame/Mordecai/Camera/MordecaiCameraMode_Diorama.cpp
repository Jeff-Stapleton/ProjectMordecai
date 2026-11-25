// Project Mordecai

#include "MordecaiCameraMode_Diorama.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiCameraMode_Diorama)

namespace
{
	const FName OcclusionFadeTag(TEXT("Mordecai.OcclusionFade"));
	const FName OcclusionFadeParameter(TEXT("MordecaiOcclusionFade"));
	constexpr int32 MaxOcclusionHitsPerRay = 4;
}

DEFINE_LOG_CATEGORY_STATIC(LogMordecaiCamera, Log, All);

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

#if !UE_BUILD_SHIPPING
	float MarketingFadeDuration = 0.0f;
	if (FParse::Param(FCommandLine::Get(), TEXT("MordecaiMarketingCapture"))
		&& FParse::Value(
			FCommandLine::Get(),
			TEXT("MordecaiMarketingFadeDuration="),
			MarketingFadeDuration))
	{
		// Dedicated non-acceptance still-capture override. The continuous marketing clip and
		// normal gameplay omit this flag and retain the production 0.25-second transition.
		OcclusionFadeDuration = FMath::Clamp(MarketingFadeDuration, 0.05f, 5.0f);
	}
#endif
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
		&& Primitive->IsA<UMeshComponent>()
		&& !Primitive->IsA<UInstancedStaticMeshComponent>();
}

void UMordecaiCameraMode_Diorama::OnDeactivation()
{
	if (AActor* TargetActor = OcclusionTarget.Get())
	{
		TargetActor->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleOcclusionTargetEndPlay);
		TargetActor->OnDestroyed.RemoveDynamic(this, &ThisClass::HandleOcclusionTargetDestroyed);
	}
	OcclusionTarget.Reset();
	RestoreOccluders();
	Super::OnDeactivation();
}

void UMordecaiCameraMode_Diorama::RestoreOccluders()
{
	for (TPair<TWeakObjectPtr<UMeshComponent>, FMordecaiOccluderFadeState>& Entry : OccluderFadeStates)
	{
		ApplyOccluderOpacity(Entry.Value, 1.0f);
	}
	OccluderFadeStates.Reset();
}

void UMordecaiCameraMode_Diorama::SetOcclusionTarget(AActor* TargetActor)
{
	if (OcclusionTarget.Get() == TargetActor)
	{
		return;
	}

	if (AActor* PreviousTarget = OcclusionTarget.Get())
	{
		PreviousTarget->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleOcclusionTargetEndPlay);
		PreviousTarget->OnDestroyed.RemoveDynamic(this, &ThisClass::HandleOcclusionTargetDestroyed);
	}
	RestoreOccluders();
	OcclusionTarget = TargetActor;
	if (TargetActor)
	{
		TargetActor->OnEndPlay.AddUniqueDynamic(this, &ThisClass::HandleOcclusionTargetEndPlay);
		TargetActor->OnDestroyed.AddUniqueDynamic(this, &ThisClass::HandleOcclusionTargetDestroyed);
	}
}

void UMordecaiCameraMode_Diorama::HandleOcclusionTargetEndPlay(
	AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	if (Actor == OcclusionTarget.Get())
	{
		RestoreOccluders();
		OcclusionTarget.Reset();
	}
}

void UMordecaiCameraMode_Diorama::HandleOcclusionTargetDestroyed(AActor* Actor)
{
	if (Actor == OcclusionTarget.Get())
	{
		RestoreOccluders();
		OcclusionTarget.Reset();
	}
}

bool UMordecaiCameraMode_Diorama::CacheOccluderMaterials(
	UMeshComponent* Mesh, FMordecaiOccluderFadeState& OutState)
{
	if (!Mesh)
	{
		return false;
	}

	const FHashedMaterialParameterInfo ParameterInfo(OcclusionFadeParameter);
	for (int32 MaterialIndex = 0; MaterialIndex < Mesh->GetNumMaterials(); ++MaterialIndex)
	{
		UMaterialInterface* Material = Mesh->GetMaterial(MaterialIndex);
		float CurrentOpacity = 1.0f;
		if (!Material || !Material->GetScalarParameterValue(ParameterInfo, CurrentOpacity))
		{
			continue;
		}

		UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(Material);
		if (!DynamicMaterial)
		{
			DynamicMaterial = Mesh->CreateDynamicMaterialInstance(MaterialIndex, Material);
		}
		if (DynamicMaterial)
		{
			OutState.Materials.Add(DynamicMaterial);
			OutState.LastAppliedOpacity = CurrentOpacity;
		}
	}

	if (OutState.Materials.IsEmpty())
	{
		const TWeakObjectPtr<UMeshComponent> WeakMesh(Mesh);
		if (!WarnedInvalidOccluders.Contains(WeakMesh))
		{
			WarnedInvalidOccluders.Add(WeakMesh);
			UE_LOG(LogMordecaiCamera, Warning,
				TEXT("Tagged occlusion candidate %s on %s has no material slot with scalar parameter %s"),
				*GetNameSafe(Mesh), *GetNameSafe(Mesh->GetOwner()), *OcclusionFadeParameter.ToString());
		}
		return false;
	}
	return true;
}

void UMordecaiCameraMode_Diorama::ApplyOccluderOpacity(
	FMordecaiOccluderFadeState& State, float Opacity)
{
	if (FMath::IsNearlyEqual(State.LastAppliedOpacity, Opacity))
	{
		return;
	}
	for (const TWeakObjectPtr<UMaterialInstanceDynamic>& Material : State.Materials)
	{
		if (UMaterialInstanceDynamic* DynamicMaterial = Material.Get())
		{
			DynamicMaterial->SetScalarParameterValue(OcclusionFadeParameter, Opacity);
		}
	}
	State.LastAppliedOpacity = Opacity;
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
	TArray<FVector, TInlineAllocator<3>> TraceTargets;
	TraceTargets.Add(TargetCenter + FVector::UpVector * HalfHeight);
	TraceTargets.Add(TargetCenter);
	TraceTargets.Add(TargetCenter - FVector::UpVector * HalfHeight);
	const FCollisionQueryParams BaseQueryParams(
		SCENE_QUERY_STAT(MordecaiCameraOcclusion), false, TargetActor);
	TSet<UMeshComponent*, DefaultKeyFuncs<UMeshComponent*>, TInlineSetAllocator<8>>
		BlockingOccluders;
	for (const FVector& TraceTarget : TraceTargets)
	{
		FCollisionQueryParams QueryParams(BaseQueryParams);
		for (int32 HitIndex = 0; HitIndex < MaxOcclusionHitsPerRay; ++HitIndex)
		{
			FHitResult Hit;
			if (!World->LineTraceSingleByChannel(
					Hit, View.Location, TraceTarget, ECC_Visibility, QueryParams))
			{
				break;
			}

			UPrimitiveComponent* HitComponent = Hit.GetComponent();
			if (!HitComponent)
			{
				break;
			}
			if (UMeshComponent* Mesh = Cast<UMeshComponent>(HitComponent);
				IsOcclusionFadeCandidate(Mesh))
			{
				FMordecaiOccluderFadeState* State = OccluderFadeStates.Find(Mesh);
				if (!State)
				{
					FMordecaiOccluderFadeState NewState;
					if (CacheOccluderMaterials(Mesh, NewState))
					{
						State = &OccluderFadeStates.Add(Mesh, MoveTemp(NewState));
					}
				}
				if (State)
				{
					BlockingOccluders.Add(Mesh);
				}
			}
			QueryParams.AddIgnoredComponent(HitComponent);
		}
	}

	for (auto It = OccluderFadeStates.CreateIterator(); It; ++It)
	{
		UMeshComponent* Mesh = It.Key().Get();
		if (!Mesh)
		{
			It.RemoveCurrent();
			continue;
		}

		const bool bBlockingPlayer = BlockingOccluders.Contains(Mesh);
		const float Progress = AdvanceOcclusionFade(
			It.Value().Progress, bBlockingPlayer, DeltaTime, OcclusionFadeDuration);
		It.Value().Progress = Progress;
		const float EasedOpacity = FMath::InterpEaseInOut(0.0f, 1.0f, Progress, 2.0f);
		ApplyOccluderOpacity(It.Value(), EasedOpacity);
		if (!bBlockingPlayer && FMath::IsNearlyEqual(Progress, 1.0f))
		{
			It.RemoveCurrent();
		}
	}
}

void UMordecaiCameraMode_Diorama::UpdateView(float DeltaTime)
{
	AActor* TargetActor = GetTargetActor();
	SetOcclusionTarget(TargetActor);
	if (!TargetActor)
	{
		RestoreOccluders();
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
