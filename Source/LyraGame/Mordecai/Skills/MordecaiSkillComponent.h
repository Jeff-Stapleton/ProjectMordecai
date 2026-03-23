// Project Mordecai — Skill Component (US-011)

#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpec.h"
#include "Mordecai/Skills/MordecaiSkillTypes.h"

#include "MordecaiSkillComponent.generated.h"

class UMordecaiSkillDataAsset;
class UAbilitySystemComponent;

/** Native C++ delegate fired when a skill reaches a milestone rank. */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnMordecaiSkillMilestoneReached, FName /*SkillName*/, int32 /*MilestoneRank*/);

/** Blueprint-exposed delegate for skill milestone events. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMordecaiSkillMilestoneReachedBP, FName, SkillName, int32, MilestoneRank);

/**
 * UMordecaiSkillComponent
 *
 *	ActorComponent on PlayerState that stores per-character skill ranks
 *	and manages the milestone grant/revoke pipeline.
 *	Provides the public API for querying and allocating skill points.
 */
UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class LYRAGAME_API UMordecaiSkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMordecaiSkillComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Skill Data Registration ---

	/** Register a skill data asset so the component can validate and grant milestones. */
	void RegisterSkillDataAsset(UMordecaiSkillDataAsset* SkillData);

	// --- Rank Queries ---

	/** Returns the current rank of a skill (0 if not learned). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Skills")
	int32 GetSkillRank(FName SkillName) const;

	/** Returns all registered skills in the given category with their current ranks. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Skills")
	TArray<FMordecaiSkillRankInfo> GetAllSkillsByCategory(EMordecaiSkillCategory Category) const;

	// --- Skill Point Allocation ---

	/** Attempt to spend one skill point on a skill. Returns true on success. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Skills")
	bool TryAllocateSkillPoint(FName SkillName);

	/** Directly set a skill's rank (debug/admin). Handles milestone grant/revoke. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Skills")
	void SetSkillRank(FName SkillName, int32 NewRank);

	// --- Skill Points ---

	UFUNCTION(BlueprintCallable, Category = "Mordecai|Skills")
	int32 GetAvailableSkillPoints() const { return AvailableSkillPoints; }

	UFUNCTION(BlueprintCallable, Category = "Mordecai|Skills")
	void SetAvailableSkillPoints(int32 NewPoints);

	// --- Milestone Queries ---

	/** Returns true if Rank is a milestone rank (1, 5, 10, 15, or 20). */
	static bool IsMilestoneRank(int32 Rank);

	/** Returns true if the milestone tag for SkillName at the given Rank is active. */
	bool HasMilestoneTag(FName SkillName, int32 Rank) const;

	/** Returns the set of active milestone tag names (Mordecai.Skill.<Name>.Rank<N>). */
	const TSet<FName>& GetActiveMilestoneTagNames() const { return ActiveMilestoneTagNames; }

	/** Returns true if a milestone ability has been granted for the given skill and rank. */
	bool HasGrantedMilestoneAbility(FName SkillName, int32 Rank) const;

	// --- Delegates ---

	/** Native C++ delegate — fires when any skill reaches a milestone rank. */
	FOnMordecaiSkillMilestoneReached OnSkillMilestoneReached;

	/** Blueprint delegate — fires when any skill reaches a milestone rank. */
	UPROPERTY(BlueprintAssignable, Category = "Mordecai|Skills")
	FOnMordecaiSkillMilestoneReachedBP OnSkillMilestoneReachedBP;

	// --- Replication ---

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Constants ---

	static constexpr int32 MaxSkillRank = 20;
	static constexpr int32 MinSkillRank = 0;

	/** Construct the milestone tag name for a given skill and rank. */
	static FName MakeSkillMilestoneTagName(FName SkillName, int32 Rank);

private:
	/** Update milestone state (tags, abilities, events) after a rank change. */
	void RefreshMilestoneState(FName SkillName, int32 OldRank, int32 NewRank);

	/** Find the ASC on the owning actor (PlayerState). */
	UAbilitySystemComponent* FindAbilitySystemComponent() const;

	/** Try to resolve a FGameplayTag for a milestone (returns Invalid if not pre-registered). */
	static FGameplayTag FindMilestoneTag(FName SkillName, int32 Rank);

	/** Find a rank entry in the replicated array, or return nullptr. */
	FMordecaiSkillRankEntry* FindRankEntry(FName SkillName);
	const FMordecaiSkillRankEntry* FindRankEntry(FName SkillName) const;

	/** Set a rank in the replicated array (adds entry if absent). */
	void SetRankInternal(FName SkillName, int32 NewRank);

	// --- Replicated State ---

	UPROPERTY(Replicated)
	TArray<FMordecaiSkillRankEntry> SkillRanks;

	UPROPERTY(Replicated)
	int32 AvailableSkillPoints = 0;

	// --- Non-Replicated (derived from SkillRanks on each side) ---

	UPROPERTY()
	TMap<FName, TObjectPtr<UMordecaiSkillDataAsset>> RegisteredSkills;

	/** Active milestone tag names — always tracked (FName-based, no tag registration needed). */
	TSet<FName> ActiveMilestoneTagNames;

	/** Tracks which milestone ranks have had their ability granted per skill. */
	TMap<FName, TSet<int32>> GrantedMilestoneAbilityRanks;

	/** Ability spec handles for granted milestone abilities (for removal). */
	TMap<FName, TMap<int32, FGameplayAbilitySpecHandle>> MilestoneAbilityHandles;
};
