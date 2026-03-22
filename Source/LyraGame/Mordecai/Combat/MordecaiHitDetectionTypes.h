// Project Mordecai — Melee Hit Detection Types (US-003)

#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"

#include "MordecaiHitDetectionTypes.generated.h"

// ---------------------------------------------------------------------------
// AC-003.6: EMordecaiTargetFilter
// ---------------------------------------------------------------------------
UENUM(BlueprintType)
enum class EMordecaiTargetFilter : uint8
{
	Enemies       UMETA(DisplayName = "Enemies"),
	Destructibles UMETA(DisplayName = "Destructibles"),
	All           UMETA(DisplayName = "All"),
};

// ---------------------------------------------------------------------------
// AC-003.11: FMordecaiHitResult
// Contains the result of a single hit from a melee query.
// ---------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct LYRAGAME_API FMordecaiHitResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HitDetection")
	TWeakObjectPtr<AActor> HitActor;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HitDetection")
	FVector HitLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HitDetection")
	FVector HitNormal = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HitDetection")
	bool bWasAirborne = false;
};

// ---------------------------------------------------------------------------
// Collision channel for Mordecai melee hit detection.
// Uses ECC_GameTraceChannel6 (channels 1-5 allocated by Lyra/ShooterCore).
// Must match DefaultEngine.ini collision profile configuration.
// ---------------------------------------------------------------------------
#define Mordecai_TraceChannel_MeleeHitDetection ECC_GameTraceChannel6

// ---------------------------------------------------------------------------
// Collision channel for Mordecai projectile detection (US-009, AC-009.7).
// Uses ECC_GameTraceChannel7 (channel 6 = melee).
// ---------------------------------------------------------------------------
#define Mordecai_TraceChannel_ProjectileDetection ECC_GameTraceChannel7
