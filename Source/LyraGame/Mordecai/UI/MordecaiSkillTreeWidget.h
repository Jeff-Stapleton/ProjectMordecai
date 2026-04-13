// Project Mordecai — Skill Tree Widget (US-067)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Mordecai/Skills/MordecaiSkillTypes.h"

#include "MordecaiSkillTreeWidget.generated.h"

class UMordecaiPauseMenuWidget;
class UMordecaiSkillComponent;
class UMordecaiSkillDataAsset;

/**
 * FMordecaiSkillTreeRow
 *
 * One skill entry surfaced to the UI — flattens SkillName, current rank, max
 * rank (always 20), category, and a precomputed bCanAllocate hint so Blueprint
 * widgets can bind button enable state without re-querying the component.
 */
USTRUCT(BlueprintType)
struct FMordecaiSkillTreeRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|SkillTree")
	FName SkillName;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|SkillTree")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|SkillTree")
	int32 CurrentRank = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|SkillTree")
	int32 MaxRank = 20;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|SkillTree")
	EMordecaiSkillCategory Category = EMordecaiSkillCategory::Weapon;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|SkillTree")
	bool bCanAllocate = false;
};

/**
 * FMordecaiSkillTreeMilestoneRow
 *
 * One milestone entry in the detail panel for a selected skill. The five
 * milestone ranks (1, 5, 10, 15, 20) each get a row, ordered ascending.
 */
USTRUCT(BlueprintType)
struct FMordecaiSkillTreeMilestoneRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|SkillTree")
	int32 Rank = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|SkillTree")
	FText Description;

	/** True if the skill's current rank is at or above this milestone. */
	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|SkillTree")
	bool bReached = false;

	/** True if the milestone ability has actually been granted on the ASC. */
	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|SkillTree")
	bool bAbilityGranted = false;

	/** False when the registered DataAsset has no description at this rank. */
	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|SkillTree")
	bool bHasDescription = false;
};

/**
 * UMordecaiSkillTreeWidget
 *
 * Content widget for the Pause Menu "Skills" tab (US-069). Binds to a
 * UMordecaiSkillComponent (attached to the local PlayerState per US-011) and
 * surfaces:
 *   - Skills grouped by EMordecaiSkillCategory (5 categories), switchable via
 *     SetActiveCategory()
 *   - Current rank / max rank (20) per skill
 *   - Milestone detail panel for the selected skill (ranks 1/5/10/15/20)
 *     showing reached state and ability-grant state
 *   - Available skill points from the component
 *   - Allocation via RequestAllocateSkillPoint() with CanAllocate() hints
 *     for disabled UI state
 *
 * Visual layout is intentionally C++-minimal; Blueprint polish is EDITOR work
 * (Phase 4). The widget subscribes to UMordecaiSkillComponent's
 * OnSkillMilestoneReachedBP delegate for live updates.
 */
UCLASS()
class LYRAGAME_API UMordecaiSkillTreeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- Tab registration ---

	/** Tab id used when registering with UMordecaiPauseMenuWidget. */
	static FName GetSkillsTabId() { return FName(TEXT("skills")); }

	/** Default display name for the Skills tab. */
	static FText GetSkillsTabDisplayName();

	/** Register this widget class under the "skills" tab on the given pause menu. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|SkillTree")
	static void RegisterWithPauseMenu(UMordecaiPauseMenuWidget* PauseMenu, TSubclassOf<UMordecaiSkillTreeWidget> WidgetClass = nullptr);

	// --- Component binding ---

	/** Subscribe to the component's milestone delegate and prime local state. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|SkillTree")
	void BindToSkillComponent(UMordecaiSkillComponent* InComponent);

	/** Unsubscribe and forget the bound component. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|SkillTree")
	void UnbindFromSkillComponent();

	/** True if currently subscribed to a live component. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|SkillTree")
	bool IsBoundToComponent() const { return BoundComponent.IsValid(); }

	// --- Category ---

	/** Switch which category of skills GetVisibleSkills() returns. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|SkillTree")
	void SetActiveCategory(EMordecaiSkillCategory Category);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|SkillTree")
	EMordecaiSkillCategory GetActiveCategory() const { return ActiveCategory; }

	/** Skills in the active category with current ranks and allocation hints. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|SkillTree")
	TArray<FMordecaiSkillTreeRow> GetVisibleSkills() const;

	// --- Skill points ---

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|SkillTree")
	int32 GetAvailableSkillPoints() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|SkillTree")
	bool HasSkillPoints() const { return GetAvailableSkillPoints() > 0; }

	// --- Allocation ---

	/** True if the player can spend a point on this skill right now. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|SkillTree")
	bool CanAllocateSkillPoint(FName SkillName) const;

	/** Allocate a point to SkillName. Returns true on success. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|SkillTree")
	bool RequestAllocateSkillPoint(FName SkillName);

	// --- Detail panel ---

	/** Set which skill the detail panel describes. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|SkillTree")
	void SetSelectedSkill(FName SkillName);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|SkillTree")
	FName GetSelectedSkill() const { return SelectedSkill; }

	/** Milestone rows for the selected skill, sorted by rank ascending. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|SkillTree")
	TArray<FMordecaiSkillTreeMilestoneRow> GetMilestonesForSelectedSkill() const;

	// --- Milestone helpers / live-update introspection ---

	/** Returns the canonical milestone rank array {1, 5, 10, 15, 20}. */
	static const TArray<int32>& GetMilestoneRanks();

	/** True if Rank is a milestone (1, 5, 10, 15, 20). */
	static bool IsMilestoneRank(int32 Rank);

	/** Skill whose milestone delegate most recently fired (tests + BP). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|SkillTree")
	FName GetLastObservedMilestoneSkill() const { return LastObservedMilestoneSkill; }

	/** Rank of the last-observed milestone event (0 = none observed yet). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|SkillTree")
	int32 GetLastObservedMilestoneRank() const { return LastObservedMilestoneRank; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleMilestoneReached(FName SkillName, int32 MilestoneRank);

	/** Resolve the DataAsset for SkillName from the bound component (may be null). */
	UMordecaiSkillDataAsset* FindSkillDataAsset(FName SkillName) const;

	TWeakObjectPtr<UMordecaiSkillComponent> BoundComponent;

	EMordecaiSkillCategory ActiveCategory = EMordecaiSkillCategory::Weapon;
	FName SelectedSkill = NAME_None;

	FName LastObservedMilestoneSkill = NAME_None;
	int32 LastObservedMilestoneRank = 0;
};
