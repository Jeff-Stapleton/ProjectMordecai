// Project Mordecai — Identification Service (US-033)
//
// TODO(DECISION): Identification cost model unspecified; currently free.

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "MordecaiIdentificationService.generated.h"

class UMordecaiInventoryComponent;

/** Fires after a successful identification. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnMordecaiItemIdentified,
	UMordecaiInventoryComponent*, Inventory,
	FGuid, InstanceId);

/**
 * UMordecaiIdentificationService
 *
 * Global (GameInstance) service that flips FMordecaiItemInstance identification
 * state. Town-facing systems (Mage Tower NPC/UI) call IdentifyInstance; this
 * subsystem only handles the data mutation. Cost models are out of scope.
 */
UCLASS(BlueprintType)
class LYRAGAME_API UMordecaiIdentificationService : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Identify an instance in the given inventory. Returns true on success,
	 * false if the instance is not found or already identified.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Identification")
	bool IdentifyInstance(UMordecaiInventoryComponent* Inventory, const FGuid& InstanceId, FName ServiceName = FName(TEXT("MageTower")));

	UPROPERTY(BlueprintAssignable, Category = "Mordecai|Identification")
	FOnMordecaiItemIdentified OnItemIdentified;
};
