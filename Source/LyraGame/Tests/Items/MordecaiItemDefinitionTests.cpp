// Project Mordecai — Item Definition & Library Tests (US-032)
// NullRHI-compatible tests for item type enums, UMordecaiItemDefinition schema,
// category helpers, and UMordecaiItemLibrary sort/default/tag helpers.

#include "Misc/AutomationTest.h"
#include "GameplayTagContainer.h"

#include "Mordecai/Items/MordecaiItemTypes.h"
#include "Mordecai/Items/MordecaiItemDefinition.h"
#include "Mordecai/Items/MordecaiItemLibrary.h"
#include "Mordecai/Weapons/MordecaiWeaponTypes.h"
#include "Mordecai/MordecaiGameplayTags.h"

namespace MordecaiItemTestHelpers
{
	static UMordecaiItemDefinition* MakeDef(FName ItemId, EMordecaiItemType Type,
		EMordecaiSortPriority Pri = EMordecaiSortPriority::Normal)
	{
		UMordecaiItemDefinition* Def = NewObject<UMordecaiItemDefinition>();
		Def->ItemId = ItemId;
		Def->ItemType = Type;
		Def->SortPriority = Pri;
		return Def;
	}
}

using namespace MordecaiItemTestHelpers;

// ===========================================================================
// 1. Mordecai.Item.ItemTypeEnumHasExpectedValues (AC-032.1)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_ItemTypeEnumHasExpectedValues,
	"Mordecai.Item.ItemTypeEnumHasExpectedValues",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_ItemTypeEnumHasExpectedValues::RunTest(const FString& Parameters)
{
	TestEqual("Weapon=0",        static_cast<uint8>(EMordecaiItemType::Weapon),        uint8(0));
	TestEqual("Armor=1",         static_cast<uint8>(EMordecaiItemType::Armor),         uint8(1));
	TestEqual("Trinket=2",       static_cast<uint8>(EMordecaiItemType::Trinket),       uint8(2));
	TestEqual("Consumable=3",    static_cast<uint8>(EMordecaiItemType::Consumable),    uint8(3));
	TestEqual("Material=4",      static_cast<uint8>(EMordecaiItemType::Material),      uint8(4));
	TestEqual("TownResource=5",  static_cast<uint8>(EMordecaiItemType::TownResource),  uint8(5));
	TestEqual("UpgradeKey=6",    static_cast<uint8>(EMordecaiItemType::UpgradeKey),    uint8(6));
	TestEqual("QuestItem=7",     static_cast<uint8>(EMordecaiItemType::QuestItem),     uint8(7));
	TestEqual("MagicalItem=8",   static_cast<uint8>(EMordecaiItemType::MagicalItem),   uint8(8));
	TestEqual("CurrencyProxy=9", static_cast<uint8>(EMordecaiItemType::CurrencyProxy), uint8(9));
	return true;
}

// ===========================================================================
// 2. Mordecai.Item.StorageEnumsDefined (AC-032.2-5)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_StorageEnumsDefined,
	"Mordecai.Item.StorageEnumsDefined",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_StorageEnumsDefined::RunTest(const FString& Parameters)
{
	TestEqual("CarryModel.Unlimited=0",    static_cast<uint8>(EMordecaiCarryModel::Unlimited),     uint8(0));
	TestEqual("CarryModel.SpecialRule=1",  static_cast<uint8>(EMordecaiCarryModel::SpecialRule),   uint8(1));

	TestEqual("StorageDomain.PlayerInventory=0", static_cast<uint8>(EMordecaiStorageDomain::PlayerInventory), uint8(0));
	TestEqual("StorageDomain.TownStorage=1",     static_cast<uint8>(EMordecaiStorageDomain::TownStorage),     uint8(1));
	TestEqual("StorageDomain.QuestState=2",      static_cast<uint8>(EMordecaiStorageDomain::QuestState),      uint8(2));

	TestEqual("SortPriority.Low=0",       static_cast<uint8>(EMordecaiSortPriority::Low),       uint8(0));
	TestEqual("SortPriority.Normal=1",    static_cast<uint8>(EMordecaiSortPriority::Normal),    uint8(1));
	TestEqual("SortPriority.High=2",      static_cast<uint8>(EMordecaiSortPriority::High),      uint8(2));
	TestEqual("SortPriority.Signature=3", static_cast<uint8>(EMordecaiSortPriority::Signature), uint8(3));
	TestEqual("SortPriority.Critical=4",  static_cast<uint8>(EMordecaiSortPriority::Critical),  uint8(4));

	TestEqual("BindType.None=0",         static_cast<uint8>(EMordecaiBindType::None),         uint8(0));
	TestEqual("BindType.BindOnPickup=1", static_cast<uint8>(EMordecaiBindType::BindOnPickup), uint8(1));
	TestEqual("BindType.BindOnEquip=2",  static_cast<uint8>(EMordecaiBindType::BindOnEquip),  uint8(2));
	TestEqual("BindType.QuestBound=3",   static_cast<uint8>(EMordecaiBindType::QuestBound),   uint8(3));
	return true;
}

// ===========================================================================
// 3. Mordecai.Item.DefinitionIdentityFields (AC-032.6)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_DefinitionIdentityFields,
	"Mordecai.Item.DefinitionIdentityFields",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_DefinitionIdentityFields::RunTest(const FString& Parameters)
{
	UMordecaiItemDefinition* Def = NewObject<UMordecaiItemDefinition>();
	Def->ItemId = FName(TEXT("Test_Iron_Ore"));
	Def->DisplayName = FText::FromString(TEXT("Iron Ore"));
	Def->Description = FText::FromString(TEXT("Raw ore for crafting."));
	Def->ShortDescription = FText::FromString(TEXT("Ore"));

	TestEqual("ItemId set",          Def->ItemId,                        FName(TEXT("Test_Iron_Ore")));
	TestEqual("DisplayName set",     Def->DisplayName.ToString(),        FString(TEXT("Iron Ore")));
	TestEqual("Description set",     Def->Description.ToString(),        FString(TEXT("Raw ore for crafting.")));
	TestEqual("ShortDescription set", Def->ShortDescription.ToString(),  FString(TEXT("Ore")));
	TestTrue("Icon is soft ptr",     Def->Icon.IsNull());
	return true;
}

// ===========================================================================
// 4. Mordecai.Item.DefinitionClassificationFields (AC-032.7)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_DefinitionClassificationFields,
	"Mordecai.Item.DefinitionClassificationFields",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_DefinitionClassificationFields::RunTest(const FString& Parameters)
{
	UMordecaiItemDefinition* Def = NewObject<UMordecaiItemDefinition>();

	// Defaults
	TestEqual("Rarity default = Common", Def->Rarity, EMordecaiItemRarity::Common);
	TestEqual("Tags default empty",      Def->Tags.Num(), 0);

	// Roundtrip
	Def->ItemType = EMordecaiItemType::Weapon;
	Def->Subtype = FName(TEXT("Sword"));
	Def->Rarity = EMordecaiItemRarity::Blue;
	Def->Tags.AddTag(FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Item.Type.Weapon")), false));

	TestEqual("ItemType", Def->ItemType, EMordecaiItemType::Weapon);
	TestEqual("Subtype",  Def->Subtype,  FName(TEXT("Sword")));
	TestEqual("Rarity",   Def->Rarity,   EMordecaiItemRarity::Blue);
	TestEqual("Tags count=1", Def->Tags.Num(), 1);
	return true;
}

// ===========================================================================
// 5. Mordecai.Item.DefinitionStorageDefaults (AC-032.8)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_DefinitionStorageDefaults,
	"Mordecai.Item.DefinitionStorageDefaults",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_DefinitionStorageDefaults::RunTest(const FString& Parameters)
{
	UMordecaiItemDefinition* Def = NewObject<UMordecaiItemDefinition>();
	TestEqual("CarryModel default = Unlimited",      Def->CarryModel,     EMordecaiCarryModel::Unlimited);
	TestFalse("AutoStoreOnPickup default = false",   Def->AutoStoreOnPickup);
	TestEqual("StorageDomain default = PlayerInventory", Def->StorageDomain,  EMordecaiStorageDomain::PlayerInventory);
	TestEqual("SortPriority default = Normal",       Def->SortPriority,   EMordecaiSortPriority::Normal);
	return true;
}

// ===========================================================================
// 6. Mordecai.Item.DefinitionStackDefaults (AC-032.9)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_DefinitionStackDefaults,
	"Mordecai.Item.DefinitionStackDefaults",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_DefinitionStackDefaults::RunTest(const FString& Parameters)
{
	UMordecaiItemDefinition* Def = NewObject<UMordecaiItemDefinition>();
	TestFalse("Stackable default = false",   Def->Stackable);
	TestEqual("MaxStackSize default = 1",    Def->MaxStackSize, 1);
	return true;
}

// ===========================================================================
// 7. Mordecai.Item.IsAutoStoredTrueWhenFlagSet (AC-032.11)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_IsAutoStoredTrueWhenFlagSet,
	"Mordecai.Item.IsAutoStoredTrueWhenFlagSet",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_IsAutoStoredTrueWhenFlagSet::RunTest(const FString& Parameters)
{
	UMordecaiItemDefinition* Def = NewObject<UMordecaiItemDefinition>();
	TestFalse("Default not auto-stored",     Def->IsAutoStored());

	Def->AutoStoreOnPickup = true;
	TestTrue("AutoStore true after flag set", Def->IsAutoStored());
	return true;
}

// ===========================================================================
// 8. Mordecai.Item.IsEquippableMatchesType (AC-032.12)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_IsEquippableMatchesType,
	"Mordecai.Item.IsEquippableMatchesType",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_IsEquippableMatchesType::RunTest(const FString& Parameters)
{
	auto MakeTyped = [](EMordecaiItemType T) -> UMordecaiItemDefinition*
	{
		UMordecaiItemDefinition* D = NewObject<UMordecaiItemDefinition>();
		D->ItemType = T;
		return D;
	};

	TestTrue ("Weapon equippable",       MakeTyped(EMordecaiItemType::Weapon)->IsEquippable());
	TestTrue ("Armor equippable",        MakeTyped(EMordecaiItemType::Armor)->IsEquippable());
	TestTrue ("Trinket equippable",      MakeTyped(EMordecaiItemType::Trinket)->IsEquippable());
	TestTrue ("MagicalItem equippable",  MakeTyped(EMordecaiItemType::MagicalItem)->IsEquippable());

	TestFalse("Material not equippable",     MakeTyped(EMordecaiItemType::Material)->IsEquippable());
	TestFalse("TownResource not equippable", MakeTyped(EMordecaiItemType::TownResource)->IsEquippable());
	TestFalse("UpgradeKey not equippable",   MakeTyped(EMordecaiItemType::UpgradeKey)->IsEquippable());
	TestFalse("QuestItem not equippable",    MakeTyped(EMordecaiItemType::QuestItem)->IsEquippable());
	TestFalse("Consumable not equippable",   MakeTyped(EMordecaiItemType::Consumable)->IsEquippable());
	return true;
}

// ===========================================================================
// 9. Mordecai.Item.IsStackableRequiresBothFlags (AC-032.13)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_IsStackableRequiresBothFlags,
	"Mordecai.Item.IsStackableRequiresBothFlags",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_IsStackableRequiresBothFlags::RunTest(const FString& Parameters)
{
	UMordecaiItemDefinition* Def = NewObject<UMordecaiItemDefinition>();

	// Default: not stackable
	TestFalse("Default not stackable", Def->IsStackable());

	// Flag only, size=1 → not stackable
	Def->Stackable = true;
	Def->MaxStackSize = 1;
	TestFalse("Stackable flag with size=1 → false", Def->IsStackable());

	// Flag + size>1 → stackable
	Def->MaxStackSize = 99;
	TestTrue("Stackable with size=99", Def->IsStackable());

	// Flag off, size>1 → not stackable
	Def->Stackable = false;
	TestFalse("size>1 without flag → false", Def->IsStackable());
	return true;
}

// ===========================================================================
// 10. Mordecai.Item.CompareSortPriorityOrdersByPriorityThenType (AC-032.14)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_CompareSortPriorityOrdersByPriorityThenType,
	"Mordecai.Item.CompareSortPriorityOrdersByPriorityThenType",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_CompareSortPriorityOrdersByPriorityThenType::RunTest(const FString& Parameters)
{
	UMordecaiItemDefinition* Critical = MakeDef(FName("A"), EMordecaiItemType::Material, EMordecaiSortPriority::Critical);
	UMordecaiItemDefinition* Normal   = MakeDef(FName("B"), EMordecaiItemType::Weapon,   EMordecaiSortPriority::Normal);

	// Critical sorts before Normal regardless of type
	TestTrue("Critical < Normal",    UMordecaiItemLibrary::CompareSortPriority(Critical, Normal)   < 0);
	TestTrue("Normal > Critical",    UMordecaiItemLibrary::CompareSortPriority(Normal,   Critical) > 0);

	// Same priority → type ordering: Weapon before Material
	UMordecaiItemDefinition* WeaponN = MakeDef(FName("W"), EMordecaiItemType::Weapon);
	UMordecaiItemDefinition* MatN    = MakeDef(FName("M"), EMordecaiItemType::Material);
	TestTrue("Weapon < Material (same prio)",  UMordecaiItemLibrary::CompareSortPriority(WeaponN, MatN)    < 0);

	// Same priority and type → ItemId lexicographic
	UMordecaiItemDefinition* W_Apple  = MakeDef(FName("Apple"),  EMordecaiItemType::Weapon);
	UMordecaiItemDefinition* W_Banana = MakeDef(FName("Banana"), EMordecaiItemType::Weapon);
	TestTrue("Apple < Banana", UMordecaiItemLibrary::CompareSortPriority(W_Apple, W_Banana) < 0);

	// Self compare = 0
	TestEqual("Same ptr = 0", UMordecaiItemLibrary::CompareSortPriority(WeaponN, WeaponN), 0);

	// Nulls sort last
	TestTrue ("A < nullptr",   UMordecaiItemLibrary::CompareSortPriority(WeaponN, nullptr) < 0);
	TestTrue ("nullptr > A",   UMordecaiItemLibrary::CompareSortPriority(nullptr, WeaponN) > 0);
	TestEqual("null == null",  UMordecaiItemLibrary::CompareSortPriority(nullptr, nullptr), 0);
	return true;
}

// ===========================================================================
// 11. Mordecai.Item.GetTypeDefaultAutoStoreMaterialsTrue (AC-032.15)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_GetTypeDefaultAutoStoreMaterialsTrue,
	"Mordecai.Item.GetTypeDefaultAutoStoreMaterialsTrue",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_GetTypeDefaultAutoStoreMaterialsTrue::RunTest(const FString& Parameters)
{
	TestTrue ("Material auto-stores",     UMordecaiItemLibrary::GetTypeDefaultAutoStore(EMordecaiItemType::Material));
	TestTrue ("TownResource auto-stores", UMordecaiItemLibrary::GetTypeDefaultAutoStore(EMordecaiItemType::TownResource));

	TestFalse("Weapon does not",       UMordecaiItemLibrary::GetTypeDefaultAutoStore(EMordecaiItemType::Weapon));
	TestFalse("Armor does not",        UMordecaiItemLibrary::GetTypeDefaultAutoStore(EMordecaiItemType::Armor));
	TestFalse("Trinket does not",      UMordecaiItemLibrary::GetTypeDefaultAutoStore(EMordecaiItemType::Trinket));
	TestFalse("Consumable does not",   UMordecaiItemLibrary::GetTypeDefaultAutoStore(EMordecaiItemType::Consumable));
	TestFalse("UpgradeKey does not",   UMordecaiItemLibrary::GetTypeDefaultAutoStore(EMordecaiItemType::UpgradeKey));
	TestFalse("QuestItem does not",    UMordecaiItemLibrary::GetTypeDefaultAutoStore(EMordecaiItemType::QuestItem));
	TestFalse("MagicalItem does not",  UMordecaiItemLibrary::GetTypeDefaultAutoStore(EMordecaiItemType::MagicalItem));
	TestFalse("CurrencyProxy does not", UMordecaiItemLibrary::GetTypeDefaultAutoStore(EMordecaiItemType::CurrencyProxy));
	return true;
}

// ===========================================================================
// 12. Mordecai.Item.GetTypeTagReturnsMatchingNativeTag (AC-032.17)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_GetTypeTagReturnsMatchingNativeTag,
	"Mordecai.Item.GetTypeTagReturnsMatchingNativeTag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_GetTypeTagReturnsMatchingNativeTag::RunTest(const FString& Parameters)
{
	TestTrue("Weapon tag",       UMordecaiItemLibrary::GetTypeTag(EMordecaiItemType::Weapon)      == MordecaiGameplayTags::Item_Type_Weapon);
	TestTrue("Armor tag",        UMordecaiItemLibrary::GetTypeTag(EMordecaiItemType::Armor)       == MordecaiGameplayTags::Item_Type_Armor);
	TestTrue("Trinket tag",      UMordecaiItemLibrary::GetTypeTag(EMordecaiItemType::Trinket)     == MordecaiGameplayTags::Item_Type_Trinket);
	TestTrue("Consumable tag",   UMordecaiItemLibrary::GetTypeTag(EMordecaiItemType::Consumable)  == MordecaiGameplayTags::Item_Type_Consumable);
	TestTrue("Material tag",     UMordecaiItemLibrary::GetTypeTag(EMordecaiItemType::Material)    == MordecaiGameplayTags::Item_Type_Material);
	TestTrue("TownResource tag", UMordecaiItemLibrary::GetTypeTag(EMordecaiItemType::TownResource) == MordecaiGameplayTags::Item_Type_TownResource);
	TestTrue("UpgradeKey tag",   UMordecaiItemLibrary::GetTypeTag(EMordecaiItemType::UpgradeKey)  == MordecaiGameplayTags::Item_Type_UpgradeKey);
	TestTrue("QuestItem tag",    UMordecaiItemLibrary::GetTypeTag(EMordecaiItemType::QuestItem)   == MordecaiGameplayTags::Item_Type_QuestItem);
	TestTrue("MagicalItem tag",  UMordecaiItemLibrary::GetTypeTag(EMordecaiItemType::MagicalItem) == MordecaiGameplayTags::Item_Type_MagicalItem);
	return true;
}

// ===========================================================================
// 13. Mordecai.Item.GetRarityTagReturnsMatchingNativeTag (AC-032.17)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_GetRarityTagReturnsMatchingNativeTag,
	"Mordecai.Item.GetRarityTagReturnsMatchingNativeTag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_GetRarityTagReturnsMatchingNativeTag::RunTest(const FString& Parameters)
{
	TestTrue("Common tag", UMordecaiItemLibrary::GetRarityTag(EMordecaiItemRarity::Common) == MordecaiGameplayTags::Item_Rarity_Common);
	TestTrue("Green tag",  UMordecaiItemLibrary::GetRarityTag(EMordecaiItemRarity::Green)  == MordecaiGameplayTags::Item_Rarity_Green);
	TestTrue("Blue tag",   UMordecaiItemLibrary::GetRarityTag(EMordecaiItemRarity::Blue)   == MordecaiGameplayTags::Item_Rarity_Blue);
	TestTrue("Purple tag", UMordecaiItemLibrary::GetRarityTag(EMordecaiItemRarity::Purple) == MordecaiGameplayTags::Item_Rarity_Purple);
	TestTrue("Red tag",    UMordecaiItemLibrary::GetRarityTag(EMordecaiItemRarity::Red)    == MordecaiGameplayTags::Item_Rarity_Red);
	return true;
}
