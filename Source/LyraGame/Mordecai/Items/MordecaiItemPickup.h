// Project Mordecai — Item Pickup Actor (US-079)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "MordecaiItemPickup.generated.h"

class APlayerCameraManager;
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
 * glance. The label shows the identification-aware display name — unidentified
 * magical items show partial info, never their true name — and is a billboard:
 * every frame it turns square to the local player's camera, reading left to
 * right, whatever the actor's own rotation.
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

	/** Turns the name label to the local player's camera (runs after the camera update, never on a dedicated server). */
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * World rotation that puts a UTextRenderComponent square to a camera at CameraRotation, reading left to right.
	 * A TextRender's readable face is its local +X, its text runs along local -Y and its up is local +Z, so the
	 * label's X is the camera's backward vector and its Z is the camera's up (its -Y is then the camera's right).
	 */
	static FRotator ComputeNameLabelRotation(const FRotator& CameraRotation);

	/** Turns the name label square to Camera's current view. False, label untouched, when Camera is null or has never updated. */
	bool FaceNameLabelTo(const APlayerCameraManager* Camera);

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
