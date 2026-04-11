// Project Mordecai — Spell Cooldown Widget (US-055)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"

#include "MordecaiSpellCooldownWidget.generated.h"

class UAbilitySystemComponent;
class UProgressBar;
class UTextBlock;

/**
 * UMordecaiSpellCooldownWidget
 *
 * Displays cooldown remaining for a single spell slot. Bound to a specific
 * cooldown gameplay tag. Shows remaining seconds as text and a linear fill
 * representing percentage of total cooldown elapsed. (AC-055.8, AC-055.9)
 *
 * When the cooldown tag is not present on the ASC, shows "ready" state.
 */
UCLASS()
class LYRAGAME_API UMordecaiSpellCooldownWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Calculate cooldown fraction elapsed (0 = just started, 1 = finished/ready). (AC-055.8) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	static float CalcCooldownPercent(float ElapsedTime, float TotalDuration);

	/** Format remaining time as display string. Empty string = ready. (AC-055.8) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	static FString FormatRemainingTime(float RemainingSeconds);

	/** Bind to a specific cooldown tag on the ASC. (AC-055.9) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void BindToCooldownTag(UAbilitySystemComponent* ASC, FGameplayTag CooldownTag, float TotalCooldown);

	/** Check if the spell is currently on cooldown. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	bool IsOnCooldown() const { return bIsOnCooldown; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> CooldownProgressBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CooldownText;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	FGameplayTag BoundCooldownTag;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	float TotalCooldownDuration = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	float CooldownStartTime = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	bool bIsOnCooldown = false;

private:
	void BuildDefaultContent();
	void OnCooldownTagChanged(const FGameplayTag Tag, int32 NewCount);
	void UpdateDisplay();

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
};
