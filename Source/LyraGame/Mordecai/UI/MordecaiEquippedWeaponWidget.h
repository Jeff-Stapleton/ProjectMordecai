// Project Mordecai — Equipped Weapon Widget (US-077)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Mordecai/Weapons/MordecaiWeaponTypes.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"

#include "MordecaiEquippedWeaponWidget.generated.h"

class UMordecaiEquipmentComponent;
class UMordecaiWeaponDataAsset;
class UImage;
class UTextBlock;
class UMordecaiUIThemeSubsystem;

/**
 * UMordecaiEquippedWeaponWidget
 *
 * Tin Tally loadout plate: weapon name, physical damage subtype, and swap key.
 * Binds to UMordecaiEquipmentComponent::OnWeaponChanged and shows "Unarmed" /
 * "Blunt" when no weapon is equipped.
 */
UCLASS()
class LYRAGAME_API UMordecaiEquippedWeaponWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Subscribe to the component's OnWeaponChanged delegate and prime display. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Equipment")
	void BindToEquipmentComponent(UMordecaiEquipmentComponent* Comp);

	/** Unsubscribe and forget the bound component. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Equipment")
	void UnbindFromEquipmentComponent();

	/** Current displayed weapon name (for tests + BP readback). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Equipment")
	FText GetDisplayedWeaponName() const { return DisplayedWeaponName; }

	/** Current displayed weapon type (for tests + BP readback). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Equipment")
	EMordecaiWeaponType GetDisplayedWeaponType() const { return DisplayedWeaponType; }

	/** Drives the real shipped loadout plate for gallery fixtures. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Equipment")
	void SetGalleryPreviewWeapon(const UMordecaiWeaponDataAsset* Weapon);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeaponNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> WeaponIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DamageSubtypeText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ElementIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SwapKey;

private:
	UFUNCTION()
	void HandleWeaponChanged(EMordecaiEquipSlot InSlot, const UMordecaiWeaponDataAsset* NewWeapon);

	void BuildDefaultContent();
	void RefreshFromComponent();
	void SetDisplayFromWeapon(const UMordecaiWeaponDataAsset* Weapon);
	void UpdateDisplay();
	void ApplyTheme(EMordecaiUITheme Theme, const FMordecaiUIThemeColors& Colors);
	static FText ResolveDamageSubtype(const UMordecaiWeaponDataAsset* Weapon);

	TWeakObjectPtr<UMordecaiEquipmentComponent> BoundComponent;
	TWeakObjectPtr<UMordecaiUIThemeSubsystem> BoundThemeSubsystem;
	FDelegateHandle ThemeChangedHandle;
	EMordecaiUITheme CurrentTheme = EMordecaiUITheme::Diorama;
	FMordecaiUIThemeColors CurrentThemeColors = FMordecaiUIThemeColors::MakeDiorama();

	FText DisplayedWeaponName;
	FText DisplayedDamageSubtype;
	EMordecaiWeaponType DisplayedWeaponType = EMordecaiWeaponType::Unarmed;
};
