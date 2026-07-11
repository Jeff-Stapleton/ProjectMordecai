// Project Mordecai — Combat HUD Root Widget (US-052, US-055, US-061)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "MordecaiCombatHUDWidget.generated.h"

class UAbilitySystemComponent;
class UMordecaiHealthBarWidget;
class UMordecaiStaminaBarWidget;
class UMordecaiPostureBarWidget;
class UMordecaiSpellPointsBarWidget;
class UMordecaiStatusEffectBarWidget;
class UMordecaiSpellCooldownWidget;
class UMordecaiComboCounterWidget;
class UMordecaiCombatFeedbackWidget;
class UMordecaiKillCounterWidget;
class UMordecaiPickupPromptWidget;

/**
 * UMordecaiCombatHUDWidget
 *
 * Root HUD container that hosts all combat UI elements:
 * Health bar, Stamina bar, Posture bar, Spell Points bar,
 * Status Effect bar, and Spell Cooldown widgets.
 *
 * Call BindToASC() to wire all child widgets to the player's ASC.
 * The UMG Blueprint will lay out the children in this container.
 */
UCLASS()
class LYRAGAME_API UMordecaiCombatHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Bind all child combat widgets to an AbilitySystemComponent. (AC-052.5, AC-055.10) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void BindToASC(UAbilitySystemComponent* ASC);

	/** Returns the number of non-null child widget slots (for smoke test verification). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	int32 GetBoundChildCount() const;

	/** Pickup/inventory assist prompts (US-079). May be null if a BP layout omits the slot. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|HUD")
	UMordecaiPickupPromptWidget* GetPickupPrompt() const { return PickupPrompt; }

protected:
	virtual void NativeOnInitialized() override;

	// --- Core Resource Bars (US-052) ---

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMordecaiHealthBarWidget> HealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMordecaiStaminaBarWidget> StaminaBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMordecaiPostureBarWidget> PostureBar;

	// --- Spell HUD (US-055) ---

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMordecaiSpellPointsBarWidget> SpellPointsBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMordecaiStatusEffectBarWidget> StatusEffectBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMordecaiSpellCooldownWidget> SpellCooldownA;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMordecaiSpellCooldownWidget> SpellCooldownB;

	// --- Combat Feedback (US-061) ---

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMordecaiComboCounterWidget> ComboCounter;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMordecaiCombatFeedbackWidget> CombatFeedback;

	// --- Kill Counter (US-063) ---

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMordecaiKillCounterWidget> KillCounter;

	// --- Pickup / Inventory Assist Prompts (US-079) ---

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMordecaiPickupPromptWidget> PickupPrompt;

private:
	void BuildDefaultLayout();
};
