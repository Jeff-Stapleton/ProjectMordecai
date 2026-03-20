// Project Mordecai — Foundation Tests (Story 1.1)

#include "Misc/AutomationTest.h"
#include "Mordecai/MordecaiGameMode.h"
#include "Mordecai/MordecaiGameState.h"
#include "Mordecai/MordecaiPlayerState.h"
#include "Mordecai/MordecaiPlayerController.h"
#include "Mordecai/AbilitySystem/MordecaiAbilitySystemComponent.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayTask.h"
#include "EnhancedInputComponent.h"
#include "Modules/ModuleManager.h"

// Mordecai.Foundation.GameModeClassExists
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFoundation_GameModeExists,
	"Mordecai.Foundation.GameModeClassExists",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFoundation_GameModeExists::RunTest(const FString& Parameters)
{
	UClass* GameModeClass = AMordecaiGameMode::StaticClass();
	TestNotNull("AMordecaiGameMode class exists", GameModeClass);
	TestTrue("AMordecaiGameMode inherits from ALyraGameMode",
		GameModeClass->IsChildOf(ALyraGameMode::StaticClass()));
	return true;
}

// Mordecai.Foundation.GameStateClassExists
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFoundation_GameStateExists,
	"Mordecai.Foundation.GameStateClassExists",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFoundation_GameStateExists::RunTest(const FString& Parameters)
{
	UClass* GameStateClass = AMordecaiGameState::StaticClass();
	TestNotNull("AMordecaiGameState class exists", GameStateClass);
	TestTrue("AMordecaiGameState inherits from ALyraGameState",
		GameStateClass->IsChildOf(ALyraGameState::StaticClass()));
	return true;
}

// Mordecai.Foundation.PlayerStateClassExists
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFoundation_PlayerStateExists,
	"Mordecai.Foundation.PlayerStateClassExists",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFoundation_PlayerStateExists::RunTest(const FString& Parameters)
{
	UClass* PSClass = AMordecaiPlayerState::StaticClass();
	TestNotNull("AMordecaiPlayerState class exists", PSClass);
	TestTrue("AMordecaiPlayerState inherits from ALyraPlayerState",
		PSClass->IsChildOf(ALyraPlayerState::StaticClass()));
	return true;
}

// Mordecai.Foundation.PlayerControllerClassExists
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFoundation_PlayerControllerExists,
	"Mordecai.Foundation.PlayerControllerClassExists",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFoundation_PlayerControllerExists::RunTest(const FString& Parameters)
{
	UClass* PCClass = AMordecaiPlayerController::StaticClass();
	TestNotNull("AMordecaiPlayerController class exists", PCClass);
	TestTrue("AMordecaiPlayerController inherits from ALyraPlayerController",
		PCClass->IsChildOf(ALyraPlayerController::StaticClass()));
	return true;
}

// Mordecai.Foundation.ASCExistsOnPlayerState
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFoundation_ASCOnPlayerState,
	"Mordecai.Foundation.ASCExistsOnPlayerState",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFoundation_ASCOnPlayerState::RunTest(const FString& Parameters)
{
	// Verify the PlayerState CDO has an ASC subobject of our custom type
	const AMordecaiPlayerState* CDO = GetDefault<AMordecaiPlayerState>();
	TestNotNull("PlayerState CDO exists", CDO);

	UAbilitySystemComponent* ASC = const_cast<AMordecaiPlayerState*>(CDO)->GetAbilitySystemComponent();
	TestNotNull("ASC exists on PlayerState", ASC);

	if (ASC)
	{
		TestTrue("ASC is UMordecaiAbilitySystemComponent",
			ASC->IsA<UMordecaiAbilitySystemComponent>());
	}

	return true;
}

// Mordecai.Foundation.RequiredPluginsLoaded
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiFoundation_RequiredPlugins,
	"Mordecai.Foundation.RequiredPluginsLoaded",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiFoundation_RequiredPlugins::RunTest(const FString& Parameters)
{
	// Verify GAS plugin is loaded
	TestTrue("GameplayAbilities module loaded",
		FModuleManager::Get().IsModuleLoaded("GameplayAbilities"));

	// GameplayTags and GameplayTasks are engine-integrated in UE 5.7;
	// verify by checking that their core types are available
	TestNotNull("GameplayTags available (FGameplayTag class exists)",
		FGameplayTag::StaticStruct());
	TestNotNull("GameplayTasks available (UGameplayTask class exists)",
		UGameplayTask::StaticClass());

	// Verify Enhanced Input
	TestNotNull("EnhancedInput available (UEnhancedInputComponent class exists)",
		UEnhancedInputComponent::StaticClass());
	return true;
}
