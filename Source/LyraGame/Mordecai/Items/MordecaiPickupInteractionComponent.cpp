// Project Mordecai — Pickup Interaction Component (US-079)

#include "Mordecai/Items/MordecaiPickupInteractionComponent.h"

#include "GameFramework/Actor.h"
#include "Mordecai/Items/MordecaiItemPickup.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiPickupInteractionComponent)

UMordecaiPickupInteractionComponent::UMordecaiPickupInteractionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	// Coarse focus refresh — only relevant while standing between pickups
	PrimaryComponentTick.TickInterval = 0.15f;
}

void UMordecaiPickupInteractionComponent::RegisterPickup(AMordecaiItemPickup* Pickup)
{
	if (!Pickup)
	{
		return;
	}
	if (!InRangePickups.Contains(Pickup))
	{
		InRangePickups.Add(Pickup);
	}
	EvaluateFocus();
}

void UMordecaiPickupInteractionComponent::UnregisterPickup(AMordecaiItemPickup* Pickup)
{
	if (!Pickup)
	{
		return;
	}
	InRangePickups.Remove(Pickup);
	EvaluateFocus();
}

int32 UMordecaiPickupInteractionComponent::GetInRangePickupCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AMordecaiItemPickup>& Weak : InRangePickups)
	{
		const AMordecaiItemPickup* Pickup = Weak.Get();
		if (Pickup && !Pickup->IsActorBeingDestroyed())
		{
			++Count;
		}
	}
	return Count;
}

bool UMordecaiPickupInteractionComponent::TryPickupFocused()
{
	EvaluateFocus();

	AMordecaiItemPickup* Pickup = FocusedPickup.Get();
	AActor* Owner = GetOwner();
	if (!Pickup || !Owner)
	{
		return false;
	}

	if (Owner->HasAuthority())
	{
		const bool bPicked = Pickup->ExecutePickup(Owner);
		// ExecutePickup unregisters on success; re-evaluate for the reject path too
		EvaluateFocus();
		return bPicked;
	}

	ServerTryPickupFocused(Pickup);
	return true; // request dispatched — server re-validates
}

void UMordecaiPickupInteractionComponent::EvaluateFocus()
{
	InRangePickups.RemoveAll([](const TWeakObjectPtr<AMordecaiItemPickup>& Weak)
	{
		const AMordecaiItemPickup* Pickup = Weak.Get();
		return !Pickup || Pickup->IsActorBeingDestroyed();
	});

	AMordecaiItemPickup* Nearest = nullptr;
	if (const AActor* Owner = GetOwner())
	{
		const FVector OwnerLocation = Owner->GetActorLocation();
		float BestDistSq = TNumericLimits<float>::Max();
		for (const TWeakObjectPtr<AMordecaiItemPickup>& Weak : InRangePickups)
		{
			AMordecaiItemPickup* Pickup = Weak.Get();
			if (!Pickup)
			{
				continue;
			}
			const float DistSq = FVector::DistSquared(OwnerLocation, Pickup->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Nearest = Pickup;
			}
		}
	}

	SetComponentTickEnabled(InRangePickups.Num() > 0);

	if (Nearest != FocusedPickup.Get())
	{
		FocusedPickup = Nearest;
		OnFocusedPickupChanged.Broadcast(Nearest, Nearest ? Nearest->GetItemDisplayName() : FText::GetEmpty());
	}
}

void UMordecaiPickupInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	EvaluateFocus();
}

bool UMordecaiPickupInteractionComponent::ServerTryPickupFocused_Validate(AMordecaiItemPickup* Pickup)
{
	return true;
}

void UMordecaiPickupInteractionComponent::ServerTryPickupFocused_Implementation(AMordecaiItemPickup* Pickup)
{
	AActor* Owner = GetOwner();
	if (!Pickup || !Owner || Pickup->IsActorBeingDestroyed())
	{
		return;
	}

	// Server-side range re-validation: reject requests for out-of-reach pickups
	// (1.5x slop covers trigger-edge + one movement frame of client/server drift)
	const float MaxDist = Pickup->GetTriggerRadius() * 1.5f;
	if (FVector::DistSquared(Owner->GetActorLocation(), Pickup->GetActorLocation()) > FMath::Square(MaxDist))
	{
		return;
	}

	Pickup->ExecutePickup(Owner);
	EvaluateFocus();
}
