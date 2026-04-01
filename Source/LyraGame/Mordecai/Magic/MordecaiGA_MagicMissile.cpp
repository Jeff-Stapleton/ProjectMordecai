// Project Mordecai — Magic Missile Spell (US-020)

#include "Mordecai/Magic/MordecaiGA_MagicMissile.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/Combat/MordecaiProjectile.h"
#include "Mordecai/Combat/MordecaiAimAssistSubsystem.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiGA_MagicMissile::UMordecaiGA_MagicMissile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// AC-020.12: Tag this ability
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(MordecaiGameplayTags::Ability_Spell_MagicMissile);
	SetAssetTags(Tags);
}

// ---------------------------------------------------------------------------
// Public Getters
// ---------------------------------------------------------------------------

int32 UMordecaiGA_MagicMissile::GetProjectileCount() const
{
	if (!SpellData) return 1;
	return FMath::Max(1, SpellData->ProjectileCount);
}

float UMordecaiGA_MagicMissile::GetDamagePerProjectile(float TotalSpellPower) const
{
	int32 Count = GetProjectileCount();
	return TotalSpellPower / static_cast<float>(Count);
}

// ---------------------------------------------------------------------------
// OnSpellCast — Channeled: fires N missiles over CastTime (AC-020.7)
// ---------------------------------------------------------------------------

void UMordecaiGA_MagicMissile::OnSpellCast()
{
	UWorld* World = GetWorld();
	if (!World || !SpellData)
	{
		return;
	}

	// Compute total spell power
	float AttributeScaling = ComputeAttributeScalingSumFromASC();
	int32 SkillRank = GetSkillRankFromComponent();
	float TotalPower = ComputeSpellPower(AttributeScaling, SkillRank, bIsUpcast);

	int32 Count = GetProjectileCount();
	CachedDamagePerMissile = GetDamagePerProjectile(TotalPower);
	MissilesRemaining = Count;

	// Fire first missile immediately
	FireSingleMissile(CachedDamagePerMissile);
	MissilesRemaining--;

	// Fire remaining missiles at intervals over CastTime
	if (MissilesRemaining > 0)
	{
		float CastTime = GetCastDuration();
		float Interval = (CastTime > 0.f && Count > 1)
			? CastTime / static_cast<float>(Count)
			: 0.1f;

		World->GetTimerManager().SetTimer(
			MissileTimerHandle,
			[this]()
			{
				if (MissilesRemaining > 0)
				{
					FireSingleMissile(CachedDamagePerMissile);
					MissilesRemaining--;

					if (MissilesRemaining <= 0)
					{
						if (UWorld* W = GetWorld())
						{
							W->GetTimerManager().ClearTimer(MissileTimerHandle);
						}
					}
				}
			},
			Interval,
			/*bLoop=*/true);
	}
}

// ---------------------------------------------------------------------------
// FireSingleMissile (AC-020.8)
// ---------------------------------------------------------------------------

void UMordecaiGA_MagicMissile::FireSingleMissile(float DamageMagnitude)
{
	UWorld* World = GetWorld();
	if (!World || !SpellData)
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return;
	}

	FVector Origin = AvatarActor->GetActorLocation();
	FVector AimForward = AvatarActor->GetActorForwardVector();

	// AC-020.8: Use aim assist to acquire nearest valid target
	if (UMordecaiAimAssistSubsystem* AimAssist = World->GetSubsystem<UMordecaiAimAssistSubsystem>())
	{
		AimForward = AimAssist->ComputeAssistedAimDirection(Origin, AimForward, SpellData->Range);
	}

	// Spawn projectile
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = AvatarActor;
	SpawnParams.Instigator = Cast<APawn>(AvatarActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FRotator SpawnRotation = AimForward.Rotation();
	AMordecaiProjectile* Projectile = World->SpawnActor<AMordecaiProjectile>(
		AMordecaiProjectile::StaticClass(), Origin, SpawnRotation, SpawnParams);

	if (Projectile)
	{
		// Build spec from data asset
		FMordecaiProjectileSpec Spec;
		Spec.Speed = SpellData->ProjectileSpeed;
		Spec.Lifetime = SpellData->ProjectileLifetime;
		Spec.MaxRange = SpellData->Range;

		// AC-020.9: Arcane/Force damage
		FMordecaiDamageProfile DamageProfile;
		DamageProfile.DamageType = EMordecaiDamageType::Arcane;
		DamageProfile.BasePower = DamageMagnitude;
		DamageProfile.CanCrit = false;
		DamageProfile.AppliesPostureDamage = false;

		Projectile->InitFromSpec(Spec, DamageProfile);
		Projectile->SetProjectileInstigator(AvatarActor);

		if (USphereComponent* CollisionComp = Projectile->GetCollisionComponent())
		{
			CollisionComp->SetSphereRadius(SpellData->ProjectileCollisionRadius);
		}
	}
}
