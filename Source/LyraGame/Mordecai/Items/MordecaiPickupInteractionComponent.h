// Project Mordecai — Pickup Interaction Component (US-079)

#pragma once

#include "Components/ActorComponent.h"

#include "MordecaiPickupInteractionComponent.generated.h"

class AMordecaiItemPickup;

/**
 * Fires whenever the focused pickup changes. FocusedPickup is null (and
 * ItemDisplayName empty) when nothing is in focus.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnMordecaiFocusedPickupChanged,
	AMordecaiItemPickup*, FocusedPickup,
	const FText&, ItemDisplayName);

/**
 * UMordecaiPickupInteractionComponent
 *
 * Press-to-pickup tracker living on AMordecaiCharacter. Pickup actors register
 * themselves on trigger overlap; the component keeps the nearest one as the
 * *focused* pickup and broadcasts OnFocusedPickupChanged for the UI prompt.
 *
 * TryPickupFocused() is bound to the Interact input (F / Gamepad X). It runs
 * the pickup directly with authority, or routes through a server RPC (with
 * server-side range re-validation) otherwise.
 *
 * Focus re-evaluates on register/unregister/consume and on a coarse tick
 * (0.15s, enabled only while pickups are in range) so walking between two
 * overlapping pickups swaps focus to the closer one.
 */
UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class LYRAGAME_API UMordecaiPickupInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMordecaiPickupInteractionComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Called by AMordecaiItemPickup on trigger begin-overlap. Duplicate-safe. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Pickup")
	void RegisterPickup(AMordecaiItemPickup* Pickup);

	/** Called by AMordecaiItemPickup on trigger end-overlap and on consume. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Pickup")
	void UnregisterPickup(AMordecaiItemPickup* Pickup);

	/** The nearest in-range pickup, or null. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Pickup")
	AMordecaiItemPickup* GetFocusedPickup() const { return FocusedPickup.Get(); }

	/** Number of live pickups currently tracked. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Pickup")
	int32 GetInRangePickupCount() const;

	/**
	 * Pick up the focused pickup (Interact input handler). Authority executes
	 * directly; remote clients route through the server RPC. Returns false
	 * only when nothing is focused or the pickup rejects (null def etc.).
	 */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Pickup")
	bool TryPickupFocused();

	/** Prune stale entries and recompute the nearest pickup; broadcasts on change. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Pickup")
	void EvaluateFocus();

	UPROPERTY(BlueprintAssignable, Category = "Mordecai|Pickup")
	FOnMordecaiFocusedPickupChanged OnFocusedPickupChanged;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Server-side pickup with range re-validation (client input path). */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerTryPickupFocused(AMordecaiItemPickup* Pickup);

private:
	TArray<TWeakObjectPtr<AMordecaiItemPickup>> InRangePickups;
	TWeakObjectPtr<AMordecaiItemPickup> FocusedPickup;
};
