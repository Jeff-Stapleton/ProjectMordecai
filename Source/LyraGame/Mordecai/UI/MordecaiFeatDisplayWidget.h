// Project Mordecai — Feat Display Widget (US-068)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Mordecai/Feats/MordecaiFeatTypes.h"

#include "MordecaiFeatDisplayWidget.generated.h"

class UMordecaiFeatComponent;
class UMordecaiPauseMenuWidget;
class UMordecaiSkillComponent;

/**
 * FMordecaiFeatConditionProgress
 *
 * Progress snapshot for a single unlock condition on a locked feat.
 * Surfaces current value, threshold, and met/unmet state so the UI can
 * render progress bars or "X / Y" text per condition.
 */
USTRUCT(BlueprintType)
struct FMordecaiFeatConditionProgress
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|FeatDisplay")
	EMordecaiFeatConditionType ConditionType = EMordecaiFeatConditionType::StatThreshold;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|FeatDisplay")
	FName StatName;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|FeatDisplay")
	FGameplayTag RequiredTag;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|FeatDisplay")
	int32 CurrentValue = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|FeatDisplay")
	int32 ThresholdValue = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|FeatDisplay")
	bool bMet = false;
};

/**
 * FMordecaiFeatDisplayRow
 *
 * Flattened data for a single feat, used by both the unlocked list and
 * locked list. For unlocked feats, AppliedRecord is populated and
 * ConditionProgress is empty. For locked feats, ConditionProgress is
 * populated and Description is replaced with teaser text.
 */
USTRUCT(BlueprintType)
struct FMordecaiFeatDisplayRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|FeatDisplay")
	FName FeatName;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|FeatDisplay")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|FeatDisplay")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|FeatDisplay")
	EMordecaiFeatTier Tier = EMordecaiFeatTier::Common;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|FeatDisplay")
	bool bUnlocked = false;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|FeatDisplay")
	FMordecaiFeatAppliedRecord AppliedRecord;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|FeatDisplay")
	TArray<FMordecaiFeatConditionProgress> ConditionProgress;
};

/**
 * FMordecaiFeatDisplaySummary
 *
 * Header counts: total unlocked and per-tier breakdown.
 */
USTRUCT(BlueprintType)
struct FMordecaiFeatDisplaySummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|FeatDisplay")
	int32 TotalUnlocked = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|FeatDisplay")
	int32 CommonCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|FeatDisplay")
	int32 RareCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|FeatDisplay")
	int32 LegendaryCount = 0;
};

/**
 * UMordecaiFeatDisplayWidget
 *
 * Content widget for the Pause Menu "Feats" tab (US-069). Binds to a
 * UMordecaiFeatComponent (attached to the local PlayerState per US-012)
 * and surfaces:
 *   - Unlocked feats with display name, tier badge, and description excerpt
 *   - Locked feats with display name, tier, teaser description, and
 *     per-condition unlock progress
 *   - Detail panel for a selected unlocked feat (full description + applied
 *     record counts)
 *   - Summary header with total and per-tier unlocked counts
 *   - Tier color-coding (Common=Gray, Rare=Blue, Legendary=Gold)
 *
 * Live updates arrive via the FeatComponent's OnFeatUnlockedBP delegate.
 * Visual layout is intentionally C++-minimal; Blueprint polish is EDITOR
 * work (Phase 4). Pure data logic is independently testable via NullRHI.
 */
UCLASS()
class LYRAGAME_API UMordecaiFeatDisplayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- Tab registration ---

	/** Tab id used when registering with UMordecaiPauseMenuWidget. */
	static FName GetFeatsTabId() { return FName(TEXT("feats")); }

	/** Default display name for the Feats tab. */
	static FText GetFeatsTabDisplayName();

	/** Register this widget class under the "feats" tab on the given pause menu. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|FeatDisplay")
	static void RegisterWithPauseMenu(UMordecaiPauseMenuWidget* PauseMenu, TSubclassOf<UMordecaiFeatDisplayWidget> WidgetClass = nullptr);

	// --- Component binding ---

	/** Subscribe to the component's OnFeatUnlockedBP delegate and read initial state. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|FeatDisplay")
	void BindToFeatComponent(UMordecaiFeatComponent* InComponent);

	/** Unsubscribe and forget the bound component. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|FeatDisplay")
	void UnbindFromFeatComponent();

	/** True if currently subscribed to a live component. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|FeatDisplay")
	bool IsBoundToComponent() const { return BoundComponent.IsValid(); }

	// --- Unlocked feats ---

	/** Returns rows for all unlocked feats (display name, tier, short description). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|FeatDisplay")
	TArray<FMordecaiFeatDisplayRow> GetUnlockedFeatRows() const;

	// --- Locked feats ---

	/** Returns rows for all locked (not-yet-unlocked) feats with progress. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|FeatDisplay")
	TArray<FMordecaiFeatDisplayRow> GetLockedFeatRows() const;

	// --- Summary ---

	/** Returns total and per-tier unlocked feat counts. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|FeatDisplay")
	FMordecaiFeatDisplaySummary GetSummary() const;

	// --- Detail panel ---

	/** Set which feat the detail panel describes. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|FeatDisplay")
	void SetSelectedFeat(FName FeatName);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|FeatDisplay")
	FName GetSelectedFeat() const { return SelectedFeat; }

	/** Full detail for the selected feat (description + applied record for unlocked feats). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|FeatDisplay")
	FMordecaiFeatDisplayRow GetSelectedFeatDetail() const;

	// --- Tier colors ---

	/** Get the display color for a tier. Reads from the configurable TierColors map. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|FeatDisplay")
	FLinearColor GetTierColor(EMordecaiFeatTier Tier) const;

	// --- Live update introspection (for tests) ---

	/** Returns the name of the last feat unlocked via the delegate. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|FeatDisplay")
	FName GetLastUnlockedFeatName() const { return LastUnlockedFeatName; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	/** Tier color map — designer-overridable via Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Mordecai|FeatDisplay")
	TMap<EMordecaiFeatTier, FLinearColor> TierColors;

private:
	UFUNCTION()
	void HandleFeatUnlocked(FName FeatName);

	/** Build condition progress array for a locked feat. */
	TArray<FMordecaiFeatConditionProgress> BuildConditionProgress(const TArray<FMordecaiFeatCondition>& Conditions) const;

	/** Find the SkillComponent for SkillRank condition queries. */
	UMordecaiSkillComponent* FindSkillComponent() const;

	TWeakObjectPtr<UMordecaiFeatComponent> BoundComponent;
	FName SelectedFeat = NAME_None;
	FName LastUnlockedFeatName = NAME_None;
};
