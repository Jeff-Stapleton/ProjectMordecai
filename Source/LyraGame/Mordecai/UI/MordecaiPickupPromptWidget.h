// Project Mordecai — Pickup Prompt Widget (US-079)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "MordecaiPickupPromptWidget.generated.h"

class AMordecaiItemPickup;
class UMordecaiPickupInteractionComponent;
class UTextBlock;

/**
 * UMordecaiPickupPromptWidget
 *
 * Player-assist prompts for the inventory vertical slice (US-079):
 *   - "[F] Pick up <item>" while a pickup is focused (identification-aware
 *     name — unidentified items show partial info)
 *   - "[I] Inventory" persistent assist hint
 *
 * Binds to UMordecaiPickupInteractionComponent::OnFocusedPickupChanged; no
 * polling. Embedded in the combat HUD (BindWidgetOptional slot or built by
 * the programmatic fallback layout).
 *
 * Key labels are static keyboard glyphs — device-aware gamepad glyphs are out
 * of scope for the slice (see US-079 technical notes).
 */
UCLASS()
class LYRAGAME_API UMordecaiPickupPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- Prompt composition (pure, testable) ---

	/** "[F] Pick up <item name>" */
	static FText MakePromptText(const FText& ItemDisplayName);

	/** "[I] Inventory" — the persistent assist hint. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|PickupPrompt")
	static FText GetInventoryHintText();

	// --- Binding lifecycle ---

	UFUNCTION(BlueprintCallable, Category = "Mordecai|PickupPrompt")
	void BindToInteraction(UMordecaiPickupInteractionComponent* Interaction);

	UFUNCTION(BlueprintCallable, Category = "Mordecai|PickupPrompt")
	void Unbind();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|PickupPrompt")
	bool IsBoundToInteraction() const { return BoundInteraction.IsValid(); }

	// --- State ---

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|PickupPrompt")
	FText GetPromptText() const { return CachedPromptText; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|PickupPrompt")
	bool IsPromptVisible() const { return bPromptVisible; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	/** Prompt line ("[F] Pick up ..."), collapsed while nothing is focused. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PromptText;

	/** Persistent "[I] Inventory" hint. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InventoryHintText;

private:
	UFUNCTION()
	void HandleFocusedPickupChanged(AMordecaiItemPickup* FocusedPickup, const FText& ItemDisplayName);

	void BuildDefaultLayout();
	void RefreshVisuals();

	TWeakObjectPtr<UMordecaiPickupInteractionComponent> BoundInteraction;
	FText CachedPromptText;
	bool bPromptVisible = false;
};
