// Project Mordecai — Recovery: enemy indicators register from spawn and render through the arena HUD (US-092).

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"
#include "Mordecai/MordecaiPlayerController.h"
#include "Mordecai/UI/MordecaiCombatHUDWidget.h"
#include "Mordecai/UI/MordecaiEnemyIndicatorComponent.h"
#include "Mordecai/UI/MordecaiEnemyIndicatorWidget.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"
#include "TimerManager.h"
#include "UI/IndicatorSystem/IndicatorDescriptor.h"
#include "UI/IndicatorSystem/IndicatorLayer.h"
#include "UI/IndicatorSystem/LyraIndicatorManagerComponent.h"
#include "UObject/SoftObjectPath.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_EnemyIndicator_RegistersOnSpawn,
	"Mordecai.Recovery.EnemyIndicator.RegistersOnSpawn",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_EnemyIndicator_RegistersOnSpawn::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryEnemyIndicatorTest;

	FRecoveryTestEnv Env;
	if (!TestTrue("test world with a player", Env.Setup(FVector::ZeroVector)))
	{
		Env.Teardown();
		return true;
	}

	AMordecaiPlayerController* PC = SpawnController(Env);
	AMordecaiEnemyCharacter* Enemy = Env.SpawnEnemy(
		FVector(500.f, 0.f, 0.f), { MakeStubProfile() });
	UMordecaiEnemyIndicatorComponent* Component = Indicator(Enemy);
	UStaticMeshComponent* Body = Enemy ? FindObject<UStaticMeshComponent>(Enemy, TEXT("BodyMesh")) : nullptr;
	ULyraIndicatorManagerComponent* Manager = PC
		? PC->FindComponentByClass<ULyraIndicatorManagerComponent>() : nullptr;
	if (!TestNotNull("native Mordecai controller", PC)
		|| !TestNotNull("enemy indicator component", Component)
		|| !TestNotNull("enemy body component", Body)
		|| !TestNotNull("controller indicator manager", Manager))
	{
		Env.Teardown();
		return true;
	}
	Body->SetStaticMesh(NewObject<UStaticMesh>(GetTransientPackage()));

	TestFalse("indicator starts unregistered", Component->IsIndicatorRegistered());
	FRecoveryTestEnv::BeginEnemyPlay(Enemy);
	TestTrue("BeginPlay registers the living enemy's indicator", Component->IsIndicatorRegistered());
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
	TestFalse("spawn registration without a manager does not set the registered flag",
		Component->IsIndicatorRegistered());

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_EnemyIndicator_RefreshesOnArenaReset,
	"Mordecai.Recovery.EnemyIndicator.RefreshesOnArenaReset",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_EnemyIndicator_RefreshesOnArenaReset::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryEnemyIndicatorTest;

	FRecoveryTestEnv Env;
	if (!TestTrue("test world with a player", Env.Setup(FVector::ZeroVector)))
	{
		Env.Teardown();
		return true;
	}

	AMordecaiPlayerController* PC = SpawnController(Env);
	AMordecaiEnemyCharacter* Enemy = Env.SpawnEnemy(
		FVector(500.f, 0.f, 0.f), { MakeStubProfile() });
	UMordecaiEnemyIndicatorComponent* Component = Indicator(Enemy);
	ULyraIndicatorManagerComponent* Manager = PC
		? PC->FindComponentByClass<ULyraIndicatorManagerComponent>() : nullptr;
	if (!TestNotNull("native Mordecai controller", PC)
		|| !TestNotNull("enemy", Enemy)
		|| !TestNotNull("enemy indicator component", Component)
		|| !TestNotNull("controller indicator manager", Manager))
	{
		Env.Teardown();
		return true;
	}

	Component->NotifyDamageReceived();
	UIndicatorDescriptor* BeforeReset = DescriptorFor(Manager, Enemy);
	TestNotNull("damaged enemy has an indicator descriptor", BeforeReset);

	FRecoveryTestEnv::ApplyHealthDelta(Env.GetPlayerASC(), Enemy, -25.f);
	const float MaxHealth = Enemy->GetMordecaiAttributeSet()->GetMaxHealth();
	TestTrue("enemy is damaged before reset",
		Enemy->GetMordecaiAttributeSet()->GetHealth() < MaxHealth);

	Enemy->ResetForArena();

	UIndicatorDescriptor* AfterReset = DescriptorFor(Manager, Enemy);
	TestEqual("arena reset restores full health",
		Enemy->GetMordecaiAttributeSet()->GetHealth(), MaxHealth);
	TestTrue("arena reset replaces the descriptor so the widget rebinds current health",
		AfterReset && AfterReset != BeforeReset);
	TestEqual("manager still has exactly one indicator after reset",
		Manager->GetIndicators().Num(), 1);

	Enemy->EnterDeathState();
	TestFalse("death unregisters the always-on indicator", Component->IsIndicatorRegistered());
	TestNull("manager removes the dead enemy's descriptor", DescriptorFor(Manager, Enemy));

	Enemy->ResetForArena();
	TestTrue("reset restores a killed enemy's indicator", Component->IsIndicatorRegistered());
	TestNotNull("manager contains the revived enemy's descriptor", DescriptorFor(Manager, Enemy));
	TestEqual("manager has exactly one indicator after revival",
		Manager->GetIndicators().Num(), 1);

	Env.Teardown();
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_EnemyIndicator_ReplacesLegacyHealthBar,
	"Mordecai.Recovery.EnemyIndicator.ReplacesLegacyHealthBar",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_EnemyIndicator_ReplacesLegacyHealthBar::RunTest(const FString& Parameters)
{
	TestNull("enemy class no longer exposes the legacy widget class",
		AMordecaiEnemyCharacter::StaticClass()->FindPropertyByName(TEXT("EnemyHealthBarWidgetClass")));
	TestNull("legacy WBP_EnemyHealthBar asset is absent",
		FSoftObjectPath(TEXT("/MordecaiCore/UI/WBP_EnemyHealthBar.WBP_EnemyHealthBar")).TryLoad());

	FRecoveryControlledTestEnv Env;
	if (!TestTrue("controlled player world", Env.SetupWithController()))
	{
		Env.TeardownWithController();
		return true;
	}

	UMordecaiEnemyIndicatorWidget* IndicatorWidget = Env.MakeWidget<UMordecaiEnemyIndicatorWidget>();
	USizeBox* HealthBarSize = IndicatorWidget && IndicatorWidget->WidgetTree
		? IndicatorWidget->WidgetTree->FindWidget<USizeBox>(TEXT("HealthBarSize"))
		: nullptr;
	if (TestNotNull("indicator builds a dedicated health-bar size box", HealthBarSize))
	{
		TestTrue("health-bar width override is set", HealthBarSize->IsWidthOverride());
		TestEqual("health-bar width matches the retired 150px bar", HealthBarSize->GetWidthOverride(), 150.f);
		TestTrue("health-bar height override is set", HealthBarSize->IsHeightOverride());
		TestEqual("health-bar height remains 20px", HealthBarSize->GetHeightOverride(), 20.f);
	}

	UProgressBar* HealthBar = IndicatorWidget && IndicatorWidget->WidgetTree
		? IndicatorWidget->WidgetTree->FindWidget<UProgressBar>(TEXT("HealthProgressBar"))
		: nullptr;
	if (TestNotNull("indicator builds its health progress bar", HealthBar))
	{
		TestEqual("replacement uses the retired bar's exact red", HealthBar->GetFillColorAndOpacity(),
			FLinearColor::Red);
	}

	Env.TeardownWithController();
	return true;
}
