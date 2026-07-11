// Project Mordecai — Pickup Test Helpers (US-079)
// Delegate spy classes live in their own header so UHT can generate reflection.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Mordecai/Items/MordecaiItemPickup.h"
#include "MordecaiPickupTestHelpers.generated.h"

UCLASS()
class UMordecaiFocusedPickupDelegateSpy : public UObject
{
	GENERATED_BODY()
public:
	int32 EventCount = 0;
	TWeakObjectPtr<AMordecaiItemPickup> LastFocusedPickup;
	FText LastItemDisplayName;

	UFUNCTION()
	void Handle(AMordecaiItemPickup* FocusedPickup, const FText& ItemDisplayName)
	{
		EventCount++;
		LastFocusedPickup = FocusedPickup;
		LastItemDisplayName = ItemDisplayName;
	}
};
