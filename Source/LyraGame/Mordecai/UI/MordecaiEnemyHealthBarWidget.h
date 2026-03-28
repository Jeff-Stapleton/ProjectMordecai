// Project Mordecai — Enemy Health Bar Widget (US-052)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "MordecaiEnemyHealthBarWidget.generated.h"

class UAbilitySystemComponent;
class UProgressBar;
struct FOnAttributeChangeData;

/**
 * UMordecaiEnemyHealthBarWidget
 *
 * World-space widget for displaying enemy health above their head.
 * Bound to the enemy's ASC via BindToASC(). Shows health as a simple
 * progress bar (Health/MaxHealth). Will be attached as a UWidgetComponent
 * on enemy Blueprints in US-054.
 */
UCLASS()
class LYRAGAME_API UMordecaiEnemyHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Get the current health percentage from cached values. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	float GetHealthPercent() const;

	/** Bind to an AbilitySystemComponent to receive attribute change updates. (AC-052.5) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void BindToASC(UAbilitySystemComponent* ASC);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthProgressBar;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	float CachedHealth = 100.f;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	float CachedMaxHealth = 100.f;

private:
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnMaxHealthChanged(const FOnAttributeChangeData& Data);
	void UpdateDisplay();

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
};
