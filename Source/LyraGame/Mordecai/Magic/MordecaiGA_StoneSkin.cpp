// Project Mordecai — Stone Skin Spell (US-020)

#include "Mordecai/Magic/MordecaiGA_StoneSkin.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiGA_StoneSkin::UMordecaiGA_StoneSkin(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// AC-020.12: Tag this ability
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(MordecaiGameplayTags::Ability_Spell_StoneSkin);
	SetAssetTags(Tags);
}

// ---------------------------------------------------------------------------
// Public Getters
// ---------------------------------------------------------------------------

float UMordecaiGA_StoneSkin::GetDamageReductionAmount() const
{
	if (!SpellData) return 0.f;
	return SpellData->BasePower;
}

float UMordecaiGA_StoneSkin::GetBuffDuration() const
{
	if (!SpellData) return 0.f;
	return SpellData->BuffDuration;
}

FGameplayTag UMordecaiGA_StoneSkin::GetStatusTag() const
{
	return MordecaiGameplayTags::Status_StoneSkin;
}

// ---------------------------------------------------------------------------
// OnSpellCast (AC-020.10, AC-020.11)
// ---------------------------------------------------------------------------

void UMordecaiGA_StoneSkin::OnSpellCast()
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

	float Duration = GetBuffDuration();
	float ReductionAmount = GetDamageReductionAmount();
	if (Duration <= 0.f)
	{
		return;
	}

	// AC-020.10: Create duration GE that grants flat DamageReduction bonus
	UGameplayEffect* StoneSkinGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_MordecaiStoneSkin"));
	StoneSkinGE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	StoneSkinGE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));

	// Modifier: +DamageReduction (additive)
	FGameplayModifierInfo& Mod = StoneSkinGE->Modifiers.AddDefaulted_GetRef();
	Mod.Attribute = UMordecaiAttributeSet::GetDamageReductionAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(ReductionAmount));

	// AC-020.11: Grant Status.StoneSkin tag for duration
	UTargetTagsGameplayEffectComponent& TargetTagsComp = StoneSkinGE->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagChanges;
	TagChanges.AddTag(MordecaiGameplayTags::Status_StoneSkin);
	TargetTagsComp.SetAndApplyTargetTagChanges(TagChanges);

	// Apply to self
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpec Spec(StoneSkinGE, Context, 1.0f);
	ASC->ApplyGameplayEffectSpecToSelf(Spec);
}
