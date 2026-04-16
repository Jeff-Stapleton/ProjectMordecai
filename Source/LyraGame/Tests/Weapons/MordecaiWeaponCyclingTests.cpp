// Project Mordecai — Weapon Cycling & HUD Widget Tests (US-077)
// NullRHI-compatible tests for cycling logic on EquipmentComponent, OnWeaponChanged
// delegate, and UMordecaiEquippedWeaponWidget display + live updates.

#include "Misc/AutomationTest.h"

#include "Mordecai/Weapons/MordecaiEquipmentComponent.h"
#include "Mordecai/Weapons/MordecaiWeaponDataAsset.h"
#include "Mordecai/Weapons/MordecaiWeaponTypes.h"
#include "Mordecai/Weapons/MordecaiWeaponProfileFactory.h"
#include "Mordecai/UI/MordecaiEquippedWeaponWidget.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace MordecaiWeaponCyclingTestHelpers
{
	static UMordecaiEquipmentComponent* MakeComp()
	{
		return NewObject<UMordecaiEquipmentComponent>();
	}

	static UMordecaiEquippedWeaponWidget* MakeWidget()
	{
		return NewObject<UMordecaiEquippedWeaponWidget>(GetTransientPackage());
	}
}

using namespace MordecaiWeaponCyclingTestHelpers;

// ===========================================================================
// 1. Mordecai.Weapon.Cycling.NextCyclesToNextWeapon (AC-077.3)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Cycling_NextCyclesToNextWeapon,
	"Mordecai.Weapon.Cycling.NextCyclesToNextWeapon",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Cycling_NextCyclesToNextWeapon::RunTest(const FString& Parameters)
{
	UObject* O = GetTransientPackage();
	UMordecaiEquipmentComponent* Comp = MakeComp();
	Comp->AddAvailableWeapon(UMordecaiWeaponProfileFactory::CreateDagger(O));
	Comp->AddAvailableWeapon(UMordecaiWeaponProfileFactory::CreateLongsword(O));
	Comp->AddAvailableWeapon(UMordecaiWeaponProfileFactory::CreateAxe(O));

	Comp->CycleNextWeapon(); // equips index 0 (dagger)
	TestEqual("Index 0 (Dagger)", Comp->GetCurrentWeaponIndex(), 0);

	Comp->CycleNextWeapon(); // index 1 (longsword)
	TestEqual("Index 1 (Longsword)", Comp->GetCurrentWeaponIndex(), 1);

	Comp->CycleNextWeapon(); // index 2 (axe)
	TestEqual("Index 2 (Axe)", Comp->GetCurrentWeaponIndex(), 2);

	return true;
}

// ===========================================================================
// 2. Mordecai.Weapon.Cycling.NextWrapsAround (AC-077.3)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Cycling_NextWrapsAround,
	"Mordecai.Weapon.Cycling.NextWrapsAround",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Cycling_NextWrapsAround::RunTest(const FString& Parameters)
{
	UObject* O = GetTransientPackage();
	UMordecaiEquipmentComponent* Comp = MakeComp();
	Comp->AddAvailableWeapon(UMordecaiWeaponProfileFactory::CreateDagger(O));
	Comp->AddAvailableWeapon(UMordecaiWeaponProfileFactory::CreateLongsword(O));

	Comp->CycleNextWeapon(); // index 0
	Comp->CycleNextWeapon(); // index 1
	Comp->CycleNextWeapon(); // wrap to index 0

	TestEqual("Wrapped to index 0", Comp->GetCurrentWeaponIndex(), 0);
	return true;
}

// ===========================================================================
// 3. Mordecai.Weapon.Cycling.NextFromNoWeaponEquipsFirst (AC-077.3)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Cycling_NextFromNoWeaponEquipsFirst,
	"Mordecai.Weapon.Cycling.NextFromNoWeaponEquipsFirst",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Cycling_NextFromNoWeaponEquipsFirst::RunTest(const FString& Parameters)
{
	UObject* O = GetTransientPackage();
	UMordecaiEquipmentComponent* Comp = MakeComp();
	Comp->AddAvailableWeapon(UMordecaiWeaponProfileFactory::CreateLongsword(O));
	Comp->AddAvailableWeapon(UMordecaiWeaponProfileFactory::CreateAxe(O));

	TestEqual("No weapon initially", Comp->GetCurrentWeaponIndex(), INDEX_NONE);

	Comp->CycleNextWeapon();
	TestEqual("Equips index 0", Comp->GetCurrentWeaponIndex(), 0);
	return true;
}

// ===========================================================================
// 4. Mordecai.Weapon.Cycling.NextWithEmptyListDoesNothing (AC-077.3)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Cycling_NextWithEmptyListDoesNothing,
	"Mordecai.Weapon.Cycling.NextWithEmptyListDoesNothing",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Cycling_NextWithEmptyListDoesNothing::RunTest(const FString& Parameters)
{
	UMordecaiEquipmentComponent* Comp = MakeComp();
	TestEqual("Empty count", Comp->GetAvailableWeaponCount(), 0);

	Comp->CycleNextWeapon(); // should not crash
	TestEqual("Still no weapon", Comp->GetCurrentWeaponIndex(), INDEX_NONE);
	return true;
}

// ===========================================================================
// 5. Mordecai.Weapon.Cycling.PrevCyclesToPrevWeapon (AC-077.4)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Cycling_PrevCyclesToPrevWeapon,
	"Mordecai.Weapon.Cycling.PrevCyclesToPrevWeapon",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Cycling_PrevCyclesToPrevWeapon::RunTest(const FString& Parameters)
{
	UObject* O = GetTransientPackage();
	UMordecaiEquipmentComponent* Comp = MakeComp();
	Comp->AddAvailableWeapon(UMordecaiWeaponProfileFactory::CreateDagger(O));
	Comp->AddAvailableWeapon(UMordecaiWeaponProfileFactory::CreateLongsword(O));
	Comp->AddAvailableWeapon(UMordecaiWeaponProfileFactory::CreateAxe(O));

	Comp->CycleNextWeapon(); // equip 0
	Comp->CycleNextWeapon(); // equip 1
	TestEqual("At index 1", Comp->GetCurrentWeaponIndex(), 1);

	Comp->CyclePrevWeapon(); // to index 0
	TestEqual("Back to index 0", Comp->GetCurrentWeaponIndex(), 0);
	return true;
}

// ===========================================================================
// 6. Mordecai.Weapon.Cycling.PrevWrapsAround (AC-077.4)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Cycling_PrevWrapsAround,
	"Mordecai.Weapon.Cycling.PrevWrapsAround",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Cycling_PrevWrapsAround::RunTest(const FString& Parameters)
{
	UObject* O = GetTransientPackage();
	UMordecaiEquipmentComponent* Comp = MakeComp();
	Comp->AddAvailableWeapon(UMordecaiWeaponProfileFactory::CreateDagger(O));
	Comp->AddAvailableWeapon(UMordecaiWeaponProfileFactory::CreateLongsword(O));
	Comp->AddAvailableWeapon(UMordecaiWeaponProfileFactory::CreateAxe(O));

	Comp->CycleNextWeapon(); // equip index 0
	TestEqual("At index 0", Comp->GetCurrentWeaponIndex(), 0);

	Comp->CyclePrevWeapon(); // should wrap to last (index 2)
	TestEqual("Wrapped to last index 2", Comp->GetCurrentWeaponIndex(), 2);
	return true;
}

// ===========================================================================
// 7. Mordecai.Weapon.Cycling.GetCurrentWeaponIndex (AC-077.5)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Cycling_GetCurrentWeaponIndex,
	"Mordecai.Weapon.Cycling.GetCurrentWeaponIndex",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Cycling_GetCurrentWeaponIndex::RunTest(const FString& Parameters)
{
	UObject* O = GetTransientPackage();
	UMordecaiEquipmentComponent* Comp = MakeComp();

	TestEqual("No weapon -1", Comp->GetCurrentWeaponIndex(), INDEX_NONE);

	Comp->AddAvailableWeapon(UMordecaiWeaponProfileFactory::CreateDagger(O));
	Comp->AddAvailableWeapon(UMordecaiWeaponProfileFactory::CreateLongsword(O));

	TestEqual("Still -1 with weapons added but none equipped", Comp->GetCurrentWeaponIndex(), INDEX_NONE);

	Comp->CycleNextWeapon();
	TestEqual("Returns 0 after equip", Comp->GetCurrentWeaponIndex(), 0);
	return true;
}

// ===========================================================================
// 8. Mordecai.Weapon.Cycling.AddAvailableWeapon (AC-077.2, AC-077.6)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Cycling_AddAvailableWeapon,
	"Mordecai.Weapon.Cycling.AddAvailableWeapon",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Cycling_AddAvailableWeapon::RunTest(const FString& Parameters)
{
	UObject* O = GetTransientPackage();
	UMordecaiEquipmentComponent* Comp = MakeComp();

	TestEqual("Starts at 0", Comp->GetAvailableWeaponCount(), 0);

	int32 Idx0 = Comp->AddAvailableWeapon(UMordecaiWeaponProfileFactory::CreateLongsword(O));
	TestEqual("First index 0", Idx0, 0);
	TestEqual("Count 1", Comp->GetAvailableWeaponCount(), 1);

	int32 Idx1 = Comp->AddAvailableWeapon(UMordecaiWeaponProfileFactory::CreateAxe(O));
	TestEqual("Second index 1", Idx1, 1);
	TestEqual("Count 2", Comp->GetAvailableWeaponCount(), 2);

	// Null asset returns INDEX_NONE
	int32 IdxNull = Comp->AddAvailableWeapon(nullptr);
	TestEqual("Null asset INDEX_NONE", IdxNull, INDEX_NONE);
	TestEqual("Count unchanged", Comp->GetAvailableWeaponCount(), 2);
	return true;
}

// ===========================================================================
// 9. Mordecai.Weapon.Cycling.OnWeaponChangedFires (AC-077.10)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Cycling_OnWeaponChangedFires,
	"Mordecai.Weapon.Cycling.OnWeaponChangedFires",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Cycling_OnWeaponChangedFires::RunTest(const FString& Parameters)
{
	UObject* O = GetTransientPackage();
	UMordecaiEquipmentComponent* Comp = MakeComp();

	// Use the widget as the observer — it's already a UObject with a UFUNCTION handler
	// for OnWeaponChanged. Bind it and verify its displayed state reflects the broadcasts.
	UMordecaiEquippedWeaponWidget* Observer = MakeWidget();
	Observer->BindToEquipmentComponent(Comp);

	// Baseline: nothing equipped, widget shows Unarmed
	TestEqual("Baseline Unarmed", Observer->GetDisplayedWeaponType(), EMordecaiWeaponType::Unarmed);

	UMordecaiWeaponDataAsset* Sword = UMordecaiWeaponProfileFactory::CreateLongsword(O);
	Comp->AddAvailableWeapon(Sword);
	Comp->CycleNextWeapon();

	// Equip fired the delegate → widget updated to Longsword
	TestEqual("Widget shows Longsword after equip broadcast",
		Observer->GetDisplayedWeaponType(), EMordecaiWeaponType::Longsword);
	TestEqual("Widget name Longsword", Observer->GetDisplayedWeaponName().ToString(), FString(TEXT("Longsword")));

	// Unequip fires delegate with nullptr → widget resets to Unarmed
	Comp->UnequipWeapon(EMordecaiEquipSlot::MainHand);
	TestEqual("Widget back to Unarmed after unequip broadcast",
		Observer->GetDisplayedWeaponType(), EMordecaiWeaponType::Unarmed);

	return true;
}

// ===========================================================================
// 10. Mordecai.Weapon.Widget.ShowsEquippedWeaponName (AC-077.7, AC-077.9)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Widget_ShowsEquippedWeaponName,
	"Mordecai.Weapon.Widget.ShowsEquippedWeaponName",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Widget_ShowsEquippedWeaponName::RunTest(const FString& Parameters)
{
	UObject* O = GetTransientPackage();
	UMordecaiEquipmentComponent* Comp = MakeComp();
	Comp->AddAvailableWeapon(UMordecaiWeaponProfileFactory::CreateLongsword(O));
	Comp->CycleNextWeapon();

	UMordecaiEquippedWeaponWidget* Widget = MakeWidget();
	Widget->BindToEquipmentComponent(Comp);

	TestEqual("Displays 'Longsword'", Widget->GetDisplayedWeaponName().ToString(), FString(TEXT("Longsword")));
	TestEqual("Type Longsword", Widget->GetDisplayedWeaponType(), EMordecaiWeaponType::Longsword);
	return true;
}

// ===========================================================================
// 11. Mordecai.Weapon.Widget.ShowsUnarmedWhenNoWeapon (AC-077.7)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Widget_ShowsUnarmedWhenNoWeapon,
	"Mordecai.Weapon.Widget.ShowsUnarmedWhenNoWeapon",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Widget_ShowsUnarmedWhenNoWeapon::RunTest(const FString& Parameters)
{
	UMordecaiEquipmentComponent* Comp = MakeComp();
	UMordecaiEquippedWeaponWidget* Widget = MakeWidget();
	Widget->BindToEquipmentComponent(Comp);

	TestEqual("Displays 'Unarmed'", Widget->GetDisplayedWeaponName().ToString(), FString(TEXT("Unarmed")));
	TestEqual("Type Unarmed", Widget->GetDisplayedWeaponType(), EMordecaiWeaponType::Unarmed);
	return true;
}

// ===========================================================================
// 12. Mordecai.Weapon.Widget.UpdatesOnWeaponChange (AC-077.8)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Widget_UpdatesOnWeaponChange,
	"Mordecai.Weapon.Widget.UpdatesOnWeaponChange",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Widget_UpdatesOnWeaponChange::RunTest(const FString& Parameters)
{
	UObject* O = GetTransientPackage();
	UMordecaiEquipmentComponent* Comp = MakeComp();
	Comp->AddAvailableWeapon(UMordecaiWeaponProfileFactory::CreateLongsword(O));
	Comp->AddAvailableWeapon(UMordecaiWeaponProfileFactory::CreateAxe(O));

	UMordecaiEquippedWeaponWidget* Widget = MakeWidget();
	Widget->BindToEquipmentComponent(Comp);

	Comp->CycleNextWeapon(); // Longsword
	TestEqual("Shows Longsword", Widget->GetDisplayedWeaponName().ToString(), FString(TEXT("Longsword")));

	Comp->CycleNextWeapon(); // Axe
	TestEqual("Shows Axe", Widget->GetDisplayedWeaponName().ToString(), FString(TEXT("Axe")));
	TestEqual("Type Axe", Widget->GetDisplayedWeaponType(), EMordecaiWeaponType::Axe);
	return true;
}
