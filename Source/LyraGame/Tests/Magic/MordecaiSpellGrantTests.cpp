// Project Mordecai — Spell grant tests
// Spells receive their UMordecaiSpellDataAsset through the granted spec's SourceObject, never by
// writing it into the ability CDO: native CDOs are in the packaged build's disregard-for-GC pool,
// and a CDO pointing at a loaded data asset is a fatal GC verification error.

#include "Misc/AutomationTest.h"
#include "GameplayAbilitySpec.h"
#include "Mordecai/Magic/MordecaiGA_Blink.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"

// Mordecai.Magic.SpellGrant.InstanceTakesSpellDataFromSpec
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Magic_SpellGrantInstanceTakesSpellDataFromSpec,
	"Mordecai.Magic.SpellGrant.InstanceTakesSpellDataFromSpec",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Magic_SpellGrantInstanceTakesSpellDataFromSpec::RunTest(const FString& Parameters)
{
	UMordecaiSpellDataAsset* Data = NewObject<UMordecaiSpellDataAsset>();
	UMordecaiGA_Blink* Instance = NewObject<UMordecaiGA_Blink>();
	const FGameplayAbilitySpec Spec(UMordecaiGA_Blink::StaticClass(), 1, INDEX_NONE, Data);

	Instance->TakeSpellDataFromSpec(Spec);
	TestTrue("Instance uses the spell data granted with its spec", Instance->SpellData == Data);
	return true;
}

// Mordecai.Magic.SpellGrant.NonSpellSourceKeepsExistingData
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Magic_SpellGrantNonSpellSourceKeepsExistingData,
	"Mordecai.Magic.SpellGrant.NonSpellSourceKeepsExistingData",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Magic_SpellGrantNonSpellSourceKeepsExistingData::RunTest(const FString& Parameters)
{
	UMordecaiSpellDataAsset* Existing = NewObject<UMordecaiSpellDataAsset>();
	UMordecaiGA_Blink* Instance = NewObject<UMordecaiGA_Blink>();
	Instance->SpellData = Existing;

	// Grants without spell data use another SourceObject (e.g. the player controller)
	UObject* NonSpellSource = NewObject<UMordecaiGA_Blink>();
	const FGameplayAbilitySpec Spec(UMordecaiGA_Blink::StaticClass(), 1, INDEX_NONE, NonSpellSource);
	Instance->TakeSpellDataFromSpec(Spec);
	TestTrue("Existing spell data is kept when the spec carries none", Instance->SpellData == Existing);
	return true;
}

// Mordecai.Magic.SpellGrant.NeverWritesTheCDO
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Magic_SpellGrantNeverWritesTheCDO,
	"Mordecai.Magic.SpellGrant.NeverWritesTheCDO",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Magic_SpellGrantNeverWritesTheCDO::RunTest(const FString& Parameters)
{
	UMordecaiGA_Blink* CDO = GetMutableDefault<UMordecaiGA_Blink>();
	TObjectPtr<UMordecaiSpellDataAsset> Before = CDO->SpellData;

	// GAS calls OnGiveAbility on the CDO when there is no instance; that must not stick data on it
	const FGameplayAbilitySpec Spec(UMordecaiGA_Blink::StaticClass(), 1, INDEX_NONE, NewObject<UMordecaiSpellDataAsset>());
	CDO->TakeSpellDataFromSpec(Spec);
	TestTrue("CDO spell data is unchanged", CDO->SpellData == Before);

	CDO->SpellData = Before;
	return true;
}
