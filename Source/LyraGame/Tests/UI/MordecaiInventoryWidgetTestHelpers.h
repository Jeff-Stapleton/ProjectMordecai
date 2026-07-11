// Project Mordecai — Inventory Widget Test Helpers (US-071)
// Delegate spy classes live in their own header so UHT can generate reflection.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MordecaiInventoryWidgetTestHelpers.generated.h"

UCLASS()
class UMordecaiInventoryFilterDelegateSpy : public UObject
{
	GENERATED_BODY()
public:
	int32 EventCount = 0;
	FName LastFilterId;

	UFUNCTION()
	void Handle(FName NewFilterId)
	{
		EventCount++;
		LastFilterId = NewFilterId;
	}
};
