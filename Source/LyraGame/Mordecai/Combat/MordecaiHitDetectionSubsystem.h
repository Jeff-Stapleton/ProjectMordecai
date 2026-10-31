// Project Mordecai — Melee Hit Detection Subsystem (US-003)

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MordecaiHitDetectionTypes.h"

#include "MordecaiHitDetectionSubsystem.generated.h"

class ACharacter;
class UAbilitySystemComponent;
class UMordecaiAttackProfileDataAsset;

/**
 * UMordecaiHitDetectionSubsystem
 *
 * World subsystem that performs spatial queries for melee hit detection.
 * Provides three shape queries (arc sector, capsule, circle) and a unified
 * dispatch function that selects the correct shape from an AttackProfile.
 *
 * All queries are server-authoritative. This subsystem does NOT apply damage
 * or effects — it only returns what was hit. See US-004 for GAS integration.
 *
 * See: attack_taxonomy_v1.md Sections 3.1-3.3
 */
UCLASS()
class LYRAGAME_API UMordecaiHitDetectionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// --- Shape Queries (AC-003.1) ---

	/** Arc sector overlap: sweeps use this shape. Gathers actors within a fan-shaped area. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HitDetection")
	TArray<FMordecaiHitResult> ArcSectorOverlapQuery(
		const FVector& Origin,
		const FVector& Forward,
		float Radius,
		float AngleDegrees,
		float StartAngleOffset,
		AActor* Attacker = nullptr,
		EMordecaiTargetFilter Filter = EMordecaiTargetFilter::Enemies,
		bool bAllowFriendlyFire = false,
		bool bJumpAvoidable = false,
		bool bHitsAirborne = false);

	/** Capsule overlap: thrusts use this shape. Gathers actors in a forward lane. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HitDetection")
	TArray<FMordecaiHitResult> CapsuleOverlapQuery(
		const FVector& Origin,
		const FVector& Forward,
		float Length,
		float Width,
		AActor* Attacker = nullptr,
		EMordecaiTargetFilter Filter = EMordecaiTargetFilter::Enemies,
		bool bAllowFriendlyFire = false,
		bool bJumpAvoidable = false,
		bool bHitsAirborne = false);

	/** Circle overlap: slams use this shape. Gathers actors within a radius from impact. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HitDetection")
	TArray<FMordecaiHitResult> CircleOverlapQuery(
		const FVector& ImpactPoint,
		float Radius,
		AActor* Attacker = nullptr,
		EMordecaiTargetFilter Filter = EMordecaiTargetFilter::Enemies,
		bool bAllowFriendlyFire = false,
		bool bJumpAvoidable = false,
		bool bHitsAirborne = false);

	// --- Unified Dispatch (AC-003.2) ---

	/** Dispatches to the correct shape query based on the profile's HitShapeType. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HitDetection")
	TArray<FMordecaiHitResult> PerformMeleeHitDetection(
		const UMordecaiAttackProfileDataAsset* AttackProfile,
		const FVector& Origin,
		const FVector& Forward,
		AActor* Attacker = nullptr,
		EMordecaiTargetFilter Filter = EMordecaiTargetFilter::Enemies,
		bool bAllowFriendlyFire = false);

	// --- Static Geometry Helpers (testable without world) ---

	/** Returns true if Point is within the arc sector defined by Origin/Forward/Radius/Angle. */
	static bool IsPointInArcSector(
		const FVector& Origin,
		const FVector& Forward,
		const FVector& Point,
		float Radius,
		float AngleDegrees,
		float StartAngleOffsetDeg);

	/** Returns true if Point is within the capsule lane defined by Origin/Forward/Length/Width. */
	static bool IsPointInCapsuleLane(
		const FVector& Origin,
		const FVector& Forward,
		const FVector& Point,
		float Length,
		float Width);

	/** Returns true if Point is within Radius of Center. */
	static bool IsPointInCircle(
		const FVector& Center,
		const FVector& Point,
		float Radius);

	// --- Static Filtering Helpers (testable without world) ---

	/**
	 * Returns true if the target should be EXCLUDED from hit results.
	 * Checks attacker self-hit, friendly fire, and airborne interaction rules.
	 */
	static bool ShouldExcludeTarget(
		bool bIsAttacker,
		bool bIsFriendly,
		bool bTargetIsAirborne,
		bool bJumpAvoidable,
		bool bHitsAirborne,
		bool bAllowFriendlyFire);

	/** Returns true if the target actor has the Mordecai.State.Airborne gameplay tag. */
	static bool IsTargetAirborne(const AActor* Target);

	/**
	 * Mordecai.State.Airborne follows the movement mode: one count while the character is falling (a jump, a drop off a
	 * ledge, a launch), none otherwise (AC-003.7, US-101). The count is set, not added, so it cannot drift. Called by the
	 * player and enemy characters on every movement-mode change and when their ASC is attached.
	 */
	static void SyncAirborneTag(UAbilitySystemComponent* ASC, const ACharacter* Character);

	/**
	 * Returns true if Attacker and Target are on the same side. The Mordecai.Team.Enemy tag decides when either
	 * carries it (both = friends, one = foes); otherwise their IGenericTeamAgentInterface ids must match.
	 */
	static bool AreActorsFriendly(const AActor* Attacker, const AActor* Target);

private:
	/** Shared filtering logic applied to overlap results from any shape query. */
	TArray<FMordecaiHitResult> FilterOverlapResults(
		const TArray<FOverlapResult>& OverlapResults,
		const AActor* Attacker,
		bool bJumpAvoidable,
		bool bHitsAirborne,
		bool bAllowFriendlyFire) const;
};
