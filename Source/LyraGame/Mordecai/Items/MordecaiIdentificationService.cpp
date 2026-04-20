// Project Mordecai — Identification Service (US-033)

#include "Mordecai/Items/MordecaiIdentificationService.h"
#include "Mordecai/Items/MordecaiInventoryComponent.h"
#include "Mordecai/Items/MordecaiItemInstance.h"
#include "Mordecai/Items/MordecaiItemDefinition.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiIdentificationService)

bool UMordecaiIdentificationService::IdentifyInstance(UMordecaiInventoryComponent* Inventory, const FGuid& InstanceId, FName ServiceName)
{
	if (!Inventory)
	{
		return false;
	}

	const FMordecaiItemInstance* Existing = Inventory->FindInstance(InstanceId);
	if (!Existing || !Existing->IsValid())
	{
		return false;
	}
	if (Existing->IsIdentified())
	{
		return false; // already identified
	}

	if (!Inventory->SetInstanceIdentificationState(InstanceId, EMordecaiIdentificationState::Identified))
	{
		return false;
	}

	OnItemIdentified.Broadcast(Inventory, InstanceId);
	return true;
}
