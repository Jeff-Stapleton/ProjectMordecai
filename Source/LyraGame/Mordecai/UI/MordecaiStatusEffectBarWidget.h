// Project Mordecai — Status Effect Bar Widget (US-055)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"

#include "MordecaiStatusEffectBarWidget.generated.h"

class UAbilitySystemComponent;
class UMordecaiStatusEffectIndicatorWidget;
class UHorizontalBox;

/**
 * UMordecaiStatusEffectBarWidget
 *
 * Manages a horizontal row of status effect indicator children (max 8).
 * Monitors the bound ASC for Mordecai.Status.* tag changes and dynamically
 * adds/removes indicator widgets. (AC-055.5, AC-055.6)
 *
 * Buff vs debuff distinction: tags in the BuffTags set show green tint,
 * all other Mordecai.Status.* tags show red tint. (AC-055.7)
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

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> IndicatorContainer;

private:
	void BuildDefaultContent();
	void OnStatusTagChanged(const FGameplayTag Tag, int32 NewCount);
	void AddIndicator(const FGameplayTag& StatusTag);
	void RemoveIndicator(const FGameplayTag& StatusTag);

	/** Active indicators keyed by their status tag. */
	UPROPERTY()
	TMap<FGameplayTag, TObjectPtr<UMordecaiStatusEffectIndicatorWidget>> ActiveIndicators;

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	/** Static set of buff tags — all other Mordecai.Status.* are debuffs. */
	static const FGameplayTagContainer& GetBuffTagContainer();
};
