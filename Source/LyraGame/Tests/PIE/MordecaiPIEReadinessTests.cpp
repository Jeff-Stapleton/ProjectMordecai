// Project Mordecai — PIE Readiness Tests
// Verify the full spawn chain is wired: Experience → PawnData → PawnClass → visible character

#include "Misc/AutomationTest.h"
#include "Character/LyraPawnData.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StreamableManager.h"
#include "UObject/SoftObjectPath.h"

// Mordecai.PIE.PawnDataAssetExists
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiPIE_PawnDataExists,
	"Mordecai.PIE.PawnDataAssetExists",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiPIE_PawnDataExists::RunTest(const FString& Parameters)
{
	// Verify PawnData_Mordecai asset can be loaded
	FSoftObjectPath PawnDataPath(TEXT("/MordecaiCore/System/PawnData_Mordecai.PawnData_Mordecai"));
	UObject* Loaded = PawnDataPath.TryLoad();
	TestNotNull("PawnData_Mordecai asset loads successfully", Loaded);

	if (Loaded)
	{
		const ULyraPawnData* PawnData = Cast<ULyraPawnData>(Loaded);
		TestNotNull("PawnData_Mordecai is a ULyraPawnData", PawnData);
	}

	return true;
}

// Mordecai.PIE.CharacterHasVisibleMesh
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiPIE_CharacterHasVisibleMesh,
	"Mordecai.PIE.CharacterHasVisibleMesh",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiPIE_CharacterHasVisibleMesh::RunTest(const FString& Parameters)
{
	// Verify the character CDO has a visible placeholder mesh
	const AMordecaiCharacter* CDO = GetDefault<AMordecaiCharacter>();
	TestNotNull("Character CDO exists", CDO);

	UStaticMeshComponent* BodyMesh = CDO->GetPlaceholderBody();
	TestNotNull("Character has PlaceholderBody component", BodyMesh);
	if (BodyMesh)
	{
		TestNotNull("PlaceholderBody has mesh assigned", BodyMesh->GetStaticMesh().Get());
		TestFalse("PlaceholderBody is not hidden in game", BodyMesh->bHiddenInGame);
	}

	return true;
}
