// Project Mordecai — Spell Points Bar Widget (US-055)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "MordecaiSpellPointsBarWidget.generated.h"

class UAbilitySystemComponent;
class UProgressBar;
class UTextBlock;
struct FOnAttributeChangeData;

/**
 * UMordecaiSpellPointsBarWidget
 *
 * Displays SpellPoints as a percentage fill bar AND numeric "Current / Max" text,
 * since SP is a discrete integer resource (unlike Health percentage).
 *
 * Follows the same pattern as UMordecaiHealthBarWidget: BindToASC, attribute
 * change delegates, cached values, and pure static helpers for testing.
 */
UCLASS()
class LYRAGAME_API UMordecaiSpellPointsBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Calculate SP percentage. Returns 0 if MaxSP is 0. (AC-055.1) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	static float CalcSpellPointsPercent(float SpellPoints, float MaxSpellPoints);

	/** Format SP as "Current / Max" integer string. (AC-055.3) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	static FString FormatSpellPoints(float SpellPoints, float MaxSpellPoints);

	/** Get the current SP percentage from cached values. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	float GetSpellPointsPercent() const;

	/** Bind to an AbilitySystemComponent to receive attribute change updates. (AC-055.2) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void BindToASC(UAbilitySystemComponent* ASC);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> SpellPointsProgressBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SpellPointsText;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	float CachedSpellPoints = 10.f;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	float CachedMaxSpellPoints = 10.f;

private:
	void OnSpellPointsChanged(const FOnAttributeChangeData& Data);
	void OnMaxSpellPointsChanged(const FOnAttributeChangeData& Data);
	void UpdateDisplay();

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
};
