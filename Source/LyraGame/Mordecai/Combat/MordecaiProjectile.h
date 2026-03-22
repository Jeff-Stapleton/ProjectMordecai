// Project Mordecai — Projectile Actor (US-009)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MordecaiCombatTypes.h"

#include "MordecaiProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UMordecaiAttackProfileDataAsset;
class UAbilitySystemComponent;

/**
 * AMordecaiProjectile
 *
 * Data-driven projectile actor. Initialized from FMordecaiProjectileSpec and
 * FMordecaiDamageProfile. Supports pierce, ricochet, and on-hit AoE.
 * Uses UProjectileMovementComponent for flight.
 *
 * See: US-009, attack_taxonomy_v1.md Section 5
 */
UCLASS()
class LYRAGAME_API AMordecaiProjectile : public AActor
{
	GENERATED_BODY()

public:
	AMordecaiProjectile();

	// --- Initialization ---

	/** Initialize projectile from spec and damage profile. Call after spawn. */
	void InitFromSpec(const FMordecaiProjectileSpec& InSpec, const FMordecaiDamageProfile& InDamageProfile);

	/** Set the instigator actor (shooter) to exclude from hits (AC-009.6). */
	void SetProjectileInstigator(AActor* InInstigator);

	// --- Component Access ---

	UFUNCTION(BlueprintCallable, Category = "Mordecai|Projectile")
	UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }

	UFUNCTION(BlueprintCallable, Category = "Mordecai|Projectile")
	USphereComponent* GetCollisionComponent() const { return CollisionComp; }

	// --- Spec Getters ---

	float GetLifetimeSeconds() const { return Spec.Lifetime; }
	float GetMaxRange() const { return Spec.MaxRange; }
	const FMordecaiDamageProfile& GetDamageProfile() const { return DamageProfile; }
	const FMordecaiProjectileSpec& GetSpec() const { return Spec; }

	// --- Pierce (AC-009.8, AC-009.9, AC-009.10) ---

	int32 GetRemainingPierceCount() const { return RemainingPierceCount; }

	/**
	 * Process a pierce hit. Returns true if projectile should continue flying.
	 * Adds target to hit list and decrements pierce counter.
	 */
	bool ProcessPierceHit(AActor* HitTarget);

	// --- Ricochet (AC-009.11-14) ---

	int32 GetRemainingRicochetCount() const { return RemainingRicochetCount; }
	bool CanRicochet() const { return RemainingRicochetCount > 0; }
	void ConsumeRicochet();

	/**
	 * Find the best ricochet target from candidates.
	 * Filters by already-hit list, angle limit, and picks nearest.
	 * Returns nullptr if no valid target found (AC-009.13).
	 */
	AActor* FindBestRicochetTarget(
		const FVector& HitLocation,
		const FVector& ProjectileForward,
		const TArray<AActor*>& Candidates) const;

	// --- Hit List ---

	bool HasAlreadyHit(const AActor* Actor) const { return HitActors.Contains(Actor); }
	void AddToHitList(AActor* Actor) { HitActors.Add(Actor); }
	bool ShouldIgnoreActor(const AActor* Actor) const;

	// --- AoE (AC-009.15-18) ---

	bool HasOnHitAoE() const { return Spec.OnHitAoERadius > 0.f; }
	float GetOnHitAoERadius() const { return Spec.OnHitAoERadius; }
	float ComputeAoEDamage() const { return DamageProfile.BasePower * Spec.OnHitAoEDamageScalar; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	/** Handle a hit on a valid target. Applies damage, processes pierce/ricochet/AoE. */
	void HandleHit(AActor* HitActor, const FVector& HitLocation);

	/** Apply projectile damage to a target via GAS (AC-009.5, AC-009.28). */
	void ApplyDamageToTarget(AActor* TargetActor, float DamageMagnitude);

	/** Apply on-hit AoE damage at location (AC-009.15-18). */
	void ApplyAoEDamage(const FVector& HitLocation, AActor* DirectHitActor);

	/** Attempt ricochet after a hit (AC-009.11-14). */
	void AttemptRicochet(const FVector& HitLocation);

	/** Check and handle max range/lifetime expiry (AC-009.4). */
	void CheckRangeAndLifetime();

private:
	UPROPERTY(VisibleAnywhere, Category = "Mordecai|Projectile")
	TObjectPtr<USphereComponent> CollisionComp;

	UPROPERTY(VisibleAnywhere, Category = "Mordecai|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	FMordecaiProjectileSpec Spec;
	FMordecaiDamageProfile DamageProfile;

	int32 RemainingPierceCount = 0;
	int32 RemainingRicochetCount = 0;

	TSet<const AActor*> HitActors;

	UPROPERTY()
	TWeakObjectPtr<AActor> ProjectileInstigator;

	FVector SpawnLocation = FVector::ZeroVector;
	float DistanceTraveled = 0.f;
};
