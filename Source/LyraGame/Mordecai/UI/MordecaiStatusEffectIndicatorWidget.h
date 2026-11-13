// Project Mordecai — Status Effect Indicator Widget (US-055)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"

#include "MordecaiStatusEffectIndicatorWidget.generated.h"

class UTextBlock;
class UImage;
class UHorizontalBox;
class UBorder;
class USizeBox;
class UMordecaiUIThemeSubsystem;

/**
 * UMordecaiStatusEffectIndicatorWidget
 *
 * Displays a single active status effect: tag display name, remaining duration
 * countdown, and a colored tint (green for buff, red for debuff).
 *
 * Created dynamically by UMordecaiStatusEffectBarWidget when a status tag is
 * added to the ASC, and destroyed when the tag is removed. (AC-055.4)
 */
UCLASS()
class LYRAGAME_API UMordecaiStatusEffectIndicatorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Initialize with the status tag and whether it is a buff. (AC-055.4) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void InitializeIndicator(FGameplayTag InStatusTag, bool bIsBuff, float InDuration);

	/** Update the remaining duration display. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void UpdateDuration(float RemainingSeconds);

	/** Update reached stack pips without rebuilding their brushes every refresh. */
	void SetStackCount(int32 StackCount);

	/** All chips switch between full and compact together. */
	void SetCompact(bool bInCompact);

	/** Deterministic desired width used by the two-row bar layout. */
	float GetChipWidth(bool bCompact) const;

	/** Get the status tag this indicator represents. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	FGameplayTag GetStatusTag() const { return StatusTag; }

	/** Calculate duration remaining from timestamps. Static for testing. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	static float CalcDurationRemaining(float StartTime, float Duration, float CurrentTime);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DurationText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TintImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> StatusGlyph;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> StackPips;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	FGameplayTag StatusTag;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	float TotalDuration = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	bool bIsBuffIndicator = false;

private:
	void BuildDefaultContent();
	void ApplyTheme(EMordecaiUITheme Theme, const FMordecaiUIThemeColors& Colors);
	void RebuildPips();
	void RefreshChipWidth();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> BuffTab;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> DebuffTeeth;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> ChipSizeBox;

	TWeakObjectPtr<UMordecaiUIThemeSubsystem> BoundThemeSubsystem;
	FDelegateHandle ThemeChangedHandle;
	FMordecaiUIThemeColors CurrentThemeColors = FMordecaiUIThemeColors::MakeDiorama();
	FName GlyphColorToken = TEXT("text");
	FText FixedValueText;
	int32 MaxVisibleStacks = 0;
	int32 CurrentStackCount = 0;
	int32 LastDurationTenths = INDEX_NONE;
	float DurationBoxWidth = 38.f;
	float FullChipWidth = 132.f;
	float CompactChipWidth = 86.f;
	bool bCompact = false;
};
