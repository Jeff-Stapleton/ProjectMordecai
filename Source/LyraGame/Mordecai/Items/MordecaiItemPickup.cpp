// Project Mordecai — Item Pickup Actor (US-079)

#include "Mordecai/Items/MordecaiItemPickup.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Mordecai/Items/MordecaiInventoryComponent.h"
#include "Mordecai/Items/MordecaiItemDefinition.h"
#include "Mordecai/Items/MordecaiItemInstance.h"
#include "Mordecai/Items/MordecaiItemLibrary.h"
#include "Mordecai/Items/MordecaiPickupInteractionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiItemPickup)

AMordecaiItemPickup::AMordecaiItemPickup()
{
	PrimaryActorTick.bCanEverTick = false;
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
	// Diorama camera looks down from the south — tilt the label up toward it
	NameLabel->SetRelativeRotation(FRotator(45.f, 90.f, 0.f));
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

void AMordecaiItemPickup::RefreshNameLabel()
{
	if (NameLabel)
	{
		NameLabel->SetText(GetItemDisplayName());
	}
}
