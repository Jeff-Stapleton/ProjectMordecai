// Project Mordecai — Status Effect Component (US-013)

#include "Mordecai/StatusEffects/MordecaiStatusEffectComponent.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectTypes.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Bleeding.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "TimerManager.h"

UMordecaiStatusEffectComponent::UMordecaiStatusEffectComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FActiveGameplayEffectHandle UMordecaiStatusEffectComponent::ApplyStatusEffect(
	TSubclassOf<UGameplayEffect> GEClass,
	AActor* Instigator,
	float Level)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !GEClass)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	if (Instigator)
	{
		Context.AddInstigator(Instigator, Instigator);
	}

	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(GEClass, Level, Context);
	if (!Spec.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

	// Auto-detect Bleeding and start tracking hit-refresh + clot (US-014)
	if (Handle.IsValid())
	{
		const UMordecaiGE_Bleeding* BleedingGE = Cast<UMordecaiGE_Bleeding>(GEClass.GetDefaultObject());
		if (BleedingGE)
		{
			StartBleedingTracking(GEClass, BleedingGE->BleedingClotTimeSec);
		}
	}

	return Handle;
}

void UMordecaiStatusEffectComponent::RemoveStatusEffect(FGameplayTag StatusTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !StatusTag.IsValid())
	{
		return;
	}

	// Remove all active GEs that grant this status tag
	FGameplayTagContainer TagsToRemove;
	TagsToRemove.AddTag(StatusTag);
	ASC->RemoveActiveEffectsWithGrantedTags(TagsToRemove);
}

void UMordecaiStatusEffectComponent::RemoveAllStatusEffects()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	// Remove GEs that grant any Mordecai.Status.* tag
	static const FGameplayTag StatusRoot = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Status")), false);
	if (!StatusRoot.IsValid())
	{
		return;
	}

	FGameplayTagContainer TagsToRemove;
	TagsToRemove.AddTag(StatusRoot);
	ASC->RemoveActiveEffectsWithGrantedTags(TagsToRemove);
}

void UMordecaiStatusEffectComponent::RemoveStatusEffectsByCategory(FGameplayTag CategoryTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !CategoryTag.IsValid())
	{
		return;
	}

	// Map CategoryTag → EMordecaiStatusEffectCategory
	EMordecaiStatusEffectCategory Category = EMordecaiStatusEffectCategory::Debuff;

	using namespace MordecaiGameplayTags;
	if (CategoryTag == Status_Category_DoT)                { Category = EMordecaiStatusEffectCategory::DoT; }
	else if (CategoryTag == Status_Category_Debuff)        { Category = EMordecaiStatusEffectCategory::Debuff; }
	else if (CategoryTag == Status_Category_Control)       { Category = EMordecaiStatusEffectCategory::Control; }
	else if (CategoryTag == Status_Category_Buff)          { Category = EMordecaiStatusEffectCategory::Buff; }
	else if (CategoryTag == Status_Category_Environmental) { Category = EMordecaiStatusEffectCategory::Environmental; }

	// Get all status tags in this category and remove their GEs
	TArray<FGameplayTag> StatusTags = GetStatusTagsForCategory(Category);
	for (const FGameplayTag& StatusTag : StatusTags)
	{
		FGameplayTagContainer TagsToRemove;
		TagsToRemove.AddTag(StatusTag);
		ASC->RemoveActiveEffectsWithGrantedTags(TagsToRemove);
	}
}

bool UMordecaiStatusEffectComponent::HasStatusEffect(FGameplayTag StatusTag) const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !StatusTag.IsValid())
	{
		return false;
	}

	return ASC->HasMatchingGameplayTag(StatusTag);
}

FGameplayTagContainer UMordecaiStatusEffectComponent::GetActiveStatusTags() const
{
	FGameplayTagContainer Result;
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return Result;
	}

	// Get all owned tags from the ASC
	FGameplayTagContainer AllTags;
	ASC->GetOwnedGameplayTags(AllTags);

	// Filter to only Mordecai.Status.* tags (exclude Category subtags)
	static const FGameplayTag StatusRoot = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Status")), false);
	static const FGameplayTag CategoryRoot = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Status.Category")), false);

	for (const FGameplayTag& Tag : AllTags)
	{
		if (Tag.MatchesTag(StatusRoot) && !Tag.MatchesTag(CategoryRoot))
		{
			Result.AddTag(Tag);
		}
	}

	return Result;
}

void UMordecaiStatusEffectComponent::SetAbilitySystemComponentOverride(UAbilitySystemComponent* InASC)
{
	ASCOverride = InASC;
}

UAbilitySystemComponent* UMordecaiStatusEffectComponent::GetAbilitySystemComponent() const
{
	if (ASCOverride)
	{
		return ASCOverride;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	// Try IAbilitySystemInterface first
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
	{
		return ASI->GetAbilitySystemComponent();
	}

	// Fall back to component search
	return Owner->FindComponentByClass<UAbilitySystemComponent>();
}

// ---------------------------------------------------------------------------
// Bleeding Management (US-014)
// ---------------------------------------------------------------------------

void UMordecaiStatusEffectComponent::NotifyDamageTaken()
{
	if (!bTrackingBleeding || !CachedBleedingGEClass)
	{
		return;
	}

	// Hit-refresh: remove current Bleeding and re-apply (AC-014.8)
	RemoveStatusEffect(MordecaiGameplayTags::Status_Bleeding);

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC)
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(CachedBleedingGEClass, 1.0f, Context);
		if (Spec.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	// Reset clot timer (AC-014.9)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BleedingClotTimerHandle);
		World->GetTimerManager().SetTimer(
			BleedingClotTimerHandle,
			this,
			&UMordecaiStatusEffectComponent::OnBleedingClotExpired,
			CachedBleedingClotDuration,
			false);
	}
}

void UMordecaiStatusEffectComponent::StartBleedingTracking(TSubclassOf<UGameplayEffect> BleedingGEClass, float ClotTimeSec)
{
	CachedBleedingGEClass = BleedingGEClass;
	CachedBleedingClotDuration = ClotTimeSec;
	bTrackingBleeding = true;

	// Start clot timer (AC-014.9)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			BleedingClotTimerHandle,
			this,
			&UMordecaiStatusEffectComponent::OnBleedingClotExpired,
			CachedBleedingClotDuration,
			false);
	}
}

void UMordecaiStatusEffectComponent::StopBleedingTracking()
{
	bTrackingBleeding = false;
	CachedBleedingGEClass = nullptr;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BleedingClotTimerHandle);
	}
}

void UMordecaiStatusEffectComponent::ForceBleedingClotExpiry()
{
	OnBleedingClotExpired();
}

void UMordecaiStatusEffectComponent::OnBleedingClotExpired()
{
	if (!bTrackingBleeding)
	{
		return;
	}

	// Clot mechanic: remove Bleeding early (AC-014.9)
	RemoveStatusEffect(MordecaiGameplayTags::Status_Bleeding);
	StopBleedingTracking();
}
