// Project Mordecai — Feat Component (US-012)

#include "MordecaiFeatComponent.h"
#include "Mordecai/Feats/MordecaiFeatDataAsset.h"
#include "Mordecai/Skills/MordecaiSkillComponent.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiFeatComponent)

UMordecaiFeatComponent::UMordecaiFeatComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

// ---------------------------------------------------------------------------
// Feat Registration
// ---------------------------------------------------------------------------

void UMordecaiFeatComponent::RegisterFeatDataAsset(UMordecaiFeatDataAsset* FeatData)
{
	if (!FeatData || FeatData->FeatName.IsNone())
	{
		return;
	}
	RegisteredFeats.Add(FeatData->FeatName, FeatData);
}

// ---------------------------------------------------------------------------
// Stat Tracking
// ---------------------------------------------------------------------------

void UMordecaiFeatComponent::IncrementFeatStat(FName StatName, int32 Amount)
{
	if (StatName.IsNone() || Amount <= 0)
	{
		return;
	}

	int32& CurrentValue = TrackedStats.FindOrAdd(StatName);
	CurrentValue += Amount;

	// Re-evaluate feats that have conditions referencing this stat
	for (auto& Pair : RegisteredFeats)
	{
		if (UnlockedFeats.Contains(Pair.Key))
		{
			continue;
		}

		bool bRelevant = false;
		for (const FMordecaiFeatCondition& Cond : Pair.Value->UnlockConditions)
		{
			if ((Cond.ConditionType == EMordecaiFeatConditionType::StatThreshold ||
				 Cond.ConditionType == EMordecaiFeatConditionType::EventCount) &&
				Cond.StatName == StatName)
			{
				bRelevant = true;
				break;
			}
		}

		if (bRelevant)
		{
			EvaluateFeatConditions(Pair.Value);
		}
	}
}

int32 UMordecaiFeatComponent::GetFeatStatValue(FName StatName) const
{
	const int32* Value = TrackedStats.Find(StatName);
	return Value ? *Value : 0;
}

// ---------------------------------------------------------------------------
// Feat Queries
// ---------------------------------------------------------------------------

bool UMordecaiFeatComponent::HasFeat(FName FeatName) const
{
	return UnlockedFeats.Contains(FeatName);
}

FMordecaiFeatAppliedRecord UMordecaiFeatComponent::GetFeatAppliedRecord(FName FeatName) const
{
	const FMordecaiFeatAppliedRecord* Record = FeatAppliedRecords.Find(FeatName);
	return Record ? *Record : FMordecaiFeatAppliedRecord();
}

bool UMordecaiFeatComponent::HasGrantedFeatTag(FGameplayTag Tag) const
{
	return GrantedFeatTags.Contains(Tag);
}

// ---------------------------------------------------------------------------
// External Notifications
// ---------------------------------------------------------------------------

void UMordecaiFeatComponent::NotifyTagChanged(FGameplayTag Tag, bool bAdded)
{
	if (!Tag.IsValid())
	{
		return;
	}

	if (bAdded)
	{
		ActiveTags.AddTag(Tag);
	}
	else
	{
		ActiveTags.RemoveTag(Tag);
	}

	ReevaluateAllFeats();
}

void UMordecaiFeatComponent::NotifySkillRankChanged(FName SkillName, int32 NewRank)
{
	ReevaluateAllFeats();
}

// ---------------------------------------------------------------------------
// Dependency Injection
// ---------------------------------------------------------------------------

void UMordecaiFeatComponent::SetSkillComponentOverride(UMordecaiSkillComponent* InSkillComp)
{
	SkillComponentOverride = InSkillComp;
}

// ---------------------------------------------------------------------------
// Replication
// ---------------------------------------------------------------------------

void UMordecaiFeatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UMordecaiFeatComponent, ReplicatedUnlockedFeats);
}

// ---------------------------------------------------------------------------
// Internal: Evaluation
// ---------------------------------------------------------------------------

void UMordecaiFeatComponent::ReevaluateAllFeats()
{
	for (auto& Pair : RegisteredFeats)
	{
		if (!UnlockedFeats.Contains(Pair.Key))
		{
			EvaluateFeatConditions(Pair.Value);
		}
	}
}

void UMordecaiFeatComponent::EvaluateFeatConditions(UMordecaiFeatDataAsset* FeatData)
{
	if (!FeatData || UnlockedFeats.Contains(FeatData->FeatName))
	{
		return;
	}

	// All conditions must pass (AND logic)
	for (const FMordecaiFeatCondition& Cond : FeatData->UnlockConditions)
	{
		if (!EvaluateCondition(Cond))
		{
			return; // At least one condition failed
		}
	}

	// All conditions passed — unlock the feat
	UnlockFeat(FeatData);
}

bool UMordecaiFeatComponent::EvaluateCondition(const FMordecaiFeatCondition& Condition) const
{
	switch (Condition.ConditionType)
	{
	case EMordecaiFeatConditionType::StatThreshold:
	case EMordecaiFeatConditionType::EventCount:
	{
		const int32* Value = TrackedStats.Find(Condition.StatName);
		return Value && (*Value >= Condition.ThresholdValue);
	}

	case EMordecaiFeatConditionType::TagPresent:
	{
		return ActiveTags.HasTag(Condition.RequiredTag);
	}

	case EMordecaiFeatConditionType::SkillRank:
	{
		UMordecaiSkillComponent* SkillComp = FindSkillComponent();
		if (!SkillComp)
		{
			return false; // Gracefully handle missing skill component
		}
		return SkillComp->GetSkillRank(Condition.StatName) >= Condition.ThresholdValue;
	}

	default:
		return false;
	}
}

// ---------------------------------------------------------------------------
// Internal: Unlock
// ---------------------------------------------------------------------------

void UMordecaiFeatComponent::UnlockFeat(UMordecaiFeatDataAsset* FeatData)
{
	if (!FeatData || UnlockedFeats.Contains(FeatData->FeatName))
	{
		return; // Duplicate protection
	}

	// Mark as unlocked
	UnlockedFeats.Add(FeatData->FeatName);
	ReplicatedUnlockedFeats.Add(FeatData->FeatName);

	// Track what we apply
	FMordecaiFeatAppliedRecord& Record = FeatAppliedRecords.Add(FeatData->FeatName);
	UAbilitySystemComponent* ASC = FindAbilitySystemComponent();

	// Always apply granted effects
	Record.GrantedEffectsCount = FeatData->GrantedEffects.Num();
	if (ASC)
	{
		for (const TSubclassOf<UGameplayEffect>& EffectClass : FeatData->GrantedEffects)
		{
			if (EffectClass)
			{
				FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
				FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1.0f, Context);
				if (Spec.IsValid())
				{
					ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
				}
			}
		}
	}

	// Always apply granted abilities
	Record.GrantedAbilitiesCount = FeatData->GrantedAbilities.Num();
	if (ASC)
	{
		TArray<FGameplayAbilitySpecHandle>& Handles = FeatAbilityHandles.FindOrAdd(FeatData->FeatName);
		for (const TSubclassOf<UGameplayAbility>& AbilityClass : FeatData->GrantedAbilities)
		{
			if (AbilityClass)
			{
				FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
				Handles.Add(Handle);
			}
		}
	}

	// Apply drawback effects only for Rare and Legendary tiers
	if (FeatData->Tier != EMordecaiFeatTier::Common)
	{
		Record.DrawbackEffectsCount = FeatData->DrawbackEffects.Num();
		if (ASC)
		{
			for (const TSubclassOf<UGameplayEffect>& EffectClass : FeatData->DrawbackEffects)
			{
				if (EffectClass)
				{
					FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
					FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1.0f, Context);
					if (Spec.IsValid())
					{
						ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
					}
				}
			}
		}
	}
	// Common: DrawbackEffectsCount stays 0 (default)

	// Grant feat tag
	if (FeatData->FeatTag.IsValid())
	{
		GrantedFeatTags.Add(FeatData->FeatTag);
		if (ASC)
		{
			ASC->AddLooseGameplayTag(FeatData->FeatTag);
		}
	}

	// Fire delegates
	OnFeatUnlocked.Broadcast(FeatData->FeatName);
	OnFeatUnlockedBP.Broadcast(FeatData->FeatName);
}

// ---------------------------------------------------------------------------
// Internal: Component Lookups
// ---------------------------------------------------------------------------

UAbilitySystemComponent* UMordecaiFeatComponent::FindAbilitySystemComponent() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}
	return Owner->FindComponentByClass<UAbilitySystemComponent>();
}

UMordecaiSkillComponent* UMordecaiFeatComponent::FindSkillComponent() const
{
	if (SkillComponentOverride)
	{
		return SkillComponentOverride;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}
	return Owner->FindComponentByClass<UMordecaiSkillComponent>();
}
