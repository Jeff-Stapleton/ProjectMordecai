// Project Mordecai — Sleep Spell (US-021)

#include "Mordecai/Magic/MordecaiGA_Sleep.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameFramework/Character.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiGA_Sleep::UMordecaiGA_Sleep(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// AC-021.11: Tag this ability
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(MordecaiGameplayTags::Ability_Spell_Sleep);
	SetAssetTags(Tags);
}

// ---------------------------------------------------------------------------
// Public Getters
// ---------------------------------------------------------------------------

float UMordecaiGA_Sleep::GetSleepRange() const
{
	if (!SpellData) return 0.f;
	return SpellData->Range;
}

float UMordecaiGA_Sleep::GetSleepDuration() const
{
	if (!SpellData) return 0.f;
	return SpellData->BuffDuration;
}

FGameplayTag UMordecaiGA_Sleep::GetSleepTag() const
{
	return MordecaiGameplayTags::Status_Sleeping;
}

TArray<FGameplayTag> UMordecaiGA_Sleep::GetSleepBlockedTags() const
{
	TArray<FGameplayTag> Tags;
	// AC-021.6: Sleeping tag blocks ability activation
	Tags.Add(MordecaiGameplayTags::Status_Sleeping);
	// AC-021.6: Rooted tag blocks movement
	Tags.Add(MordecaiGameplayTags::State_Rooted);
	return Tags;
}

bool UMordecaiGA_Sleep::ShouldWakeOnDamage() const
{
	return true;
}

// ---------------------------------------------------------------------------
// OnSpellCast (AC-021.5, AC-021.6, AC-021.7)
// ---------------------------------------------------------------------------

void UMordecaiGA_Sleep::OnSpellCast()
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
	float Range = GetSleepRange();

	// AC-021.5: Line trace in aim direction to find first enemy
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
		ApplySleepToTarget(HitResult.GetActor());
	}
}

// ---------------------------------------------------------------------------
// ApplySleepToTarget (AC-021.6, AC-021.7)
// ---------------------------------------------------------------------------

void UMordecaiGA_Sleep::ApplySleepToTarget(AActor* Target)
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

	float Duration = GetSleepDuration();
	if (Duration <= 0.f)
	{
		return;
	}

	// AC-021.6: Create Sleep GE with duration
	UGameplayEffect* SleepGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_MordecaiSleep"));
	SleepGE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	SleepGE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));

	// AC-021.6: Grant Sleeping and Rooted tags for duration
	// Sleeping blocks ability activation; Rooted blocks movement
	UTargetTagsGameplayEffectComponent& TargetTagsComp = SleepGE->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagChanges;
	TagChanges.AddTag(MordecaiGameplayTags::Status_Sleeping);
	TagChanges.AddTag(MordecaiGameplayTags::State_Rooted);
	TargetTagsComp.SetAndApplyTargetTagChanges(TagChanges);

	// Apply to target
	UAbilitySystemComponent* InstigatorASC = GetAbilitySystemComponentFromActorInfo();
	FGameplayEffectContextHandle Context = InstigatorASC ?
		InstigatorASC->MakeEffectContext() :
		TargetASC->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpec Spec(SleepGE, Context, 1.0f);
	TargetASC->ApplyGameplayEffectSpecToSelf(Spec);

	// AC-021.7: Register to monitor Health attribute changes on target
	// When Health decreases while Sleeping tag is active, remove the Sleep GE
	// This is handled by PostAttributeChange monitoring on the target's ASC
	// We register a delegate on the target's attribute change
	TargetASC->GetGameplayAttributeValueChangeDelegate(
		UMordecaiAttributeSet::GetHealthAttribute()).AddWeakLambda(
		Target,
		[TargetASC](const FOnAttributeChangeData& Data)
		{
			// Health decreased while sleeping → wake up
			if (Data.NewValue < Data.OldValue &&
				TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_Sleeping))
			{
				// Remove all active GEs that grant the Sleeping tag
				FGameplayEffectQuery Query;
				Query.CustomMatchDelegate.BindLambda(
					[](const FActiveGameplayEffect& Effect) -> bool
					{
						// Check if this GE grants the Sleeping tag
						FGameplayTagContainer GrantedTags;
						Effect.Spec.GetAllGrantedTags(GrantedTags);
						return GrantedTags.HasTag(MordecaiGameplayTags::Status_Sleeping);
					});
				TargetASC->RemoveActiveEffects(Query);
			}
		});
}
