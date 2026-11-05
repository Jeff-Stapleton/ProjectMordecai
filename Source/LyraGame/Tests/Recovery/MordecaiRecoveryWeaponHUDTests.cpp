// Project Mordecai — Recovery: the equipped weapon plate renders and belongs to the CommonUI HUD root (US-091).

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "CommonInputModeTypes.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/UI/MordecaiCombatHUDWidget.h"
#include "Mordecai/UI/MordecaiEquippedWeaponWidget.h"
#include "Mordecai/Weapons/MordecaiEquipmentComponent.h"
#include "Mordecai/Weapons/MordecaiWeaponProfileFactory.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"
#include "UI/LyraActivatableWidget.h"
#include "UObject/UnrealType.h"

namespace MordecaiRecoveryWeaponHUDTest
{
	UTextBlock* Text(const UUserWidget* Widget, const TCHAR* Name)
	{
		return Widget && Widget->WidgetTree
			? Widget->WidgetTree->FindWidget<UTextBlock>(Name)
			: nullptr;
	}

	FString DrawnText(const UUserWidget* Widget, const TCHAR* Name)
	{
		const UTextBlock* Block = Text(Widget, Name);
		return Block ? Block->GetText().ToString() : FString(TEXT("<missing>"));
	}

	bool HasExactRenderedText(const UUserWidget* Widget, const FString& Expected)
	{
		bool bFound = false;
		if (Widget && Widget->WidgetTree)
		{
			Widget->WidgetTree->ForEachWidget([&bFound, &Expected](UWidget* Child)
			{
				if (const UTextBlock* Block = Cast<UTextBlock>(Child))
				{
					bFound |= Block->GetText().ToString().Equals(Expected, ESearchCase::CaseSensitive);
				}
			});
		}
		return bFound;
	}

	UMordecaiEquippedWeaponWidget* FindWeaponPlate(const UUserWidget* Widget)
	{
		UMordecaiEquippedWeaponWidget* Plate = nullptr;
		if (Widget && Widget->WidgetTree)
		{
			Widget->WidgetTree->ForEachWidget([&Plate](UWidget* Child)
			{
				if (!Plate)
				{
					Plate = Cast<UMordecaiEquippedWeaponWidget>(Child);
				}
			});
		}
		return Plate;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_WeaponHUD_BuildsDefaultContent,
	"Mordecai.Recovery.WeaponHUD.BuildsDefaultContent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_WeaponHUD_BuildsDefaultContent::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryTest;
	using namespace MordecaiRecoveryWeaponHUDTest;

	FRecoveryControlledTestEnv Env;
	if (!TestTrue("controlled player world", Env.SetupWithController()))
	{
		Env.TeardownWithController();
		return true;
	}

	UMordecaiEquipmentComponent* Equipment = NewObject<UMordecaiEquipmentComponent>();
	UMordecaiEquippedWeaponWidget* Widget = Env.MakeWidget<UMordecaiEquippedWeaponWidget>();
	Widget->BindToEquipmentComponent(Equipment);

	TestTrue("native widget builds a root", Widget->WidgetTree && Widget->WidgetTree->RootWidget);
	TestNotNull("WeaponNameText", Text(Widget, TEXT("WeaponNameText")));
	TestNotNull("DamageSubtypeText", Text(Widget, TEXT("DamageSubtypeText")));
	TestNotNull("SwapKey", Text(Widget, TEXT("SwapKey")));
	TestEqualSensitive("unarmed name", DrawnText(Widget, TEXT("WeaponNameText")), FString(TEXT("Unarmed")));
	TestEqualSensitive("unarmed subtype", DrawnText(Widget, TEXT("DamageSubtypeText")), FString(TEXT("Blunt")));
	TestEqualSensitive("swap key", DrawnText(Widget, TEXT("SwapKey")), FString(TEXT("Tab")));

	Widget->UnbindFromEquipmentComponent();
	Env.TeardownWithController();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_WeaponHUD_UpdatesOnWeaponChanged,
	"Mordecai.Recovery.WeaponHUD.UpdatesOnWeaponChanged",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_WeaponHUD_UpdatesOnWeaponChanged::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryTest;
	using namespace MordecaiRecoveryWeaponHUDTest;

	FRecoveryControlledTestEnv Env;
	if (!TestTrue("controlled player world", Env.SetupWithController()))
	{
		Env.TeardownWithController();
		return true;
	}

	UObject* Outer = GetTransientPackage();
	UMordecaiEquipmentComponent* Equipment = NewObject<UMordecaiEquipmentComponent>();
	Equipment->AddAvailableWeapon(UMordecaiWeaponProfileFactory::CreateLongsword(Outer));
	Equipment->AddAvailableWeapon(UMordecaiWeaponProfileFactory::CreateMace(Outer));
	Equipment->CycleNextWeapon();

	UMordecaiEquippedWeaponWidget* Widget = Env.MakeWidget<UMordecaiEquippedWeaponWidget>();
	Widget->BindToEquipmentComponent(Equipment);
	TestEqualSensitive("Longsword name", DrawnText(Widget, TEXT("WeaponNameText")), FString(TEXT("Longsword")));
	TestEqualSensitive("Longsword subtype", DrawnText(Widget, TEXT("DamageSubtypeText")), FString(TEXT("Slash")));

	Equipment->CycleNextWeapon();
	TestEqualSensitive("Mace name after OnWeaponChanged", DrawnText(Widget, TEXT("WeaponNameText")), FString(TEXT("Mace")));
	TestEqualSensitive("Mace subtype after OnWeaponChanged", DrawnText(Widget, TEXT("DamageSubtypeText")), FString(TEXT("Blunt")));
	TestFalse("no stale Longsword text", HasExactRenderedText(Widget, TEXT("Longsword")));

	Widget->UnbindFromEquipmentComponent();
	Env.TeardownWithController();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_WeaponHUD_IsCombatHUDChildAtMapBox,
	"Mordecai.Recovery.WeaponHUD.IsCombatHUDChildAtMapBox",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_WeaponHUD_IsCombatHUDChildAtMapBox::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryTest;
	using namespace MordecaiRecoveryWeaponHUDTest;

	FRecoveryControlledTestEnv Env;
	if (!TestTrue("controlled player world", Env.SetupWithController()))
	{
		Env.TeardownWithController();
		return true;
	}

	UMordecaiCombatHUDWidget* HUD = Env.MakeWidget<UMordecaiCombatHUDWidget>();
	TestTrue("combat HUD is a Lyra activatable widget", HUD->IsA(ULyraActivatableWidget::StaticClass()));

	UMordecaiEquippedWeaponWidget* Plate = FindWeaponPlate(HUD);
	TestNotNull("one equipped weapon plate is a child of the HUD root", Plate);
	const UCanvasPanelSlot* Slot = Plate ? Cast<UCanvasPanelSlot>(Plate->Slot) : nullptr;
	if (TestNotNull("weapon plate has a canvas slot", Slot))
	{
		TestEqual("bottom-right minimum anchor", Slot->GetAnchors().Minimum, FVector2D(1.f, 1.f));
		TestEqual("bottom-right maximum anchor", Slot->GetAnchors().Maximum, FVector2D(1.f, 1.f));
		TestEqual("offset", Slot->GetPosition(), FVector2D(-32.f, -32.f));
		TestEqual("alignment", Slot->GetAlignment(), FVector2D(1.f, 1.f));
		TestEqual("size", Slot->GetSize(), FVector2D(340.f, 84.f));
		TestFalse("fixed size, not auto-size", Slot->GetAutoSize());
	}

	Env.TeardownWithController();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_WeaponHUD_CharacterOwnsNoStandaloneWidget,
	"Mordecai.Recovery.WeaponHUD.CharacterOwnsNoStandaloneWidget",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_WeaponHUD_CharacterOwnsNoStandaloneWidget::RunTest(const FString& Parameters)
{
	TestNull("the character no longer owns a separately-created weapon widget",
		FindFProperty<FObjectPropertyBase>(AMordecaiCharacter::StaticClass(), TEXT("EquippedWeaponWidget")));
	TestNull("the character no longer owns a separate weapon widget class",
		FindFProperty<FObjectPropertyBase>(AMordecaiCharacter::StaticClass(), TEXT("EquippedWeaponWidgetClass")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_WeaponHUD_PassesGameplayInput,
	"Mordecai.Recovery.WeaponHUD.PassesGameplayInput",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_WeaponHUD_PassesGameplayInput::RunTest(const FString& Parameters)
{
	const UMordecaiCombatHUDWidget* HUD = GetDefault<UMordecaiCombatHUDWidget>();
	const TOptional<FUIInputConfig> Config = HUD->GetDesiredInputConfig();
	if (TestTrue("the active combat HUD supplies an explicit input config", Config.IsSet()))
	{
		TestTrue("combat HUD leaves gameplay input enabled", Config->GetInputMode() == ECommonInputMode::Game);
		TestTrue("combat HUD preserves the project's initial-click capture mode",
			Config->GetMouseCaptureMode() == EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_WeaponHUD_HidesBehindModalLayer,
	"Mordecai.Recovery.WeaponHUD.HidesBehindModalLayer",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_WeaponHUD_HidesBehindModalLayer::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryTest;

	FRecoveryControlledTestEnv Env;
	if (!TestTrue("controlled player world", Env.SetupWithController()))
	{
		Env.TeardownWithController();
		return true;
	}

	UMordecaiCombatHUDWidget* HUD = Env.MakeWidget<UMordecaiCombatHUDWidget>();
	HUD->ActivateWidget();
	HUD->DeactivateWidget();
	TestTrue("deactivated HUD is collapsed behind the death screen",
		HUD->GetVisibility() == ESlateVisibility::Collapsed);
	HUD->ActivateWidget();
	TestTrue("reactivated HUD is visible after respawn",
		HUD->GetVisibility() == ESlateVisibility::SelfHitTestInvisible);

	Env.TeardownWithController();
	return true;
}
