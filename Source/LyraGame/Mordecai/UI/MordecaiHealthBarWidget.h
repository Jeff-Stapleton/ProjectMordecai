// Project Mordecai — Health Bar Widget (US-052)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "MordecaiHealthBarWidget.generated.h"

class UAbilitySystemComponent;
class UProgressBar;
struct FOnAttributeChangeData;

/**
 * UMordecaiHealthBarWidget
 *
 * Displays Health as a percentage (0-100%) of MaxHealth using a progress bar
 * bound to the player's Health/MaxHealth attributes via GAS delegates.
 *
 * Static helper CalcHealthPercent() is independently testable without a widget tree.
 */
UCLASS()
class LYRAGAME_API UMordecaiHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Calculate health percentage. Returns 0 if MaxHealth is 0. (AC-052.6, AC-052.11) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	static float CalcHealthPercent(float Health, float MaxHealth);

	/** Get the current health percentage from cached values. (AC-052.6) */
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
