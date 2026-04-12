// Project Mordecai — Illusion Decoy Actor (US-060)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "MordecaiIllusionActor.generated.h"

class UCapsuleComponent;

/**
 * AMordecaiIllusionActor
 *
 *	Minimal decoy actor spawned by the Illusion spell. Has configurable HP
 *	and duration. Enemies with aggro-range prefer targeting it due to the
 *	Mordecai.Team.Ally tag. Destroyed when HP reaches 0 or duration expires.
 *
 *	Does NOT have a full ASC — just enough to take damage and be targetable.
 *
 *	See: US-060, AC-060.1 through AC-060.4
 */
UCLASS()
class LYRAGAME_API AMordecaiIllusionActor : public AActor
{
	GENERATED_BODY()

public:
	AMordecaiIllusionActor();

	// --- Configuration ---

	/** Initialize the decoy with HP and duration. Call after spawn. */
	void InitDecoy(float InMaxHP, float InDurationSec);

	// --- Public API ---

	/** Get current HP. */
	float GetCurrentHP() const { return CurrentHP; }

	/** Get max HP. */
	float GetMaxHP() const { return MaxHP; }

	/** Get configured duration in seconds. */
	float GetDurationSec() const { return DurationSec; }

	/** Whether the decoy is still alive. */
	bool IsAlive() const { return CurrentHP > 0.f && !bExpired; }

	/** Apply damage to the decoy. Destroys if HP reaches 0. */
	void ApplyDamage(float DamageAmount);

	/** Get the team tag (Mordecai.Team.Ally). */
	FGameplayTag GetTeamTag() const;

	/** Get the collision component. */
	UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComp; }

	/** Static helper: compute spawn location in front of caster (AC-060.1). */
	static FVector ComputeSpawnLocation(const FVector& CasterLocation, const FVector& CasterForward, float SpawnDistance);

	/** Spawn distance in front of caster (cm). */
	static constexpr float DefaultSpawnDistance = 200.f;

protected:
	virtual void BeginPlay() override;

private:
	void OnDurationExpired();

	UPROPERTY(VisibleAnywhere, Category = "Mordecai|Illusion")
	TObjectPtr<UCapsuleComponent> CapsuleComp;

	float MaxHP = 0.f;
	float CurrentHP = 0.f;
	float DurationSec = 0.f;
	bool bExpired = false;

	FTimerHandle DurationTimerHandle;
};
