// Project Mordecai — Equipped Weapon Widget (US-077)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Mordecai/Weapons/MordecaiWeaponTypes.h"

#include "MordecaiEquippedWeaponWidget.generated.h"

class UMordecaiEquipmentComponent;
class UMordecaiWeaponDataAsset;
class UImage;
class UTextBlock;

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

protected:
	virtual void NativeOnInitialized() override;
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
	static FText ResolveDamageSubtype(const UMordecaiWeaponDataAsset* Weapon);

	TWeakObjectPtr<UMordecaiEquipmentComponent> BoundComponent;

	FText DisplayedWeaponName;
	FText DisplayedDamageSubtype;
	EMordecaiWeaponType DisplayedWeaponType = EMordecaiWeaponType::Unarmed;
};
