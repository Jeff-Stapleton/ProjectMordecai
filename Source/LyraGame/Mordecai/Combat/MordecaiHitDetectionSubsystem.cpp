// Project Mordecai — Melee Hit Detection Subsystem (US-003)

#include "Mordecai/Combat/MordecaiHitDetectionSubsystem.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GenericTeamAgentInterface.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"
#include "HAL/IConsoleManager.h"

#if UE_ENABLE_DEBUG_DRAWING
#include "DrawDebugHelpers.h"
#endif

namespace
{
	constexpr int32 ArcDegreesPerSegment = 10;

#if UE_ENABLE_DEBUG_DRAWING
	// Issue #92: this is intentionally the single obvious default to flip when the overlay
	// should become opt-in.
	static int32 DrawMeleeHitShapes = 1;
	static FAutoConsoleVariableRef CVarDrawMeleeHitShapes(
		TEXT("Mordecai.Debug.DrawMeleeHitShapes"),
		DrawMeleeHitShapes,
		TEXT("Draw melee hit-query shapes (0 = off, 1 = on)."),
		ECVF_Default);

	static float DrawMeleeHitShapesDuration = 1.0f;
	static FAutoConsoleVariableRef CVarDrawMeleeHitShapesDuration(
		TEXT("Mordecai.Debug.DrawMeleeHitShapesDuration"),
		DrawMeleeHitShapesDuration,
		TEXT("Lifetime in seconds for melee hit-query debug shapes."),
		ECVF_Default);

	constexpr int32 CircleSegments = 48;
	constexpr float ShapeThickness = 2.f;
	constexpr float HitPointSize = 28.f;
	constexpr float HitBoxHalfExtent = 18.f;
	constexpr float HitMarkerMargin = 15.f;

	void DrawHitMarkers(
		const UWorld* World,
		const TArray<FMordecaiHitResult>& Results,
		const FColor& Color)
	{
		for (const FMordecaiHitResult& Result : Results)
		{
			const AActor* HitActor = Result.HitActor.Get();
			if (!UMordecaiHitDetectionSubsystem::IsValidDebugHitTarget(HitActor))
			{
				continue;
			}

			FVector BoundsOrigin = Result.HitLocation;
			FVector BoundsExtent = FVector::ZeroVector;
			HitActor->GetActorBounds(
				/*bOnlyCollidingComponents=*/true,
				BoundsOrigin,
				BoundsExtent,
				/*bIncludeFromChildActors=*/false);
			const FVector MarkerLocation =
				UMordecaiHitDetectionSubsystem::BuildDebugHitMarkerLocation(
					Result.HitLocation, BoundsOrigin, BoundsExtent, HitMarkerMargin);
			DrawDebugPoint(
				World,
				MarkerLocation,
				HitPointSize,
				Color,
				false,
				DrawMeleeHitShapesDuration,
				SDPG_Foreground);
			DrawDebugBox(
				World,
				MarkerLocation,
				FVector(HitBoxHalfExtent),
				Color,
				false,
				DrawMeleeHitShapesDuration,
				SDPG_Foreground,
				ShapeThickness);
		}
	}

	bool HasValidDebugHitTarget(const TArray<FMordecaiHitResult>& Results)
	{
		return Results.ContainsByPredicate([](const FMordecaiHitResult& Result)
		{
			return UMordecaiHitDetectionSubsystem::IsValidDebugHitTarget(Result.HitActor.Get());
		});
	}
#endif
}

// ---------------------------------------------------------------------------
// Static Geometry Helpers
// ---------------------------------------------------------------------------

bool UMordecaiHitDetectionSubsystem::IsPointInArcSector(
	const FVector& Origin,
	const FVector& Forward,
	const FVector& Point,
	float Radius,
	float AngleDegrees,
	float StartAngleOffsetDeg)
{
	FVector ToPoint = Point - Origin;
	ToPoint.Z = 0.f;

	const float DistSq = ToPoint.SizeSquared();
	if (DistSq > Radius * Radius)
	{
		return false;
	}
	if (DistSq < SMALL_NUMBER)
	{
		return true;
	}

	const FVector DirToTarget = ToPoint.GetSafeNormal();

	// Apply start angle offset to forward direction (rotate around Up axis)
	FVector ArcCenter = Forward;
	ArcCenter.Z = 0.f;
	if (!FMath::IsNearlyZero(StartAngleOffsetDeg))
	{
		ArcCenter = ArcCenter.RotateAngleAxis(StartAngleOffsetDeg, FVector::UpVector);
	}
	ArcCenter.Normalize();

	const float HalfAngleRad = FMath::DegreesToRadians(AngleDegrees * 0.5f);
	const float DotProduct = FVector::DotProduct(ArcCenter, DirToTarget);
	const float AngleRad = FMath::Acos(FMath::Clamp(DotProduct, -1.f, 1.f));

	return AngleRad <= HalfAngleRad;
}

bool UMordecaiHitDetectionSubsystem::IsPointInCapsuleLane(
	const FVector& Origin,
	const FVector& Forward,
	const FVector& Point,
	float Length,
	float Width)
{
	FVector ToPoint = Point - Origin;
	ToPoint.Z = 0.f;

	FVector Forward2D = Forward;
	Forward2D.Z = 0.f;
	Forward2D.Normalize();

	// Project onto forward direction — must be within [0, Length]
	const float ForwardDist = FVector::DotProduct(ToPoint, Forward2D);
	if (ForwardDist < 0.f || ForwardDist > Length)
	{
		return false;
	}

	// Lateral distance from center line — must be within half width
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward2D);
	const float LateralDist = FMath::Abs(FVector::DotProduct(ToPoint, Right));
	const float HalfWidth = Width * 0.5f;

	return LateralDist <= HalfWidth;
}

bool UMordecaiHitDetectionSubsystem::IsPointInCircle(
	const FVector& Center,
	const FVector& Point,
	float Radius)
{
	FVector Delta = Point - Center;
	Delta.Z = 0.f;
	return Delta.SizeSquared() <= Radius * Radius;
}

FMordecaiDebugArcGeometry UMordecaiHitDetectionSubsystem::BuildDebugArcGeometry(
	const FVector& Origin,
	const FVector& Forward,
	float Radius,
	float AngleDegrees,
	float StartAngleOffsetDeg)
{
	FMordecaiDebugArcGeometry Geometry;
	Geometry.Origin = Origin;

	FVector CenterDirection = Forward;
	CenterDirection.Z = 0.f;
	CenterDirection = CenterDirection.GetSafeNormal();
	CenterDirection = CenterDirection.RotateAngleAxis(StartAngleOffsetDeg, FVector::UpVector);

	const int32 SegmentCount = FMath::Max(
		1,
		FMath::CeilToInt(FMath::Abs(AngleDegrees) / static_cast<float>(ArcDegreesPerSegment)));
	Geometry.ArcPoints.Reserve(SegmentCount + 1);
	for (int32 Index = 0; Index <= SegmentCount; ++Index)
	{
		const float Alpha = static_cast<float>(Index) / static_cast<float>(SegmentCount);
		const float Angle = FMath::Lerp(-AngleDegrees * 0.5f, AngleDegrees * 0.5f, Alpha);
		Geometry.ArcPoints.Add(
			Origin + CenterDirection.RotateAngleAxis(Angle, FVector::UpVector) * Radius);
	}
	Geometry.StartEdge = Geometry.ArcPoints[0];
	Geometry.EndEdge = Geometry.ArcPoints.Last();
	return Geometry;
}

FMordecaiDebugCapsuleGeometry UMordecaiHitDetectionSubsystem::BuildDebugCapsuleGeometry(
	const FVector& Origin,
	const FVector& Forward,
	float Length,
	float Width)
{
	FMordecaiDebugCapsuleGeometry Geometry;
	const FVector Direction = Forward.GetSafeNormal();
	FVector Right = FVector::CrossProduct(FVector::UpVector, Direction).GetSafeNormal();
	if (Right.IsNearlyZero())
	{
		Right = FVector::RightVector;
	}

	Geometry.SegmentStart = Origin;
	Geometry.SegmentEnd = Origin + Direction * Length;
	Geometry.CapRadius = Width * 0.5f;
	Geometry.StartLeft = Geometry.SegmentStart - Right * Geometry.CapRadius;
	Geometry.StartRight = Geometry.SegmentStart + Right * Geometry.CapRadius;
	Geometry.EndLeft = Geometry.SegmentEnd - Right * Geometry.CapRadius;
	Geometry.EndRight = Geometry.SegmentEnd + Right * Geometry.CapRadius;
	return Geometry;
}

FMordecaiDebugCircleGeometry UMordecaiHitDetectionSubsystem::BuildDebugCircleGeometry(
	const FVector& Origin,
	const FVector& Forward,
	float Length,
	float Radius)
{
	FMordecaiDebugCircleGeometry Geometry;
	Geometry.Center = Origin + Forward.GetSafeNormal() * Length;
	Geometry.Radius = Radius;
	return Geometry;
}

FVector UMordecaiHitDetectionSubsystem::BuildDebugHitMarkerLocation(
	const FVector& HitLocation,
	const FVector& BoundsOrigin,
	const FVector& BoundsExtent,
	float Margin)
{
	return FVector(
		BoundsOrigin.X,
		BoundsOrigin.Y,
		FMath::Max(HitLocation.Z, BoundsOrigin.Z + FMath::Max(0.f, BoundsExtent.Z)) + Margin);
}

#if UE_ENABLE_DEBUG_DRAWING
bool UMordecaiHitDetectionSubsystem::IsValidDebugHitTarget(const AActor* Target)
{
	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target) != nullptr;
}
#endif

bool UMordecaiHitDetectionSubsystem::ShouldDrawMeleeHitShapes()
{
#if UE_ENABLE_DEBUG_DRAWING
	return DrawMeleeHitShapes != 0;
#else
	return false;
#endif
}

// ---------------------------------------------------------------------------
// Static Filtering Helpers
// ---------------------------------------------------------------------------

bool UMordecaiHitDetectionSubsystem::ShouldExcludeTarget(
	bool bIsAttacker,
	bool bIsFriendly,
	bool bTargetIsAirborne,
	bool bJumpAvoidable,
	bool bHitsAirborne,
	bool bAllowFriendlyFire)
{
	// Always exclude the attacker from their own results (AC-003.6)
	if (bIsAttacker)
	{
		return true;
	}

	// Friendly fire check (AC-003.9)
	if (bIsFriendly && !bAllowFriendlyFire)
	{
		return true;
	}

	// Airborne interaction (AC-003.7, AC-003.8)
	// JumpAvoidable attacks skip airborne targets unless HitsAirborne overrides
	if (bTargetIsAirborne && bJumpAvoidable && !bHitsAirborne)
	{
		return true;
	}

	return false;
}

bool UMordecaiHitDetectionSubsystem::IsTargetAirborne(const AActor* Target)
{
	if (!Target)
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!ASC)
	{
		return false;
	}

	return ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Airborne);
}

void UMordecaiHitDetectionSubsystem::SyncAirborneTag(UAbilitySystemComponent* ASC, const ACharacter* Character)
{
	if (!ASC)
	{
		return;
	}
	const UCharacterMovementComponent* CMC = Character ? Character->GetCharacterMovement() : nullptr;
	// TODO(DECISION): any fall counts, not only a jump; a dodge's launch is a few Falling frames inside its i-frames.
	ASC->SetLooseGameplayTagCount(MordecaiGameplayTags::State_Airborne, CMC && CMC->IsFalling() ? 1 : 0);
}

bool UMordecaiHitDetectionSubsystem::AreActorsFriendly(const AActor* Attacker, const AActor* Target)
{
	if (!Attacker || !Target)
	{
		return false;
	}

	// Issue #12 (N7): the Mordecai.Team.Enemy tag decides whenever an enemy is involved. Mordecai experiences create
	// no Lyra teams, so players and enemies all carry FGenericTeamId::NoTeam, and comparing the ids made them friends.
	const UAbilitySystemComponent* AttackerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Attacker);
	const UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	const bool bAttackerIsEnemy = AttackerASC && AttackerASC->HasMatchingGameplayTag(MordecaiGameplayTags::Team_Enemy);
	const bool bTargetIsEnemy = TargetASC && TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::Team_Enemy);
	if (bAttackerIsEnemy || bTargetIsEnemy)
	{
		return bAttackerIsEnemy == bTargetIsEnemy;
	}

	const IGenericTeamAgentInterface* AttackerTeam = Cast<IGenericTeamAgentInterface>(Attacker);
	const IGenericTeamAgentInterface* TargetTeam = Cast<IGenericTeamAgentInterface>(Target);

	// Neither is an enemy (e.g. two co-op players, both NoTeam): same team id, same side
	if (AttackerTeam && TargetTeam)
	{
		return AttackerTeam->GetGenericTeamId() == TargetTeam->GetGenericTeamId();
	}

	// If either doesn't implement the interface, treat as not friendly (safe default)
	return false;
}

// ---------------------------------------------------------------------------
// Shape Queries (AC-003.1)
// ---------------------------------------------------------------------------

TArray<FMordecaiHitResult> UMordecaiHitDetectionSubsystem::ArcSectorOverlapQuery(
	const FVector& Origin,
	const FVector& Forward,
	float Radius,
	float AngleDegrees,
	float StartAngleOffset,
	AActor* Attacker,
	EMordecaiTargetFilter Filter,
	bool bAllowFriendlyFire,
	bool bJumpAvoidable,
	bool bHitsAirborne)
{
	TArray<FMordecaiHitResult> Results;
	UWorld* World = GetWorld();
	if (!World)
	{
		return Results;
	}

	// AC-003.10: Use UE collision for the broad-phase sphere overlap
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MordecaiArcSector), false, Attacker);
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(
		Overlaps,
		Origin,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(Radius),
		QueryParams);

	// Filter by angle (arc sector shape) and combat rules
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!HitActor)
		{
			continue;
		}

		// Angle check
		if (!IsPointInArcSector(Origin, Forward, HitActor->GetActorLocation(), Radius, AngleDegrees, StartAngleOffset))
		{
			continue;
		}

		// Combat filtering
		const bool bIsAttacker = (HitActor == Attacker);
		const bool bIsFriendly = Attacker ? AreActorsFriendly(Attacker, HitActor) : false;
		const bool bAirborne = IsTargetAirborne(HitActor);

		if (ShouldExcludeTarget(bIsAttacker, bIsFriendly, bAirborne, bJumpAvoidable, bHitsAirborne, bAllowFriendlyFire))
		{
			continue;
		}

		FMordecaiHitResult HitResult;
		HitResult.HitActor = HitActor;
		HitResult.HitLocation = HitActor->GetActorLocation();
		HitResult.HitNormal = (Origin - HitActor->GetActorLocation()).GetSafeNormal();
		HitResult.bWasAirborne = bAirborne;
		Results.Add(HitResult);
	}

	return Results;
}

TArray<FMordecaiHitResult> UMordecaiHitDetectionSubsystem::CapsuleOverlapQuery(
	const FVector& Origin,
	const FVector& Forward,
	float Length,
	float Width,
	AActor* Attacker,
	EMordecaiTargetFilter Filter,
	bool bAllowFriendlyFire,
	bool bJumpAvoidable,
	bool bHitsAirborne)
{
	TArray<FMordecaiHitResult> Results;
	UWorld* World = GetWorld();
	if (!World)
	{
		return Results;
	}

	// AC-003.10: Use UE collision — sweep a sphere along the forward direction
	const float SweepRadius = Width * 0.5f;
	const FVector SweepStart = Origin;
	const FVector SweepEnd = Origin + Forward.GetSafeNormal() * Length;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MordecaiCapsule), false, Attacker);
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	TArray<FHitResult> SweepHits;
	World->SweepMultiByObjectType(
		SweepHits,
		SweepStart,
		SweepEnd,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(SweepRadius),
		QueryParams);

	// Deduplicate actors and apply filtering
	TSet<AActor*> ProcessedActors;
	for (const FHitResult& Hit : SweepHits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || ProcessedActors.Contains(HitActor))
		{
			continue;
		}
		ProcessedActors.Add(HitActor);

		const bool bIsAttacker = (HitActor == Attacker);
		const bool bIsFriendly = Attacker ? AreActorsFriendly(Attacker, HitActor) : false;
		const bool bAirborne = IsTargetAirborne(HitActor);

		if (ShouldExcludeTarget(bIsAttacker, bIsFriendly, bAirborne, bJumpAvoidable, bHitsAirborne, bAllowFriendlyFire))
		{
			continue;
		}

		FMordecaiHitResult HitResult;
		HitResult.HitActor = HitActor;
		HitResult.HitLocation = Hit.ImpactPoint;
		HitResult.HitNormal = Hit.ImpactNormal;
		HitResult.bWasAirborne = bAirborne;
		Results.Add(HitResult);
	}

	return Results;
}

TArray<FMordecaiHitResult> UMordecaiHitDetectionSubsystem::CircleOverlapQuery(
	const FVector& ImpactPoint,
	float Radius,
	AActor* Attacker,
	EMordecaiTargetFilter Filter,
	bool bAllowFriendlyFire,
	bool bJumpAvoidable,
	bool bHitsAirborne)
{
	TArray<FMordecaiHitResult> Results;
	UWorld* World = GetWorld();
	if (!World)
	{
		return Results;
	}

	// AC-003.10: Use UE collision — sphere overlap at impact point
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MordecaiCircle), false, Attacker);
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(
		Overlaps,
		ImpactPoint,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(Radius),
		QueryParams);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!HitActor)
		{
			continue;
		}

		const bool bIsAttacker = (HitActor == Attacker);
		const bool bIsFriendly = Attacker ? AreActorsFriendly(Attacker, HitActor) : false;
		const bool bAirborne = IsTargetAirborne(HitActor);

		if (ShouldExcludeTarget(bIsAttacker, bIsFriendly, bAirborne, bJumpAvoidable, bHitsAirborne, bAllowFriendlyFire))
		{
			continue;
		}

		FMordecaiHitResult HitResult;
		HitResult.HitActor = HitActor;
		HitResult.HitLocation = HitActor->GetActorLocation();
		HitResult.HitNormal = (ImpactPoint - HitActor->GetActorLocation()).GetSafeNormal();
		HitResult.bWasAirborne = bAirborne;
		Results.Add(HitResult);
	}

	return Results;
}

// ---------------------------------------------------------------------------
// Unified Dispatch (AC-003.2)
// ---------------------------------------------------------------------------

TArray<FMordecaiHitResult> UMordecaiHitDetectionSubsystem::PerformMeleeHitDetection(
	const UMordecaiAttackProfileDataAsset* AttackProfile,
	const FVector& Origin,
	const FVector& Forward,
	AActor* Attacker,
	EMordecaiTargetFilter Filter,
	bool bAllowFriendlyFire)
{
	if (!AttackProfile)
	{
		return TArray<FMordecaiHitResult>();
	}

	const bool bJumpAvoidable = AttackProfile->JumpAvoidable;
	const bool bHitsAirborne = AttackProfile->HitsAirborne;
	const FMordecaiHitShapeParams& Shape = AttackProfile->HitShapeParams;

	switch (AttackProfile->HitShapeType)
	{
	case EMordecaiHitShapeType::ArcSector:
	{
		TArray<FMordecaiHitResult> Results = ArcSectorOverlapQuery(
			Origin, Forward,
			Shape.Radius, Shape.Angle, Shape.StartAngleOffset,
			Attacker, Filter, bAllowFriendlyFire,
			bJumpAvoidable, bHitsAirborne);
#if UE_ENABLE_DEBUG_DRAWING
		if (ShouldDrawMeleeHitShapes())
		{
			const FMordecaiDebugArcGeometry Geometry = BuildDebugArcGeometry(
				Origin, Forward, Shape.Radius, Shape.Angle, Shape.StartAngleOffset);
			const FColor Color = HasValidDebugHitTarget(Results) ? FColor::Green : FColor::Red;
			DrawDebugLine(
				GetWorld(), Geometry.Origin, Geometry.StartEdge, Color, false,
				DrawMeleeHitShapesDuration, 0, ShapeThickness);
			DrawDebugLine(
				GetWorld(), Geometry.Origin, Geometry.EndEdge, Color, false,
				DrawMeleeHitShapesDuration, 0, ShapeThickness);
			for (int32 Index = 1; Index < Geometry.ArcPoints.Num(); ++Index)
			{
				DrawDebugLine(
					GetWorld(), Geometry.ArcPoints[Index - 1], Geometry.ArcPoints[Index],
					Color, false, DrawMeleeHitShapesDuration, 0, ShapeThickness);
			}
			DrawHitMarkers(GetWorld(), Results, Color);
		}
#endif
		return Results;
	}

	case EMordecaiHitShapeType::Capsule:
	case EMordecaiHitShapeType::Box:
	{
		TArray<FMordecaiHitResult> Results = CapsuleOverlapQuery(
			Origin, Forward,
			Shape.Length, Shape.Width,
			Attacker, Filter, bAllowFriendlyFire,
			bJumpAvoidable, bHitsAirborne);
#if UE_ENABLE_DEBUG_DRAWING
		if (ShouldDrawMeleeHitShapes())
		{
			const FMordecaiDebugCapsuleGeometry Geometry = BuildDebugCapsuleGeometry(
				Origin, Forward, Shape.Length, Shape.Width);
			const FColor Color = HasValidDebugHitTarget(Results) ? FColor::Green : FColor::Red;
			const FVector Segment = Geometry.SegmentEnd - Geometry.SegmentStart;
			const FVector Direction = Segment.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
			const FVector Center = (Geometry.SegmentStart + Geometry.SegmentEnd) * 0.5f;
			const FQuat Rotation = FQuat::FindBetweenNormals(FVector::UpVector, Direction);
			DrawDebugCapsule(
				GetWorld(),
				Center,
				Segment.Size() * 0.5f + Geometry.CapRadius,
				Geometry.CapRadius,
				Rotation,
				Color,
				false,
				DrawMeleeHitShapesDuration,
				0,
				ShapeThickness);
			DrawDebugLine(
				GetWorld(), Geometry.StartLeft, Geometry.EndLeft, Color, false,
				DrawMeleeHitShapesDuration, 0, ShapeThickness);
			DrawDebugLine(
				GetWorld(), Geometry.StartRight, Geometry.EndRight, Color, false,
				DrawMeleeHitShapesDuration, 0, ShapeThickness);
			DrawHitMarkers(GetWorld(), Results, Color);
		}
#endif
		return Results;
	}

	case EMordecaiHitShapeType::Circle:
	{
		const FVector ImpactPoint = Origin + Forward.GetSafeNormal() * Shape.Length;
		TArray<FMordecaiHitResult> Results = CircleOverlapQuery(
			ImpactPoint,
			Shape.Radius,
			Attacker, Filter, bAllowFriendlyFire,
			bJumpAvoidable, bHitsAirborne);
#if UE_ENABLE_DEBUG_DRAWING
		if (ShouldDrawMeleeHitShapes())
		{
			const FMordecaiDebugCircleGeometry Geometry = BuildDebugCircleGeometry(
				Origin, Forward, Shape.Length, Shape.Radius);
			const FColor Color = HasValidDebugHitTarget(Results) ? FColor::Green : FColor::Red;
			DrawDebugCircle(
				GetWorld(),
				Geometry.Center,
				Geometry.Radius,
				CircleSegments,
				Color,
				false,
				DrawMeleeHitShapesDuration,
				0,
				ShapeThickness,
				FVector::ForwardVector,
				FVector::RightVector,
				false);
			DrawHitMarkers(GetWorld(), Results, Color);
		}
#endif
		return Results;
	}

	default:
		return TArray<FMordecaiHitResult>();
	}
}
