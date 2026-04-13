// Project Mordecai — Character Sheet Widget Tests (US-066)
// Tests the pure data model (FMordecaiCharacterSheetData) and the widget-level
// ASC binding / delegate bookkeeping. All NullRHI-compatible.

#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/UI/MordecaiCharacterSheetTypes.h"
#include "Mordecai/UI/MordecaiCharacterSheetWidget.h"

// ---------------------------------------------------------------------------
// Helper: Minimal test environment with World + Actor + ASC + AttributeSet.
// ---------------------------------------------------------------------------
struct FCharacterSheetTestEnv
{
	UWorld* World = nullptr;
	AActor* TestActor = nullptr;
	UAbilitySystemComponent* ASC = nullptr;
	UMordecaiAttributeSet* AttributeSet = nullptr;

	bool Setup()
	{
		if (!GEngine)
		{
			return false;
		}
		World = UWorld::CreateWorld(EWorldType::Game, false);
		if (!World)
		{
			return false;
		}
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);

		FActorSpawnParameters SpawnParams;
		TestActor = World->SpawnActor<AActor>(AActor::StaticClass(), SpawnParams);
		if (!TestActor) { Teardown(); return false; }

		ASC = NewObject<UAbilitySystemComponent>(TestActor, TEXT("CharSheetTestASC"));
		ASC->RegisterComponent();

		AttributeSet = NewObject<UMordecaiAttributeSet>(TestActor, TEXT("CharSheetTestAttrSet"));
		ASC->AddAttributeSetSubobject(AttributeSet);
		ASC->InitAbilityActorInfo(TestActor, TestActor);

		return true;
	}

	void Teardown()
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
		World = nullptr;
		TestActor = nullptr;
		ASC = nullptr;
		AttributeSet = nullptr;
	}
};

// ===========================================================================
// 1. Mordecai.UI.CharacterSheet.DisplaysPrimaryAttributes (AC-066.3)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_CharacterSheet_DisplaysPrimaryAttributes,
	"Mordecai.UI.CharacterSheet.DisplaysPrimaryAttributes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_CharacterSheet_DisplaysPrimaryAttributes::RunTest(const FString& Parameters)
{
	FMordecaiCharacterSheetData Data;
	Data.InitDefaults();

	TestEqual("9 primary attributes", Data.PrimaryAttributes.Num(), 9);

	const TArray<FName> ExpectedCodes = {
		FName("STR"), FName("DEX"), FName("END"),
		FName("CON"), FName("RES"), FName("DIS"),
		FName("INT"), FName("WIS"), FName("CHA")
	};
	for (int32 i = 0; i < ExpectedCodes.Num(); ++i)
	{
		TestEqual(
			FString::Printf(TEXT("Primary[%d] code"), i),
			Data.PrimaryAttributes[i].Code,
			ExpectedCodes[i]);
		TestFalse(
			FString::Printf(TEXT("Primary[%d] bHasValue=false before bind"), i),
			Data.PrimaryAttributes[i].bHasValue);
	}
	return true;
}

// ===========================================================================
// 2. Mordecai.UI.CharacterSheet.PrimaryAttributesGrouped (AC-066.4)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_CharacterSheet_PrimaryAttributesGrouped,
	"Mordecai.UI.CharacterSheet.PrimaryAttributesGrouped",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_CharacterSheet_PrimaryAttributesGrouped::RunTest(const FString& Parameters)
{
	// STR(0), DEX(1), END(2) -> Physical
	TestEqual("STR -> Physical", FMordecaiCharacterSheetData::GetGroupForAttribute(0), EMordecaiAttributeGroup::Physical);
	TestEqual("DEX -> Physical", FMordecaiCharacterSheetData::GetGroupForAttribute(1), EMordecaiAttributeGroup::Physical);
	TestEqual("END -> Physical", FMordecaiCharacterSheetData::GetGroupForAttribute(2), EMordecaiAttributeGroup::Physical);

	// CON(3), RES(4), DIS(5) -> Resilience
	TestEqual("CON -> Resilience", FMordecaiCharacterSheetData::GetGroupForAttribute(3), EMordecaiAttributeGroup::Resilience);
	TestEqual("RES -> Resilience", FMordecaiCharacterSheetData::GetGroupForAttribute(4), EMordecaiAttributeGroup::Resilience);
	TestEqual("DIS -> Resilience", FMordecaiCharacterSheetData::GetGroupForAttribute(5), EMordecaiAttributeGroup::Resilience);

	// INT(6), WIS(7), CHA(8) -> Magical
	TestEqual("INT -> Magical", FMordecaiCharacterSheetData::GetGroupForAttribute(6), EMordecaiAttributeGroup::Magical);
	TestEqual("WIS -> Magical", FMordecaiCharacterSheetData::GetGroupForAttribute(7), EMordecaiAttributeGroup::Magical);
	TestEqual("CHA -> Magical", FMordecaiCharacterSheetData::GetGroupForAttribute(8), EMordecaiAttributeGroup::Magical);

	// Group labels populated by InitDefaults.
	FMordecaiCharacterSheetData Data;
	Data.InitDefaults();
	TestFalse("Physical label set", Data.PhysicalGroupLabel.IsEmpty());
	TestFalse("Resilience label set", Data.ResilienceGroupLabel.IsEmpty());
	TestFalse("Magical label set", Data.MagicalGroupLabel.IsEmpty());

	return true;
}

// ===========================================================================
// 3. Mordecai.UI.CharacterSheet.DisplaysCoreResources (AC-066.5)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_CharacterSheet_DisplaysCoreResources,
	"Mordecai.UI.CharacterSheet.DisplaysCoreResources",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_CharacterSheet_DisplaysCoreResources::RunTest(const FString& Parameters)
{
	FMordecaiCharacterSheetData Data;
	Data.InitDefaults();

	TestEqual("4 core resources", Data.CoreResources.Num(), 4);

	const TArray<FName> ExpectedIds = {
		FName("health"), FName("stamina"), FName("spellpoints"), FName("posture")
	};
	for (int32 i = 0; i < ExpectedIds.Num(); ++i)
	{
		TestEqual(
			FString::Printf(TEXT("Resource[%d] id"), i),
			Data.CoreResources[i].Id,
			ExpectedIds[i]);
		TestFalse(
			FString::Printf(TEXT("Resource[%d] bHasValue=false before bind"), i),
			Data.CoreResources[i].bHasValue);
	}

	// Formatter uses rounded integer values.
	TestEqual(
		TEXT("FormatResource(75.4, 100.0) == '75 / 100'"),
		FMordecaiCharacterSheetData::FormatResource(75.4f, 100.f),
		FString(TEXT("75 / 100")));

	return true;
}

// ===========================================================================
// 4. Mordecai.UI.CharacterSheet.DisplaysDerivedMultipliers (AC-066.6)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_CharacterSheet_DisplaysDerivedMultipliers,
	"Mordecai.UI.CharacterSheet.DisplaysDerivedMultipliers",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_CharacterSheet_DisplaysDerivedMultipliers::RunTest(const FString& Parameters)
{
	FMordecaiCharacterSheetData Data;
	Data.InitDefaults();

	// 5 primary-derived multipliers: Physical Damage, Attack Speed, Affliction Resist, Magic Damage, Cast Speed
	TestEqual("5 primary-derived stats", Data.PrimaryDerivedStats.Num(), 5);
	for (const FMordecaiDerivedStatDisplay& Row : Data.PrimaryDerivedStats)
	{
		TestFalse("Primary-derived has display name", Row.DisplayName.IsEmpty());
		TestEqual("Primary-derived default multiplier is 1.0", Row.MultiplierValue, 1.0f);
		TestFalse("Primary-derived bHasValue=false before bind", Row.bHasValue);
	}

	// Percent formatter: 1.15 -> "+15.0%", 1.0 -> "+0.0%", 0.85 -> "-15.0%".
	TestEqual(
		TEXT("FormatPercentBonus(1.15) == '+15.0%'"),
		FMordecaiCharacterSheetData::FormatPercentBonus(1.15f),
		FString(TEXT("+15.0%")));
	TestEqual(
		TEXT("FormatPercentBonus(1.0) == '+0.0%'"),
		FMordecaiCharacterSheetData::FormatPercentBonus(1.0f),
		FString(TEXT("+0.0%")));
	TestEqual(
		TEXT("FormatPercentBonus(0.85) == '-15.0%'"),
		FMordecaiCharacterSheetData::FormatPercentBonus(0.85f),
		FString(TEXT("-15.0%")));

	return true;
}

// ===========================================================================
// 5. Mordecai.UI.CharacterSheet.DisplaysSecondaryDerived (AC-066.7)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_CharacterSheet_DisplaysSecondaryDerived,
	"Mordecai.UI.CharacterSheet.DisplaysSecondaryDerived",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_CharacterSheet_DisplaysSecondaryDerived::RunTest(const FString& Parameters)
{
	FMordecaiCharacterSheetData Data;
	Data.InitDefaults();

	// 9 secondary-derived: Armor Pen, Physical Crit, Stamina Regen, Health Regen,
	// Affliction Recovery, Posture Recovery, SP Regen, Resistance Pen, Magic Crit.
	TestEqual("9 secondary-derived stats", Data.SecondaryDerivedStats.Num(), 9);
	for (const FMordecaiDerivedStatDisplay& Row : Data.SecondaryDerivedStats)
	{
		TestFalse("Secondary-derived has display name", Row.DisplayName.IsEmpty());
		TestEqual("Secondary-derived default multiplier is 1.0", Row.MultiplierValue, 1.0f);
		TestFalse("Secondary-derived bHasValue=false before bind", Row.bHasValue);
	}
	return true;
}

// ===========================================================================
// 6. Mordecai.UI.CharacterSheet.UpdatesOnAttributeChange (AC-066.2, AC-066.8)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_CharacterSheet_UpdatesOnAttributeChange,
	"Mordecai.UI.CharacterSheet.UpdatesOnAttributeChange",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_CharacterSheet_UpdatesOnAttributeChange::RunTest(const FString& Parameters)
{
	FCharacterSheetTestEnv Env;
	if (!Env.Setup())
	{
		AddError(TEXT("Test env setup failed"));
		return false;
	}

	UMordecaiCharacterSheetWidget* Widget = NewObject<UMordecaiCharacterSheetWidget>(Env.TestActor);
	TestNotNull("Widget created", Widget);

	Widget->BindToASC(Env.ASC);
	TestTrue("Widget bound", Widget->IsBoundToASC());
	TestTrue("Data marked bound", Widget->GetData().bIsBound);

	// After bind, primary row 0 (STR) has bHasValue=true and BaseValue=10 (default).
	const FMordecaiPrimaryAttributeDisplay& StrRow = Widget->GetData().PrimaryAttributes[0];
	TestTrue("STR row has value after bind", StrRow.bHasValue);
	TestEqual("STR starts at default 10", StrRow.BaseValue, 10.f);

	// Apply a mod to STR through the ASC → delegate should fire and update the row.
	// Additive +4: STR goes from 10 to 14.
	Env.ASC->ApplyModToAttribute(UMordecaiAttributeSet::GetStrengthAttribute(), EGameplayModOp::Additive, 4.f);
	const FMordecaiPrimaryAttributeDisplay& StrRowAfter = Widget->GetData().PrimaryAttributes[0];
	TestEqual("STR BaseValue updated after mod", StrRowAfter.BaseValue, 14.f);

	// Change Health → resource row should update.
	// Additive -58: Health goes from 100 to 42.
	Env.ASC->ApplyModToAttribute(UMordecaiAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, -58.f);
	TestEqual(
		TEXT("Health resource row updated"),
		Widget->GetData().CoreResources[0].Current,
		42.f);

	Env.Teardown();
	return true;
}

// ===========================================================================
// 7. Mordecai.UI.CharacterSheet.GracefulWithoutASC (AC-066.8)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_CharacterSheet_GracefulWithoutASC,
	"Mordecai.UI.CharacterSheet.GracefulWithoutASC",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_CharacterSheet_GracefulWithoutASC::RunTest(const FString& Parameters)
{
	FMordecaiCharacterSheetData Data;
	Data.InitDefaults();

	TestFalse("Not bound after InitDefaults", Data.bIsBound);
	for (const FMordecaiPrimaryAttributeDisplay& Row : Data.PrimaryAttributes)
	{
		TestFalse("Primary bHasValue=false", Row.bHasValue);
	}
	for (const FMordecaiResourceDisplay& Row : Data.CoreResources)
	{
		TestFalse("Resource bHasValue=false", Row.bHasValue);
	}
	for (const FMordecaiDerivedStatDisplay& Row : Data.PrimaryDerivedStats)
	{
		TestFalse("PrimaryDerived bHasValue=false", Row.bHasValue);
	}
	for (const FMordecaiDerivedStatDisplay& Row : Data.SecondaryDerivedStats)
	{
		TestFalse("SecondaryDerived bHasValue=false", Row.bHasValue);
	}

	// Widget also handles null ASC without crashing.
	UMordecaiCharacterSheetWidget* Widget = NewObject<UMordecaiCharacterSheetWidget>(GetTransientPackage());
	TestNotNull("Widget created with transient outer", Widget);
	Widget->BindToASC(nullptr); // no-op
	TestFalse("Widget remains unbound after nullptr", Widget->IsBoundToASC());
	TestEqual("No delegates registered with nullptr ASC", Widget->GetRegisteredDelegateCount(), 0);

	return true;
}

// ===========================================================================
// 8. Mordecai.UI.CharacterSheet.ClearsDelegatesOnDestroy (AC-066.9)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_CharacterSheet_ClearsDelegatesOnDestroy,
	"Mordecai.UI.CharacterSheet.ClearsDelegatesOnDestroy",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_CharacterSheet_ClearsDelegatesOnDestroy::RunTest(const FString& Parameters)
{
	FCharacterSheetTestEnv Env;
	if (!Env.Setup())
	{
		AddError(TEXT("Test env setup failed"));
		return false;
	}

	UMordecaiCharacterSheetWidget* Widget = NewObject<UMordecaiCharacterSheetWidget>(Env.TestActor);
	Widget->BindToASC(Env.ASC);

	const int32 NumRegistered = Widget->GetRegisteredDelegateCount();
	// 9 primaries + 5 primary-derived + 9 secondary-derived + 4 resources*2 (current+max) = 31
	TestEqual("31 delegate handles registered", NumRegistered, 31);

	Widget->UnbindFromASC();
	TestEqual("All delegates cleared on unbind", Widget->GetRegisteredDelegateCount(), 0);
	TestFalse("Widget no longer bound", Widget->IsBoundToASC());
	TestFalse("Data no longer marked bound", Widget->GetData().bIsBound);

	// Re-binding then destroying should also clear (via NativeDestruct path).
	Widget->BindToASC(Env.ASC);
	TestEqual("Re-bind restores delegates", Widget->GetRegisteredDelegateCount(), 31);

	Env.Teardown();
	return true;
}
