// Project Mordecai — Status Effect Component (US-013)

#include "Mordecai/StatusEffects/MordecaiStatusEffectComponent.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectTypes.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Bleeding.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Burning.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Drenched.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Rooted.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Frostbitten.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Shocked.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Frozen.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_MicroStunned.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
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

	// AC-018.5/018.6: Handle Drenched elemental interactions before application
	if (!HandleDrenchedInteractions(GEClass))
	{
		// Application cancelled (e.g., Burning on Drenched target)
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

	// Auto-detect Rooted and start tracking break-free event (US-017)
	if (Handle.IsValid())
	{
		const UMordecaiGE_Rooted* RootedGE = Cast<UMordecaiGE_Rooted>(GEClass.GetDefaultObject());
		if (RootedGE)
		{
			StartRootedBreakFreeTracking(RootedGE->RootedBreakFreeStaminaCost);
		}
	}

	// AC-018.5: When Drenched is applied, douse Burning
	if (Handle.IsValid())
	{
		const UMordecaiGE_Drenched* DrenchedGE = Cast<UMordecaiGE_Drenched>(GEClass.GetDefaultObject());
		if (DrenchedGE && HasStatusEffect(MordecaiGameplayTags::Status_Burning))
		{
			RemoveStatusEffect(MordecaiGameplayTags::Status_Burning);
		}
	}

	// Auto-detect Frostbitten and start stack tracking (US-015)
	if (Handle.IsValid())
	{
		const UMordecaiGE_Frostbitten* FrostbittenGE = Cast<UMordecaiGE_Frostbitten>(GEClass.GetDefaultObject());
		if (FrostbittenGE && HasStatusEffect(MordecaiGameplayTags::Status_Frostbitten))
		{
			StartFrostbittenTracking(Handle);

			// Drenched bonus stacks: apply extra stacks if target is Drenched.
			// Guard: stop if Frozen triggers during a bonus application (delegate fires synchronously).
			const int32 BonusStacks = UMordecaiGE_Drenched::GetFrostBonusStacks(ASC);
			for (int32 i = 0; i < BonusStacks && bTrackingFrostbitten; ++i)
			{
				FGameplayEffectSpecHandle BonusSpec = ASC->MakeOutgoingSpec(GEClass, Level, Context);
				if (BonusSpec.IsValid())
				{
					ASC->ApplyGameplayEffectSpecToSelf(*BonusSpec.Data.Get());
				}
			}
		}
	}

	// Auto-detect Shocked and start stack tracking (US-015)
	if (Handle.IsValid())
	{
		const UMordecaiGE_Shocked* ShockedGE = Cast<UMordecaiGE_Shocked>(GEClass.GetDefaultObject());
		if (ShockedGE)
		{
			StartShockedTracking(Handle);
		}
	}

	return Handle;
}

// ---------------------------------------------------------------------------
// Drenched Elemental Interactions (US-018)
// ---------------------------------------------------------------------------

bool UMordecaiStatusEffectComponent::HandleDrenchedInteractions(TSubclassOf<UGameplayEffect> GEClass)
{
	if (!GEClass)
	{
		return true;
	}

	// AC-018.6: Burning on a Drenched target → remove Drenched, cancel Burning
	const UMordecaiGE_Burning* BurningGE = Cast<UMordecaiGE_Burning>(GEClass.GetDefaultObject());
	if (BurningGE && HasStatusEffect(MordecaiGameplayTags::Status_Drenched))
	{
		RemoveStatusEffect(MordecaiGameplayTags::Status_Drenched);
		return false; // Cancel Burning application
	}

	return true; // Allow normal application
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
	// AC-016.12: Consume Exposed on first hit (bonus damage already applied via attribute multiplier)
	if (HasStatusEffect(MordecaiGameplayTags::Status_Exposed))
	{
		RemoveStatusEffect(MordecaiGameplayTags::Status_Exposed);
	}

	// AC-015.11: Shocked micro-stun on hit
	if (bTrackingShocked && HasStatusEffect(MordecaiGameplayTags::Status_Shocked))
	{
		TryShockedMicroStun();

		// AC-015.13: Shocked cast interrupt on hit while casting
		TryShockedCastInterrupt();
	}

	// Bleeding hit-refresh (AC-014.8)
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
// Rooted Break-Free Management (US-017)
// ---------------------------------------------------------------------------

void UMordecaiStatusEffectComponent::StartRootedBreakFreeTracking(float BreakFreeStaminaCost)
{
	CachedBreakFreeStaminaCost = BreakFreeStaminaCost;
	bTrackingRootedBreakFree = true;

	// Register for BreakFree gameplay event on the ASC
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC)
	{
		FGameplayEventMulticastDelegate& Delegate = ASC->GenericGameplayEventCallbacks.FindOrAdd(
			MordecaiGameplayTags::Event_BreakFree);
		BreakFreeDelegateHandle = Delegate.AddUObject(
			this, &UMordecaiStatusEffectComponent::OnBreakFreeEvent);
	}
}

void UMordecaiStatusEffectComponent::StopRootedBreakFreeTracking()
{
	bTrackingRootedBreakFree = false;

	// Unregister event callback
	if (BreakFreeDelegateHandle.IsValid())
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
		if (ASC)
		{
			ASC->GenericGameplayEventCallbacks.FindOrAdd(
				MordecaiGameplayTags::Event_BreakFree).Remove(BreakFreeDelegateHandle);
		}
		BreakFreeDelegateHandle.Reset();
	}
}

void UMordecaiStatusEffectComponent::OnBreakFreeEvent(const FGameplayEventData* Payload)
{
	if (!bTrackingRootedBreakFree)
	{
		return;
	}

	// AC-017.9: Check Rooted is active
	if (!HasStatusEffect(MordecaiGameplayTags::Status_Rooted))
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	// Check stamina >= cost
	float CurrentStamina = ASC->GetNumericAttribute(UMordecaiAttributeSet::GetStaminaAttribute());
	if (CurrentStamina < CachedBreakFreeStaminaCost)
	{
		return;
	}

	// Deduct stamina via instant GE
	UGameplayEffect* StaminaCostGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_MordecaiBreakFreeStaminaCost"));
	StaminaCostGE->DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& Mod = StaminaCostGE->Modifiers.AddDefaulted_GetRef();
	Mod.Attribute = UMordecaiAttributeSet::GetStaminaAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-CachedBreakFreeStaminaCost));

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpec Spec(StaminaCostGE, Context, 1.0f);
	ASC->ApplyGameplayEffectSpecToSelf(Spec);

	// Remove Root
	RemoveStatusEffect(MordecaiGameplayTags::Status_Rooted);
	StopRootedBreakFreeTracking();
}

// ---------------------------------------------------------------------------
// Stack Count Query (US-015)
// ---------------------------------------------------------------------------

int32 UMordecaiStatusEffectComponent::GetStatusEffectStackCount(FGameplayTag StatusTag) const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !StatusTag.IsValid())
	{
		return 0;
	}

	if (!ASC->HasMatchingGameplayTag(StatusTag))
	{
		return 0;
	}

	// Query active effects by owning tags (same query used by RemoveActiveEffectsWithGrantedTags)
	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(StatusTag);

	FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(TagContainer);
	TArray<FActiveGameplayEffectHandle> Handles = ASC->GetActiveEffects(Query);
	if (Handles.Num() > 0)
	{
		return ASC->GetCurrentStackCount(Handles[0]);
	}

	return 0;
}

// ---------------------------------------------------------------------------
// Frostbitten Stack Tracking (US-015)
// ---------------------------------------------------------------------------

void UMordecaiStatusEffectComponent::StartFrostbittenTracking(FActiveGameplayEffectHandle InHandle)
{
	if (bTrackingFrostbitten)
	{
		// Already tracking — stack increments handled by OnFrostbittenStackChanged
		return;
	}

	CachedFrostbittenHandle = InHandle;
	bTrackingFrostbitten = true;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC)
	{
		FOnActiveGameplayEffectStackChange* Delegate = ASC->OnGameplayEffectStackChangeDelegate(InHandle);
		if (Delegate)
		{
			FrostbittenStackChangeDelegateHandle = Delegate->AddUObject(
				this, &UMordecaiStatusEffectComponent::OnFrostbittenStackChanged);
		}
	}
}

void UMordecaiStatusEffectComponent::StopFrostbittenTracking()
{
	if (!bTrackingFrostbitten)
	{
		return;
	}

	bTrackingFrostbitten = false;

	if (FrostbittenStackChangeDelegateHandle.IsValid())
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
		if (ASC)
		{
			FOnActiveGameplayEffectStackChange* Delegate = ASC->OnGameplayEffectStackChangeDelegate(CachedFrostbittenHandle);
			if (Delegate)
			{
				Delegate->Remove(FrostbittenStackChangeDelegateHandle);
			}
		}
		FrostbittenStackChangeDelegateHandle.Reset();
	}

	CachedFrostbittenHandle = FActiveGameplayEffectHandle();
}

void UMordecaiStatusEffectComponent::OnFrostbittenStackChanged(
	FActiveGameplayEffectHandle Handle,
	int32 NewStackCount,
	int32 PreviousStackCount)
{
	if (!bTrackingFrostbitten)
	{
		return;
	}

	const UMordecaiGE_Frostbitten* CDO = GetDefault<UMordecaiGE_Frostbitten>();

	// AC-015.5/015.6: At max stacks, trigger Frozen and clear Frostbitten
	if (NewStackCount >= CDO->FrostbittenMaxStacks)
	{
		// Remove Frostbitten first (clears all stacks)
		StopFrostbittenTracking();
		RemoveStatusEffect(MordecaiGameplayTags::Status_Frostbitten);

		// Apply Frozen
		UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
		if (ASC)
		{
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			FGameplayEffectSpecHandle FrozenSpec = ASC->MakeOutgoingSpec(
				UMordecaiGE_Frozen::StaticClass(), 1.0f, Context);
			if (FrozenSpec.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*FrozenSpec.Data.Get());
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Shocked Stack Tracking (US-015)
// ---------------------------------------------------------------------------

void UMordecaiStatusEffectComponent::StartShockedTracking(FActiveGameplayEffectHandle InHandle)
{
	if (bTrackingShocked)
	{
		// Already tracking — stack increments handled by OnShockedStackChanged
		return;
	}

	CachedShockedHandle = InHandle;
	bTrackingShocked = true;
	CachedShockedStackCount = 1;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC)
	{
		FOnActiveGameplayEffectStackChange* Delegate = ASC->OnGameplayEffectStackChangeDelegate(InHandle);
		if (Delegate)
		{
			ShockedStackChangeDelegateHandle = Delegate->AddUObject(
				this, &UMordecaiStatusEffectComponent::OnShockedStackChanged);
		}
	}

	// Apply initial companion modifier for 1 stack
	UpdateShockedBlockCostModifier(1);
}

void UMordecaiStatusEffectComponent::StopShockedTracking()
{
	if (!bTrackingShocked)
	{
		return;
	}

	bTrackingShocked = false;
	CachedShockedStackCount = 0;
	ShockedMicroStunChanceOverride = -1.0f;
	ShockedCastInterruptChanceOverride = -1.0f;

	// Remove companion modifier GE
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC && ShockedBlockCostCompanionHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(ShockedBlockCostCompanionHandle);
		ShockedBlockCostCompanionHandle = FActiveGameplayEffectHandle();
	}

	// Unregister stack change delegate
	if (ShockedStackChangeDelegateHandle.IsValid())
	{
		if (ASC)
		{
			FOnActiveGameplayEffectStackChange* Delegate = ASC->OnGameplayEffectStackChangeDelegate(CachedShockedHandle);
			if (Delegate)
			{
				Delegate->Remove(ShockedStackChangeDelegateHandle);
			}
		}
		ShockedStackChangeDelegateHandle.Reset();
	}

	CachedShockedHandle = FActiveGameplayEffectHandle();
}

void UMordecaiStatusEffectComponent::OnShockedStackChanged(
	FActiveGameplayEffectHandle Handle,
	int32 NewStackCount,
	int32 PreviousStackCount)
{
	if (!bTrackingShocked)
	{
		return;
	}

	CachedShockedStackCount = NewStackCount;

	if (NewStackCount <= 0)
	{
		StopShockedTracking();
		return;
	}

	// Update companion modifier GE magnitude for new stack count
	UpdateShockedBlockCostModifier(NewStackCount);
}

void UMordecaiStatusEffectComponent::UpdateShockedBlockCostModifier(int32 StackCount)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	const UMordecaiGE_Shocked* CDO = GetDefault<UMordecaiGE_Shocked>();
	const float Magnitude = CDO->ShockedBlockStaminaCostMultiplierPerStack * static_cast<float>(StackCount);

	// Remove old companion GE if it exists
	if (ShockedBlockCostCompanionHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(ShockedBlockCostCompanionHandle);
		ShockedBlockCostCompanionHandle = FActiveGameplayEffectHandle();
	}

	// Create new dynamic Infinite GE with the correct magnitude
	UGameplayEffect* CompanionGE = NewObject<UGameplayEffect>(
		GetTransientPackage(), TEXT("GE_MordecaiShockedBlockCostModifier"));
	CompanionGE->DurationPolicy = EGameplayEffectDurationType::Infinite;

	FGameplayModifierInfo& Mod = CompanionGE->Modifiers.AddDefaulted_GetRef();
	Mod.Attribute = UMordecaiAttributeSet::GetBlockStaminaCostMultiplierAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Magnitude));

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpec Spec(CompanionGE, Context, 1.0f);
	ShockedBlockCostCompanionHandle = ASC->ApplyGameplayEffectSpecToSelf(Spec);
}

bool UMordecaiStatusEffectComponent::TryShockedMicroStun()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !bTrackingShocked)
	{
		return false;
	}

	if (!HasStatusEffect(MordecaiGameplayTags::Status_Shocked))
	{
		return false;
	}

	const UMordecaiGE_Shocked* CDO = GetDefault<UMordecaiGE_Shocked>();
	const int32 StackCount = FMath::Max(CachedShockedStackCount, 1);

	// Calculate effective chance: base * stacks * Drenched multiplier
	float EffectiveChance = CDO->ShockedMicroStunChancePerStack * static_cast<float>(StackCount);
	EffectiveChance *= UMordecaiGE_Drenched::GetShockedChanceMultiplier(ASC);

	// Test override
	if (ShockedMicroStunChanceOverride >= 0.0f)
	{
		EffectiveChance = ShockedMicroStunChanceOverride;
	}

	// Roll
	const float Roll = FMath::FRand();
	if (Roll >= EffectiveChance)
	{
		return false;
	}

	// Apply MicroStunned
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle StunSpec = ASC->MakeOutgoingSpec(
		UMordecaiGE_MicroStunned::StaticClass(), 1.0f, Context);
	if (StunSpec.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*StunSpec.Data.Get());
	}

	return true;
}

bool UMordecaiStatusEffectComponent::TryShockedCastInterrupt()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !bTrackingShocked)
	{
		return false;
	}

	// AC-015.13: Only interrupt if target is currently casting
	if (!ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Casting))
	{
		return false;
	}

	const UMordecaiGE_Shocked* CDO = GetDefault<UMordecaiGE_Shocked>();
	const int32 StackCount = FMath::Max(CachedShockedStackCount, 1);

	float EffectiveChance = CDO->ShockedCastInterruptChance * static_cast<float>(StackCount);

	// Test override
	if (ShockedCastInterruptChanceOverride >= 0.0f)
	{
		EffectiveChance = ShockedCastInterruptChanceOverride;
	}

	const float Roll = FMath::FRand();
	if (Roll >= EffectiveChance)
	{
		return false;
	}

	// Send CastInterrupted event (actual handling deferred to Epic 5)
	FGameplayEventData Payload;
	Payload.EventTag = MordecaiGameplayTags::Event_CastInterrupted;
	ASC->HandleGameplayEvent(Payload.EventTag, &Payload);

	return true;
}
