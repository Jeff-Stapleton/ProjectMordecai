// Project Mordecai — Fireball Spell (US-020)

#include "Mordecai/Magic/MordecaiGA_Fireball.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/Combat/MordecaiProjectile.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiAimAssistSubsystem.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiGA_Fireball::UMordecaiGA_Fireball(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// AC-020.12: Tag this ability
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(MordecaiGameplayTags::Ability_Spell_Fireball);
	SetAssetTags(Tags);
}

// ---------------------------------------------------------------------------
// Public Getters (testable API)
// ---------------------------------------------------------------------------

FMordecaiProjectileSpec UMordecaiGA_Fireball::BuildProjectileSpec() const
{
	FMordecaiProjectileSpec Spec;
	if (SpellData)
	{
		Spec.Speed = SpellData->ProjectileSpeed;
		Spec.Lifetime = SpellData->ProjectileLifetime;
		Spec.MaxRange = SpellData->Range;
	}
	return Spec;
}

FMordecaiDamageProfile UMordecaiGA_Fireball::BuildDamageProfile(float SpellPower) const
{
	FMordecaiDamageProfile Profile;
	Profile.DamageType = EMordecaiDamageType::Fire;
	Profile.BasePower = SpellPower;
	Profile.CanCrit = true;
	Profile.AppliesPostureDamage = false;
	return Profile;
}

// ---------------------------------------------------------------------------
// OnSpellCast (AC-020.1)
// ---------------------------------------------------------------------------

void UMordecaiGA_Fireball::OnSpellCast()
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

	// Compute spell power
	float AttributeScaling = ComputeAttributeScalingSumFromASC();
	int32 SkillRank = GetSkillRankFromComponent();
	float Power = ComputeSpellPower(AttributeScaling, SkillRank, bIsUpcast);

	// Get aim direction with aim assist
	FVector Origin = AvatarActor->GetActorLocation();
	FVector AimForward = AvatarActor->GetActorForwardVector();

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
		// Configure projectile
		FMordecaiProjectileSpec Spec = BuildProjectileSpec();
		FMordecaiDamageProfile DamageProfile = BuildDamageProfile(Power);

		Projectile->InitFromSpec(Spec, DamageProfile);
		Projectile->SetProjectileInstigator(AvatarActor);

		// AC-020.2: Set collision radius from data asset
		if (USphereComponent* CollisionComp = Projectile->GetCollisionComponent())
		{
			CollisionComp->SetSphereRadius(SpellData->ProjectileCollisionRadius);
		}
	}
}
