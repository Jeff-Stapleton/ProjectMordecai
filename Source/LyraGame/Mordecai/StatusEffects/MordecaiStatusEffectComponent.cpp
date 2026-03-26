// Project Mordecai — Status Effect Component (US-013)

#include "Mordecai/StatusEffects/MordecaiStatusEffectComponent.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectTypes.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Bleeding.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Frostbitten.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Frozen.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Shocked.h"
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

		// Auto-detect Frostbitten and check for max stacks → Frozen (US-015)
		const UMordecaiGE_Frostbitten* FrostbittenGE = Cast<UMordecaiGE_Frostbitten>(GEClass.GetDefaultObject());
		if (FrostbittenGE)
		{
			if (!bTrackingFrostbitten)
			{
				StartFrostbittenTracking(FrostbittenGE->FrostbittenMaxStacks);
			}
			FrostbittenActiveHandle = Handle;

			// Check if max stacks reached (AC-015.5)
			int32 CurrentStacks = ASC->GetCurrentStackCount(Handle);
			if (CurrentStacks >= CachedFrostbittenMaxStacks)
			{
				// Apply Frozen (AC-015.6)
				ApplyStatusEffect(UMordecaiGE_Frozen::StaticClass(), Instigator);
				// Remove all Frostbitten stacks
				RemoveStatusEffect(MordecaiGameplayTags::Status_Frostbitten);
				StopFrostbittenTracking();
			}
		}

		// Auto-detect Shocked and start tracking (US-015)
		const UMordecaiGE_Shocked* ShockedGE = Cast<UMordecaiGE_Shocked>(GEClass.GetDefaultObject());
		if (ShockedGE)
		{
			if (!bTrackingShocked)
			{
				StartShockedTracking(ShockedGE->ShockedMicroStunChancePerStack, ShockedGE->ShockedCastInterruptChance);
			}
			ShockedActiveHandle = Handle;
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
	// Shocked on-hit processing: micro-stun + cast interrupt (US-015)
	if (bTrackingShocked)
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
		if (ASC)
		{
			int32 ShockedStacks = ASC->GetCurrentStackCount(ShockedActiveHandle);
			if (ShockedStacks > 0)
			{
				// Micro-stun roll (AC-015.11): EffectiveChance = BaseChance * StackCount
				float MicroStunChance = CachedShockedMicroStunChance * ShockedStacks;
				UMordecaiGE_Shocked::TryMicroStun(ASC, MicroStunChance);

				// Cast interrupt roll (AC-015.13): EffectiveChance = BaseChance * StackCount
				float CastInterruptChance = CachedShockedCastInterruptChance * ShockedStacks;
				UMordecaiGE_Shocked::TryShockedCastInterrupt(ASC, CastInterruptChance);
			}
		}
	}

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

// ---------------------------------------------------------------------------
// Frostbitten Management (US-015)
// ---------------------------------------------------------------------------

void UMordecaiStatusEffectComponent::StartFrostbittenTracking(int32 MaxStacks)
{
	CachedFrostbittenMaxStacks = MaxStacks;
	bTrackingFrostbitten = true;
}

void UMordecaiStatusEffectComponent::StopFrostbittenTracking()
{
	bTrackingFrostbitten = false;
	FrostbittenActiveHandle.Invalidate();
}

int32 UMordecaiStatusEffectComponent::GetFrostbittenStackCount() const
{
	if (!bTrackingFrostbitten || !FrostbittenActiveHandle.IsValid())
	{
		return 0;
	}

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return 0;
	}

	return ASC->GetCurrentStackCount(FrostbittenActiveHandle);
}

void UMordecaiStatusEffectComponent::ReduceFrostbittenStacks(int32 CountToRemove)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !FrostbittenActiveHandle.IsValid())
	{
		return;
	}

	ASC->RemoveActiveGameplayEffect(FrostbittenActiveHandle, CountToRemove);

	// Check if all stacks removed
	if (ASC->GetCurrentStackCount(FrostbittenActiveHandle) <= 0)
	{
		StopFrostbittenTracking();
	}
}

// ---------------------------------------------------------------------------
// Shocked Management (US-015)
// ---------------------------------------------------------------------------

void UMordecaiStatusEffectComponent::StartShockedTracking(float MicroStunChance, float CastInterruptChance)
{
	CachedShockedMicroStunChance = MicroStunChance;
	CachedShockedCastInterruptChance = CastInterruptChance;
	bTrackingShocked = true;
}

void UMordecaiStatusEffectComponent::StopShockedTracking()
{
	bTrackingShocked = false;
	ShockedActiveHandle.Invalidate();
}

int32 UMordecaiStatusEffectComponent::GetShockedStackCount() const
{
	if (!bTrackingShocked || !ShockedActiveHandle.IsValid())
	{
		return 0;
	}

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return 0;
	}

	return ASC->GetCurrentStackCount(ShockedActiveHandle);
}

void UMordecaiStatusEffectComponent::ReduceShockedStacks(int32 CountToRemove)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !ShockedActiveHandle.IsValid())
	{
		return;
	}

	ASC->RemoveActiveGameplayEffect(ShockedActiveHandle, CountToRemove);

	// Check if all stacks removed
	if (ASC->GetCurrentStackCount(ShockedActiveHandle) <= 0)
	{
		StopShockedTracking();
	}
}
