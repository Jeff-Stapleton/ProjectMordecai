// Project Mordecai — Combat HUD Root Widget (US-052)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "MordecaiCombatHUDWidget.generated.h"

class UAbilitySystemComponent;
class UMordecaiHealthBarWidget;
class UMordecaiStaminaBarWidget;
class UMordecaiPostureBarWidget;

/**
 * UMordecaiCombatHUDWidget
 *
 * Root HUD container that hosts all combat UI elements:
 * Health bar, Stamina bar, and Posture bar.
 *
 * Call BindToASC() to wire all child widgets to the player's ASC.
 * The UMG Blueprint (US-054) will lay out the children in this container.
 */
UCLASS()
class LYRAGAME_API UMordecaiCombatHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Bind all child combat widgets to an AbilitySystemComponent. (AC-052.5) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void BindToASC(UAbilitySystemComponent* ASC);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMordecaiHealthBarWidget> HealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMordecaiStaminaBarWidget> StaminaBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMordecaiPostureBarWidget> PostureBar;
};
