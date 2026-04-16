// Project Mordecai — Equipped Weapon Widget (US-077)

#include "Mordecai/UI/MordecaiEquippedWeaponWidget.h"
#include "Mordecai/Weapons/MordecaiEquipmentComponent.h"
#include "Mordecai/Weapons/MordecaiWeaponDataAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiEquippedWeaponWidget)

#define LOCTEXT_NAMESPACE "MordecaiEquippedWeapon"

void UMordecaiEquippedWeaponWidget::BindToEquipmentComponent(UMordecaiEquipmentComponent* Comp)
{
	UnbindFromEquipmentComponent();
	if (!Comp)
	{
		return;
	}

	BoundComponent = Comp;
	Comp->OnWeaponChanged.AddDynamic(this, &UMordecaiEquippedWeaponWidget::HandleWeaponChanged);
	RefreshFromComponent();
}

void UMordecaiEquippedWeaponWidget::UnbindFromEquipmentComponent()
{
	if (UMordecaiEquipmentComponent* Comp = BoundComponent.Get())
	{
		Comp->OnWeaponChanged.RemoveDynamic(this, &UMordecaiEquippedWeaponWidget::HandleWeaponChanged);
	}
	BoundComponent.Reset();
}

void UMordecaiEquippedWeaponWidget::NativeDestruct()
{
	UnbindFromEquipmentComponent();
	Super::NativeDestruct();
}

void UMordecaiEquippedWeaponWidget::HandleWeaponChanged(EMordecaiEquipSlot InSlot, const UMordecaiWeaponDataAsset* NewWeapon)
{
	// Only react to MainHand / TwoHand changes (the displayed weapon)
	if (InSlot != EMordecaiEquipSlot::MainHand && InSlot != EMordecaiEquipSlot::TwoHand)
	{
		return;
	}

	if (NewWeapon)
	{
		SetDisplayFromWeapon(NewWeapon);
	}
	else
	{
		// Unequipped — refresh from component in case another slot is still set
		RefreshFromComponent();
	}
}

void UMordecaiEquippedWeaponWidget::RefreshFromComponent()
{
	UMordecaiEquipmentComponent* Comp = BoundComponent.Get();
	if (!Comp)
	{
		SetDisplayFromWeapon(nullptr);
		return;
	}

	const FMordecaiWeaponInstance* Main = Comp->GetEquippedWeapon(EMordecaiEquipSlot::MainHand);
	SetDisplayFromWeapon(Main ? Main->WeaponDataAsset.Get() : nullptr);
}

void UMordecaiEquippedWeaponWidget::SetDisplayFromWeapon(const UMordecaiWeaponDataAsset* Weapon)
{
	if (Weapon)
	{
		DisplayedWeaponName = Weapon->DisplayName;
		DisplayedWeaponType = Weapon->WeaponType;
	}
	else
	{
		DisplayedWeaponName = LOCTEXT("UnarmedDisplay", "Unarmed");
		DisplayedWeaponType = EMordecaiWeaponType::Unarmed;
	}
}

#undef LOCTEXT_NAMESPACE
