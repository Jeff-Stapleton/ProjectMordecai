// Project Mordecai — Aim Assist Subsystem (US-009)

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"

#include "MordecaiAimAssistSubsystem.generated.h"

/**
 * FMordecaiAimCandidate
 *
 * Represents a candidate direction for aim assist evaluation.
 * Used internally by the aim assist subsystem for scoring.
 */
USTRUCT(BlueprintType)
struct LYRAGAME_API FMordecaiAimCandidate
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|AimAssist")
	FVector Direction = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|AimAssist")
	float AngleOffset = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|AimAssist")
	float Distance = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|AimAssist")
	bool bHitTarget = false;
};

/**
 * UMordecaiAimAssistSubsystem
 *
 * World subsystem that provides soft aim assist for ranged attacks.
 * Generates candidate directions around the player's aim, raycasts each,
 * and returns a slightly corrected direction toward the best valid target.
 *
 * Pure function — no persistent state. Called before projectile spawn.
 *
 * See: US-009, player_attacks_agent_brief_v1.md Section 5.1
 */
UCLASS()
class LYRAGAME_API UMordecaiAimAssistSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// --- Main API (AC-009.19) ---

	/**
	 * Compute an aim-assisted direction for projectile firing.
	 * Uses default candidate angles {0, 2, 4, 6} and Team.Enemy filter.
	 * @param Origin          Firing origin (character position)
	 * @param AimForward      Player's raw aim direction
	 * @param AssistRange     Max range for candidate raycasts (default 3000)
	 * @return Corrected aim direction (or original if no targets found)
	 */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|AimAssist")
	FVector ComputeAssistedAimDirection(
		const FVector& Origin,
		const FVector& AimForward,
		float AssistRange = 3000.f);

	/**
	 * Compute an aim-assisted direction with custom candidate angles and filter tag.
	 * @param Origin          Firing origin (character position)
	 * @param AimForward      Player's raw aim direction
	 * @param AssistRange     Max range for candidate raycasts
	 * @param CandidateAngles Angular offsets to test
	 * @param FilterTag       Only consider targets with this tag
	 * @return Corrected aim direction (or original if no targets found)
	 */
	FVector ComputeAssistedAimDirectionEx(
		const FVector& Origin,
		const FVector& AimForward,
		float AssistRange,
		const TArray<float>& CandidateAngles,
		FGameplayTag FilterTag);

	// --- Static Helpers (testable without world) ---

	/**
	 * Generate candidate directions around AimForward at angular offsets.
	 * Produces: 0°, +first, -first, +second, -second, ... in the horizontal plane.
	 * (AC-009.20)
	 */
	static TArray<FVector> GenerateCandidateDirections(
		const FVector& AimForward,
		const TArray<float>& CandidateAngles);

	/**
	 * Select the best candidate from scored results.
	 * Priority: (1) smallest AngleOffset, (2) closest Distance.
	 * Returns an invalid candidate (bHitTarget=false) if none found.
	 * (AC-009.22)
	 */
	static FMordecaiAimCandidate SelectBestCandidate(const TArray<FMordecaiAimCandidate>& Candidates);

	/**
	 * Get the corrected direction from a best candidate result.
	 * If candidate is invalid, returns original AimForward.
	 * (AC-009.23, AC-009.24)
	 */
	static FVector GetCorrectedDirection(const FVector& AimForward, const FMordecaiAimCandidate& BestCandidate);

	/**
	 * Check if an actor is a valid aim assist target.
	 * Must be non-null and (if FilterTag is valid) have the filter tag.
	 * (AC-009.26)
	 */
	static bool IsValidAimTarget(const AActor* Actor, const FGameplayTag& FilterTag);
};
