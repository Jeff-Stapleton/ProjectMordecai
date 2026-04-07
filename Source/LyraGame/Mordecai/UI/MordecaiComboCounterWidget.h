// Project Mordecai — Combo Counter Widget (US-061)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"

#include "MordecaiComboCounterWidget.generated.h"

class UAbilitySystemComponent;
class UTextBlock;

/**
 * UMordecaiComboCounterWidget
 *
 * Shows the current combo hit number (Hit 1, Hit 2, Hit 3) during melee combos.
 * Listens for Mordecai.State.ComboWindow tag and queries the melee attack ability
 * for the current combo index. Auto-fades when the combo window closes.
 */
UCLASS()
class LYRAGAME_API UMordecaiComboCounterWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Format combo index (0-based) as display text. Returns empty for invalid index. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	static FString FormatComboHit(int32 ComboIndex);

	/** Bind to ASC to listen for combo window tag changes. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void BindToASC(UAbilitySystemComponent* ASC);

	/** Manually set the displayed combo index (called by melee ability or controller). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void SetComboIndex(int32 ComboIndex);

	/** Show or hide the combo counter. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void SetComboVisible(bool bVisible);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ComboText;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	int32 CachedComboIndex = -1;

private:
	void OnComboWindowTagChanged(const FGameplayTag Tag, int32 NewCount);
	void UpdateDisplay();

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
};
