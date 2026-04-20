// Project Mordecai — Resource Ledger (US-031)
//
// TODO(DECISION): Migrate to PlayerState when town persistence ships.

#pragma once

#include "Components/ActorComponent.h"
#include "Mordecai/Items/MordecaiItemInstance.h"
#include "MordecaiResourceLedger.generated.h"

class UMordecaiItemDefinition;

/** Broadcast whenever an auto-stored resource count changes. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnMordecaiResourceChanged,
	FName, ItemId,
	int32, NewCount);

/**
 * UMordecaiResourceLedger
 *
 * Per-character store of auto-stored resources (Material / TownResource).
 * Keyed by ItemId; value is count. Replicated via the component's TMap UPROPERTY.
 * The inventory component routes AutoStoreOnPickup items here.
 */
UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class LYRAGAME_API UMordecaiResourceLedger : public UActorComponent
{
	GENERATED_BODY()

public:
	UMordecaiResourceLedger(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Increment the count for Def->ItemId. Rejects (warns, no-op) if Def is not auto-stored. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Ledger")
	void AddResource(UMordecaiItemDefinition* Def, int32 Quantity);

	/** Returns the stored count for ItemId, or 0 if missing. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Ledger")
	int32 GetResourceCount(FName ItemId) const;

	/** Atomically decrement. Returns false (no change) if current count < Quantity. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Ledger")
	bool ConsumeResource(FName ItemId, int32 Quantity);

	/** Snapshot of all entries for UI / town-bank queries. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Ledger")
	TArray<FMordecaiResourceEntry> GetAllResources() const;

	UPROPERTY(BlueprintAssignable, Category = "Mordecai|Ledger")
	FOnMordecaiResourceChanged OnResourceChanged;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** FName → count, stored as TArray for replication (UE doesn't support replicated TMap). */
	UPROPERTY(Replicated)
	TArray<FMordecaiResourceEntry> Entries;

	/** Find the entry index for ItemId, or INDEX_NONE. */
	int32 FindEntryIndex(FName ItemId) const;
};
