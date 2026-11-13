// Project Mordecai — Status Effect Bar Widget (US-055)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"

#include "MordecaiStatusEffectBarWidget.generated.h"

class UAbilitySystemComponent;
class UBorder;
class UMordecaiStatusEffectIndicatorWidget;
class UPanelWidget;
class USizeBox;
class UTextBlock;
class UMordecaiUIThemeSubsystem;

/**
 * UMordecaiStatusEffectBarWidget
 *
 * Manages a horizontal row of status effect indicator children (max 8).
 * Monitors the bound ASC for Mordecai.Status.* tag changes and dynamically
 * adds/removes indicator widgets. (AC-055.5, AC-055.6)
 *
 * Polarity comes from the approved status display-data source and is conveyed
 * by both shape and live-theme colour. (AC-055.7)
 */
UCLASS()
class LYRAGAME_API UMordecaiStatusEffectBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Returns true if the given status tag is a buff (green indicator). (AC-055.7) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	static bool IsBuffTag(const FGameplayTag& StatusTag);

	/** Returns the tint color for a status tag: green for buffs, red for debuffs. (AC-055.7) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	static FLinearColor GetTintForStatusTag(const FGameplayTag& StatusTag);

	/** Bind to an ASC to monitor Mordecai.Status.* tag changes. (AC-055.6) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void BindToASC(UAbilitySystemComponent* ASC);

	/** Maximum number of visible status indicators. */
	static constexpr int32 MaxVisibleIndicators = 8;

	/** Widget class used to create individual indicator widgets. Set in Blueprint or CDO. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mordecai|HUD")
	TSubclassOf<UMordecaiStatusEffectIndicatorWidget> IndicatorWidgetClass;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> IndicatorContainer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OverflowChip;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> OverflowPlate;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> OverflowSize;

private:
	void BuildDefaultContent();
	void OnStatusTagChanged(const FGameplayTag Tag, int32 NewCount);
	void AddIndicator(const FGameplayTag& StatusTag);
	void RemoveIndicator(const FGameplayTag& StatusTag);
	void RebuildIndicators();
	void RefreshDurationsAndStacks();
	void ApplyTheme(EMordecaiUITheme Theme, const FMordecaiUIThemeColors& Colors);

	/** Active indicators keyed by their status tag. */
	UPROPERTY()
	TMap<FGameplayTag, TObjectPtr<UMordecaiStatusEffectIndicatorWidget>> ActiveIndicators;

	TArray<FGameplayTag> ActiveTagOrder;
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	TWeakObjectPtr<UMordecaiUIThemeSubsystem> BoundThemeSubsystem;
	FDelegateHandle ThemeChangedHandle;
	float DurationRefreshAccumulator = 0.f;
};
