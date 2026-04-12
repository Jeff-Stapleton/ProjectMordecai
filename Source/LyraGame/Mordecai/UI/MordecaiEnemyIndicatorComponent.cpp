// Project Mordecai — Enemy Indicator Component (US-064)

#include "Mordecai/UI/MordecaiEnemyIndicatorComponent.h"
#include "Mordecai/UI/MordecaiEnemyIndicatorWidget.h"
#include "UI/IndicatorSystem/LyraIndicatorManagerComponent.h"
#include "UI/IndicatorSystem/IndicatorDescriptor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiEnemyIndicatorComponent)

UMordecaiEnemyIndicatorComponent::UMordecaiEnemyIndicatorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void UMordecaiEnemyIndicatorComponent::BeginPlay()
{
	Super::BeginPlay();

	// Start visibility check timer (every 0.25s)
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			VisibilityTimerHandle,
			this,
			&UMordecaiEnemyIndicatorComponent::UpdateVisibility,
			0.25f,
			/*bLoop=*/true
		);
	}
}

void UMordecaiEnemyIndicatorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterIndicator();

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(VisibilityTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// Registration Triggers (AC-064.2)
// ---------------------------------------------------------------------------

void UMordecaiEnemyIndicatorComponent::NotifyDamageReceived()
{
	if (!bIndicatorRegistered)
	{
		RegisterIndicator();
	}
}

void UMordecaiEnemyIndicatorComponent::NotifyAggroActivated()
{
	if (!bIndicatorRegistered)
	{
		RegisterIndicator();
	}
}

// ---------------------------------------------------------------------------
// Death (AC-064.3)
// ---------------------------------------------------------------------------

void UMordecaiEnemyIndicatorComponent::NotifyDeath()
{
	UnregisterIndicator();
}

// ---------------------------------------------------------------------------
// Queries (AC-064.4)
// ---------------------------------------------------------------------------

bool UMordecaiEnemyIndicatorComponent::IsWithinVisibilityRange(float Distance) const
{
	return Distance <= IndicatorVisibilityRange;
}

// ---------------------------------------------------------------------------
// Registration / Unregistration
// ---------------------------------------------------------------------------

void UMordecaiEnemyIndicatorComponent::RegisterIndicator()
{
	if (bIndicatorRegistered)
	{
		return;
	}

	bIndicatorRegistered = true;

	ULyraIndicatorManagerComponent* Manager = FindIndicatorManager();
	if (!Manager)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	USceneComponent* RootComp = Owner->GetRootComponent();
	if (!RootComp)
	{
		return;
	}

	// Create the descriptor
	ActiveDescriptor = NewObject<UIndicatorDescriptor>(this);
	ActiveDescriptor->SetSceneComponent(RootComp);
	ActiveDescriptor->SetWorldPositionOffset(FVector(0.0, 0.0, IndicatorHeightOffset));
	ActiveDescriptor->SetAutoRemoveWhenIndicatorComponentIsNull(true);
	ActiveDescriptor->SetHAlign(HAlign_Center);
	ActiveDescriptor->SetVAlign(VAlign_Bottom);
	ActiveDescriptor->SetDataObject(Owner);

	if (!IndicatorWidgetClass.IsNull())
	{
		ActiveDescriptor->SetIndicatorClass(IndicatorWidgetClass);
	}

	Manager->AddIndicator(ActiveDescriptor);
}

void UMordecaiEnemyIndicatorComponent::UnregisterIndicator()
{
	if (!bIndicatorRegistered)
	{
		return;
	}

	bIndicatorRegistered = false;

	if (ActiveDescriptor)
	{
		ActiveDescriptor->UnregisterIndicator();
		ActiveDescriptor = nullptr;
	}
}

// ---------------------------------------------------------------------------
// Visibility (AC-064.4)
// ---------------------------------------------------------------------------

void UMordecaiEnemyIndicatorComponent::UpdateVisibility()
{
	if (!bIndicatorRegistered || !ActiveDescriptor)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->GetPawn())
	{
		return;
	}

	const float Distance = FVector::Dist(Owner->GetActorLocation(), PC->GetPawn()->GetActorLocation());
	const bool bShouldBeVisible = IsWithinVisibilityRange(Distance);
	ActiveDescriptor->SetDesiredVisibility(bShouldBeVisible);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

ULyraIndicatorManagerComponent* UMordecaiEnemyIndicatorComponent::FindIndicatorManager() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		return nullptr;
	}

	return ULyraIndicatorManagerComponent::GetComponent(PC);
}
