// Project Mordecai — Identification Tests (US-033)

#include "Misc/AutomationTest.h"
#include "GameplayTagContainer.h"
#include "Engine/GameInstance.h"

#include "Mordecai/Items/MordecaiItemTypes.h"
#include "Mordecai/Items/MordecaiItemDefinition.h"
#include "Mordecai/Items/MordecaiItemInstance.h"
#include "Mordecai/Items/MordecaiItemLibrary.h"
#include "Mordecai/Items/MordecaiInventoryComponent.h"
#include "Mordecai/Items/MordecaiResourceLedger.h"
#include "Mordecai/Items/MordecaiIdentificationService.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "MordecaiInventoryTestHelpers.h"

namespace MordecaiIdentTestHelpers
{
	/** Build a magical item definition that uses identification. */
	static UMordecaiItemDefinition* MakeMagicalDef(
		FName ItemId,
		EMordecaiItemRarity Rarity = EMordecaiItemRarity::Purple,
		EMordecaiIdentificationState DefaultState = EMordecaiIdentificationState::Unidentified,
		bool bRequireForEquip = true,
		bool bShowPartial = true)
	{
		UMordecaiItemDefinition* Def = NewObject<UMordecaiItemDefinition>();
		Def->ItemId = ItemId;
		Def->ItemType = EMordecaiItemType::Weapon;
		Def->Subtype = FName(TEXT("Sword"));
		Def->Rarity = Rarity;
		Def->DisplayName = FText::FromString(TEXT("Sword of Testing"));
		Def->Description = FText::FromString(TEXT("A powerful test blade."));
		Def->ShortDescription = FText::FromString(TEXT("A blade."));
		Def->UsesIdentification = true;
		Def->DefaultIdentificationState = DefaultState;
		Def->RequiresIdentificationToEquip = bRequireForEquip;
		Def->ShowPartialInfoBeforeIdentify = bShowPartial;
		Def->Tags.AddTag(MordecaiGameplayTags::Item_Type_Weapon);
		Def->Tags.AddTag(MordecaiGameplayTags::Item_Rarity_Purple);
		Def->Tags.AddTag(MordecaiGameplayTags::Damage_Fire); // should be hidden when unidentified
		return Def;
	}

	static UMordecaiInventoryComponent* MakeIdentInv()
	{
		UMordecaiInventoryComponent* Inv = NewObject<UMordecaiInventoryComponent>();
		UMordecaiResourceLedger* Led = NewObject<UMordecaiResourceLedger>();
		Inv->SetResourceLedger(Led);
		return Inv;
	}

	static FMordecaiItemInstance MakeIdentInstance(UMordecaiItemDefinition* Def, EMordecaiIdentificationState State)
	{
		FMordecaiItemInstance Inst;
		Inst.InstanceId = FGuid::NewGuid();
		Inst.ItemDefinition = Def;
		Inst.Quantity = 1;
		Inst.IdentificationState = State;
		return Inst;
	}

	/** Create the identification service with a transient GameInstance outer (subsystem requires it). */
	static UMordecaiIdentificationService* MakeService()
	{
		UGameInstance* GI = NewObject<UGameInstance>();
		return NewObject<UMordecaiIdentificationService>(GI);
	}
}

using namespace MordecaiIdentTestHelpers;

// ===========================================================================
// 1. IdentificationStateEnumExists (AC-033.1)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_IdentificationStateEnumExists,
	"Mordecai.Item.IdentificationStateEnumExists",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_IdentificationStateEnumExists::RunTest(const FString& Parameters)
{
	TestEqual("Identified=0",   static_cast<uint8>(EMordecaiIdentificationState::Identified),   uint8(0));
	TestEqual("Unidentified=1", static_cast<uint8>(EMordecaiIdentificationState::Unidentified), uint8(1));
	return true;
}

// ===========================================================================
// 2. DefinitionIdentificationFieldsDefault (AC-033.2)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_DefinitionIdentificationFieldsDefault,
	"Mordecai.Item.DefinitionIdentificationFieldsDefault",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_DefinitionIdentificationFieldsDefault::RunTest(const FString& Parameters)
{
	UMordecaiItemDefinition* Def = NewObject<UMordecaiItemDefinition>();
	TestFalse("UsesIdentification default = false",         Def->UsesIdentification);
	TestEqual("DefaultIdentificationState default = Identified",
		Def->DefaultIdentificationState, EMordecaiIdentificationState::Identified);
	TestFalse("RequiresIdentificationToEquip default = false", Def->RequiresIdentificationToEquip);
	TestTrue ("ShowPartialInfoBeforeIdentify default = true",  Def->ShowPartialInfoBeforeIdentify);
	TestEqual("IdentificationService default = 'MageTower'",   Def->IdentificationService, FName(TEXT("MageTower")));
	return true;
}

// ===========================================================================
// 3. InstanceUsesDefinitionDefaultState (AC-033.4)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_InstanceUsesDefinitionDefaultState,
	"Mordecai.Item.InstanceUsesDefinitionDefaultState",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_InstanceUsesDefinitionDefaultState::RunTest(const FString& Parameters)
{
	UMordecaiInventoryComponent* Inv = MakeIdentInv();
	UMordecaiItemDefinition* Def = MakeMagicalDef(FName("Sword_Unid"));

	FGuid Id = Inv->AddItem(Def, 1);
	TestTrue("Id valid", Id.IsValid());
	const FMordecaiItemInstance* Inst = Inv->FindInstance(Id);
	TestTrue("Instance exists", Inst != nullptr);
	TestEqual("Instance.IdentificationState = Unidentified",
		Inst->IdentificationState, EMordecaiIdentificationState::Unidentified);
	return true;
}

// ===========================================================================
// 4. InstanceAlwaysIdentifiedWhenUsesIdentificationFalse (AC-033.4)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_InstanceAlwaysIdentifiedWhenUsesIdentificationFalse,
	"Mordecai.Item.InstanceAlwaysIdentifiedWhenUsesIdentificationFalse",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_InstanceAlwaysIdentifiedWhenUsesIdentificationFalse::RunTest(const FString& Parameters)
{
	UMordecaiInventoryComponent* Inv = MakeIdentInv();
	UMordecaiItemDefinition* Def = NewObject<UMordecaiItemDefinition>();
	Def->ItemId = FName("Sword_Plain");
	Def->ItemType = EMordecaiItemType::Weapon;
	Def->UsesIdentification = false;
	Def->DefaultIdentificationState = EMordecaiIdentificationState::Unidentified; // intentional trap

	FGuid Id = Inv->AddItem(Def, 1);
	const FMordecaiItemInstance* Inst = Inv->FindInstance(Id);
	TestEqual("Forced Identified when UsesIdentification=false",
		Inst->IdentificationState, EMordecaiIdentificationState::Identified);
	return true;
}

// ===========================================================================
// 5. GetDisplayNameShowsPartialWhenUnidentified (AC-033.6)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_GetDisplayNameShowsPartialWhenUnidentified,
	"Mordecai.Item.GetDisplayNameShowsPartialWhenUnidentified",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_GetDisplayNameShowsPartialWhenUnidentified::RunTest(const FString& Parameters)
{
	UMordecaiItemDefinition* Def = MakeMagicalDef(FName("PurpleSword"), EMordecaiItemRarity::Purple);
	FMordecaiItemInstance Inst = MakeIdentInstance(Def, EMordecaiIdentificationState::Unidentified);

	const FString Name = UMordecaiItemLibrary::GetDisplayName(Inst).ToString();
	TestTrue("Contains 'Unidentified'", Name.Contains(TEXT("Unidentified")));
	TestTrue("Contains 'Purple'", Name.Contains(TEXT("Purple")));
	TestTrue("Contains subtype 'Sword'", Name.Contains(TEXT("Sword")));
	TestFalse("Does NOT contain DisplayName", Name.Contains(TEXT("Sword of Testing")));
	return true;
}

// ===========================================================================
// 6. GetDisplayNameShowsFullWhenIdentified (AC-033.6)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_GetDisplayNameShowsFullWhenIdentified,
	"Mordecai.Item.GetDisplayNameShowsFullWhenIdentified",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_GetDisplayNameShowsFullWhenIdentified::RunTest(const FString& Parameters)
{
	UMordecaiItemDefinition* Def = MakeMagicalDef(FName("PurpleSword"));
	FMordecaiItemInstance Inst = MakeIdentInstance(Def, EMordecaiIdentificationState::Identified);

	TestEqual("Returns DisplayName",
		UMordecaiItemLibrary::GetDisplayName(Inst).ToString(),
		FString(TEXT("Sword of Testing")));
	return true;
}

// ===========================================================================
// 7. GetDisplayNameHidesEverythingWhenShowPartialFalse (AC-033.6)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_GetDisplayNameHidesEverythingWhenShowPartialFalse,
	"Mordecai.Item.GetDisplayNameHidesEverythingWhenShowPartialFalse",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_GetDisplayNameHidesEverythingWhenShowPartialFalse::RunTest(const FString& Parameters)
{
	UMordecaiItemDefinition* Def = MakeMagicalDef(FName("HiddenSword"),
		EMordecaiItemRarity::Purple, EMordecaiIdentificationState::Unidentified, true, /*bShowPartial*/false);
	FMordecaiItemInstance Inst = MakeIdentInstance(Def, EMordecaiIdentificationState::Unidentified);

	TestEqual("Returns 'Unknown Item'",
		UMordecaiItemLibrary::GetDisplayName(Inst).ToString(),
		FString(TEXT("Unknown Item")));
	return true;
}

// ===========================================================================
// 8. GetDescriptionRespectsPartialInfoFlag (AC-033.7)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_GetDescriptionRespectsPartialInfoFlag,
	"Mordecai.Item.GetDescriptionRespectsPartialInfoFlag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_GetDescriptionRespectsPartialInfoFlag::RunTest(const FString& Parameters)
{
	// Identified
	UMordecaiItemDefinition* Def = MakeMagicalDef(FName("S1"));
	FMordecaiItemInstance IdInst  = MakeIdentInstance(Def, EMordecaiIdentificationState::Identified);
	FMordecaiItemInstance UnInst  = MakeIdentInstance(Def, EMordecaiIdentificationState::Unidentified);

	TestEqual("Identified returns full description",
		UMordecaiItemLibrary::GetDescription(IdInst).ToString(), FString(TEXT("A powerful test blade.")));

	TestEqual("Unidentified + ShowPartial=true returns short description",
		UMordecaiItemLibrary::GetDescription(UnInst).ToString(), FString(TEXT("A blade.")));

	// ShowPartial=false
	UMordecaiItemDefinition* Def2 = MakeMagicalDef(FName("S2"),
		EMordecaiItemRarity::Purple, EMordecaiIdentificationState::Unidentified, true, false);
	FMordecaiItemInstance UnInst2 = MakeIdentInstance(Def2, EMordecaiIdentificationState::Unidentified);
	TestTrue("Unidentified + ShowPartial=false returns empty",
		UMordecaiItemLibrary::GetDescription(UnInst2).IsEmpty());
	return true;
}

// ===========================================================================
// 9. GetVisibleTagsFiltersRevealingTags (AC-033.8)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_GetVisibleTagsFiltersRevealingTags,
	"Mordecai.Item.GetVisibleTagsFiltersRevealingTags",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_GetVisibleTagsFiltersRevealingTags::RunTest(const FString& Parameters)
{
	UMordecaiItemDefinition* Def = MakeMagicalDef(FName("FireSword"));
	FMordecaiItemInstance UnInst = MakeIdentInstance(Def, EMordecaiIdentificationState::Unidentified);

	FGameplayTagContainer Visible = UMordecaiItemLibrary::GetVisibleTags(UnInst);
	TestTrue ("Type tag visible",   Visible.HasTag(MordecaiGameplayTags::Item_Type_Weapon));
	TestTrue ("Rarity tag visible", Visible.HasTag(MordecaiGameplayTags::Item_Rarity_Purple));
	TestFalse("Damage tag hidden",  Visible.HasTag(MordecaiGameplayTags::Damage_Fire));

	// Identified shows everything
	FMordecaiItemInstance IdInst = MakeIdentInstance(Def, EMordecaiIdentificationState::Identified);
	FGameplayTagContainer All = UMordecaiItemLibrary::GetVisibleTags(IdInst);
	TestTrue("Identified shows damage tag", All.HasTag(MordecaiGameplayTags::Damage_Fire));
	return true;
}

// ===========================================================================
// 10. CanEquipBlockedByUnidentified (AC-033.9)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_CanEquipBlockedByUnidentified,
	"Mordecai.Item.CanEquipBlockedByUnidentified",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_CanEquipBlockedByUnidentified::RunTest(const FString& Parameters)
{
	UMordecaiItemDefinition* Def = MakeMagicalDef(FName("LockedSword"),
		EMordecaiItemRarity::Purple, EMordecaiIdentificationState::Unidentified, /*requireForEquip*/true);
	FMordecaiItemInstance Inst = MakeIdentInstance(Def, EMordecaiIdentificationState::Unidentified);

	TestFalse("Blocked when unidentified + required-to-equip", UMordecaiItemLibrary::CanEquipInstance(Inst));
	return true;
}

// ===========================================================================
// 11. CanEquipAllowedAfterIdentification (AC-033.9)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_CanEquipAllowedAfterIdentification,
	"Mordecai.Item.CanEquipAllowedAfterIdentification",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_CanEquipAllowedAfterIdentification::RunTest(const FString& Parameters)
{
	UMordecaiItemDefinition* Def = MakeMagicalDef(FName("UnlockableSword"));
	FMordecaiItemInstance Inst = MakeIdentInstance(Def, EMordecaiIdentificationState::Identified);
	TestTrue("Identified + equippable → true", UMordecaiItemLibrary::CanEquipInstance(Inst));
	return true;
}

// ===========================================================================
// 12. CanEquipFalseForNonEquippableType (AC-033.9)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_CanEquipFalseForNonEquippableType,
	"Mordecai.Item.CanEquipFalseForNonEquippableType",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_CanEquipFalseForNonEquippableType::RunTest(const FString& Parameters)
{
	UMordecaiItemDefinition* Def = NewObject<UMordecaiItemDefinition>();
	Def->ItemId = FName("Ore");
	Def->ItemType = EMordecaiItemType::Material;
	FMordecaiItemInstance Inst = MakeIdentInstance(Def, EMordecaiIdentificationState::Identified);
	TestFalse("Material not equippable even when Identified",
		UMordecaiItemLibrary::CanEquipInstance(Inst));
	return true;
}

// ===========================================================================
// 13. ServiceIdentifiesInstance (AC-033.12)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_ServiceIdentifiesInstance,
	"Mordecai.Item.ServiceIdentifiesInstance",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_ServiceIdentifiesInstance::RunTest(const FString& Parameters)
{
	UMordecaiInventoryComponent* Inv = MakeIdentInv();
	UMordecaiItemDefinition* Def = MakeMagicalDef(FName("IdSword"));
	FGuid Id = Inv->AddItem(Def, 1);

	UMordecaiIdentificationService* Svc = MakeService();
	TestTrue("Identify succeeds", Svc->IdentifyInstance(Inv, Id));
	const FMordecaiItemInstance* Inst = Inv->FindInstance(Id);
	TestTrue("Instance now identified", Inst->IsIdentified());
	return true;
}

// ===========================================================================
// 14. ServiceRejectsAlreadyIdentified (AC-033.12)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_ServiceRejectsAlreadyIdentified,
	"Mordecai.Item.ServiceRejectsAlreadyIdentified",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_ServiceRejectsAlreadyIdentified::RunTest(const FString& Parameters)
{
	UMordecaiInventoryComponent* Inv = MakeIdentInv();
	UMordecaiItemDefinition* Def = MakeMagicalDef(FName("IdSword2"));
	FGuid Id = Inv->AddItem(Def, 1);

	UMordecaiIdentificationService* Svc = MakeService();
	TestTrue("First identify", Svc->IdentifyInstance(Inv, Id));
	TestFalse("Second identify rejected", Svc->IdentifyInstance(Inv, Id));
	return true;
}

// ===========================================================================
// 15. ServiceFiresDelegateOnSuccess (AC-033.13)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_ServiceFiresDelegateOnSuccess,
	"Mordecai.Item.ServiceFiresDelegateOnSuccess",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_ServiceFiresDelegateOnSuccess::RunTest(const FString& Parameters)
{
	UMordecaiInventoryComponent* Inv = MakeIdentInv();
	UMordecaiItemDefinition* Def = MakeMagicalDef(FName("IdSword3"));
	FGuid Id = Inv->AddItem(Def, 1);

	UMordecaiIdentificationService* Svc = MakeService();
	UMordecaiIdentifyDelegateSpy* Spy = NewObject<UMordecaiIdentifyDelegateSpy>();
	Svc->OnItemIdentified.AddDynamic(Spy, &UMordecaiIdentifyDelegateSpy::Handle);

	Svc->IdentifyInstance(Inv, Id);
	TestEqual("Delegate fired once", Spy->EventCount, 1);
	TestEqual("InstanceId matches", Spy->LastInstanceId, Id);
	return true;
}

// ===========================================================================
// 16. InventoryDelegateFiresOnIdentificationStateChange (AC-033.14)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_InventoryDelegateFiresOnIdentificationStateChange,
	"Mordecai.Item.InventoryDelegateFiresOnIdentificationStateChange",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_InventoryDelegateFiresOnIdentificationStateChange::RunTest(const FString& Parameters)
{
	UMordecaiInventoryComponent* Inv = MakeIdentInv();
	UMordecaiItemDefinition* Def = MakeMagicalDef(FName("IdSword4"));
	FGuid Id = Inv->AddItem(Def, 1);

	UMordecaiInventoryDelegateSpy* Spy = NewObject<UMordecaiInventoryDelegateSpy>();
	Inv->OnInventoryChanged.AddDynamic(Spy, &UMordecaiInventoryDelegateSpy::Handle);

	Inv->SetInstanceIdentificationState(Id, EMordecaiIdentificationState::Identified);
	TestEqual("Delta=0 fired", Spy->LastDelta, 0);
	return true;
}

// ===========================================================================
// 17. GetIdentificationTagReturnsMatchingTag (AC-033.16)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Item_GetIdentificationTagReturnsMatchingTag,
	"Mordecai.Item.GetIdentificationTagReturnsMatchingTag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Item_GetIdentificationTagReturnsMatchingTag::RunTest(const FString& Parameters)
{
	TestTrue("Identified tag",
		UMordecaiItemLibrary::GetIdentificationTag(EMordecaiIdentificationState::Identified)
			== MordecaiGameplayTags::Item_Identification_Identified);
	TestTrue("Unidentified tag",
		UMordecaiItemLibrary::GetIdentificationTag(EMordecaiIdentificationState::Unidentified)
			== MordecaiGameplayTags::Item_Identification_Unidentified);
	return true;
}
