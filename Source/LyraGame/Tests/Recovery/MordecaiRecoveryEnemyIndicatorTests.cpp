// Project Mordecai — Recovery: enemy indicators register on aggro and render through the arena HUD (US-092).

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Mordecai/MordecaiPlayerController.h"
#include "Mordecai/UI/MordecaiCombatHUDWidget.h"
#include "Mordecai/UI/MordecaiEnemyIndicatorComponent.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"
#include "TimerManager.h"
#include "UI/IndicatorSystem/IndicatorDescriptor.h"
#include "UI/IndicatorSystem/IndicatorLayer.h"
#include "UI/IndicatorSystem/LyraIndicatorManagerComponent.h"

using namespace MordecaiRecoveryTest;

namespace MordecaiRecoveryEnemyIndicatorTest
{
	AMordecaiPlayerController* SpawnController(FRecoveryTestEnv& Env)
	{
		AMordecaiPlayerController* PC = Env.World
			? Env.World->SpawnActor<AMordecaiPlayerController>(AMordecaiPlayerController::StaticClass())
			: nullptr;
		if (PC)
		{
			PC->SetPawn(Env.Player);
		}
		return PC;
	}

	UMordecaiEnemyIndicatorComponent* Indicator(AMordecaiEnemyCharacter* Enemy)
	{
		return Enemy ? Enemy->FindComponentByClass<UMordecaiEnemyIndicatorComponent>() : nullptr;
	}

	UIndicatorDescriptor* DescriptorFor(
		const ULyraIndicatorManagerComponent* Manager, const AMordecaiEnemyCharacter* Enemy)
	{
		if (Manager)
		{
			for (UIndicatorDescriptor* Descriptor : Manager->GetIndicators())
			{
				if (Descriptor && Descriptor->GetDataObject() == Enemy)
				{
					return Descriptor;
				}
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_EnemyIndicator_RegistersOnAggro,
	"Mordecai.Recovery.EnemyIndicator.RegistersOnAggro",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_EnemyIndicator_RegistersOnAggro::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryEnemyIndicatorTest;

	FRecoveryTestEnv Env;
	if (!TestTrue("test world with a player", Env.Setup(FVector::ZeroVector)))
	{
		Env.Teardown();
		return true;
	}

	AMordecaiPlayerController* PC = SpawnController(Env);
	AMordecaiEnemyCharacter* Enemy = Env.SpawnEnemy(FVector(500.f, 0.f, 0.f), {});
	AMordecaiEnemyAIController* AI = Enemy ? Cast<AMordecaiEnemyAIController>(Enemy->GetController()) : nullptr;
	UMordecaiEnemyIndicatorComponent* Component = Indicator(Enemy);
	ULyraIndicatorManagerComponent* Manager = PC
		? PC->FindComponentByClass<ULyraIndicatorManagerComponent>() : nullptr;
	if (!TestNotNull("native Mordecai controller", PC)
		|| !TestNotNull("enemy AI", AI)
		|| !TestNotNull("enemy indicator component", Component)
		|| !TestNotNull("controller indicator manager", Manager))
	{
		Env.Teardown();
		return true;
	}

	TestFalse("indicator starts unregistered", Component->IsIndicatorRegistered());
	AI->UpdateStateMachine(0.016f, Env.Player);
	TestEqual("real state machine enters Approach", AI->GetCurrentState(), EMordecaiEnemyAIState::Approach);
	TestTrue("aggro registers the component", Component->IsIndicatorRegistered());
	TestNotNull("manager contains this enemy's descriptor", DescriptorFor(Manager, Enemy));
	TestEqual("manager has exactly one indicator", Manager->GetIndicators().Num(), 1);

	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_EnemyIndicator_RetriesWhenManagerMissing,
	"Mordecai.Recovery.EnemyIndicator.RetriesWhenManagerMissing",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_EnemyIndicator_RetriesWhenManagerMissing::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryEnemyIndicatorTest;

	FRecoveryControlledTestEnv Env;
	if (!TestTrue("controlled player world", Env.SetupWithController()))
	{
		Env.TeardownWithController();
		return true;
	}

	AMordecaiEnemyCharacter* Enemy = Env.SpawnEnemy(FVector(500.f, 0.f, 0.f), { MakeStubProfile() });
	UMordecaiEnemyIndicatorComponent* Component = Indicator(Enemy);
	UStaticMeshComponent* Body = Enemy ? FindObject<UStaticMeshComponent>(Enemy, TEXT("BodyMesh")) : nullptr;
	if (!TestNotNull("enemy", Enemy)
		|| !TestNotNull("enemy indicator component", Component)
		|| !TestNotNull("enemy body component", Body))
	{
		Env.TeardownWithController();
		return true;
	}
	Body->SetStaticMesh(NewObject<UStaticMesh>(GetTransientPackage()));
	FRecoveryTestEnv::BeginEnemyPlay(Enemy);

	TestNull("base test controller starts without a manager",
		Env.PC->FindComponentByClass<ULyraIndicatorManagerComponent>());
	Component->NotifyDamageReceived();
	TestFalse("failed AddIndicator attempt does not set the registered flag", Component->IsIndicatorRegistered());

	ULyraIndicatorManagerComponent* Manager = NewObject<ULyraIndicatorManagerComponent>(
		Env.PC, ULyraIndicatorManagerComponent::StaticClass(), TEXT("LateIndicatorManager"));
	Manager->RegisterComponent();

	// BeginPlay's 0.25 s timer is pending until the first timer-manager frame, then fires on the second.
	for (int32 Frame = 0; Frame < 2; ++Frame)
	{
		++GFrameCounter;
		Env.World->GetTimerManager().Tick(0.3f);
	}

	TestTrue("latched request registers after the manager appears", Component->IsIndicatorRegistered());
	TestNotNull("late manager contains this enemy's descriptor", DescriptorFor(Manager, Enemy));
	TestEqual("late manager has exactly one indicator", Manager->GetIndicators().Num(), 1);

	Env.TeardownWithController();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_EnemyIndicator_ArenaHasManagerAndLayer,
	"Mordecai.Recovery.EnemyIndicator.ArenaHasManagerAndLayer",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_EnemyIndicator_ArenaHasManagerAndLayer::RunTest(const FString& Parameters)
{
	const AMordecaiPlayerController* ControllerCDO = GetDefault<AMordecaiPlayerController>();
	TestNotNull("native Mordecai controller has an indicator manager",
		ControllerCDO->FindComponentByClass<ULyraIndicatorManagerComponent>());

	const UMordecaiEnemyIndicatorComponent* IndicatorCDO = GetDefault<UMordecaiEnemyIndicatorComponent>();
	TestEqual("native enemy indicator widget class is the default",
		IndicatorCDO->IndicatorWidgetClass.ToSoftObjectPath().ToString(),
		FString(TEXT("/Script/LyraGame.MordecaiEnemyIndicatorWidget")));

	FRecoveryControlledTestEnv Env;
	if (!TestTrue("controlled player world", Env.SetupWithController()))
	{
		Env.TeardownWithController();
		return true;
	}

	UMordecaiCombatHUDWidget* HUD = Env.MakeWidget<UMordecaiCombatHUDWidget>();
	UIndicatorLayer* Layer = nullptr;
	if (HUD && HUD->WidgetTree)
	{
		HUD->WidgetTree->ForEachWidget([&Layer](UWidget* Child)
		{
			if (!Layer)
			{
				Layer = Cast<UIndicatorLayer>(Child);
			}
		});
	}
	if (TestNotNull("default combat HUD contains an IndicatorLayer", Layer))
	{
		const UCanvasPanelSlot* LayerSlot = Cast<UCanvasPanelSlot>(Layer->Slot);
		if (TestNotNull("IndicatorLayer is a root-canvas child", LayerSlot))
		{
			TestEqual("full-screen minimum anchor", LayerSlot->GetAnchors().Minimum, FVector2D(0.f, 0.f));
			TestEqual("full-screen maximum anchor", LayerSlot->GetAnchors().Maximum, FVector2D(1.f, 1.f));
			TestEqual("full-screen offsets", LayerSlot->GetOffsets(), FMargin(0.f));
			TestEqual("indicator layer draws below ordinary HUD controls", LayerSlot->GetZOrder(), -1);
		}
	}

	Env.TeardownWithController();
	return true;
}
