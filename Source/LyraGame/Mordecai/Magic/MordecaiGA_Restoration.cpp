// Project Mordecai — Restoration Spell (US-022)

#include "Mordecai/Magic/MordecaiGA_Restoration.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiGA_Restoration::UMordecaiGA_Restoration(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// AC-022.7: Tag this ability
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(MordecaiGameplayTags::Ability_Spell_Restoration);
	SetAssetTags(Tags);
}

// ---------------------------------------------------------------------------
// Public Getters
// ---------------------------------------------------------------------------

float UMordecaiGA_Restoration::GetRestorationDuration() const
{
	if (!SpellData) return 0.f;
	return SpellData->BuffDuration;
}

FGameplayTag UMordecaiGA_Restoration::GetRestorationTag() const
{
	return MordecaiGameplayTags::Status_Restoration;
}

// ---------------------------------------------------------------------------
// Static Helpers
// ---------------------------------------------------------------------------

float UMordecaiGA_Restoration::ComputeHealPerTick(float TotalHeal, float Duration)
{
	if (Duration <= 0.f) return 0.f;
	return TotalHeal / Duration;
}

float UMordecaiGA_Restoration::ClampHeal(float CurrentHealth, float HealAmount, float MaxHealth)
{
	float Room = MaxHealth - CurrentHealth;
	if (Room <= 0.f) return 0.f;
	return FMath::Min(HealAmount, Room);
}

// ---------------------------------------------------------------------------
// OnSpellCast (AC-022.4, AC-022.5, AC-022.6)
// ---------------------------------------------------------------------------

void UMordecaiGA_Restoration::OnSpellCast()
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

	float Duration = GetRestorationDuration();
	if (Duration <= 0.f)
	{
		return;
	}

	// Compute total heal from spell power (BasePower scaled by attributes/rank)
	float AttributeScaling = ComputeAttributeScalingSumFromASC();
	int32 SkillRank = GetSkillRankFromComponent();
	float TotalHeal = ComputeSpellPower(AttributeScaling, SkillRank, bIsUpcast);

	float HealPerTick = ComputeHealPerTick(TotalHeal, Duration);
	if (FMath::IsNearlyZero(HealPerTick))
	{
		return;
	}

	// AC-022.4/AC-022.5: Create periodic duration GE for HoT
	UGameplayEffect* HoTGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_MordecaiRestoration"));
	HoTGE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	HoTGE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));

	// AC-022.5: Periodic tick every 1 second, no tick on application
	HoTGE->Period = FScalableFloat(GetTickPeriod());
	HoTGE->bExecutePeriodicEffectOnApplication = false;

	// AC-022.5: Each tick heals Health by HealPerTick (additive)
	{
		FGameplayModifierInfo HealMod;
		HealMod.Attribute = UMordecaiAttributeSet::GetHealthAttribute();
		HealMod.ModifierOp = EGameplayModOp::Additive;
		HealMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(HealPerTick));
		HoTGE->Modifiers.Add(HealMod);
	}

	// AC-022.6: Non-stacking — refresh duration and recalculate on reapply
	HoTGE->StackingType = EGameplayEffectStackingType::AggregateBySource;
	HoTGE->StackLimitCount = 1;
	HoTGE->StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
	HoTGE->StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::ClearEntireStack;
	HoTGE->StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::ResetOnSuccessfulApplication;

	// AC-022.6: Grant Restoration status tag for duration
	UTargetTagsGameplayEffectComponent& TargetTagsComp = HoTGE->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagChanges;
	TagChanges.AddTag(MordecaiGameplayTags::Status_Restoration);
	TargetTagsComp.SetAndApplyTargetTagChanges(TagChanges);

	// Apply to self
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpec Spec(HoTGE, Context, 1.0f);
	ASC->ApplyGameplayEffectSpecToSelf(Spec);
}
