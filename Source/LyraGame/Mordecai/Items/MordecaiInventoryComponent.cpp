// Project Mordecai — Inventory Component (US-031)

#include "Mordecai/Items/MordecaiInventoryComponent.h"
#include "Mordecai/Items/MordecaiItemDefinition.h"
#include "Mordecai/Items/MordecaiItemLibrary.h"
#include "Mordecai/Items/MordecaiResourceLedger.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiInventoryComponent)

UMordecaiInventoryComponent::UMordecaiInventoryComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UMordecaiInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UMordecaiInventoryComponent, ItemList);
}

// ---------------------------------------------------------------------------
// Pickup routing
// ---------------------------------------------------------------------------

void UMordecaiInventoryComponent::PickupItem(UMordecaiItemDefinition* Def, int32 Quantity)
{
	if (!Def || Quantity <= 0)
	{
		return;
	}

	if (Def->IsAutoStored())
	{
		if (ResourceLedger)
		{
			ResourceLedger->AddResource(Def, Quantity);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("MORDECAI: InventoryComponent has no ResourceLedger — auto-stored '%s' dropped"),
				*Def->ItemId.ToString());
		}
		return;
	}

	AddItem(Def, Quantity);
}

// ---------------------------------------------------------------------------
// Add / Remove
// ---------------------------------------------------------------------------

FGuid UMordecaiInventoryComponent::AddItem(UMordecaiItemDefinition* Def, int32 Quantity)
{
	if (!Def || Quantity <= 0)
	{
		return FGuid();
	}

	// Auto-stored items route through the ledger — inventory unchanged
	if (Def->IsAutoStored())
	{
		if (ResourceLedger)
		{
			ResourceLedger->AddResource(Def, Quantity);
		}
		return FGuid();
	}

	FGuid FirstModified;
	int32 Remaining = Quantity;

	// Try to merge into existing non-full stacks if stackable
	if (Def->IsStackable())
	{
		const int32 MaxStack = Def->MaxStackSize;
		for (FMordecaiItemInstance& Inst : ItemList.Items)
		{
			if (Remaining <= 0) break;
			if (Inst.ItemDefinition != Def) continue;
			if (Inst.Quantity >= MaxStack) continue;

			const int32 Space = MaxStack - Inst.Quantity;
			const int32 ToAdd = FMath::Min(Remaining, Space);
			Inst.Quantity += ToAdd;
			Remaining -= ToAdd;
			if (!FirstModified.IsValid())
			{
				FirstModified = Inst.InstanceId;
			}
			OnInventoryChanged.Broadcast(Inst.InstanceId, ToAdd);
			ItemList.MarkItemDirty(Inst);
		}
	}

	// Create new stacks for any remaining quantity
	while (Remaining > 0)
	{
		const int32 ThisStack = Def->IsStackable()
			? FMath::Min(Remaining, Def->MaxStackSize)
			: 1;

		FMordecaiItemInstance NewInst;
		NewInst.InstanceId = FGuid::NewGuid();
		NewInst.ItemDefinition = Def;
		NewInst.Quantity = ThisStack;
		NewInst.IsEquipped = false;
		ItemList.Items.Add(NewInst);
		ItemList.MarkArrayDirty();

		OnInventoryChanged.Broadcast(NewInst.InstanceId, ThisStack);

		if (!FirstModified.IsValid())
		{
			FirstModified = NewInst.InstanceId;
		}
		Remaining -= ThisStack;
	}

	return FirstModified;
}

bool UMordecaiInventoryComponent::RemoveItem(const FGuid& InstanceId, int32 Quantity)
{
	if (Quantity <= 0)
	{
		return false;
	}

	for (int32 Idx = 0; Idx < ItemList.Items.Num(); ++Idx)
	{
		FMordecaiItemInstance& Inst = ItemList.Items[Idx];
		if (Inst.InstanceId != InstanceId) continue;

		if (Inst.Quantity < Quantity)
		{
			return false;
		}

		Inst.Quantity -= Quantity;
		const FGuid CapturedId = Inst.InstanceId;

		OnInventoryChanged.Broadcast(CapturedId, -Quantity);

		if (Inst.Quantity <= 0)
		{
			ItemList.Items.RemoveAt(Idx);
			ItemList.MarkArrayDirty();
		}
		else
		{
			ItemList.MarkItemDirty(Inst);
		}
		return true;
	}
	return false;
}

bool UMordecaiInventoryComponent::ConsumeByDefinition(UMordecaiItemDefinition* Def, int32 Quantity)
{
	if (!Def || Quantity <= 0)
	{
		return false;
	}

	// Atomicity check: sum matching stacks first
	int32 Total = 0;
	for (const FMordecaiItemInstance& Inst : ItemList.Items)
	{
		if (Inst.ItemDefinition == Def)
		{
			Total += Inst.Quantity;
		}
	}
	if (Total < Quantity)
	{
		return false;
	}

	// Gather indices of matching stacks, sorted by quantity ascending (prefer lowest)
	TArray<int32> MatchIndices;
	for (int32 Idx = 0; Idx < ItemList.Items.Num(); ++Idx)
	{
		if (ItemList.Items[Idx].ItemDefinition == Def)
		{
			MatchIndices.Add(Idx);
		}
	}
	MatchIndices.Sort([this](int32 A, int32 B)
	{
		return ItemList.Items[A].Quantity < ItemList.Items[B].Quantity;
	});

	// Drain in ascending-quantity order; decrement then remove empties.
	int32 Remaining = Quantity;
	TArray<int32> ToRemove;
	for (int32 MatchIdx : MatchIndices)
	{
		if (Remaining <= 0) break;
		FMordecaiItemInstance& Inst = ItemList.Items[MatchIdx];
		const int32 Take = FMath::Min(Remaining, Inst.Quantity);
		Inst.Quantity -= Take;
		Remaining -= Take;
		OnInventoryChanged.Broadcast(Inst.InstanceId, -Take);

		if (Inst.Quantity <= 0)
		{
			ToRemove.Add(MatchIdx);
		}
		else
		{
			ItemList.MarkItemDirty(Inst);
		}
	}

	// Remove empties in descending order to keep indices valid
	ToRemove.Sort([](int32 A, int32 B) { return A > B; });
	for (int32 Idx : ToRemove)
	{
		ItemList.Items.RemoveAt(Idx);
	}
	if (ToRemove.Num() > 0)
	{
		ItemList.MarkArrayDirty();
	}

	return true;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

TArray<FMordecaiItemInstance> UMordecaiInventoryComponent::GetItemsByType(EMordecaiItemType Type) const
{
	TArray<FMordecaiItemInstance> Result;
	for (const FMordecaiItemInstance& Inst : ItemList.Items)
	{
		if (Inst.ItemDefinition && Inst.ItemDefinition->ItemType == Type)
		{
			Result.Add(Inst);
		}
	}
	return Result;
}

TArray<FMordecaiItemInstance> UMordecaiInventoryComponent::GetSortedItems() const
{
	TArray<FMordecaiItemInstance> Result = ItemList.Items;
	Result.Sort([](const FMordecaiItemInstance& A, const FMordecaiItemInstance& B)
	{
		return UMordecaiItemLibrary::CompareSortPriority(A.ItemDefinition, B.ItemDefinition) < 0;
	});
	return Result;
}

const FMordecaiItemInstance* UMordecaiInventoryComponent::FindInstance(const FGuid& InstanceId) const
{
	for (const FMordecaiItemInstance& Inst : ItemList.Items)
	{
		if (Inst.InstanceId == InstanceId)
		{
			return &Inst;
		}
	}
	return nullptr;
}

int32 UMordecaiInventoryComponent::GetTotalQuantityOfDefinition(UMordecaiItemDefinition* Def) const
{
	if (!Def)
	{
		return 0;
	}
	int32 Total = 0;
	for (const FMordecaiItemInstance& Inst : ItemList.Items)
	{
		if (Inst.ItemDefinition == Def)
		{
			Total += Inst.Quantity;
		}
	}
	return Total;
}
