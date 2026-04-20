// Project Mordecai — Resource Ledger (US-031)

#include "Mordecai/Items/MordecaiResourceLedger.h"
#include "Mordecai/Items/MordecaiItemDefinition.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiResourceLedger)

UMordecaiResourceLedger::UMordecaiResourceLedger(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UMordecaiResourceLedger::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UMordecaiResourceLedger, Entries);
}

int32 UMordecaiResourceLedger::FindEntryIndex(FName ItemId) const
{
	for (int32 Idx = 0; Idx < Entries.Num(); ++Idx)
	{
		if (Entries[Idx].ItemId == ItemId)
		{
			return Idx;
		}
	}
	return INDEX_NONE;
}

void UMordecaiResourceLedger::AddResource(UMordecaiItemDefinition* Def, int32 Quantity)
{
	if (!Def)
	{
		UE_LOG(LogTemp, Warning, TEXT("MORDECAI: ResourceLedger::AddResource called with null definition"));
		return;
	}
	if (!Def->IsAutoStored())
	{
		UE_LOG(LogTemp, Warning, TEXT("MORDECAI: ResourceLedger::AddResource rejected non-auto-stored item '%s'"),
			*Def->ItemId.ToString());
		return;
	}
	if (Quantity <= 0)
	{
		return;
	}

	int32 Idx = FindEntryIndex(Def->ItemId);
	int32 NewCount = 0;
	if (Idx == INDEX_NONE)
	{
		FMordecaiResourceEntry Entry;
		Entry.ItemId = Def->ItemId;
		Entry.Count = Quantity;
		Entry.Def = Def;
		NewCount = Quantity;
		Entries.Add(Entry);
	}
	else
	{
		Entries[Idx].Count += Quantity;
		Entries[Idx].Def = Def; // refresh cached pointer
		NewCount = Entries[Idx].Count;
	}

	OnResourceChanged.Broadcast(Def->ItemId, NewCount);
}

int32 UMordecaiResourceLedger::GetResourceCount(FName ItemId) const
{
	const int32 Idx = FindEntryIndex(ItemId);
	return (Idx == INDEX_NONE) ? 0 : Entries[Idx].Count;
}

bool UMordecaiResourceLedger::ConsumeResource(FName ItemId, int32 Quantity)
{
	if (Quantity <= 0)
	{
		return true;
	}

	const int32 Idx = FindEntryIndex(ItemId);
	if (Idx == INDEX_NONE || Entries[Idx].Count < Quantity)
	{
		return false; // atomic — no change
	}

	Entries[Idx].Count -= Quantity;
	const int32 NewCount = Entries[Idx].Count;
	if (NewCount == 0)
	{
		Entries.RemoveAt(Idx);
	}

	OnResourceChanged.Broadcast(ItemId, NewCount);
	return true;
}

TArray<FMordecaiResourceEntry> UMordecaiResourceLedger::GetAllResources() const
{
	return Entries;
}
