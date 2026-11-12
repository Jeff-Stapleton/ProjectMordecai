// Project Mordecai — Posture Bar Widget (US-052)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"

#include "MordecaiPostureBarWidget.generated.h"

class UAbilitySystemComponent;
class UMaterialInstanceDynamic;
class UProgressBar;
class UMordecaiUIThemeSubsystem;
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

	/** Deterministically advances the post-refill row-hide delay. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void AdvanceVisuals(float DeltaTime);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

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
	void BuildDefaultContent();
	void OnPostureChanged(const FOnAttributeChangeData& Data);
	void OnMaxPostureChanged(const FOnAttributeChangeData& Data);
	void OnPostureBrokenTagChanged(const FGameplayTag Tag, int32 NewCount);
	void UpdateDisplay();
	void ApplyTheme(EMordecaiUITheme Theme, const FMordecaiUIThemeColors& Colors);

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	TWeakObjectPtr<UMordecaiUIThemeSubsystem> BoundThemeSubsystem;
	FDelegateHandle ThemeChangedHandle;
	EMordecaiUITheme CurrentTheme = EMordecaiUITheme::Diorama;
	FMordecaiUIThemeColors CurrentThemeColors = FMordecaiUIThemeColors::MakeDiorama();
	float FullPostureDelayRemaining = 0.f;
	bool bWaitingToHide = false;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BrokenPatternMaterial;
};
