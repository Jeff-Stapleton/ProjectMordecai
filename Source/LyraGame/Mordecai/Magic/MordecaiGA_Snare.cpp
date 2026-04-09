// Project Mordecai — Snare Spell (US-023)

#include "Mordecai/Magic/MordecaiGA_Snare.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiGA_Snare::UMordecaiGA_Snare(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// AC-023.3: Tag this ability
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(MordecaiGameplayTags::Ability_Spell_Snare);
	SetAssetTags(Tags);
}

// ---------------------------------------------------------------------------
// Public Getters
// ---------------------------------------------------------------------------

float UMordecaiGA_Snare::GetSnareRange() const
{
	if (!SpellData) return 0.f;
	return SpellData->Range;
}

float UMordecaiGA_Snare::GetSnareDuration() const
{
	if (!SpellData) return 0.f;
	return SpellData->BuffDuration;
}

FGameplayTag UMordecaiGA_Snare::GetRootTag() const
{
	return MordecaiGameplayTags::Status_Rooted;
}

bool UMordecaiGA_Snare::GetRootAllowsAttack() const
{
	return true;
}

bool UMordecaiGA_Snare::GetRootAllowsBlock() const
{
	return true;
}

// ---------------------------------------------------------------------------
// OnSpellCast (AC-023.1, AC-023.2)
// ---------------------------------------------------------------------------

void UMordecaiGA_Snare::OnSpellCast()
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
	float Range = GetSnareRange();

	// AC-023.1: Line trace in aim direction to find first enemy
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
		ApplyRootToTarget(HitResult.GetActor());
	}
}

// ---------------------------------------------------------------------------
// ApplyRootToTarget (AC-023.2)
// ---------------------------------------------------------------------------

void UMordecaiGA_Snare::ApplyRootToTarget(AActor* Target)
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

	float Duration = GetSnareDuration();
	if (Duration <= 0.f)
	{
		return;
	}

	// AC-023.2: Create Root GE with duration
	UGameplayEffect* RootGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_MordecaiSnareRoot"));
	RootGE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	RootGE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));

	// AC-023.2: Grant Status_Rooted tag for duration (blocks movement, allows attack/block)
	UTargetTagsGameplayEffectComponent& TargetTagsComp = RootGE->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagChanges;
	TagChanges.AddTag(MordecaiGameplayTags::Status_Rooted);
	TargetTagsComp.SetAndApplyTargetTagChanges(TagChanges);

	// Apply to target
	UAbilitySystemComponent* InstigatorASC = GetAbilitySystemComponentFromActorInfo();
	FGameplayEffectContextHandle Context = InstigatorASC ?
		InstigatorASC->MakeEffectContext() :
		TargetASC->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpec Spec(RootGE, Context, 1.0f);
	TargetASC->ApplyGameplayEffectSpecToSelf(Spec);
}
