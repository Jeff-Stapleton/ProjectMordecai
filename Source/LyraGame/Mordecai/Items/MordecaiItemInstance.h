// Project Mordecai — Item Instance (US-031)

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Mordecai/Items/MordecaiItemTypes.h"
#include "MordecaiItemInstance.generated.h"

class UMordecaiItemDefinition;

/**
 * FMordecaiItemInstance
 *
 * Runtime representation of a player-owned item. References the static
 * definition plus per-instance state (quantity, affix rolls, equipped flag).
 * Replicated via FFastArraySerializer in UMordecaiInventoryComponent.
 */
USTRUCT(BlueprintType)
struct LYRAGAME_API FMordecaiItemInstance : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Item")
	FGuid InstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Item")
	TObjectPtr<UMordecaiItemDefinition> ItemDefinition = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Item")
	int32 Quantity = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Item")
	TArray<FName> AffixRolls;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Item")
	bool IsEquipped = false;

	/** Identification state (US-033). Defaults to Identified; inventory routes DefaultState on add when definition uses identification. */
	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Item")
	EMordecaiIdentificationState IdentificationState = EMordecaiIdentificationState::Identified;

	bool IsValid() const { return ItemDefinition != nullptr && Quantity > 0; }

	bool IsIdentified() const { return IdentificationState == EMordecaiIdentificationState::Identified; }
};

/**
 * FMordecaiInventoryList
 *
 * FastArraySerializer container for replicated inventory instances.
 */
USTRUCT(BlueprintType)
struct LYRAGAME_API FMordecaiInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FMordecaiItemInstance> Items;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FMordecaiItemInstance, FMordecaiInventoryList>(Items, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FMordecaiInventoryList> : public TStructOpsTypeTraitsBase2<FMordecaiInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};

/**
 * FMordecaiResourceEntry
 *
 * Read-only snapshot of a single ledger entry (for UI / town bank queries).
 */
USTRUCT(BlueprintType)
struct LYRAGAME_API FMordecaiResourceEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Item")
	FName ItemId;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Item")
	int32 Count = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Item")
	TObjectPtr<UMordecaiItemDefinition> Def = nullptr;
};
