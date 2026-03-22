// Project Mordecai — Aim Assist Subsystem (US-009)

#include "Mordecai/Combat/MordecaiAimAssistSubsystem.h"
#include "Mordecai/Combat/MordecaiHitDetectionTypes.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/World.h"

// ---------------------------------------------------------------------------
// Main API — Simple Overload (AC-009.19)
// ---------------------------------------------------------------------------

FVector UMordecaiAimAssistSubsystem::ComputeAssistedAimDirection(
	const FVector& Origin,
	const FVector& AimForward,
	float AssistRange)
{
	TArray<float> DefaultAngles = { 0.f, 2.f, 4.f, 6.f };
	return ComputeAssistedAimDirectionEx(Origin, AimForward, AssistRange, DefaultAngles, FGameplayTag());
}

// ---------------------------------------------------------------------------
// Main API — Extended (AC-009.19)
// ---------------------------------------------------------------------------

FVector UMordecaiAimAssistSubsystem::ComputeAssistedAimDirectionEx(
	const FVector& Origin,
	const FVector& AimForward,
	float AssistRange,
	const TArray<float>& CandidateAngles,
	FGameplayTag FilterTag)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return AimForward;
	}

	// Use Team.Enemy if no specific tag provided
	FGameplayTag EffectiveFilterTag = FilterTag.IsValid() ? FilterTag : MordecaiGameplayTags::Team_Enemy;

	// AC-009.20: Generate candidate directions
	TArray<FVector> CandidateDirs = GenerateCandidateDirections(AimForward, CandidateAngles);

	TArray<FMordecaiAimCandidate> ScoredCandidates;

	// AC-009.21: Raycast each candidate direction to AssistRange
	for (int32 i = 0; i < CandidateDirs.Num(); ++i)
	{
		const FVector& Dir = CandidateDirs[i];
		FVector TraceEnd = Origin + Dir * AssistRange;

		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.bTraceComplex = false;

		bool bHit = World->LineTraceSingleByChannel(
			HitResult,
			Origin,
			TraceEnd,
			ECC_Pawn,
			QueryParams);

		if (bHit && HitResult.GetActor() && IsValidAimTarget(HitResult.GetActor(), EffectiveFilterTag))
		{
			FMordecaiAimCandidate Candidate;
			Candidate.Direction = Dir;
			Candidate.Distance = HitResult.Distance;
			Candidate.bHitTarget = true;

			// Compute angle offset from original forward
			float Dot = FVector::DotProduct(Dir.GetSafeNormal(), AimForward.GetSafeNormal());
			Candidate.AngleOffset = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));

			ScoredCandidates.Add(Candidate);
		}
	}

	// AC-009.22: Select best candidate
	FMordecaiAimCandidate Best = SelectBestCandidate(ScoredCandidates);

	// AC-009.23, AC-009.24: Return corrected or original direction
	return GetCorrectedDirection(AimForward, Best);
}

// ---------------------------------------------------------------------------
// Static: Generate Candidate Directions (AC-009.20)
// ---------------------------------------------------------------------------

TArray<FVector> UMordecaiAimAssistSubsystem::GenerateCandidateDirections(
	const FVector& AimForward,
	const TArray<float>& CandidateAngles)
{
	TArray<FVector> Candidates;

	for (float Angle : CandidateAngles)
	{
		if (FMath::IsNearlyZero(Angle))
		{
			// 0° offset = original direction
			Candidates.Add(AimForward.GetSafeNormal());
		}
		else
		{
			// Horizontal plane offsets (fixed diorama camera)
			FVector PosOffset = AimForward.RotateAngleAxis(Angle, FVector::UpVector).GetSafeNormal();
			FVector NegOffset = AimForward.RotateAngleAxis(-Angle, FVector::UpVector).GetSafeNormal();
			Candidates.Add(PosOffset);
			Candidates.Add(NegOffset);
		}
	}

	return Candidates;
}

// ---------------------------------------------------------------------------
// Static: Select Best Candidate (AC-009.22)
// ---------------------------------------------------------------------------

FMordecaiAimCandidate UMordecaiAimAssistSubsystem::SelectBestCandidate(
	const TArray<FMordecaiAimCandidate>& Candidates)
{
	FMordecaiAimCandidate Best;
	Best.bHitTarget = false;
	Best.AngleOffset = TNumericLimits<float>::Max();
	Best.Distance = TNumericLimits<float>::Max();

	for (const FMordecaiAimCandidate& C : Candidates)
	{
		if (!C.bHitTarget)
		{
			continue;
		}

		// Priority 1: Smallest angle offset
		// Priority 2: Closest distance (tiebreaker)
		if (C.AngleOffset < Best.AngleOffset ||
			(FMath::IsNearlyEqual(C.AngleOffset, Best.AngleOffset, 0.01f) && C.Distance < Best.Distance))
		{
			Best = C;
		}
	}

	return Best;
}

// ---------------------------------------------------------------------------
// Static: Get Corrected Direction (AC-009.23, AC-009.24)
// ---------------------------------------------------------------------------

FVector UMordecaiAimAssistSubsystem::GetCorrectedDirection(
	const FVector& AimForward,
	const FMordecaiAimCandidate& BestCandidate)
{
	// AC-009.24: No valid candidate — return original
	if (!BestCandidate.bHitTarget)
	{
		return AimForward.GetSafeNormal();
	}

	// AC-009.23: Return the candidate direction (already constrained by max angle offsets)
	return BestCandidate.Direction.GetSafeNormal();
}

// ---------------------------------------------------------------------------
// Static: Target Validation (AC-009.26)
// ---------------------------------------------------------------------------

bool UMordecaiAimAssistSubsystem::IsValidAimTarget(const AActor* Actor, const FGameplayTag& FilterTag)
{
	if (!Actor)
	{
		return false;
	}

	// If no filter tag specified, accept any actor
	if (!FilterTag.IsValid())
	{
		return true;
	}

	// Check if actor has the required tag via ASC
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
	if (!ASC)
	{
		return false;
	}

	return ASC->HasMatchingGameplayTag(FilterTag);
}
