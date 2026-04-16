// Project Mordecai — Equipped Weapon Widget (US-077)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Mordecai/Weapons/MordecaiWeaponTypes.h"

#include "MordecaiEquippedWeaponWidget.generated.h"

class UMordecaiEquipmentComponent;
class UMordecaiWeaponDataAsset;

/**
 * UMordecaiEquippedWeaponWidget
 *
 * Minimal HUD widget that displays the currently equipped weapon's name and
 * type. Binds to a UMordecaiEquipmentComponent via OnWeaponChanged delegate.
 * Shows "Unarmed" when no weapon is equipped.
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
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleWeaponChanged(EMordecaiEquipSlot InSlot, const UMordecaiWeaponDataAsset* NewWeapon);

	void RefreshFromComponent();
	void SetDisplayFromWeapon(const UMordecaiWeaponDataAsset* Weapon);

	TWeakObjectPtr<UMordecaiEquipmentComponent> BoundComponent;

	FText DisplayedWeaponName;
	EMordecaiWeaponType DisplayedWeaponType = EMordecaiWeaponType::Unarmed;
};
