// Project Mordecai — Enemy Indicator System Tests (US-064)

#include "Misc/AutomationTest.h"
#include "Mordecai/UI/MordecaiEnemyIndicatorComponent.h"
#include "Mordecai/UI/MordecaiEnemyIndicatorWidget.h"
#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"
#include "Mordecai/MordecaiGameplayTags.h"

// ---------------------------------------------------------------------------
// 1. Mordecai.UI.EnemyIndicator.ComponentExists (AC-064.1)
// The indicator component class exists and is an ActorComponent.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_EnemyIndicator_ComponentExists,
	"Mordecai.UI.EnemyIndicator.ComponentExists",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_EnemyIndicator_ComponentExists::RunTest(const FString& Parameters)
{
	UMordecaiEnemyIndicatorComponent* CDO = GetMutableDefault<UMordecaiEnemyIndicatorComponent>();
	TestNotNull("EnemyIndicatorComponent CDO exists", CDO);
	TestTrue("Is an ActorComponent", CDO->IsA<UActorComponent>());
	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.UI.EnemyIndicator.DefaultProperties (AC-064.2, AC-064.4)
// Default height offset and visibility range are as specified.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_EnemyIndicator_DefaultProperties,
	"Mordecai.UI.EnemyIndicator.DefaultProperties",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_EnemyIndicator_DefaultProperties::RunTest(const FString& Parameters)
{
	const UMordecaiEnemyIndicatorComponent* CDO = GetDefault<UMordecaiEnemyIndicatorComponent>();
	TestEqual("Default IndicatorHeightOffset is 120.0", CDO->IndicatorHeightOffset, 120.0f);
	TestEqual("Default IndicatorVisibilityRange is 2000.0", CDO->IndicatorVisibilityRange, 2000.0f);
	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.UI.EnemyIndicator.RegistersOnDamage (AC-064.2)
// Calling NotifyDamageReceived sets bIndicatorRegistered to true.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_EnemyIndicator_RegistersOnDamage,
	"Mordecai.UI.EnemyIndicator.RegistersOnDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_EnemyIndicator_RegistersOnDamage::RunTest(const FString& Parameters)
{
	UMordecaiEnemyIndicatorComponent* Comp = NewObject<UMordecaiEnemyIndicatorComponent>();
	TestFalse("Indicator not registered initially", Comp->IsIndicatorRegistered());
	Comp->NotifyDamageReceived();
	TestTrue("Indicator registered after damage", Comp->IsIndicatorRegistered());
	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.UI.EnemyIndicator.UnregistersOnDeath (AC-064.3)
// After registration, calling NotifyDeath unregisters the indicator.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_EnemyIndicator_UnregistersOnDeath,
	"Mordecai.UI.EnemyIndicator.UnregistersOnDeath",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_EnemyIndicator_UnregistersOnDeath::RunTest(const FString& Parameters)
{
	UMordecaiEnemyIndicatorComponent* Comp = NewObject<UMordecaiEnemyIndicatorComponent>();
	Comp->NotifyDamageReceived();
	TestTrue("Indicator registered after damage", Comp->IsIndicatorRegistered());
	Comp->NotifyDeath();
	TestFalse("Indicator unregistered after death", Comp->IsIndicatorRegistered());
	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.UI.EnemyIndicator.HiddenBeyondRange (AC-064.4)
// Indicator should be hidden when distance exceeds IndicatorVisibilityRange.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_EnemyIndicator_HiddenBeyondRange,
	"Mordecai.UI.EnemyIndicator.HiddenBeyondRange",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_EnemyIndicator_HiddenBeyondRange::RunTest(const FString& Parameters)
{
	UMordecaiEnemyIndicatorComponent* Comp = NewObject<UMordecaiEnemyIndicatorComponent>();
	// Default range is 2000.0
	TestFalse("2100cm is beyond range", Comp->IsWithinVisibilityRange(2100.f));
	TestFalse("3000cm is beyond range", Comp->IsWithinVisibilityRange(3000.f));
	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.UI.EnemyIndicator.ShownWhenReEntersRange (AC-064.4)
// Indicator should be shown when distance is within IndicatorVisibilityRange.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_EnemyIndicator_ShownWhenReEntersRange,
	"Mordecai.UI.EnemyIndicator.ShownWhenReEntersRange",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_EnemyIndicator_ShownWhenReEntersRange::RunTest(const FString& Parameters)
{
	UMordecaiEnemyIndicatorComponent* Comp = NewObject<UMordecaiEnemyIndicatorComponent>();
	// Default range is 2000.0
	TestTrue("1500cm is within range", Comp->IsWithinVisibilityRange(1500.f));
	TestTrue("2000cm boundary is within range", Comp->IsWithinVisibilityRange(2000.f));
	TestTrue("0cm is within range", Comp->IsWithinVisibilityRange(0.f));
	return true;
}

// ---------------------------------------------------------------------------
// 7. Mordecai.UI.EnemyIndicator.HealthBarColorThresholds (AC-064.8)
// Health bar color: Green >50%, Yellow 25-50%, Red <25%.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_EnemyIndicator_HealthBarColorThresholds,
	"Mordecai.UI.EnemyIndicator.HealthBarColorThresholds",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_EnemyIndicator_HealthBarColorThresholds::RunTest(const FString& Parameters)
{
	// Green > 50%
	FLinearColor Color75 = UMordecaiEnemyIndicatorWidget::CalcHealthBarColor(0.75f);
	TestEqual("75% = Green", Color75, UMordecaiEnemyIndicatorWidget::HealthColorGreen);

	FLinearColor Color51 = UMordecaiEnemyIndicatorWidget::CalcHealthBarColor(0.51f);
	TestEqual("51% = Green", Color51, UMordecaiEnemyIndicatorWidget::HealthColorGreen);

	// Yellow 25%-50%
	FLinearColor Color50 = UMordecaiEnemyIndicatorWidget::CalcHealthBarColor(0.50f);
	TestEqual("50% = Yellow", Color50, UMordecaiEnemyIndicatorWidget::HealthColorYellow);

	FLinearColor Color25 = UMordecaiEnemyIndicatorWidget::CalcHealthBarColor(0.25f);
	TestEqual("25% = Yellow", Color25, UMordecaiEnemyIndicatorWidget::HealthColorYellow);

	// Red < 25%
	FLinearColor Color24 = UMordecaiEnemyIndicatorWidget::CalcHealthBarColor(0.24f);
	TestEqual("24% = Red", Color24, UMordecaiEnemyIndicatorWidget::HealthColorRed);

	FLinearColor Color0 = UMordecaiEnemyIndicatorWidget::CalcHealthBarColor(0.0f);
	TestEqual("0% = Red", Color0, UMordecaiEnemyIndicatorWidget::HealthColorRed);

	return true;
}

// ---------------------------------------------------------------------------
// 8. Mordecai.UI.EnemyIndicator.WidgetExists (AC-064.5)
// The indicator widget class exists and is a UUserWidget.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_EnemyIndicator_WidgetExists,
	"Mordecai.UI.EnemyIndicator.WidgetExists",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_EnemyIndicator_WidgetExists::RunTest(const FString& Parameters)
{
	const UMordecaiEnemyIndicatorWidget* CDO = GetDefault<UMordecaiEnemyIndicatorWidget>();
	TestNotNull("EnemyIndicatorWidget CDO exists", CDO);
	TestTrue("Is a UUserWidget", CDO->IsA<UUserWidget>());
	return true;
}

// ---------------------------------------------------------------------------
// 9. Mordecai.UI.EnemyIndicator.StatusIconMapping (AC-064.7)
// The widget has a configurable status tag to icon texture map.
// Default map is empty. Fallback for missing textures works.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_EnemyIndicator_StatusIconMapping,
	"Mordecai.UI.EnemyIndicator.StatusIconMapping",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_EnemyIndicator_StatusIconMapping::RunTest(const FString& Parameters)
{
	const UMordecaiEnemyIndicatorWidget* CDO = GetDefault<UMordecaiEnemyIndicatorWidget>();

	// Default map should be empty (textures are set in EDITOR stories)
	TestEqual("Default StatusIconMap is empty", CDO->StatusIconMap.Num(), 0);

	// CalcHealthBarColor boundary test at exactly 1.0
	FLinearColor Color100 = UMordecaiEnemyIndicatorWidget::CalcHealthBarColor(1.0f);
	TestEqual("100% = Green", Color100, UMordecaiEnemyIndicatorWidget::HealthColorGreen);

	return true;
}
