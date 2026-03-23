// Project Mordecai — Skill Component (US-011)

#include "Mordecai/Skills/MordecaiSkillComponent.h"
#include "Mordecai/Skills/MordecaiSkillDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagsManager.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiSkillComponent)

namespace MordecaiSkillConstants
{
	static const int32 MilestoneRanks[] = { 1, 5, 10, 15, 20 };
	static constexpr int32 NumMilestoneRanks = 5;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

UMordecaiSkillComponent::UMordecaiSkillComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

// ---------------------------------------------------------------------------
// Skill Data Registration
// ---------------------------------------------------------------------------

void UMordecaiSkillComponent::RegisterSkillDataAsset(UMordecaiSkillDataAsset* SkillData)
{
	if (!SkillData) return;
	RegisteredSkills.Add(SkillData->SkillName, SkillData);
}

// ---------------------------------------------------------------------------
// Rank Entry Helpers (TArray-based storage for replication)
// ---------------------------------------------------------------------------

FMordecaiSkillRankEntry* UMordecaiSkillComponent::FindRankEntry(FName SkillName)
{
	for (FMordecaiSkillRankEntry& Entry : SkillRanks)
	{
		if (Entry.SkillName == SkillName) return &Entry;
	}
	return nullptr;
}

const FMordecaiSkillRankEntry* UMordecaiSkillComponent::FindRankEntry(FName SkillName) const
{
	for (const FMordecaiSkillRankEntry& Entry : SkillRanks)
	{
		if (Entry.SkillName == SkillName) return &Entry;
	}
	return nullptr;
}

void UMordecaiSkillComponent::SetRankInternal(FName SkillName, int32 NewRank)
{
	if (FMordecaiSkillRankEntry* Entry = FindRankEntry(SkillName))
	{
		Entry->Rank = NewRank;
	}
	else
	{
		FMordecaiSkillRankEntry NewEntry;
		NewEntry.SkillName = SkillName;
		NewEntry.Rank = NewRank;
		SkillRanks.Add(NewEntry);
	}
}

// ---------------------------------------------------------------------------
// Rank Queries
// ---------------------------------------------------------------------------

int32 UMordecaiSkillComponent::GetSkillRank(FName SkillName) const
{
	const FMordecaiSkillRankEntry* Entry = FindRankEntry(SkillName);
	return Entry ? Entry->Rank : 0;
}

TArray<FMordecaiSkillRankInfo> UMordecaiSkillComponent::GetAllSkillsByCategory(EMordecaiSkillCategory Category) const
{
	TArray<FMordecaiSkillRankInfo> Result;
	for (const auto& Pair : RegisteredSkills)
	{
		if (Pair.Value && Pair.Value->Category == Category)
		{
			FMordecaiSkillRankInfo Info;
			Info.SkillName = Pair.Key;
			Info.CurrentRank = GetSkillRank(Pair.Key);
			Result.Add(Info);
		}
	}
	return Result;
}

// ---------------------------------------------------------------------------
// Skill Point Allocation
// ---------------------------------------------------------------------------

bool UMordecaiSkillComponent::TryAllocateSkillPoint(FName SkillName)
{
	if (!RegisteredSkills.Contains(SkillName))
	{
		return false;
	}

	if (AvailableSkillPoints <= 0)
	{
		return false;
	}

	const int32 CurrentRank = GetSkillRank(SkillName);
	if (CurrentRank >= MaxSkillRank)
	{
		return false;
	}

	const int32 NewRank = CurrentRank + 1;
	SetRankInternal(SkillName, NewRank);
	AvailableSkillPoints = FMath::Max(0, AvailableSkillPoints - 1);

	RefreshMilestoneState(SkillName, CurrentRank, NewRank);
	return true;
}

void UMordecaiSkillComponent::SetSkillRank(FName SkillName, int32 NewRank)
{
	if (!RegisteredSkills.Contains(SkillName))
	{
		return;
	}

	NewRank = FMath::Clamp(NewRank, MinSkillRank, MaxSkillRank);
	const int32 OldRank = GetSkillRank(SkillName);

	if (NewRank == 0)
	{
		SkillRanks.RemoveAll([SkillName](const FMordecaiSkillRankEntry& E) { return E.SkillName == SkillName; });
	}
	else
	{
		SetRankInternal(SkillName, NewRank);
	}

	RefreshMilestoneState(SkillName, OldRank, NewRank);
}

// ---------------------------------------------------------------------------
// Skill Points
// ---------------------------------------------------------------------------

void UMordecaiSkillComponent::SetAvailableSkillPoints(int32 NewPoints)
{
	AvailableSkillPoints = FMath::Max(0, NewPoints);
}

// ---------------------------------------------------------------------------
// Milestone Helpers
// ---------------------------------------------------------------------------

bool UMordecaiSkillComponent::IsMilestoneRank(int32 Rank)
{
	for (int32 MR : MordecaiSkillConstants::MilestoneRanks)
	{
		if (Rank == MR) return true;
	}
	return false;
}

bool UMordecaiSkillComponent::HasMilestoneTag(FName SkillName, int32 Rank) const
{
	return ActiveMilestoneTagNames.Contains(MakeSkillMilestoneTagName(SkillName, Rank));
}

bool UMordecaiSkillComponent::HasGrantedMilestoneAbility(FName SkillName, int32 Rank) const
{
	const TSet<int32>* Ranks = GrantedMilestoneAbilityRanks.Find(SkillName);
	return Ranks && Ranks->Contains(Rank);
}

FName UMordecaiSkillComponent::MakeSkillMilestoneTagName(FName SkillName, int32 Rank)
{
	return FName(*FString::Printf(TEXT("Mordecai.Skill.%s.Rank%d"), *SkillName.ToString(), Rank));
}

FGameplayTag UMordecaiSkillComponent::FindMilestoneTag(FName SkillName, int32 Rank)
{
	const FName TagName = MakeSkillMilestoneTagName(SkillName, Rank);
	return UGameplayTagsManager::Get().RequestGameplayTag(TagName, false);
}

// ---------------------------------------------------------------------------
// Milestone State Refresh
// ---------------------------------------------------------------------------

void UMordecaiSkillComponent::RefreshMilestoneState(FName SkillName, int32 OldRank, int32 NewRank)
{
	UAbilitySystemComponent* ASC = FindAbilitySystemComponent();
	UMordecaiSkillDataAsset* SkillData = RegisteredSkills.FindRef(SkillName);

	for (int32 MilestoneRank : MordecaiSkillConstants::MilestoneRanks)
	{
		const bool bWasActive = OldRank >= MilestoneRank;
		const bool bShouldBeActive = NewRank >= MilestoneRank;

		if (bShouldBeActive && !bWasActive)
		{
			// --- Milestone newly reached ---

			// Fire delegates
			OnSkillMilestoneReached.Broadcast(SkillName, MilestoneRank);
			OnSkillMilestoneReachedBP.Broadcast(SkillName, MilestoneRank);

			// Send GAS gameplay event through ASC
			if (ASC)
			{
				FGameplayEventData EventData;
				EventData.EventTag = MordecaiGameplayTags::Event_SkillMilestone;
				ASC->HandleGameplayEvent(MordecaiGameplayTags::Event_SkillMilestone, &EventData);
			}

			// Grant milestone ability if configured
			if (SkillData)
			{
				const TSubclassOf<UGameplayAbility>* AbilityClass = SkillData->MilestoneAbilities.Find(MilestoneRank);
				if (AbilityClass && *AbilityClass)
				{
					GrantedMilestoneAbilityRanks.FindOrAdd(SkillName).Add(MilestoneRank);

					if (ASC)
					{
						FGameplayAbilitySpec Spec(*AbilityClass, 1, INDEX_NONE, this);
						FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
						MilestoneAbilityHandles.FindOrAdd(SkillName).Add(MilestoneRank, Handle);
					}
				}
			}

			// Track milestone tag (FName-based, always works)
			const FName TagName = MakeSkillMilestoneTagName(SkillName, MilestoneRank);
			ActiveMilestoneTagNames.Add(TagName);

			// Apply to ASC if the tag is pre-registered in the project
			if (ASC)
			{
				FGameplayTag Tag = FindMilestoneTag(SkillName, MilestoneRank);
				if (Tag.IsValid())
				{
					ASC->AddLooseGameplayTag(Tag);
				}
			}
		}
		else if (!bShouldBeActive && bWasActive)
		{
			// --- Milestone lost (rank dropped) ---

			// Remove milestone ability
			GrantedMilestoneAbilityRanks.FindOrAdd(SkillName).Remove(MilestoneRank);

			if (ASC)
			{
				TMap<int32, FGameplayAbilitySpecHandle>* Handles = MilestoneAbilityHandles.Find(SkillName);
				if (Handles)
				{
					FGameplayAbilitySpecHandle* Handle = Handles->Find(MilestoneRank);
					if (Handle && Handle->IsValid())
					{
						ASC->ClearAbility(*Handle);
						Handles->Remove(MilestoneRank);
					}
				}
			}

			// Remove milestone tag
			const FName TagName = MakeSkillMilestoneTagName(SkillName, MilestoneRank);
			ActiveMilestoneTagNames.Remove(TagName);

			if (ASC)
			{
				FGameplayTag Tag = FindMilestoneTag(SkillName, MilestoneRank);
				if (Tag.IsValid())
				{
					ASC->RemoveLooseGameplayTag(Tag);
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
// ASC Lookup
// ---------------------------------------------------------------------------

UAbilitySystemComponent* UMordecaiSkillComponent::FindAbilitySystemComponent() const
{
	AActor* Owner = GetOwner();
	if (!Owner) return nullptr;

	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
	{
		return ASI->GetAbilitySystemComponent();
	}

	return nullptr;
}

// ---------------------------------------------------------------------------
// Replication
// ---------------------------------------------------------------------------

void UMordecaiSkillComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UMordecaiSkillComponent, SkillRanks);
	DOREPLIFETIME(UMordecaiSkillComponent, AvailableSkillPoints);
}
