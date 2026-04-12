// Project Mordecai — Blur Spell (US-060)

#include "Mordecai/Magic/MordecaiGA_Blur.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiGA_Blur)

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiGA_Blur::UMordecaiGA_Blur(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// AC-060.8: Tag this ability
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(MordecaiGameplayTags::Ability_Spell_Blur);
	SetAssetTags(Tags);
}

// ---------------------------------------------------------------------------
// Public Getters
// ---------------------------------------------------------------------------

float UMordecaiGA_Blur::GetBlurDuration() const
{
	if (!SpellData) return 0.f;
	return SpellData->BuffDuration;
}

float UMordecaiGA_Blur::GetMissChance() const
{
	if (!SpellData) return 0.f;
	return ComputeMissChance(SpellData->BasePower);
}

FGameplayTag UMordecaiGA_Blur::GetBlurredTag() const
{
	return MordecaiGameplayTags::Status_Blurred;
}

FGameplayTag UMordecaiGA_Blur::GetAbilityTag() const
{
	return MordecaiGameplayTags::Ability_Spell_Blur;
}

float UMordecaiGA_Blur::ComputeMissChance(float BasePower)
{
	// AC-060.6: BasePower as percentage, e.g., 30 means 30% = 0.30
	return FMath::Clamp(BasePower / 100.f, 0.f, 1.f);
}

bool UMordecaiGA_Blur::RollMiss(float EvasionChance, float Roll)
{
	// AC-060.7: If roll < evasion chance, the attack misses
	return Roll < EvasionChance;
}

// ---------------------------------------------------------------------------
// OnSpellCast (AC-060.5, AC-060.6)
// ---------------------------------------------------------------------------

void UMordecaiGA_Blur::OnSpellCast()
{
	if (!SpellData)
	{
		return;
	}

	ApplyBlurGE();
}

// ---------------------------------------------------------------------------
// ApplyBlurGE (AC-060.5, AC-060.6)
// ---------------------------------------------------------------------------

void UMordecaiGA_Blur::ApplyBlurGE()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	float Duration = GetBlurDuration();
	if (Duration <= 0.f)
	{
		return;
	}

	// AC-060.6: Create duration GE granting Blurred tag and RangedEvasionChance
	UGameplayEffect* BlurGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_MordecaiBlur"));
	BlurGE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	BlurGE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));

	// Grant Status_Blurred tag (AC-060.8)
	UTargetTagsGameplayEffectComponent& TargetTagsComp = BlurGE->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagChanges;
	TagChanges.AddTag(MordecaiGameplayTags::Status_Blurred);
	TargetTagsComp.SetAndApplyTargetTagChanges(TagChanges);

	// AC-060.6: Set RangedEvasionChance attribute to miss chance value
	FGameplayModifierInfo& EvasionMod = BlurGE->Modifiers.AddDefaulted_GetRef();
	EvasionMod.Attribute = UMordecaiAttributeSet::GetRangedEvasionChanceAttribute();
	EvasionMod.ModifierOp = EGameplayModOp::Additive;
	EvasionMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(GetMissChance()));

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpec Spec(BlurGE, Context, 1.0f);
	ASC->ApplyGameplayEffectSpecToSelf(Spec);
}
