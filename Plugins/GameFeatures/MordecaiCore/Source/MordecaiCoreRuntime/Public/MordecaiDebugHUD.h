// Copyright Project Mordecai. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"

#include "MordecaiDebugHUD.generated.h"

class UTextBlock;

/**
 * UMordecaiDebugHUD
 *
 * Simple debug overlay widget that displays Health%, Stamina, and SpellPoints
 * read from UMordecaiAttributeSet on the owning player's ASC.
 *
 * This widget can be created in Blueprint (subclass this, add a UMG layout with
 * TextBlocks named "HealthText", "StaminaText", "SpellPointsText", "PostureText")
 * or used as a C++ base that draws via NativeTick + PrintString.
 */
UCLASS(Abstract, Blueprintable)
class UMordecaiDebugHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	UMordecaiDebugHUD(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Updates text blocks if they are bound. Override in Blueprint for custom layout. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Debug")
	void RefreshAttributeDisplay();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Debug")
	float CurrentHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Debug")
	float CurrentMaxHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Debug")
	float CurrentStamina;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Debug")
	float CurrentMaxStamina;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Debug")
	float CurrentSpellPoints;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Debug")
	float CurrentMaxSpellPoints;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Debug")
	float CurrentPosture;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Debug")
	float CurrentMaxPosture;

	/** How often to refresh (seconds). 0 = every tick. */
	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Debug")
	float RefreshInterval;

private:
	float TimeSinceLastRefresh;
};
