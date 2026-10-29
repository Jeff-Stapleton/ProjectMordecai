// Project Mordecai — Item Pickup Actor (US-079)

#include "Mordecai/Items/MordecaiItemPickup.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Mordecai/Items/MordecaiInventoryComponent.h"
#include "Mordecai/Items/MordecaiItemDefinition.h"
#include "Mordecai/Items/MordecaiItemInstance.h"
#include "Mordecai/Items/MordecaiItemLibrary.h"
#include "Mordecai/Items/MordecaiPickupInteractionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiItemPickup)

AMordecaiItemPickup::AMordecaiItemPickup()
{
	// Ticks only to turn the name label to the local camera: after the camera update (engine LevelTick.cpp
	// updates cameras between TG_PostPhysics and TG_PostUpdateWork), and never on a server without one
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	PrimaryActorTick.bAllowTickOnDedicatedServer = false;
	bReplicates = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	PickupTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("PickupTrigger"));
	PickupTrigger->SetupAttachment(Root);
	PickupTrigger->InitSphereRadius(120.0f);
	PickupTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PickupTrigger->SetGenerateOverlapEvents(true);

	DisplayMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DisplayMesh"));
	DisplayMesh->SetupAttachment(Root);
	DisplayMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	NameLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameLabel"));
	NameLabel->SetupAttachment(Root);
	NameLabel->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
	// A billboard: its rotation is a world rotation, whatever the actor's. Until Tick turns it to the live
	// camera it faces the default diorama camera (pitch -50, yaw 0: ComputeNameLabelRotation of that view).
	NameLabel->SetUsingAbsoluteRotation(true);
	NameLabel->SetRelativeRotation(FRotator(50.f, 180.f, 0.f));
	NameLabel->SetHorizontalAlignment(EHTA_Center);
	NameLabel->SetWorldSize(22.f);
	NameLabel->SetTextRenderColor(FColor::White);
}

void AMordecaiItemPickup::BeginPlay()
{
	Super::BeginPlay();

	RefreshNameLabel();

	if (PickupTrigger)
	{
		PickupTrigger->OnComponentBeginOverlap.AddDynamic(this, &AMordecaiItemPickup::OnTriggerBeginOverlap);
		PickupTrigger->OnComponentEndOverlap.AddDynamic(this, &AMordecaiItemPickup::OnTriggerEndOverlap);
	}
}

bool AMordecaiItemPickup::ExecutePickup(AActor* Collector)
{
	if (!HasAuthority() || !ItemDefinition || !Collector)
	{
		return false;
	}

	UMordecaiInventoryComponent* Inventory = Collector->FindComponentByClass<UMordecaiInventoryComponent>();
	if (!Inventory)
	{
		return false;
	}

	// PickupItem handles the auto-store routing (materials/town resources → ledger)
	Inventory->PickupItem(ItemDefinition, Quantity);

	// Drop out of the collector's focus set before (possibly) destroying
	if (UMordecaiPickupInteractionComponent* Interaction = Collector->FindComponentByClass<UMordecaiPickupInteractionComponent>())
	{
		Interaction->UnregisterPickup(this);
	}

	if (bDestroyOnPickup)
	{
		Destroy();
	}
	return true;
}

FText AMordecaiItemPickup::GetItemDisplayName() const
{
	if (!ItemDefinition)
	{
		return FText::GetEmpty();
	}

	// Transient instance so the library applies the identification display rules
	FMordecaiItemInstance Preview;
	Preview.ItemDefinition = ItemDefinition;
	Preview.Quantity = Quantity;
	Preview.IdentificationState = ItemDefinition->UsesIdentification
		? ItemDefinition->DefaultIdentificationState
		: EMordecaiIdentificationState::Identified;
	return UMordecaiItemLibrary::GetDisplayName(Preview);
}

float AMordecaiItemPickup::GetTriggerRadius() const
{
	return PickupTrigger ? PickupTrigger->GetScaledSphereRadius() : 120.0f;
}

void AMordecaiItemPickup::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this)
	{
		return;
	}
	if (UMordecaiPickupInteractionComponent* Interaction = OtherActor->FindComponentByClass<UMordecaiPickupInteractionComponent>())
	{
		Interaction->RegisterPickup(this);
	}
}

void AMordecaiItemPickup::OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor)
	{
		return;
	}
	if (UMordecaiPickupInteractionComponent* Interaction = OtherActor->FindComponentByClass<UMordecaiPickupInteractionComponent>())
	{
		Interaction->UnregisterPickup(this);
	}
}

void AMordecaiItemPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// The first local player's camera (a listen server's first controller may be a remote one)
	const APlayerCameraManager* LocalCamera = nullptr;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It && !LocalCamera; ++It)
	{
		const APlayerController* Controller = It->Get();
		if (Controller && Controller->IsLocalController())
		{
			LocalCamera = Controller->PlayerCameraManager.Get();
		}
	}
	FaceNameLabelTo(LocalCamera);
}

FRotator AMordecaiItemPickup::ComputeNameLabelRotation(const FRotator& CameraRotation)
{
	const FRotationMatrix View(CameraRotation);
	return FRotationMatrix::MakeFromXZ(-View.GetUnitAxis(EAxis::X), View.GetUnitAxis(EAxis::Z)).Rotator();
}

bool AMordecaiItemPickup::FaceNameLabelTo(const APlayerCameraManager* Camera)
{
	// A camera that never updated has no view yet (the rule APlayerController::GetPlayerViewPoint uses)
	if (!NameLabel || !Camera || Camera->GetCameraCacheTime() <= 0.f)
	{
		return false;
	}
	// Absolute rotation: the relative rotation is the world rotation, and setting an unchanged one is free
	NameLabel->SetRelativeRotation(ComputeNameLabelRotation(Camera->GetCameraRotation()));
	return true;
}

void AMordecaiItemPickup::RefreshNameLabel()
{
	if (NameLabel)
	{
		NameLabel->SetText(GetItemDisplayName());
	}
}
