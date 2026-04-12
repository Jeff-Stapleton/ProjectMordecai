// Project Mordecai — Floating Damage Number Tests (US-065)

#include "Misc/AutomationTest.h"
#include "Mordecai/UI/MordecaiDamagePopComponent.h"
#include "Mordecai/MordecaiGameplayTags.h"

// ---------------------------------------------------------------------------
// 1. Mordecai.UI.DamagePop.FiresOnDamage (AC-065.3)
// Requesting a damage pop creates an active pop entry.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_DamagePop_FiresOnDamage,
	"Mordecai.UI.DamagePop.FiresOnDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_DamagePop_FiresOnDamage::RunTest(const FString& Parameters)
{
	UMordecaiDamagePopComponent* Comp = NewObject<UMordecaiDamagePopComponent>();
	TestEqual("No active pops initially", Comp->GetActivePopCount(), 0);

	Comp->RequestDamagePop(25, MordecaiGameplayTags::Damage_Physical, FVector(100.f, 200.f, 50.f), false);

	TestEqual("One active pop after damage request", Comp->GetActivePopCount(), 1);
	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.UI.DamagePop.CorrectDamageAmount (AC-065.3)
// Pop entry contains the correct damage number.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_DamagePop_CorrectDamageAmount,
	"Mordecai.UI.DamagePop.CorrectDamageAmount",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_DamagePop_CorrectDamageAmount::RunTest(const FString& Parameters)
{
	UMordecaiDamagePopComponent* Comp = NewObject<UMordecaiDamagePopComponent>();

	Comp->RequestDamagePop(42, MordecaiGameplayTags::Damage_Fire, FVector::ZeroVector, false);

	TestEqual("Pop count", Comp->GetActivePopCount(), 1);
	const FMordecaiDamagePopEntry& Pop = Comp->GetActivePops()[0];
	TestEqual("Damage amount matches", Pop.DamageAmount, 42);
	TestFalse("Not a healing pop", Pop.bIsHealing);
	TestFalse("Not a critical pop", Pop.bIsCritical);
	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.UI.DamagePop.ColorMatchesPhysical (AC-065.5)
// Physical damage maps to White color.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_DamagePop_ColorMatchesPhysical,
	"Mordecai.UI.DamagePop.ColorMatchesPhysical",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_DamagePop_ColorMatchesPhysical::RunTest(const FString& Parameters)
{
	UMordecaiDamagePopComponent* Comp = NewObject<UMordecaiDamagePopComponent>();

	FLinearColor PhysicalColor = Comp->GetColorForDamageType(MordecaiGameplayTags::Damage_Physical);

	TestEqual("Physical R", PhysicalColor.R, 1.0f);
	TestEqual("Physical G", PhysicalColor.G, 1.0f);
	TestEqual("Physical B", PhysicalColor.B, 1.0f);
	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.UI.DamagePop.ColorMatchesFire (AC-065.5)
// Fire damage maps to Orange color.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_DamagePop_ColorMatchesFire,
	"Mordecai.UI.DamagePop.ColorMatchesFire",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_DamagePop_ColorMatchesFire::RunTest(const FString& Parameters)
{
	UMordecaiDamagePopComponent* Comp = NewObject<UMordecaiDamagePopComponent>();

	FLinearColor FireColor = Comp->GetColorForDamageType(MordecaiGameplayTags::Damage_Fire);

	TestEqual("Fire R", FireColor.R, 1.0f);
	TestEqual("Fire G", FireColor.G, 0.5f);
	TestEqual("Fire B", FireColor.B, 0.0f);
	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.UI.DamagePop.ColorMatchesFrost (AC-065.5)
// Frost damage maps to Cyan color.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_DamagePop_ColorMatchesFrost,
	"Mordecai.UI.DamagePop.ColorMatchesFrost",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_DamagePop_ColorMatchesFrost::RunTest(const FString& Parameters)
{
	UMordecaiDamagePopComponent* Comp = NewObject<UMordecaiDamagePopComponent>();

	FLinearColor FrostColor = Comp->GetColorForDamageType(MordecaiGameplayTags::Damage_Frost);

	TestEqual("Frost R", FrostColor.R, 0.3f);
	TestEqual("Frost G", FrostColor.G, 0.8f);
	TestEqual("Frost B", FrostColor.B, 1.0f);
	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.UI.DamagePop.CustomColorOverride (AC-065.6)
// Setting a custom color in the map overrides the default.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_DamagePop_CustomColorOverride,
	"Mordecai.UI.DamagePop.CustomColorOverride",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_DamagePop_CustomColorOverride::RunTest(const FString& Parameters)
{
	UMordecaiDamagePopComponent* Comp = NewObject<UMordecaiDamagePopComponent>();

	// Override fire color to bright red
	FLinearColor CustomRed(1.0f, 0.0f, 0.0f, 1.0f);
	Comp->DamageTypeColors.Add(MordecaiGameplayTags::Damage_Fire, CustomRed);

	FLinearColor Result = Comp->GetColorForDamageType(MordecaiGameplayTags::Damage_Fire);

	TestEqual("Custom override R", Result.R, 1.0f);
	TestEqual("Custom override G", Result.G, 0.0f);
	TestEqual("Custom override B", Result.B, 0.0f);
	return true;
}

// ---------------------------------------------------------------------------
// 7. Mordecai.UI.DamagePop.CriticalHitScalesUp (AC-065.7)
// Critical hit flag produces pop at CriticalHitScale.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_DamagePop_CriticalHitScalesUp,
	"Mordecai.UI.DamagePop.CriticalHitScalesUp",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_DamagePop_CriticalHitScalesUp::RunTest(const FString& Parameters)
{
	UMordecaiDamagePopComponent* Comp = NewObject<UMordecaiDamagePopComponent>();
	Comp->PopScale = 1.0f;
	Comp->CriticalHitScale = 1.5f;

	// Normal hit
	Comp->RequestDamagePop(10, MordecaiGameplayTags::Damage_Physical, FVector::ZeroVector, false);
	TestEqual("Normal hit scale", Comp->GetActivePops()[0].Scale, 1.0f);

	// Critical hit
	Comp->RequestDamagePop(20, MordecaiGameplayTags::Damage_Physical, FVector::ZeroVector, true);
	TestEqual("Critical hit scale", Comp->GetActivePops()[1].Scale, 1.5f);

	return true;
}

// ---------------------------------------------------------------------------
// 8. Mordecai.UI.DamagePop.HealingShowsGreen (AC-065.8)
// Healing events produce green pops with positive display flag.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_DamagePop_HealingShowsGreen,
	"Mordecai.UI.DamagePop.HealingShowsGreen",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_DamagePop_HealingShowsGreen::RunTest(const FString& Parameters)
{
	UMordecaiDamagePopComponent* Comp = NewObject<UMordecaiDamagePopComponent>();

	Comp->RequestHealingPop(15, FVector(0.f, 0.f, 100.f));

	TestEqual("One active pop", Comp->GetActivePopCount(), 1);
	const FMordecaiDamagePopEntry& Pop = Comp->GetActivePops()[0];
	TestTrue("Marked as healing", Pop.bIsHealing);
	TestEqual("Heal amount is positive", Pop.DamageAmount, 15);
	TestEqual("Healing color R", Pop.Color.R, Comp->HealingColor.R);
	TestEqual("Healing color G", Pop.Color.G, Comp->HealingColor.G);
	TestEqual("Healing color B", Pop.Color.B, Comp->HealingColor.B);
	return true;
}

// ---------------------------------------------------------------------------
// 9. Mordecai.UI.DamagePop.ScatterOffset (AC-065.9)
// Multiple pops at the same target have different world locations.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_DamagePop_ScatterOffset,
	"Mordecai.UI.DamagePop.ScatterOffset",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_DamagePop_ScatterOffset::RunTest(const FString& Parameters)
{
	UMordecaiDamagePopComponent* Comp = NewObject<UMordecaiDamagePopComponent>();
	Comp->PopScatterRadius = 30.f;

	const FVector TargetLocation(500.f, 500.f, 100.f);

	// Create many pops at the same base location
	constexpr int32 PopCount = 20;
	for (int32 i = 0; i < PopCount; ++i)
	{
		Comp->RequestDamagePop(10, MordecaiGameplayTags::Damage_Physical, TargetLocation, false);
	}

	TestEqual("All pops created", Comp->GetActivePopCount(), PopCount);

	// At least some pops should have different locations due to scatter
	bool bFoundDifferentLocation = false;
	const FVector& FirstLoc = Comp->GetActivePops()[0].WorldLocation;
	for (int32 i = 1; i < PopCount; ++i)
	{
		if (!Comp->GetActivePops()[i].WorldLocation.Equals(FirstLoc, 0.01f))
		{
			bFoundDifferentLocation = true;
			break;
		}
	}
	TestTrue("Multiple pops have different locations due to scatter", bFoundDifferentLocation);

	// All pop locations should be within scatter radius of original
	for (int32 i = 0; i < PopCount; ++i)
	{
		float Dist2D = FVector::Dist2D(Comp->GetActivePops()[i].WorldLocation, TargetLocation);
		TestTrue(FString::Printf(TEXT("Pop %d within scatter radius (dist=%f)"), i, Dist2D),
			Dist2D <= Comp->PopScatterRadius + 0.01f);
	}

	return true;
}

// ---------------------------------------------------------------------------
// 10. Mordecai.UI.DamagePop.MaxConcurrentRecycles (AC-065.10)
// Exceeding MaxConcurrentPops recycles the oldest pop.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_DamagePop_MaxConcurrentRecycles,
	"Mordecai.UI.DamagePop.MaxConcurrentRecycles",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_DamagePop_MaxConcurrentRecycles::RunTest(const FString& Parameters)
{
	UMordecaiDamagePopComponent* Comp = NewObject<UMordecaiDamagePopComponent>();
	Comp->MaxConcurrentPops = 3;

	// Fill up the pool
	Comp->RequestDamagePop(10, MordecaiGameplayTags::Damage_Physical, FVector::ZeroVector, false);
	Comp->RequestDamagePop(20, MordecaiGameplayTags::Damage_Fire, FVector::ZeroVector, false);
	Comp->RequestDamagePop(30, MordecaiGameplayTags::Damage_Frost, FVector::ZeroVector, false);
	TestEqual("Pool at max capacity", Comp->GetActivePopCount(), 3);

	// Adding one more should recycle oldest (10 Physical)
	Comp->RequestDamagePop(40, MordecaiGameplayTags::Damage_Lightning, FVector::ZeroVector, false);
	TestEqual("Pool still at max after recycle", Comp->GetActivePopCount(), 3);

	// Verify the oldest (10) was removed and the newest (40) is present
	const TArray<FMordecaiDamagePopEntry>& Pops = Comp->GetActivePops();
	bool bFoundOldest = false;
	bool bFoundNewest = false;
	for (const FMordecaiDamagePopEntry& Pop : Pops)
	{
		if (Pop.DamageAmount == 10) bFoundOldest = true;
		if (Pop.DamageAmount == 40) bFoundNewest = true;
	}
	TestFalse("Oldest pop (10) was recycled", bFoundOldest);
	TestTrue("Newest pop (40) is present", bFoundNewest);

	return true;
}
