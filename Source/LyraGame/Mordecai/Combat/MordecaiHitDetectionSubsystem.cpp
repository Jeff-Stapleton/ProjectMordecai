// Project Mordecai — Melee Hit Detection Subsystem (US-003)

#include "Mordecai/Combat/MordecaiHitDetectionSubsystem.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GenericTeamAgentInterface.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"

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

bool UMordecaiHitDetectionSubsystem::AreActorsFriendly(const AActor* Attacker, const AActor* Target)
{
	if (!Attacker || !Target)
	{
		return false;
	}

	const IGenericTeamAgentInterface* AttackerTeam = Cast<IGenericTeamAgentInterface>(Attacker);
	const IGenericTeamAgentInterface* TargetTeam = Cast<IGenericTeamAgentInterface>(Target);

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
		return ArcSectorOverlapQuery(
			Origin, Forward,
			Shape.Radius, Shape.Angle, Shape.StartAngleOffset,
			Attacker, Filter, bAllowFriendlyFire,
			bJumpAvoidable, bHitsAirborne);

	case EMordecaiHitShapeType::Capsule:
	case EMordecaiHitShapeType::Box:
		return CapsuleOverlapQuery(
			Origin, Forward,
			Shape.Length, Shape.Width,
			Attacker, Filter, bAllowFriendlyFire,
			bJumpAvoidable, bHitsAirborne);

	case EMordecaiHitShapeType::Circle:
		return CircleOverlapQuery(
			Origin + Forward.GetSafeNormal() * Shape.Length,
			Shape.Radius,
			Attacker, Filter, bAllowFriendlyFire,
			bJumpAvoidable, bHitsAirborne);

	default:
		return TArray<FMordecaiHitResult>();
	}
}
