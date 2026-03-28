// Project Mordecai — Stamina Bar Widget (US-052)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Mordecai/Combat/MordecaiStaminaSystem.h"

#include "MordecaiStaminaBarWidget.generated.h"

class UAbilitySystemComponent;
class UProgressBar;
struct FOnAttributeChangeData;

/**
 * UMordecaiStaminaBarWidget
 *
 * Displays the stamina bar fill amount AND the current stamina tier as a color
 * indicator (Green/Yellow/Red/Exhausted) using tier threshold logic.
 *
 * Static helpers CalcStaminaPercent() and CalcStaminaTier() are independently
 * testable without a widget tree.
 */
UCLASS()
class LYRAGAME_API UMordecaiStaminaBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Calculate stamina percentage. Returns 0 if MaxStamina is 0. (AC-052.7, AC-052.11) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	static float CalcStaminaPercent(float Stamina, float MaxStamina);

	/** Calculate stamina tier from ratio. Green >= 66%, Yellow >= 33%, Red > 0%, Exhausted <= 0. (AC-052.8, AC-052.11) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	static EMordecaiStaminaTier CalcStaminaTier(float Stamina, float MaxStamina);

	/** Get the current stamina percentage from cached values. (AC-052.7) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	float GetStaminaPercent() const;

	/** Get the current stamina tier from cached values. (AC-052.8) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	EMordecaiStaminaTier GetStaminaTier() const;

	/** Bind to an AbilitySystemComponent to receive attribute change updates. (AC-052.5) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void BindToASC(UAbilitySystemComponent* ASC);

	/** Tier color defaults — configurable in editor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mordecai|HUD|Colors")
	FLinearColor GreenTierColor = FLinearColor(0.f, 1.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mordecai|HUD|Colors")
	FLinearColor YellowTierColor = FLinearColor(1.f, 1.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mordecai|HUD|Colors")
	FLinearColor RedTierColor = FLinearColor(1.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mordecai|HUD|Colors")
	FLinearColor ExhaustedTierColor = FLinearColor(0.5f, 0.f, 0.f);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> StaminaProgressBar;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	float CachedStamina = 100.f;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	float CachedMaxStamina = 100.f;

private:
	void OnStaminaChanged(const FOnAttributeChangeData& Data);
	void OnMaxStaminaChanged(const FOnAttributeChangeData& Data);
	void UpdateDisplay();

	FLinearColor GetColorForTier(EMordecaiStaminaTier Tier) const;

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
};
