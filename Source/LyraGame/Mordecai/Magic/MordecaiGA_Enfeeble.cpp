// Project Mordecai — Enfeeble Spell (US-023)

#include "Mordecai/Magic/MordecaiGA_Enfeeble.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiGA_Enfeeble::UMordecaiGA_Enfeeble(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// AC-023.6: Tag this ability
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(MordecaiGameplayTags::Ability_Spell_Enfeeble);
	SetAssetTags(Tags);
}

// ---------------------------------------------------------------------------
// Public Getters
// ---------------------------------------------------------------------------

float UMordecaiGA_Enfeeble::GetEnfeebleRange() const
{
	if (!SpellData) return 0.f;
	return SpellData->Range;
}

float UMordecaiGA_Enfeeble::GetEnfeebleDuration() const
{
	if (!SpellData) return 0.f;
	return SpellData->BuffDuration;
}

FGameplayTag UMordecaiGA_Enfeeble::GetWeakenedTag() const
{
	return MordecaiGameplayTags::Status_Weakened;
}

float UMordecaiGA_Enfeeble::GetWeakenedReduction() const
{
	if (!SpellData) return 0.f;
	return ComputeWeakenedReduction(SpellData->BasePower);
}

FGameplayAttribute UMordecaiGA_Enfeeble::GetDebuffedAttribute() const
{
	return UMordecaiAttributeSet::GetPhysicalDamageMultiplierAttribute();
}

// ---------------------------------------------------------------------------
// Static Helpers
// ---------------------------------------------------------------------------

float UMordecaiGA_Enfeeble::ComputeWeakenedReduction(float BasePower)
{
	return BasePower / 100.f;
}

// ---------------------------------------------------------------------------
// OnSpellCast (AC-023.4, AC-023.5)
// ---------------------------------------------------------------------------

void UMordecaiGA_Enfeeble::OnSpellCast()
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

	FVector Origin = AvatarActor->GetActorLocation();
	FVector AimDir = AvatarActor->GetActorForwardVector();
	float Range = GetEnfeebleRange();

	// AC-023.4: Line trace in aim direction to find first enemy
	FHitResult HitResult;
	FVector TraceEnd = Origin + AimDir * Range;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(AvatarActor);

	bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		Origin,
		TraceEnd,
		ECC_Pawn,
		QueryParams);

	if (bHit && HitResult.GetActor())
	{
		ApplyWeakenedToTarget(HitResult.GetActor());
	}
}

// ---------------------------------------------------------------------------
// ApplyWeakenedToTarget (AC-023.5)
// ---------------------------------------------------------------------------

void UMordecaiGA_Enfeeble::ApplyWeakenedToTarget(AActor* Target)
{
	if (!Target || !SpellData)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!TargetASC)
	{
		return;
	}

	float Duration = GetEnfeebleDuration();
	float Reduction = GetWeakenedReduction();
	if (Duration <= 0.f || FMath::IsNearlyZero(Reduction))
	{
		return;
	}

	// AC-023.5: Create Weakened GE with duration
	UGameplayEffect* WeakenedGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_MordecaiEnfeebleWeakened"));
	WeakenedGE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	WeakenedGE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));

	// AC-023.5: Reduce PhysicalDamageMultiplier by Reduction (negative additive)
	{
		FGameplayModifierInfo PhysMod;
		PhysMod.Attribute = UMordecaiAttributeSet::GetPhysicalDamageMultiplierAttribute();
		PhysMod.ModifierOp = EGameplayModOp::Additive;
		PhysMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-Reduction));
		WeakenedGE->Modifiers.Add(PhysMod);
	}

	// AC-023.5: Grant Status_Weakened tag for duration
	UTargetTagsGameplayEffectComponent& TargetTagsComp = WeakenedGE->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagChanges;
	TagChanges.AddTag(MordecaiGameplayTags::Status_Weakened);
	TargetTagsComp.SetAndApplyTargetTagChanges(TagChanges);

	// Apply to target
	UAbilitySystemComponent* InstigatorASC = GetAbilitySystemComponentFromActorInfo();
	FGameplayEffectContextHandle Context = InstigatorASC ?
		InstigatorASC->MakeEffectContext() :
		TargetASC->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpec Spec(WeakenedGE, Context, 1.0f);
	TargetASC->ApplyGameplayEffectSpecToSelf(Spec);
}
