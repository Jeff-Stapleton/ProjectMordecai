// Project Mordecai — Cone of Cold Spell (US-020)

#include "Mordecai/Magic/MordecaiGA_ConeOfCold.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/Combat/MordecaiHitDetectionSubsystem.h"
#include "Mordecai/Combat/MordecaiHitDetectionTypes.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "Engine/World.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiGA_ConeOfCold::UMordecaiGA_ConeOfCold(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// AC-020.12: Tag this ability
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(MordecaiGameplayTags::Ability_Spell_ConeOfCold);
	SetAssetTags(Tags);
}

// ---------------------------------------------------------------------------
// Public Getters
// ---------------------------------------------------------------------------

float UMordecaiGA_ConeOfCold::GetConeArcDegrees() const
{
	if (!SpellData) return 0.f;
	return SpellData->ArcDegrees;
}

float UMordecaiGA_ConeOfCold::GetConeRange() const
{
	if (!SpellData) return 0.f;
	return SpellData->Range;
}

// ---------------------------------------------------------------------------
// OnSpellCast (AC-020.4)
// ---------------------------------------------------------------------------

void UMordecaiGA_ConeOfCold::OnSpellCast()
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

	UMordecaiHitDetectionSubsystem* HitDetection = World->GetSubsystem<UMordecaiHitDetectionSubsystem>();
	if (!HitDetection)
	{
		return;
	}

	// Compute spell power
	float AttributeScaling = ComputeAttributeScalingSumFromASC();
	int32 SkillRank = GetSkillRankFromComponent();
	float Power = ComputeSpellPower(AttributeScaling, SkillRank, bIsUpcast);

	// AC-020.4: Arc sector query using ArcDegrees and Range from SpellDataAsset
	FVector Origin = AvatarActor->GetActorLocation();
	FVector Forward = AvatarActor->GetActorForwardVector();

	TArray<FMordecaiHitResult> Hits = HitDetection->ArcSectorOverlapQuery(
		Origin,
		Forward,
		SpellData->Range,
		SpellData->ArcDegrees,
		/*StartAngleOffset=*/0.f,
		AvatarActor,
		EMordecaiTargetFilter::Enemies,
		/*bAllowFriendlyFire=*/false,
		/*bJumpAvoidable=*/false,
		/*bHitsAirborne=*/true);

	// Apply cold damage to each hit target
	for (const FMordecaiHitResult& Hit : Hits)
	{
		if (Hit.HitActor.IsValid())
		{
			ApplyColdDamageToTarget(Hit.HitActor.Get(), Power);
		}
	}
}

// ---------------------------------------------------------------------------
// Damage Application (AC-020.5)
// ---------------------------------------------------------------------------

void UMordecaiGA_ConeOfCold::ApplyColdDamageToTarget(AActor* TargetActor, float DamageMagnitude)
{
	if (!TargetActor)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
	if (!TargetASC)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	FGameplayEffectContextHandle Context;
	if (SourceASC)
	{
		Context = SourceASC->MakeEffectContext();
	}
	else
	{
		Context = TargetASC->MakeEffectContext();
	}
	Context.AddSourceObject(this);

	// Instant health damage GE
	UGameplayEffect* HealthGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_MordecaiConeOfColdDamage"));
	HealthGE->DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& Mod = HealthGE->Modifiers.AddDefaulted_GetRef();
	Mod.Attribute = UMordecaiAttributeSet::GetHealthAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-DamageMagnitude));

	FGameplayEffectSpec HealthSpec(HealthGE, Context, 1.0f);

	// AC-020.5: Tag with cold/frost damage type
	FGameplayTag DamageTag = UMordecaiGA_MeleeAttack::GetDamageTagForType(EMordecaiDamageType::Frost);
	if (DamageTag.IsValid())
	{
		HealthSpec.AddDynamicAssetTag(DamageTag);
	}

	TargetASC->ApplyGameplayEffectSpecToSelf(HealthSpec);
}
