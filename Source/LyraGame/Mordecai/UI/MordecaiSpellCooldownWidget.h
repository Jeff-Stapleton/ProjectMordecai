// Project Mordecai — Spell Cooldown Widget (US-055)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"

#include "MordecaiSpellCooldownWidget.generated.h"

class UAbilitySystemComponent;
class UHorizontalBox;
class UImage;
class UMaterialInstanceDynamic;
class UOverlay;
class UProgressBar;
class UTextBlock;
class UBorder;
class UMordecaiSpellDataAsset;
class UMordecaiUIThemeSubsystem;

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

	/** Set the loadout key shown for this slot (Q / E / R / 1). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void SetSlotKeyLabel(const FText& Label);

	/** Bind the shipped glyph and SP-cost rivets for one spell. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void ConfigureFromSpellData(UMordecaiSpellDataAsset* SpellData);

	/** Gallery/test preview without bypassing the live widget presentation. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void SetPreviewCooldownFraction(float Fraction);

	/** Check if the spell is currently on cooldown. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	bool IsOnCooldown() const { return bIsOnCooldown; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> CooldownProgressBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CooldownText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotKeyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> SpellIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> SlotKey;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> CostRivets;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> LockChip;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> MuzzleBadge;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CooldownShutterLip;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	FGameplayTag BoundCooldownTag;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	float TotalCooldownDuration = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	float CooldownStartTime = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	bool bIsOnCooldown = false;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|HUD")
	FText SlotKeyLabel;

private:
	void BuildDefaultContent();
	void UnbindFromCooldownTag();
	void OnCooldownTagChanged(const FGameplayTag Tag, int32 NewCount);
	void UpdateDisplay();
	void ApplyCooldownFraction(float Fraction);
	void ApplyTheme(EMordecaiUITheme Theme, const FMordecaiUIThemeColors& Colors);

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	TWeakObjectPtr<UMordecaiUIThemeSubsystem> BoundThemeSubsystem;
	FDelegateHandle CooldownTagChangedHandle;
	FDelegateHandle ThemeChangedHandle;
	EMordecaiUITheme CurrentTheme = EMordecaiUITheme::Diorama;
	FMordecaiUIThemeColors CurrentThemeColors = FMordecaiUIThemeColors::MakeDiorama();
	FName ConfiguredSpellGlyph;
	float PreviewCooldownFraction = 0.f;
	bool bPreviewCooldownHeld = false;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ShutterMaterial;
};
