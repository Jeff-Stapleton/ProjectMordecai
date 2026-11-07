// Project Mordecai — Fireball Spell (US-020)

#include "Mordecai/Magic/MordecaiGA_Fireball.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/Combat/MordecaiProjectile.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiAimAssistSubsystem.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
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
	const FVector CasterOrigin = AvatarActor->GetActorLocation();
	FVector AimForward = AvatarActor->GetActorForwardVector();

	if (UMordecaiAimAssistSubsystem* AimAssist = World->GetSubsystem<UMordecaiAimAssistSubsystem>())
	{
		AimForward = AimAssist->ComputeAssistedAimDirection(CasterOrigin, AimForward, SpellData->Range);
	}

	// Configure before FinishSpawning so BeginPlay sees the real speed/lifetime/range. Spawn beyond
	// the caster and projectile collision radii so the first sweep cannot begin inside the caster.
	const UCapsuleComponent* CasterCapsule = AvatarActor->FindComponentByClass<UCapsuleComponent>();
	const float CasterRadius = CasterCapsule ? CasterCapsule->GetScaledCapsuleRadius() : 0.f;
	constexpr float SpawnMargin = 5.f;
	const FVector SpawnLocation = CasterOrigin
		+ AimForward * (CasterRadius + SpellData->ProjectileCollisionRadius + SpawnMargin);
	const FTransform SpawnTransform(AimForward.Rotation(), SpawnLocation);
	AMordecaiProjectile* Projectile = World->SpawnActorDeferred<AMordecaiProjectile>(
		AMordecaiProjectile::StaticClass(), SpawnTransform, AvatarActor, Cast<APawn>(AvatarActor),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (Projectile)
	{
		FMordecaiProjectileSpec Spec = BuildProjectileSpec();
		FMordecaiDamageProfile DamageProfile = BuildDamageProfile(Power);

		Projectile->InitFromSpec(Spec, DamageProfile);
		Projectile->SetProjectileInstigator(AvatarActor);
		if (USphereComponent* CollisionComp = Projectile->GetCollisionComponent())
		{
			CollisionComp->SetSphereRadius(SpellData->ProjectileCollisionRadius);
		}
		Projectile->FinishSpawning(SpawnTransform);
	}
}
