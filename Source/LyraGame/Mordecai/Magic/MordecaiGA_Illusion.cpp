// Project Mordecai — Illusion Spell (US-060)

#include "Mordecai/Magic/MordecaiGA_Illusion.h"
#include "Mordecai/Magic/MordecaiIllusionActor.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiGA_Illusion)

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiGA_Illusion::UMordecaiGA_Illusion(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// AC-060.4: Tag this ability
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(MordecaiGameplayTags::Ability_Spell_Illusion);
	SetAssetTags(Tags);
}

// ---------------------------------------------------------------------------
// Public Getters
// ---------------------------------------------------------------------------

float UMordecaiGA_Illusion::GetDecoyHP() const
{
	if (!SpellData) return 0.f;
	return ComputeDecoyHP(SpellData->BasePower);
}

float UMordecaiGA_Illusion::GetDecoyDuration() const
{
	if (!SpellData) return 0.f;
	return SpellData->BuffDuration;
}

FGameplayTag UMordecaiGA_Illusion::GetAbilityTag() const
{
	return MordecaiGameplayTags::Ability_Spell_Illusion;
}

float UMordecaiGA_Illusion::ComputeDecoyHP(float BasePower)
{
	return BasePower;
}

// ---------------------------------------------------------------------------
// OnSpellCast (AC-060.1)
// ---------------------------------------------------------------------------

void UMordecaiGA_Illusion::OnSpellCast()
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

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// AC-060.1: Spawn position slightly in front of the caster
	FVector SpawnLoc = AMordecaiIllusionActor::ComputeSpawnLocation(
		AvatarActor->GetActorLocation(),
		AvatarActor->GetActorForwardVector(),
		AMordecaiIllusionActor::DefaultSpawnDistance);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = AvatarActor;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AMordecaiIllusionActor* Decoy = World->SpawnActor<AMordecaiIllusionActor>(
		AMordecaiIllusionActor::StaticClass(),
		SpawnLoc,
		AvatarActor->GetActorRotation(),
		SpawnParams);

	if (Decoy)
	{
		// AC-060.2: Configure HP from BasePower and Duration from SpellDataAsset
		Decoy->InitDecoy(GetDecoyHP(), GetDecoyDuration());
	}
}
