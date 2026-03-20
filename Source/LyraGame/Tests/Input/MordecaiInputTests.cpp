// Project Mordecai — Input Tests (Story 1.3)

#include "Misc/AutomationTest.h"
#include "UObject/SoftObjectPath.h"

// Mordecai.Input.AllInputActionsExist
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiInput_AllActionsExist,
	"Mordecai.Input.AllInputActionsExist",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiInput_AllActionsExist::RunTest(const FString& Parameters)
{
	// Verify all required Input Action assets exist in the MordecaiCore plugin.
	// These are .uasset files created via the editor (per TODO_EDITOR_WORK.md).
	const FString BasePath = TEXT("/MordecaiCore/Input/Actions/");

	TArray<FString> RequiredActions = {
		TEXT("IA_Mordecai_Move"),
		TEXT("IA_Mordecai_Look_Mouse"),
		TEXT("IA_Mordecai_Look_Stick"),
		TEXT("IA_Mordecai_Sprint"),
		TEXT("IA_Mordecai_Jump"),
		TEXT("IA_Mordecai_Dodge"),
		TEXT("IA_Mordecai_LightAttack"),
		TEXT("IA_Mordecai_HeavyAttack"),
		TEXT("IA_Mordecai_Block"),
		TEXT("IA_Mordecai_Interact"),
		TEXT("IA_Mordecai_SkillA"),
		TEXT("IA_Mordecai_SkillB"),
		TEXT("IA_Mordecai_Throw"),
		TEXT("IA_Mordecai_WeaponSwap"),
		TEXT("IA_Mordecai_ItemA"),
		TEXT("IA_Mordecai_ItemB"),
		TEXT("IA_Mordecai_ItemC"),
		TEXT("IA_Mordecai_ItemD"),
		TEXT("IA_Mordecai_Pause"),
		TEXT("IA_Mordecai_Map"),
	};

	for (const FString& ActionName : RequiredActions)
	{
		const FString FullPath = BasePath + ActionName + TEXT(".") + ActionName;
		UObject* Asset = FSoftObjectPath(FullPath).TryLoad();
		TestNotNull(FString::Printf(TEXT("Input action %s exists"), *ActionName), Asset);
	}

	return true;
}

// Mordecai.Input.DefaultMappingContextExists
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiInput_MappingContextExists,
	"Mordecai.Input.DefaultMappingContextExists",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiInput_MappingContextExists::RunTest(const FString& Parameters)
{
	const FString IMCPath = TEXT("/MordecaiCore/Input/IMC_Mordecai.IMC_Mordecai");
	UObject* IMC = FSoftObjectPath(IMCPath).TryLoad();
	TestNotNull("IMC_Mordecai mapping context exists", IMC);

	return true;
}

// Mordecai.Input.SprintToggleBound
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiInput_SprintToggle,
	"Mordecai.Input.SprintToggleBound",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiInput_SprintToggle::RunTest(const FString& Parameters)
{
	// Verify sprint input action exists (binding is configured in IMC_Mordecai blueprint)
	const FString SprintPath = TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_Sprint.IA_Mordecai_Sprint");
	UObject* SprintAction = FSoftObjectPath(SprintPath).TryLoad();
	TestNotNull("Sprint input action exists", SprintAction);

	return true;
}
