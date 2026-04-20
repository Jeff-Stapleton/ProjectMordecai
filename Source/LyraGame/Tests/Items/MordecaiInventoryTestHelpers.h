// Project Mordecai — Inventory Test Helpers (US-031)
// Delegate spy classes live in their own header so UHT can generate reflection.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MordecaiInventoryTestHelpers.generated.h"

UCLASS()
class UMordecaiInventoryDelegateSpy : public UObject
{
	GENERATED_BODY()
public:
	int32 AddEvents = 0;
	int32 RemoveEvents = 0;
	int32 LastDelta = 0;

	UFUNCTION()
	void Handle(const FGuid& InstanceId, int32 Delta)
	{
		LastDelta = Delta;
		if (Delta > 0) AddEvents++;
		else RemoveEvents++;
	}
};

UCLASS()
class UMordecaiLedgerDelegateSpy : public UObject
{
	GENERATED_BODY()
public:
	int32 EventCount = 0;
	FName LastItemId;
	int32 LastCount = 0;

	UFUNCTION()
	void Handle(FName ItemId, int32 NewCount)
	{
		EventCount++;
		LastItemId = ItemId;
		LastCount = NewCount;
	}
};
