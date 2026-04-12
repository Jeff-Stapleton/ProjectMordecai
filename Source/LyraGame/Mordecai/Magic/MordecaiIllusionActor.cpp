// Project Mordecai — Illusion Decoy Actor (US-060)

#include "Mordecai/Magic/MordecaiIllusionActor.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Components/CapsuleComponent.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiIllusionActor)

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AMordecaiIllusionActor::AMordecaiIllusionActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	CapsuleComp->InitCapsuleSize(42.f, 96.f);
	CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CapsuleComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetRootComponent(CapsuleComp);

	bReplicates = true;
}

// ---------------------------------------------------------------------------
// InitDecoy (AC-060.2)
// ---------------------------------------------------------------------------

void AMordecaiIllusionActor::InitDecoy(float InMaxHP, float InDurationSec)
{
	MaxHP = InMaxHP;
	CurrentHP = InMaxHP;
	DurationSec = InDurationSec;
}

// ---------------------------------------------------------------------------
// BeginPlay
// ---------------------------------------------------------------------------

void AMordecaiIllusionActor::BeginPlay()
{
	Super::BeginPlay();

	// Start duration timer (AC-060.2)
	if (DurationSec > 0.f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				DurationTimerHandle,
				this, &AMordecaiIllusionActor::OnDurationExpired,
				DurationSec,
				false);
		}
	}
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

FGameplayTag AMordecaiIllusionActor::GetTeamTag() const
{
	return MordecaiGameplayTags::Team_Ally;
}

void AMordecaiIllusionActor::ApplyDamage(float DamageAmount)
{
	if (!IsAlive())
	{
		return;
	}

	CurrentHP = FMath::Max(0.f, CurrentHP - DamageAmount);

	if (CurrentHP <= 0.f)
	{
		Destroy();
	}
}

FVector AMordecaiIllusionActor::ComputeSpawnLocation(
	const FVector& CasterLocation,
	const FVector& CasterForward,
	float SpawnDistance)
{
	FVector Forward = CasterForward.GetSafeNormal();
	if (Forward.IsNearlyZero())
	{
		Forward = FVector::ForwardVector;
	}
	return CasterLocation + Forward * SpawnDistance;
}

// ---------------------------------------------------------------------------
// Duration Expiry (AC-060.2)
// ---------------------------------------------------------------------------

void AMordecaiIllusionActor::OnDurationExpired()
{
	bExpired = true;
	Destroy();
}
