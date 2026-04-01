// Project Mordecai — Fire Ward Spell (US-021)

#include "Mordecai/Magic/MordecaiGA_FireWard.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiGA_FireWard::UMordecaiGA_FireWard(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// AC-021.11: Tag this ability
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(MordecaiGameplayTags::Ability_Spell_FireWard);
	SetAssetTags(Tags);
}

// ---------------------------------------------------------------------------
// Public Getters
// ---------------------------------------------------------------------------

float UMordecaiGA_FireWard::GetShieldHP() const
{
	if (!SpellData) return 0.f;
	return SpellData->BasePower;
}

float UMordecaiGA_FireWard::GetWardDuration() const
{
	if (!SpellData) return 0.f;
	return SpellData->BuffDuration;
}

FGameplayTag UMordecaiGA_FireWard::GetWardTag() const
{
	return MordecaiGameplayTags::Status_FireWard;
}

FGameplayTag UMordecaiGA_FireWard::GetAbsorbedDamageTag() const
{
	return MordecaiGameplayTags::Damage_Fire;
}

// ---------------------------------------------------------------------------
// Static Helpers (testable pure functions)
// ---------------------------------------------------------------------------

float UMordecaiGA_FireWard::ComputeAbsorption(float IncomingDamage, float CurrentShieldHP, float& OutShieldRemaining)
{
	if (IncomingDamage <= 0.f)
	{
		OutShieldRemaining = CurrentShieldHP;
		return 0.f;
	}

	if (CurrentShieldHP <= 0.f)
	{
		OutShieldRemaining = 0.f;
		return IncomingDamage;
	}

	float Absorbed = FMath::Min(IncomingDamage, CurrentShieldHP);
	OutShieldRemaining = CurrentShieldHP - Absorbed;
	float Passthrough = IncomingDamage - Absorbed;

	return FMath::Max(0.f, Passthrough);
}

bool UMordecaiGA_FireWard::ShouldAbsorbDamageType(const FGameplayTag& DamageTag)
{
	return DamageTag == MordecaiGameplayTags::Damage_Fire;
}

bool UMordecaiGA_FireWard::ShouldRemoveWard(float CurrentShieldHP)
{
	return CurrentShieldHP <= 0.f;
}

// ---------------------------------------------------------------------------
// OnSpellCast (AC-021.8, AC-021.9, AC-021.10)
// ---------------------------------------------------------------------------

void UMordecaiGA_FireWard::OnSpellCast()
{
	if (!SpellData)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	float Duration = GetWardDuration();
	float ShieldAmount = GetShieldHP();
	if (Duration <= 0.f || ShieldAmount <= 0.f)
	{
		return;
	}

	// AC-021.8: Set FireWardShieldHP attribute to BasePower
	ASC->ApplyModToAttribute(UMordecaiAttributeSet::GetFireWardShieldHPAttribute(),
		EGameplayModOp::Override, ShieldAmount);

	// AC-021.9: Create duration GE that grants Status.FireWard tag
	UGameplayEffect* WardGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_MordecaiFireWard"));
	WardGE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	WardGE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));

	// Grant FireWard status tag for duration
	UTargetTagsGameplayEffectComponent& TargetTagsComp = WardGE->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagChanges;
	TagChanges.AddTag(MordecaiGameplayTags::Status_FireWard);
	TargetTagsComp.SetAndApplyTargetTagChanges(TagChanges);

	// Apply to self
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpec Spec(WardGE, Context, 1.0f);
	ASC->ApplyGameplayEffectSpecToSelf(Spec);

	// AC-021.9/AC-021.10: Monitor FireWardShieldHP attribute for depletion
	// When shield HP reaches 0, remove the ward GE
	ASC->GetGameplayAttributeValueChangeDelegate(
		UMordecaiAttributeSet::GetFireWardShieldHPAttribute()).AddWeakLambda(
		GetAvatarActorFromActorInfo(),
		[ASC](const FOnAttributeChangeData& Data)
		{
			if (Data.NewValue <= 0.f &&
				ASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_FireWard))
			{
				// Shield depleted — remove all GEs that grant FireWard tag
				FGameplayEffectQuery Query;
				Query.CustomMatchDelegate.BindLambda(
					[](const FActiveGameplayEffect& Effect) -> bool
					{
						FGameplayTagContainer GrantedTags;
						Effect.Spec.GetAllGrantedTags(GrantedTags);
						return GrantedTags.HasTag(MordecaiGameplayTags::Status_FireWard);
					});
				ASC->RemoveActiveEffects(Query);
			}
		});
}
