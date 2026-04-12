// Project Mordecai — Enemy Indicator Widget (US-064)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "UI/IndicatorSystem/IActorIndicatorWidget.h"

#include "MordecaiEnemyIndicatorWidget.generated.h"

class UAbilitySystemComponent;
class UIndicatorDescriptor;
class UProgressBar;
class UTextBlock;
class UHorizontalBox;
class UImage;
class UTexture2D;
struct FOnAttributeChangeData;

/**
 * UMordecaiEnemyIndicatorWidget
 *
 * World-space enemy indicator that renders via Lyra's IndicatorSystem.
 * Shows: enemy name, health bar (color-coded), and active status effect icons.
 *
 * Implements IIndicatorWidgetInterface to receive BindIndicator/UnbindIndicator
 * calls from the SActorCanvas when this widget is created/destroyed.
 *
 * See: US-064, AC-064.5 through AC-064.8
 */
UCLASS()
class LYRAGAME_API UMordecaiEnemyIndicatorWidget : public UUserWidget, public IIndicatorWidgetInterface
{
	GENERATED_BODY()

public:
	// --- IIndicatorWidgetInterface ---

	virtual void BindIndicator_Implementation(UIndicatorDescriptor* Indicator) override;
	virtual void UnbindIndicator_Implementation(const UIndicatorDescriptor* Indicator) override;

	// --- Health Bar Color (AC-064.8) ---

	/** Calculate health bar color from health percentage.
	 *  Green >50%, Yellow 25%-50%, Red <25%. Static for testability. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Indicator")
	static FLinearColor CalcHealthBarColor(float HealthPercent);

	/** Configurable color thresholds. */
	static const FLinearColor HealthColorGreen;
	static const FLinearColor HealthColorYellow;
	static const FLinearColor HealthColorRed;

	// --- Status Icon Configuration (AC-064.7) ---

	/** Maps status tags to icon textures. Missing textures show a fallback placeholder. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mordecai|Indicator")
	TMap<FGameplayTag, TSoftObjectPtr<UTexture2D>> StatusIconMap;

	/** Enemy display name shown above health bar (AC-064.5). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mordecai|Indicator")
	FText EnemyDisplayName;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthProgressBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> StatusIconContainer;

private:
	void BuildDefaultContent();
	void BindToASC(UAbilitySystemComponent* ASC);
	void UnbindFromASC();

	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnMaxHealthChanged(const FOnAttributeChangeData& Data);
	void OnStatusTagChanged(const FGameplayTag Tag, int32 NewCount);

	void UpdateHealthDisplay();
	void AddStatusIcon(const FGameplayTag& StatusTag);
	void RemoveStatusIcon(const FGameplayTag& StatusTag);

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	float CachedHealth = 100.f;
	float CachedMaxHealth = 100.f;

	/** Active status icons keyed by tag. */
	UPROPERTY()
	TMap<FGameplayTag, TObjectPtr<UImage>> ActiveStatusIcons;
};
