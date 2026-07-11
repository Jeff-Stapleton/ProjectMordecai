// Project Mordecai — Item Pickup Actor (US-079)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "MordecaiItemPickup.generated.h"

class UMordecaiItemDefinition;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * AMordecaiItemPickup
 *
 * World actor representing an item lying on the ground. Press-to-pickup
 * (US-079 revised): overlapping AMordecaiCharacter's sphere trigger registers
 * this pickup with the character's UMordecaiPickupInteractionComponent; the
 * actual pickup happens when the player presses Interact and the component
 * calls ExecutePickup().
 *
 * Server-authoritative: ExecutePickup only runs with authority; the actor is
 * replicated so destruction propagates to clients.
 *
 * Visuals: optional display mesh (assigned per-instance by placement scripts
 * or a BP subclass) plus an in-world name label so pickups are readable at a
 * glance. Both use the identification-aware display name — unidentified
 * magical items show partial info, never their true name.
 */
UCLASS(BlueprintType)
class LYRAGAME_API AMordecaiItemPickup : public AActor
{
	GENERATED_BODY()

public:
	AMordecaiItemPickup();

	/** The item this pickup grants. Null definition makes ExecutePickup a safe no-op. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mordecai|Pickup")
	TObjectPtr<UMordecaiItemDefinition> ItemDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mordecai|Pickup", meta = (ClampMin = 1))
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mordecai|Pickup")
	bool bDestroyOnPickup = true;

	/**
	 * Grant the item to the collector's inventory (auto-store routing included).
	 * Authority-only; safe no-op (returns false) on null definition, null
	 * collector, missing inventory component, or non-authority.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Pickup")
	bool ExecutePickup(AActor* Collector);

	/** Identification-aware display name (partial info for unidentified items). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Pickup")
	FText GetItemDisplayName() const;

	/** Trigger radius, used by the server RPC's range re-validation. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Pickup")
	float GetTriggerRadius() const;

	USphereComponent* GetPickupTrigger() const { return PickupTrigger; }
	UStaticMeshComponent* GetDisplayMesh() const { return DisplayMesh; }
	UTextRenderComponent* GetNameLabel() const { return NameLabel; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	UPROPERTY(VisibleAnywhere, Category = "Mordecai|Pickup")
	TObjectPtr<USceneComponent> Root;

	/** Proximity trigger (default 120cm) — drives interaction registration, not pickup itself. */
	UPROPERTY(VisibleAnywhere, Category = "Mordecai|Pickup")
	TObjectPtr<USphereComponent> PickupTrigger;

	/** Optional placeholder visual — mesh assigned per-instance (placement script / BP). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mordecai|Pickup", Meta = (AllowPrivateAccess = true))
	TObjectPtr<UStaticMeshComponent> DisplayMesh;

	/** In-world name label so Jeff can tell pickups apart at a glance (AC-079.10). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mordecai|Pickup", Meta = (AllowPrivateAccess = true))
	TObjectPtr<UTextRenderComponent> NameLabel;

	void RefreshNameLabel();
};
