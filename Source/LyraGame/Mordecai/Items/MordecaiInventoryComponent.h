// Project Mordecai — Inventory Component (US-031)

#pragma once

#include "Components/ActorComponent.h"
#include "Mordecai/Items/MordecaiItemInstance.h"
#include "Mordecai/Items/MordecaiItemTypes.h"
#include "MordecaiInventoryComponent.generated.h"

class UMordecaiItemDefinition;
class UMordecaiResourceLedger;

/** Broadcast whenever an inventory instance is added/removed/mutated. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnMordecaiInventoryChanged,
	const FGuid&, InstanceId,
	int32, QuantityDelta);

/**
 * UMordecaiInventoryComponent
 *
 * Unlimited-carry flat list of per-character item instances. Handles stacking,
 * auto-store routing via the sibling UMordecaiResourceLedger, and atomic
 * consume-by-definition.
 *
 * No capacity limits (enforces agent_rules_v2 Section 2 unlimited-carry rule).
 */
UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class LYRAGAME_API UMordecaiInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMordecaiInventoryComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Pickup routing (canonical entry point for loot drops) ---

	/** Route a pickup: auto-stored items go to the ledger, everything else to inventory. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Inventory")
	void PickupItem(UMordecaiItemDefinition* Def, int32 Quantity = 1);

	// --- Direct add/remove ---

	/**
	 * Add an item instance. Returns the InstanceId of the first modified stack,
	 * or an invalid FGuid if the item was auto-stored (ledger-routed).
	 */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Inventory")
	FGuid AddItem(UMordecaiItemDefinition* Def, int32 Quantity = 1);

	/** Decrement quantity; removes instance entirely at zero. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Inventory")
	bool RemoveItem(const FGuid& InstanceId, int32 Quantity = 1);

	/**
	 * Atomically remove Quantity total across any instances of Def (lowest-quantity
	 * stacks first). Returns false and makes NO modifications if the total < Quantity.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Inventory")
	bool ConsumeByDefinition(UMordecaiItemDefinition* Def, int32 Quantity);

	// --- Queries ---

	/** All instances in pickup order (const reference — no copy). */
	const TArray<FMordecaiItemInstance>& GetAllItems() const { return ItemList.Items; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Inventory")
	TArray<FMordecaiItemInstance> GetItemsByType(EMordecaiItemType Type) const;

	/** All items sorted via UMordecaiItemLibrary::CompareSortPriority. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Inventory")
	TArray<FMordecaiItemInstance> GetSortedItems() const;

	/** Find instance by InstanceId. C++ only (returns pointer). */
	const FMordecaiItemInstance* FindInstance(const FGuid& InstanceId) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Inventory")
	int32 GetTotalQuantityOfDefinition(UMordecaiItemDefinition* Def) const;

	// --- Delegate ---

	UPROPERTY(BlueprintAssignable, Category = "Mordecai|Inventory")
	FOnMordecaiInventoryChanged OnInventoryChanged;

	// --- Ledger wiring (set by owning character during construction) ---

	/** Assign the paired ledger used for auto-stored pickups. */
	void SetResourceLedger(UMordecaiResourceLedger* InLedger) { ResourceLedger = InLedger; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Inventory")
	UMordecaiResourceLedger* GetResourceLedger() const { return ResourceLedger; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(Replicated)
	FMordecaiInventoryList ItemList;

	UPROPERTY()
	TObjectPtr<UMordecaiResourceLedger> ResourceLedger;
};
