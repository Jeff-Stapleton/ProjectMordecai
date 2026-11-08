// Project Mordecai — Reflected delegate observers for the recovery projectile tests (issue #88)

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "Mordecai/AbilitySystem/MordecaiAbilitySystemComponent.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiHitDetectionTypes.h"
#include "UObject/Object.h"

#include "MordecaiRecoveryProjectileTestHelpers.generated.h"

/** Minimal real Pawn-channel blocker with an actor-owned ASC, used only by issue #88's isolated test world. */
UCLASS()
class AMordecaiRecoveryProjectileASCTestPawn : public APawn, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AMordecaiRecoveryProjectileASCTestPawn()
	{
		Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
		SetRootComponent(Collision);
		Collision->InitSphereRadius(45.f);
		Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Collision->SetGenerateOverlapEvents(true);
		Collision->SetCollisionObjectType(ECC_Pawn);
		Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
		Collision->SetCollisionResponseToChannel(Mordecai_TraceChannel_ProjectileDetection, ECR_Block);

		AbilitySystem = CreateDefaultSubobject<UMordecaiAbilitySystemComponent>(TEXT("AbilitySystem"));
		Attributes = CreateDefaultSubobject<UMordecaiAttributeSet>(TEXT("Attributes"));
	}

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override
	{
		return AbilitySystem;
	}

	USphereComponent* GetCollision() const
	{
		return Collision;
	}

protected:
	virtual void PostInitializeComponents() override
	{
		Super::PostInitializeComponents();
		AbilitySystem->InitAbilityActorInfo(this, this);
		AbilitySystem->SetNumericAttributeBase(UMordecaiAttributeSet::GetMaxHealthAttribute(), 100.f);
		AbilitySystem->SetNumericAttributeBase(UMordecaiAttributeSet::GetHealthAttribute(), 100.f);
	}

private:
	UPROPERTY()
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY()
	TObjectPtr<UMordecaiAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<UMordecaiAttributeSet> Attributes;
};

/** Observes the engine's real primitive-component overlap delegate; it never calls projectile code itself. */
UCLASS()
class UMordecaiRecoveryProjectileOverlapSpy : public UObject
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<AActor> ExpectedActor;
	int32 ExpectedActorCallbacks = 0;

	UFUNCTION()
	void HandleComponentBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult)
	{
		(void)OverlappedComponent;
		(void)OtherComponent;
		(void)OtherBodyIndex;
		(void)bFromSweep;
		(void)SweepResult;
		if (OtherActor == ExpectedActor)
		{
			++ExpectedActorCallbacks;
		}
	}
};
