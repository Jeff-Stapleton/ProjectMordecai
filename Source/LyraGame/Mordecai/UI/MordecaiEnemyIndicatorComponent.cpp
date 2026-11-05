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
	IndicatorWidgetClass = UMordecaiEnemyIndicatorWidget::StaticClass();
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
	bIndicatorRequested = false;
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
	bIndicatorRequested = true;
	RegisterIndicator();
}

void UMordecaiEnemyIndicatorComponent::NotifyAggroActivated()
{
	bIndicatorRequested = true;
	RegisterIndicator();
}

// ---------------------------------------------------------------------------
// Death (AC-064.3)
// ---------------------------------------------------------------------------

void UMordecaiEnemyIndicatorComponent::NotifyDeath()
{
	bIndicatorRequested = false;
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
	if (!bIndicatorRequested || bIndicatorRegistered)
	{
		return;
	}

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

	UIndicatorDescriptor* Descriptor = NewObject<UIndicatorDescriptor>(this);
	Descriptor->SetSceneComponent(RootComp);
	Descriptor->SetWorldPositionOffset(FVector(0.0, 0.0, IndicatorHeightOffset));
	Descriptor->SetAutoRemoveWhenIndicatorComponentIsNull(true);
	Descriptor->SetHAlign(HAlign_Center);
	Descriptor->SetVAlign(VAlign_Bottom);
	Descriptor->SetDataObject(Owner);

	if (!IndicatorWidgetClass.IsNull())
	{
		Descriptor->SetIndicatorClass(IndicatorWidgetClass);
	}

	Manager->AddIndicator(Descriptor);
	ActiveDescriptor = Descriptor;
	bIndicatorRegistered = true;
}

void UMordecaiEnemyIndicatorComponent::UnregisterIndicator()
{
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
	if (bIndicatorRequested && !bIndicatorRegistered)
	{
		RegisterIndicator();
	}

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
