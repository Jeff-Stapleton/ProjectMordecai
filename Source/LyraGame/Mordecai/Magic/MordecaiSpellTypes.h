// Project Mordecai — Spell Types (US-019)

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"

#include "MordecaiSpellTypes.generated.h"

/** Movement restriction policy during spell casting phases. */
UENUM(BlueprintType)
enum class EMordecaiMovementPolicy : uint8
{
	FreeMove,
	SlowWhileCasting,
	RootWhileCasting,
};

/** Spell targeting shape / mode. */
UENUM(BlueprintType)
enum class EMordecaiSpellTargetingType : uint8
{
	Self,
	MeleeArc,
	Cone,
	Circle,
	Line,
	Projectile,
	Dash,
	PersistentArea,
};

/** How the spell effect is delivered to targets. */
UENUM(BlueprintType)
enum class EMordecaiSpellDeliveryType : uint8
{
	InstantApply,
	SpawnProjectile,
	TraceHit,
	DashMovement,
	SpawnPersistentArea,
	SummonActor,
};

/** Casting phase state machine. */
UENUM(BlueprintType)
enum class EMordecaiCastingPhase : uint8
{
	None,
	Windup,
	Cast,
	Recovery,
};

/** A single attribute scaling entry: which attribute and its coefficient. */
USTRUCT(BlueprintType)
struct FMordecaiSpellScalingStat
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Spell")
	FGameplayAttribute Stat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Spell")
	float Coefficient = 0.f;
};
