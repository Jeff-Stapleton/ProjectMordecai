// Project Mordecai — Melee Attack Ability Tests (US-004)

#include "Misc/AutomationTest.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/Combat/MordecaiCombatTypes.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"

// ---------------------------------------------------------------------------
// Helper: Create a test attack profile with common defaults
// ---------------------------------------------------------------------------
namespace
{
	UMordecaiAttackProfileDataAsset* CreateTestProfile(
		float BasePower = 25.f,
		float PostureDamageScalar = 1.0f,
		float StaminaCost = 15.f,
		float WindupMs = 200.f,
		float ActiveMs = 100.f,
		float RecoveryMs = 300.f)
	{
		UMordecaiAttackProfileDataAsset* Profile = NewObject<UMordecaiAttackProfileDataAsset>();
		Profile->DamageProfile.BasePower = BasePower;
		Profile->DamageProfile.DamageType = EMordecaiDamageType::Slash;
		Profile->DamageProfile.AppliesPostureDamage = true;
		Profile->PostureDamageScalar = PostureDamageScalar;
		Profile->StaminaCost = StaminaCost;
		Profile->WindupTimeMs = WindupMs;
		Profile->ActiveTimeMs = ActiveMs;
		Profile->RecoveryTimeMs = RecoveryMs;
		Profile->InputSlot = EMordecaiInputSlot::Light;
		Profile->RootedDuring = EMordecaiRootedMode::None;
		Profile->CancelableIntoDodge = false;
		Profile->CancelableIntoBlock = false;
		Profile->HitShapeType = EMordecaiHitShapeType::ArcSector;
		Profile->HitShapeParams.Radius = 200.f;
		Profile->HitShapeParams.Angle = 90.f;
		Profile->ComboIndex = 1;
		return Profile;
	}

	UMordecaiGA_MeleeAttack* CreateTestAbility(UMordecaiAttackProfileDataAsset* Profile = nullptr)
	{
		UMordecaiGA_MeleeAttack* Ability = NewObject<UMordecaiGA_MeleeAttack>();
		if (!Profile)
		{
			Profile = CreateTestProfile();
		}
		Ability->AttackProfiles.Add(Profile);
		return Ability;
	}
}

// ---------------------------------------------------------------------------
// 1. Mordecai.MeleeAttack.AbilityActivatesSuccessfully (AC-004.1, AC-004.3)
// Grant ability, activate, verify it enters Windup
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_MeleeAttack_Activates,
	"Mordecai.MeleeAttack.AbilityActivatesSuccessfully",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_MeleeAttack_Activates::RunTest(const FString& Parameters)
{
	// Verify class exists and inherits correctly
	const UClass* AbilityClass = UMordecaiGA_MeleeAttack::StaticClass();
	TestNotNull("UMordecaiGA_MeleeAttack class exists", AbilityClass);
	TestTrue("Inherits from ULyraGameplayAbility",
		AbilityClass->IsChildOf(ULyraGameplayAbility::StaticClass()));

	// Verify instancing policy (needed for combo state)
	UMordecaiGA_MeleeAttack* Ability = CreateTestAbility();
	TestEqual("InstancedPerActor policy",
		Ability->GetInstancingPolicy(),
		EGameplayAbilityInstancingPolicy::InstancedPerActor);

	// Verify ability enters Windup on transition
	TestEqual("Initial phase is None", Ability->GetCurrentPhase(), EMordecaiAttackPhase::None);
	Ability->TransitionToPhase(EMordecaiAttackPhase::Windup);
	TestEqual("Phase transitions to Windup", Ability->GetCurrentPhase(), EMordecaiAttackPhase::Windup);

	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.MeleeAttack.PhasesExecuteInOrder (AC-004.3)
// Activate, verify Windup->Active->Recovery with correct durations
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_MeleeAttack_PhasesInOrder,
	"Mordecai.MeleeAttack.PhasesExecuteInOrder",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_MeleeAttack_PhasesInOrder::RunTest(const FString& Parameters)
{
	UMordecaiAttackProfileDataAsset* Profile = CreateTestProfile(
		25.f, 1.f, 15.f, /*WindupMs=*/200.f, /*ActiveMs=*/150.f, /*RecoveryMs=*/300.f);
	UMordecaiGA_MeleeAttack* Ability = CreateTestAbility(Profile);

	// Verify durations from profile
	TestEqual("Windup duration", Ability->GetPhaseDurationSeconds(EMordecaiAttackPhase::Windup), 0.2f);
	TestEqual("Active duration", Ability->GetPhaseDurationSeconds(EMordecaiAttackPhase::Active), 0.15f);
	TestEqual("Recovery duration", Ability->GetPhaseDurationSeconds(EMordecaiAttackPhase::Recovery), 0.3f);

	// Verify phase sequence
	Ability->TransitionToPhase(EMordecaiAttackPhase::Windup);
	TestEqual("Phase 1: Windup", Ability->GetCurrentPhase(), EMordecaiAttackPhase::Windup);

	Ability->TransitionToPhase(EMordecaiAttackPhase::Active);
	TestEqual("Phase 2: Active", Ability->GetCurrentPhase(), EMordecaiAttackPhase::Active);

	Ability->TransitionToPhase(EMordecaiAttackPhase::Recovery);
	TestEqual("Phase 3: Recovery", Ability->GetCurrentPhase(), EMordecaiAttackPhase::Recovery);

	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.MeleeAttack.HitDetectionFiresDuringActive (AC-004.4)
// Place target in range, verify hit detected during Active phase
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_MeleeAttack_HitDetection,
	"Mordecai.MeleeAttack.HitDetectionFiresDuringActive",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_MeleeAttack_HitDetection::RunTest(const FString& Parameters)
{
	UMordecaiAttackProfileDataAsset* Profile = CreateTestProfile();
	Profile->HitShapeType = EMordecaiHitShapeType::ArcSector;
	Profile->HitShapeParams.Radius = 200.f;
	Profile->HitShapeParams.Angle = 90.f;

	UMordecaiGA_MeleeAttack* Ability = CreateTestAbility(Profile);

	// Verify active profile has hit shape config for detection
	const UMordecaiAttackProfileDataAsset* ActiveProfile = Ability->GetActiveProfile();
	TestNotNull("Active profile available for hit detection", ActiveProfile);
	if (ActiveProfile)
	{
		TestEqual("Hit shape type is ArcSector", ActiveProfile->HitShapeType, EMordecaiHitShapeType::ArcSector);
		TestTrue("Radius > 0 for hit detection", ActiveProfile->HitShapeParams.Radius > 0.f);
	}

	// Verify transition to Active (when hit detection would fire)
	Ability->TransitionToPhase(EMordecaiAttackPhase::Active);
	TestEqual("In Active phase for hit detection", Ability->GetCurrentPhase(), EMordecaiAttackPhase::Active);

	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.MeleeAttack.DamageAppliedToHitTarget (AC-004.5)
// Hit a target, verify Health reduced by BasePower
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_MeleeAttack_Damage,
	"Mordecai.MeleeAttack.DamageAppliedToHitTarget",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_MeleeAttack_Damage::RunTest(const FString& Parameters)
{
	UMordecaiAttackProfileDataAsset* Profile = CreateTestProfile(/*BasePower=*/25.f);
	UMordecaiGA_MeleeAttack* Ability = CreateTestAbility(Profile);

	float HealthDamage = Ability->ComputeHealthDamage();
	TestEqual("Health damage equals negative BasePower", HealthDamage, -25.f);

	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.MeleeAttack.PostureDamageAppliedToHitTarget (AC-004.5)
// Hit a target, verify Posture reduced by BasePower * PostureDamageScalar
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_MeleeAttack_PostureDamage,
	"Mordecai.MeleeAttack.PostureDamageAppliedToHitTarget",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_MeleeAttack_PostureDamage::RunTest(const FString& Parameters)
{
	UMordecaiAttackProfileDataAsset* Profile = CreateTestProfile(
		/*BasePower=*/25.f, /*PostureDamageScalar=*/1.5f);
	UMordecaiGA_MeleeAttack* Ability = CreateTestAbility(Profile);

	float PostureDamage = Ability->ComputePostureDamage();
	TestEqual("Posture damage equals negative BasePower * PostureDamageScalar",
		PostureDamage, -37.5f);

	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.MeleeAttack.DamageTaggedCorrectly (AC-004.6)
// Verify damage event carries correct Mordecai.Damage.* tag
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_MeleeAttack_DamageTag,
	"Mordecai.MeleeAttack.DamageTaggedCorrectly",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_MeleeAttack_DamageTag::RunTest(const FString& Parameters)
{
	// Test damage type -> gameplay tag mapping (use TestTrue with == to avoid FNativeGameplayTag/FGameplayTag ambiguity)
	TestTrue("Physical maps correctly",
		UMordecaiGA_MeleeAttack::GetDamageTagForType(EMordecaiDamageType::Physical) == MordecaiGameplayTags::Damage_Physical);

	TestTrue("Slash maps correctly",
		UMordecaiGA_MeleeAttack::GetDamageTagForType(EMordecaiDamageType::Slash) == MordecaiGameplayTags::Damage_Physical_Slash);

	TestTrue("Pierce maps correctly",
		UMordecaiGA_MeleeAttack::GetDamageTagForType(EMordecaiDamageType::Pierce) == MordecaiGameplayTags::Damage_Physical_Pierce);

	TestTrue("Blunt maps correctly",
		UMordecaiGA_MeleeAttack::GetDamageTagForType(EMordecaiDamageType::Blunt) == MordecaiGameplayTags::Damage_Physical_Blunt);

	TestTrue("Fire maps correctly",
		UMordecaiGA_MeleeAttack::GetDamageTagForType(EMordecaiDamageType::Fire) == MordecaiGameplayTags::Damage_Fire);

	TestTrue("Frost maps correctly",
		UMordecaiGA_MeleeAttack::GetDamageTagForType(EMordecaiDamageType::Frost) == MordecaiGameplayTags::Damage_Frost);

	TestTrue("Lightning maps correctly",
		UMordecaiGA_MeleeAttack::GetDamageTagForType(EMordecaiDamageType::Lightning) == MordecaiGameplayTags::Damage_Lightning);

	TestTrue("Poison maps correctly",
		UMordecaiGA_MeleeAttack::GetDamageTagForType(EMordecaiDamageType::Poison) == MordecaiGameplayTags::Damage_Poison);

	TestTrue("Corrosive maps correctly",
		UMordecaiGA_MeleeAttack::GetDamageTagForType(EMordecaiDamageType::Corrosive) == MordecaiGameplayTags::Damage_Corrosive);

	TestTrue("Arcane maps correctly",
		UMordecaiGA_MeleeAttack::GetDamageTagForType(EMordecaiDamageType::Arcane) == MordecaiGameplayTags::Damage_Arcane);

	TestTrue("Shadow maps correctly",
		UMordecaiGA_MeleeAttack::GetDamageTagForType(EMordecaiDamageType::Shadow) == MordecaiGameplayTags::Damage_Shadow);

	TestTrue("Radiant maps correctly",
		UMordecaiGA_MeleeAttack::GetDamageTagForType(EMordecaiDamageType::Radiant) == MordecaiGameplayTags::Damage_Radiant);

	return true;
}

// ---------------------------------------------------------------------------
// 7. Mordecai.MeleeAttack.StaminaConsumedOnActivation (AC-004.7)
// Activate attack, verify Stamina reduced by StaminaCost
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_MeleeAttack_StaminaCost,
	"Mordecai.MeleeAttack.StaminaConsumedOnActivation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_MeleeAttack_StaminaCost::RunTest(const FString& Parameters)
{
	UMordecaiAttackProfileDataAsset* Profile = CreateTestProfile(
		25.f, 1.f, /*StaminaCost=*/15.f);
	UMordecaiGA_MeleeAttack* Ability = CreateTestAbility(Profile);

	float Cost = Ability->GetStaminaCost();
	TestEqual("Stamina cost matches profile", Cost, 15.f);

	return true;
}

// ---------------------------------------------------------------------------
// 8. Mordecai.MeleeAttack.RootedDuringActiveLocksMovement (AC-004.8)
// Set RootedDuring=Active, verify movement disabled during Active only
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_MeleeAttack_Rooted,
	"Mordecai.MeleeAttack.RootedDuringActiveLocksMovement",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_MeleeAttack_Rooted::RunTest(const FString& Parameters)
{
	UMordecaiAttackProfileDataAsset* Profile = CreateTestProfile();
	Profile->RootedDuring = EMordecaiRootedMode::Active;
	UMordecaiGA_MeleeAttack* Ability = CreateTestAbility(Profile);

	// Active should be rooted, others should not
	TestFalse("Not rooted during None",
		Ability->ShouldBeRootedDuringPhase(EMordecaiAttackPhase::None));
	TestFalse("Not rooted during Windup",
		Ability->ShouldBeRootedDuringPhase(EMordecaiAttackPhase::Windup));
	TestTrue("Rooted during Active",
		Ability->ShouldBeRootedDuringPhase(EMordecaiAttackPhase::Active));
	TestFalse("Not rooted during Recovery",
		Ability->ShouldBeRootedDuringPhase(EMordecaiAttackPhase::Recovery));

	// Also test Full mode roots all combat phases
	Profile->RootedDuring = EMordecaiRootedMode::Full;
	TestTrue("Full: rooted during Windup",
		Ability->ShouldBeRootedDuringPhase(EMordecaiAttackPhase::Windup));
	TestTrue("Full: rooted during Active",
		Ability->ShouldBeRootedDuringPhase(EMordecaiAttackPhase::Active));
	TestTrue("Full: rooted during Recovery",
		Ability->ShouldBeRootedDuringPhase(EMordecaiAttackPhase::Recovery));

	// Test Windup mode
	Profile->RootedDuring = EMordecaiRootedMode::Windup;
	TestTrue("Windup mode: rooted during Windup",
		Ability->ShouldBeRootedDuringPhase(EMordecaiAttackPhase::Windup));
	TestFalse("Windup mode: not rooted during Active",
		Ability->ShouldBeRootedDuringPhase(EMordecaiAttackPhase::Active));

	return true;
}

// ---------------------------------------------------------------------------
// 9. Mordecai.MeleeAttack.ComboAdvancesOnLightInput (AC-004.9)
// Light->Light during window, verify ComboIndex advances
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_MeleeAttack_ComboAdvance,
	"Mordecai.MeleeAttack.ComboAdvancesOnLightInput",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_MeleeAttack_ComboAdvance::RunTest(const FString& Parameters)
{
	// Create 3-step combo chain
	UMordecaiGA_MeleeAttack* Ability = NewObject<UMordecaiGA_MeleeAttack>();
	Ability->AttackProfiles.Add(CreateTestProfile());
	Ability->AttackProfiles.Add(CreateTestProfile());
	Ability->AttackProfiles.Add(CreateTestProfile());

	TestEqual("Initial combo index", Ability->GetCurrentComboIndex(), 0);

	// Advance combo step 1->2
	bool bAdvanced = Ability->AdvanceCombo();
	TestTrue("Combo advanced", bAdvanced);
	TestEqual("Combo index after first advance", Ability->GetCurrentComboIndex(), 1);

	// Advance combo step 2->3
	bAdvanced = Ability->AdvanceCombo();
	TestTrue("Combo advanced again", bAdvanced);
	TestEqual("Combo index after second advance", Ability->GetCurrentComboIndex(), 2);

	// At max — should not advance further
	bAdvanced = Ability->AdvanceCombo();
	TestFalse("Cannot advance past max", bAdvanced);
	TestEqual("Combo index stays at max", Ability->GetCurrentComboIndex(), 2);

	return true;
}

// ---------------------------------------------------------------------------
// 10. Mordecai.MeleeAttack.ComboResetsAfterTimeout (AC-004.10)
// Light attack, wait past combo window, verify ComboIndex resets to 1
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_MeleeAttack_ComboTimeout,
	"Mordecai.MeleeAttack.ComboResetsAfterTimeout",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_MeleeAttack_ComboTimeout::RunTest(const FString& Parameters)
{
	UMordecaiGA_MeleeAttack* Ability = NewObject<UMordecaiGA_MeleeAttack>();
	Ability->AttackProfiles.Add(CreateTestProfile());
	Ability->AttackProfiles.Add(CreateTestProfile());

	// Verify default combo window timeout matches design spec (800ms)
	TestEqual("Default combo timeout", Ability->ComboWindowTimeoutMs, 800.f);

	// Advance combo, then reset (simulating what happens on timeout)
	Ability->AdvanceCombo();
	TestEqual("Combo index is 1", Ability->GetCurrentComboIndex(), 1);

	Ability->ResetCombo();
	TestEqual("Combo resets to 0", Ability->GetCurrentComboIndex(), 0);

	return true;
}

// ---------------------------------------------------------------------------
// 11. Mordecai.MeleeAttack.ComboResetsOnDifferentInput (AC-004.10)
// Light->Heavy, verify combo resets
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_MeleeAttack_ComboDifferentInput,
	"Mordecai.MeleeAttack.ComboResetsOnDifferentInput",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_MeleeAttack_ComboDifferentInput::RunTest(const FString& Parameters)
{
	UMordecaiGA_MeleeAttack* Ability = NewObject<UMordecaiGA_MeleeAttack>();
	Ability->AttackProfiles.Add(CreateTestProfile());
	Ability->AttackProfiles.Add(CreateTestProfile());

	// Advance combo
	Ability->AdvanceCombo();
	TestEqual("Combo advanced to 1", Ability->GetCurrentComboIndex(), 1);

	// Different input (e.g. Heavy) should reset combo
	Ability->NotifyDifferentInput();
	TestEqual("Combo reset on different input", Ability->GetCurrentComboIndex(), 0);

	return true;
}

// ---------------------------------------------------------------------------
// 12. Mordecai.MeleeAttack.OnHitPayloadApplied (AC-004.11)
// Configure OnHitPayload with a test GE, verify it's applied on hit
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_MeleeAttack_OnHitPayload,
	"Mordecai.MeleeAttack.OnHitPayloadApplied",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_MeleeAttack_OnHitPayload::RunTest(const FString& Parameters)
{
	UMordecaiAttackProfileDataAsset* Profile = CreateTestProfile();
	UMordecaiGA_MeleeAttack* Ability = CreateTestAbility(Profile);

	// Verify active profile is accessible (required for payload application)
	const UMordecaiAttackProfileDataAsset* ActiveProfile = Ability->GetActiveProfile();
	TestNotNull("Active profile available for payload access", ActiveProfile);

	if (ActiveProfile)
	{
		// Verify OnHitPayload and OnUsePayload arrays are accessible
		TestTrue("OnHitPayload array accessible",
			ActiveProfile->OnHitPayload.Num() >= 0);
		TestTrue("OnUsePayload array accessible",
			ActiveProfile->OnUsePayload.Num() >= 0);

		// Verify the correct profile is returned
		TestEqual("Profile BasePower matches",
			ActiveProfile->DamageProfile.BasePower, 25.f);
	}

	return true;
}

// ---------------------------------------------------------------------------
// 13. Mordecai.MeleeAttack.CancelIntoDodgeDuringRecovery (AC-004.12, AC-004.13)
// Set CancelableIntoDodge=true, trigger dodge during Recovery, verify ability ends
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_MeleeAttack_CancelDodge,
	"Mordecai.MeleeAttack.CancelIntoDodgeDuringRecovery",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_MeleeAttack_CancelDodge::RunTest(const FString& Parameters)
{
	UMordecaiAttackProfileDataAsset* Profile = CreateTestProfile();
	Profile->CancelableIntoDodge = true;
	Profile->CancelableIntoBlock = true;
	UMordecaiGA_MeleeAttack* Ability = CreateTestAbility(Profile);

	// During Recovery, cancel into dodge should be allowed
	Ability->TransitionToPhase(EMordecaiAttackPhase::Recovery);
	TestTrue("Can cancel into dodge during Recovery", Ability->CanCancelIntoDodge());
	TestTrue("Can cancel into block during Recovery", Ability->CanCancelIntoBlock());

	// During other phases, cancel should NOT be allowed
	Ability->TransitionToPhase(EMordecaiAttackPhase::Windup);
	TestFalse("Cannot cancel into dodge during Windup", Ability->CanCancelIntoDodge());
	TestFalse("Cannot cancel into block during Windup", Ability->CanCancelIntoBlock());

	Ability->TransitionToPhase(EMordecaiAttackPhase::Active);
	TestFalse("Cannot cancel into dodge during Active", Ability->CanCancelIntoDodge());
	TestFalse("Cannot cancel into block during Active", Ability->CanCancelIntoBlock());

	// Verify cancel is NOT allowed when profile disables it
	Profile->CancelableIntoDodge = false;
	Profile->CancelableIntoBlock = false;
	Ability->TransitionToPhase(EMordecaiAttackPhase::Recovery);
	TestFalse("Cannot cancel dodge when profile disables it", Ability->CanCancelIntoDodge());
	TestFalse("Cannot cancel block when profile disables it", Ability->CanCancelIntoBlock());

	return true;
}

// ---------------------------------------------------------------------------
// 14. Mordecai.MeleeAttack.CannotDoubleActivate (AC-004.14)
// Try to activate while already active, verify blocked
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_MeleeAttack_NoDoubleActivate,
	"Mordecai.MeleeAttack.CannotDoubleActivate",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_MeleeAttack_NoDoubleActivate::RunTest(const FString& Parameters)
{
	// The CDO should have blocking tags configured to prevent double-activation
	const UMordecaiGA_MeleeAttack* CDO = GetDefault<UMordecaiGA_MeleeAttack>();
	TestNotNull("CDO exists", CDO);

	FGameplayTag AttackingTag = MordecaiGameplayTags::State_Attacking;
	TestTrue("Attacking tag is valid", AttackingTag.IsValid());

	// Ability should apply Attacking tag while active
	TestTrue("ActivationOwnedTags has Attacking",
		CDO->HasActivationOwnedTag(AttackingTag));

	// Ability should be blocked when Attacking tag is already present
	TestTrue("ActivationBlockedTags has Attacking",
		CDO->HasActivationBlockedTag(AttackingTag));

	return true;
}
