// Project Mordecai — Posture Bar Widget (US-052)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"

#include "MordecaiPostureBarWidget.generated.h"

class UAbilitySystemComponent;
class UProgressBar;
struct FOnAttributeChangeData;
struct FGameplayTag;

/**
 * UMordecaiPostureBarWidget
 *
 * Displays the player's posture meter as a progress bar (Posture/MaxPosture),
 * with visual distinction when the Mordecai.State.PostureBroken tag is active.
 *
 * Static helper CalcPosturePercent() is independently testable without a widget tree.
 */
UCLASS()
class LYRAGAME_API UMordecaiPostureBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Calculate posture percentage. Returns 0 if MaxPosture is 0. (AC-052.9, AC-052.11) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	static float CalcPosturePercent(float Posture, float MaxPosture);

	/** Get the current posture percentage from cached values. (AC-052.9) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	float GetPosturePercent() const;

	/** Whether the posture broken tag is currently active. (AC-052.4) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	bool IsPostureBroken() const;

	/** Bind to an AbilitySystemComponent to receive attribute change updates. (AC-052.5) */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void BindToASC(UAbilitySystemComponent* ASC);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> PostureProgressBar;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	float CachedPosture = 100.f;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	float CachedMaxPosture = 100.f;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	bool bPostureBroken = false;

	/** Color when posture is normal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mordecai|HUD|Colors")
	FLinearColor NormalPostureColor = FLinearColor(0.2f, 0.6f, 1.f);

	/** Color when posture is broken. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mordecai|HUD|Colors")
	FLinearColor BrokenPostureColor = FLinearColor(1.f, 0.2f, 0.2f);

private:
	void OnPostureChanged(const FOnAttributeChangeData& Data);
	void OnMaxPostureChanged(const FOnAttributeChangeData& Data);
	void OnPostureBrokenTagChanged(const FGameplayTag Tag, int32 NewCount);
	void UpdateDisplay();

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
};
