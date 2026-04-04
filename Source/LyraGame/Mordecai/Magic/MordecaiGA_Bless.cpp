// Project Mordecai — Bless Spell (US-022)

#include "Mordecai/Magic/MordecaiGA_Bless.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiGA_Bless::UMordecaiGA_Bless(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// AC-022.7: Tag this ability
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(MordecaiGameplayTags::Ability_Spell_Bless);
	SetAssetTags(Tags);
}

// ---------------------------------------------------------------------------
// Public Getters
// ---------------------------------------------------------------------------

float UMordecaiGA_Bless::GetBlessBonus() const
{
	if (!SpellData) return 0.f;
	return ComputeBlessBonus(SpellData->BasePower);
}

float UMordecaiGA_Bless::GetBlessDuration() const
{
	if (!SpellData) return 0.f;
	return SpellData->BuffDuration;
}

FGameplayTag UMordecaiGA_Bless::GetBlessTag() const
{
	return MordecaiGameplayTags::Status_Blessed;
}

TArray<FGameplayAttribute> UMordecaiGA_Bless::GetBuffedAttributes() const
{
	TArray<FGameplayAttribute> Attributes;
	Attributes.Add(UMordecaiAttributeSet::GetPhysicalDamageMultiplierAttribute());
	Attributes.Add(UMordecaiAttributeSet::GetMagicDamageMultiplierAttribute());
	return Attributes;
}

// ---------------------------------------------------------------------------
// Static Helpers
// ---------------------------------------------------------------------------

float UMordecaiGA_Bless::ComputeBlessBonus(float BasePower)
{
	return BasePower / 100.f;
}

// ---------------------------------------------------------------------------
// OnSpellCast (AC-022.1, AC-022.2, AC-022.3)
// ---------------------------------------------------------------------------

void UMordecaiGA_Bless::OnSpellCast()
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

	float Duration = GetBlessDuration();
	float BlessBonus = GetBlessBonus();
	if (Duration <= 0.f || FMath::IsNearlyZero(BlessBonus))
	{
		return;
	}

	// AC-022.1/AC-022.2: Create duration GE that increases both damage multipliers
	UGameplayEffect* BlessGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_MordecaiBless"));
	BlessGE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	BlessGE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));

	// AC-022.1: Additive modifier on PhysicalDamageMultiplier
	{
		FGameplayModifierInfo PhysMod;
		PhysMod.Attribute = UMordecaiAttributeSet::GetPhysicalDamageMultiplierAttribute();
		PhysMod.ModifierOp = EGameplayModOp::Additive;
		PhysMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(BlessBonus));
		BlessGE->Modifiers.Add(PhysMod);
	}

	// AC-022.1: Additive modifier on MagicDamageMultiplier
	{
		FGameplayModifierInfo MagicMod;
		MagicMod.Attribute = UMordecaiAttributeSet::GetMagicDamageMultiplierAttribute();
		MagicMod.ModifierOp = EGameplayModOp::Additive;
		MagicMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(BlessBonus));
		BlessGE->Modifiers.Add(MagicMod);
	}

	// AC-022.3: Non-stacking — AggregateBySource, StackCount=1, refresh on reapply
	BlessGE->StackingType = EGameplayEffectStackingType::AggregateBySource;
	BlessGE->StackLimitCount = 1;
	BlessGE->StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
	BlessGE->StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::ClearEntireStack;

	// AC-022.2: Grant Blessed status tag for duration
	UTargetTagsGameplayEffectComponent& TargetTagsComp = BlessGE->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagChanges;
	TagChanges.AddTag(MordecaiGameplayTags::Status_Blessed);
	TargetTagsComp.SetAndApplyTargetTagChanges(TagChanges);

	// Apply to self
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpec Spec(BlessGE, Context, 1.0f);
	ASC->ApplyGameplayEffectSpecToSelf(Spec);
}
