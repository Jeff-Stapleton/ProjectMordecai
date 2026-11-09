// Project Mordecai — Blink Spell (US-021)

#include "Mordecai/Magic/MordecaiGA_Blink.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiGA_Blink::UMordecaiGA_Blink(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// AC-021.11: Tag this ability
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(MordecaiGameplayTags::Ability_Spell_Blink);
	SetAssetTags(Tags);
}

// ---------------------------------------------------------------------------
// Public Getters
// ---------------------------------------------------------------------------

float UMordecaiGA_Blink::GetTeleportRange() const
{
	if (!SpellData) return 0.f;
	return SpellData->Range;
}

float UMordecaiGA_Blink::GetInvulnerabilityDuration() const
{
	if (!SpellData) return 0.f;
	return SpellData->BuffDuration;
}

FGameplayTag UMordecaiGA_Blink::GetInvulnerabilityTag() const
{
	return MordecaiGameplayTags::State_Invulnerable;
}

// ---------------------------------------------------------------------------
// ComputeTeleportDestination (AC-021.1, AC-021.2)
// ---------------------------------------------------------------------------

FVector UMordecaiGA_Blink::ComputeTeleportDestination(
	const FVector& Origin,
	const FVector& AimDirection,
	float Range,
	bool bWallHit,
	const FVector& WallHitPoint)
{
	FVector Direction = AimDirection.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		return Origin;
	}

	if (!bWallHit)
	{
		// No wall: teleport full range
		return Origin + Direction * Range;
	}

	// AC-021.2: Clamp to wall hit point minus buffer
	float DistToWall = FVector::Dist(Origin, WallHitPoint);
	float ClampedDist = FMath::Max(0.f, DistToWall - WallBuffer);

	return Origin + Direction * ClampedDist;
}

// ---------------------------------------------------------------------------
// Activation and destination resolution (AC-021.1, AC-021.2, AC-021.3)
// ---------------------------------------------------------------------------

void UMordecaiGA_Blink::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	AActor* AvatarActor = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	FVector Origin;
	FVector Destination;

	// Blink is configured as an instant cast. Move before the spell base commits SP, cooldown, cast event or
	// invulnerability so a zero/failed teleport has no cast side effects, and never cache a destination across a windup.
	const bool bInstantCast = FMath::IsNearlyZero(GetWindupDuration(), UE_KINDA_SMALL_NUMBER)
		&& FMath::IsNearlyZero(GetCastDuration(), UE_KINDA_SMALL_NUMBER);
	if (!bInstantCast || !TryResolveTeleportDestination(AvatarActor, Origin, Destination))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const bool bMoveSucceeded = AvatarActor->SetActorLocation(
		Destination, false, nullptr, ETeleportType::TeleportPhysics);
	const FVector ActualDelta = AvatarActor->GetActorLocation() - Origin;
	if (!bMoveSucceeded || ActualDelta.IsNearlyZero(UE_KINDA_SMALL_NUMBER))
	{
		if (!ActualDelta.IsNearlyZero(UE_KINDA_SMALL_NUMBER))
		{
			AvatarActor->SetActorLocation(Origin, false, nullptr, ETeleportType::TeleportPhysics);
		}
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Local prediction and the authoritative server each resolve and apply the same collision-constrained move.
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

bool UMordecaiGA_Blink::TryResolveTeleportDestination(
	AActor* AvatarActor,
	FVector& OutOrigin,
	FVector& OutDestination) const
{
	UWorld* World = AvatarActor ? AvatarActor->GetWorld() : nullptr;
	const float Range = GetTeleportRange();
	if (!SpellData || !World || Range <= UE_KINDA_SMALL_NUMBER)
	{
		return false;
	}

	OutOrigin = AvatarActor->GetActorLocation();
	const FVector AimDir = AvatarActor->GetActorForwardVector().GetSafeNormal();
	if (AimDir.IsNearlyZero())
	{
		return false;
	}

	// AC-021.2: Line trace to prevent teleporting through walls
	bool bWallHit = false;
	FVector WallHitPoint = FVector::ZeroVector;
	const FVector TraceEnd = OutOrigin + AimDir * Range;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MordecaiBlinkPath), false, AvatarActor);

	// Keep the existing ECC_WorldStatic response semantics for every non-Pawn component. If a Pawn blocks that
	// channel, ignore that actor locally and continue the same trace so geometry behind it still clamps the Blink.
	for (;;)
	{
		FHitResult HitResult;
		if (!World->LineTraceSingleByChannel(HitResult, OutOrigin, TraceEnd, ECC_WorldStatic, QueryParams))
		{
			break;
		}
		if (APawn* HitPawn = Cast<APawn>(HitResult.GetActor()))
		{
			QueryParams.AddIgnoredActor(HitPawn);
			continue;
		}
		bWallHit = true;
		WallHitPoint = HitResult.ImpactPoint;
		break;
	}

	const FVector DesiredDestination =
		ComputeTeleportDestination(OutOrigin, AimDir, Range, bWallHit, WallHitPoint);
	float Distance = FVector::DotProduct(DesiredDestination - OutOrigin, AimDir);
	if (Distance <= UE_KINDA_SMALL_NUMBER)
	{
		return false;
	}

	// Test the real actor/capsule and search only backward on the cast line. This avoids FindTeleportSpot's
	// sideways/upward MTD adjustments while retaining positive partial movement when the exact endpoint is occupied.
	constexpr float Backstep = 1.f;
	while (Distance > UE_KINDA_SMALL_NUMBER)
	{
		OutDestination = OutOrigin + AimDir * Distance;
		if (!World->EncroachingBlockingGeometry(
			AvatarActor, OutDestination, AvatarActor->GetActorRotation()))
		{
			return true;
		}
		Distance = FMath::Max(0.f, Distance - Backstep);
	}

	return false;
}

void UMordecaiGA_Blink::OnSpellCast()
{
	// Movement succeeded before the spell base committed this instant cast.
	ApplyInvulnerability();
}

// ---------------------------------------------------------------------------
// Invulnerability (AC-021.3)
// ---------------------------------------------------------------------------

void UMordecaiGA_Blink::ApplyInvulnerability()
{
	const float Duration = GetInvulnerabilityDuration();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (Duration <= 0.f || !ASC)
	{
		return;
	}

	// A duration GE grants the tag, so GAS removes it when the duration ends. The invulnerability (BuffDuration 0.3 s in
	// DA_Spell_Blink) outlasts the spell (RecoveryTime 0.2 s), and EndAbility clears every timer bound to the ability:
	// a loose tag removed by such a timer was never removed (N19, US-101). Unnamed, so each cast gets its own definition.
	UGameplayEffect* InvulnerabilityGE = NewObject<UGameplayEffect>(GetTransientPackage());
	InvulnerabilityGE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	InvulnerabilityGE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));

	UTargetTagsGameplayEffectComponent& TargetTagsComp = InvulnerabilityGE->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagChanges;
	TagChanges.AddTag(MordecaiGameplayTags::State_Invulnerable);
	TargetTagsComp.SetAndApplyTargetTagChanges(TagChanges);

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpec Spec(InvulnerabilityGE, Context, 1.0f);
	ASC->ApplyGameplayEffectSpecToSelf(Spec);
}
