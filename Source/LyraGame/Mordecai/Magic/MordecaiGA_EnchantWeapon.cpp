// Project Mordecai — Enchant Weapon Spell (US-023)

#include "Mordecai/Magic/MordecaiGA_EnchantWeapon.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiGA_EnchantWeapon::UMordecaiGA_EnchantWeapon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// AC-023.9: Tag this ability
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(MordecaiGameplayTags::Ability_Spell_EnchantWeapon);
	SetAssetTags(Tags);
}

// ---------------------------------------------------------------------------
// Public Getters
// ---------------------------------------------------------------------------

float UMordecaiGA_EnchantWeapon::GetEnchantDuration() const
{
	if (!SpellData) return 0.f;
	return SpellData->BuffDuration;
}

float UMordecaiGA_EnchantWeapon::GetEnchantBonusDamage() const
{
	if (!SpellData) return 0.f;
	return ComputeBonusDamage(SpellData->BasePower);
}

FGameplayTag UMordecaiGA_EnchantWeapon::GetEnchantTag() const
{
	return MordecaiGameplayTags::Status_EnchantedWeapon;
}

FGameplayTag UMordecaiGA_EnchantWeapon::GetEnchantDamageTag() const
{
	return MordecaiGameplayTags::Damage_Fire;
}

// ---------------------------------------------------------------------------
// Static Helpers
// ---------------------------------------------------------------------------

float UMordecaiGA_EnchantWeapon::ComputeBonusDamage(float BasePower)
{
	return BasePower;
}

// ---------------------------------------------------------------------------
// OnSpellCast (AC-023.7, AC-023.8)
// ---------------------------------------------------------------------------

void UMordecaiGA_EnchantWeapon::OnSpellCast()
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

	float Duration = GetEnchantDuration();
	float BonusDamage = GetEnchantBonusDamage();
	if (Duration <= 0.f || FMath::IsNearlyZero(BonusDamage))
	{
		return;
	}

	// AC-023.8: Create Enchant Weapon GE with duration
	UGameplayEffect* EnchantGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_MordecaiEnchantWeapon"));
	EnchantGE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	EnchantGE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));

	// AC-023.8: Set EnchantWeaponBonusDamage attribute to BasePower
	// When the GE expires, this modifier is removed and attribute reverts to 0
	{
		FGameplayModifierInfo BonusDmgMod;
		BonusDmgMod.Attribute = UMordecaiAttributeSet::GetEnchantWeaponBonusDamageAttribute();
		BonusDmgMod.ModifierOp = EGameplayModOp::Additive;
		BonusDmgMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(BonusDamage));
		EnchantGE->Modifiers.Add(BonusDmgMod);
	}

	// AC-023.8: Non-stacking — refresh duration on reapply
	EnchantGE->StackingType = EGameplayEffectStackingType::AggregateBySource;
	EnchantGE->StackLimitCount = 1;
	EnchantGE->StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
	EnchantGE->StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::ClearEntireStack;

	// AC-023.9: Grant Status_EnchantedWeapon tag for duration
	UTargetTagsGameplayEffectComponent& TargetTagsComp = EnchantGE->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagChanges;
	TagChanges.AddTag(MordecaiGameplayTags::Status_EnchantedWeapon);
	TargetTagsComp.SetAndApplyTargetTagChanges(TagChanges);

	// Apply to self
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpec Spec(EnchantGE, Context, 1.0f);
	ASC->ApplyGameplayEffectSpecToSelf(Spec);
}
