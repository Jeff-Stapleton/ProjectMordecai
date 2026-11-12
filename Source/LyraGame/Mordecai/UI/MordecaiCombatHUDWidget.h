// Project Mordecai — Combat HUD Root Widget (US-052, US-055, US-061)

#pragma once

#include "CoreMinimal.h"
#include "UI/LyraActivatableWidget.h"

#include "MordecaiCombatHUDWidget.generated.h"

class UAbilitySystemComponent;
class UUserWidget;
class UWidget;
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
class UMordecaiEquippedWeaponWidget;
class UMordecaiEquipmentComponent;
class UMordecaiPickupInteractionComponent;
class UMordecaiSpellDataAsset;
class UIndicatorLayer;
class UCanvasPanel;
class UOverlay;
struct FOnAttributeChangeData;

/**
 * UMordecaiCombatHUDWidget
 *
 * Activatable CommonUI root that hosts all combat UI elements:
 * Health bar, Stamina bar, Posture bar, Spell Points bar,
 * Status Effect bar, and Spell Cooldown widgets.
 *
 * Call BindToASC() to wire all child widgets to the player's ASC.
 * The UMG Blueprint will lay out the children in this container.
 */
UCLASS()
class LYRAGAME_API UMordecaiCombatHUDWidget : public ULyraActivatableWidget
{
	GENERATED_BODY()

public:
	/** A persistent HUD must keep Enhanced Input routed to gameplay while it is the active CommonUI leaf. */
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	/** Bind all child combat widgets to an AbilitySystemComponent. (AC-052.5, AC-055.10) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void BindToASC(UAbilitySystemComponent* ASC);

	/** Bind cooldown widgets A-D to spell loadout slots 0-3 after the abilities are granted. */
	void BindSpellCooldownSlots(
		UAbilitySystemComponent* ASC,
		const TArray<TObjectPtr<UMordecaiSpellDataAsset>>& SpellDataAssets);

	/** Bind the loadout plate to the local pawn's equipment state. */
	void BindToEquipmentComponent(UMordecaiEquipmentComponent* Equipment);

	/** Bind pickup/inventory assist prompts to the local pawn. */
	void BindToInteraction(UMordecaiPickupInteractionComponent* Interaction);

	/** Returns the number of non-null child widget slots (for smoke test verification). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	int32 GetBoundChildCount() const;

	/** Pickup/inventory assist prompts (US-079). May be null if a BP layout omits the slot. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|HUD")
	UMordecaiPickupPromptWidget* GetPickupPrompt() const { return PickupPrompt; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|HUD")
	UMordecaiEquippedWeaponWidget* GetEquippedWeaponWidget() const { return EquippedWeaponWidget; }

	/** Reflection-safe widget lookup used by PIE integration checks and designer diagnostics. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|HUD")
	UWidget* GetNamedWidget(UUserWidget* Owner, FName WidgetName) const;

	/** Applies the design system's 32px or 96px edge margin without moving centre lanes. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void SetHUDMargin(float InMargin);

	/** Boss/banner width that clears the left HUD column at the active margin. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|HUD")
	float GetTopCenterSafeWidth() const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Full-screen projection surface for Lyra IndicatorSystem descriptors (US-092). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UIndicatorLayer> IndicatorLayer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> GaugePlate;

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

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMordecaiSpellCooldownWidget> SpellCooldownC;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMordecaiSpellCooldownWidget> SpellCooldownD;

	// --- Combat Feedback (US-061) ---

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMordecaiComboCounterWidget> ComboCounter;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMordecaiCombatFeedbackWidget> CombatFeedback;

	// --- Kill Counter (US-063) ---

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMordecaiKillCounterWidget> KillCounter;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> TallyRailHead;

	// --- Pickup / Inventory Assist Prompts (US-079) ---

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMordecaiPickupPromptWidget> PickupPrompt;

	// --- Loadout Plate (US-091 / D20) ---

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMordecaiEquippedWeaponWidget> EquippedWeaponWidget;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> LowHealthFrame;

private:
	void BuildDefaultLayout();
	void BuildLowHealthFrame(UCanvasPanel* Root);
	void OnHUDHealthChanged(const FOnAttributeChangeData& Data);
	void OnHUDMaxHealthChanged(const FOnAttributeChangeData& Data);
	void UpdateLowHealthFrame();

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	TWeakObjectPtr<UMordecaiEquipmentComponent> BoundEquipment;
	TWeakObjectPtr<UMordecaiPickupInteractionComponent> BoundInteraction;
	float HUDMargin = 32.f;
	float CachedHUDHealth = 100.f;
	float CachedHUDMaxHealth = 100.f;
};
