// Project Mordecai — Blink Spell (US-021)

#include "Mordecai/Magic/MordecaiGA_Blink.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
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
// OnSpellCast (AC-021.1, AC-021.2, AC-021.3)
// ---------------------------------------------------------------------------

void UMordecaiGA_Blink::OnSpellCast()
{
	if (!SpellData)
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return;
	}

	FVector Origin = AvatarActor->GetActorLocation();
	FVector AimDir = AvatarActor->GetActorForwardVector();
	float Range = GetTeleportRange();

	// AC-021.2: Line trace to prevent teleporting through walls
	bool bWallHit = false;
	FVector WallHitPoint = FVector::ZeroVector;

	if (UWorld* World = GetWorld())
	{
		FHitResult HitResult;
		FVector TraceEnd = Origin + AimDir * Range;

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(AvatarActor);

		bWallHit = World->LineTraceSingleByChannel(
			HitResult,
			Origin,
			TraceEnd,
			ECC_WorldStatic,
			QueryParams);

		if (bWallHit)
		{
			WallHitPoint = HitResult.ImpactPoint;
		}
	}

	// AC-021.1: Compute destination, AC-021.2: wall clamping
	FVector Destination = ComputeTeleportDestination(Origin, AimDir, Range, bWallHit, WallHitPoint);

	// AC-021.1: Teleport via SetActorLocation (no sweep — trace already done)
	AvatarActor->SetActorLocation(Destination, false, nullptr, ETeleportType::TeleportPhysics);

	// AC-021.3: Grant brief invulnerability
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
