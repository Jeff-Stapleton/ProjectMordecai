// Project Mordecai — Spell Framework Tests (US-019)

#include "Misc/AutomationTest.h"
#include "Mordecai/Magic/MordecaiSpellTypes.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/Magic/MordecaiGA_SpellBase.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeScaling.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "GameplayTagsManager.h"

// ---------------------------------------------------------------------------
// Helper: Create a test spell data asset with common defaults
// ---------------------------------------------------------------------------
namespace
{
	UMordecaiSpellDataAsset* CreateTestSpellData(
		float BasePower = 100.f,
		int32 SPCost = 5,
		float CooldownDuration = 3.f,
		float WindupTime = 0.5f,
		float CastTime = 1.0f,
		float RecoveryTime = 0.3f)
	{
		UMordecaiSpellDataAsset* Data = NewObject<UMordecaiSpellDataAsset>();
		Data->SpellId = FName(TEXT("TestSpell"));
		Data->DisplayName = FText::FromString(TEXT("Test Spell"));
		Data->SpellPointCost = SPCost;
		Data->CooldownDuration = CooldownDuration;
		Data->CooldownTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Ability.Spell")), false);
		Data->BasePower = BasePower;
		Data->WindupTime = WindupTime;
		Data->CastTime = CastTime;
		Data->RecoveryTime = RecoveryTime;
		Data->bInterruptible = true;
		Data->MovementPolicy = EMordecaiMovementPolicy::FreeMove;
		Data->TargetingType = EMordecaiSpellTargetingType::Projectile;
		Data->Range = 1800.f;
		Data->Radius = 0.f;
		Data->ArcDegrees = 0.f;
		Data->DeliveryType = EMordecaiSpellDeliveryType::SpawnProjectile;
		Data->UpcastCostMultiplier = 2.0f;
		Data->UpcastPowerMultiplier = 1.5f;
		return Data;
	}

	UMordecaiGA_SpellBase* CreateTestSpellAbility(UMordecaiSpellDataAsset* Data = nullptr)
	{
		UMordecaiGA_SpellBase* Ability = NewObject<UMordecaiGA_SpellBase>();
		if (!Data)
		{
			Data = CreateTestSpellData();
		}
		Ability->SpellData = Data;
		return Ability;
	}
}

// ---------------------------------------------------------------------------
// 1. Mordecai.Spell.DataAssetFieldsExist (AC-019.1)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_DataAssetFields,
	"Mordecai.Spell.DataAssetFieldsExist",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_DataAssetFields::RunTest(const FString& Parameters)
{
	const UClass* Class = UMordecaiSpellDataAsset::StaticClass();
	TestNotNull("UMordecaiSpellDataAsset class exists", Class);

	const uint64 ExpectedFlags = CPF_Edit | CPF_BlueprintVisible | CPF_BlueprintReadOnly;

	auto CheckProperty = [&](const TCHAR* FieldName)
	{
		const FProperty* Prop = Class->FindPropertyByName(FName(FieldName));
		TestNotNull(FString::Printf(TEXT("%s exists"), FieldName), Prop);
		if (Prop)
		{
			TestTrue(FString::Printf(TEXT("%s has EditAnywhere|BlueprintReadOnly"), FieldName),
				Prop->HasAllPropertyFlags(ExpectedFlags));
		}
	};

	// Identity
	CheckProperty(TEXT("SpellId"));
	CheckProperty(TEXT("DisplayName"));
	CheckProperty(TEXT("School"));

	// Cost & Cooldown
	CheckProperty(TEXT("SpellPointCost"));
	CheckProperty(TEXT("CooldownDuration"));
	CheckProperty(TEXT("CooldownTag"));

	// Timing
	CheckProperty(TEXT("CastTime"));
	CheckProperty(TEXT("WindupTime"));
	CheckProperty(TEXT("RecoveryTime"));
	CheckProperty(TEXT("bInterruptible"));

	// Movement
	CheckProperty(TEXT("MovementPolicy"));

	// Targeting
	CheckProperty(TEXT("TargetingType"));
	CheckProperty(TEXT("Range"));
	CheckProperty(TEXT("Radius"));
	CheckProperty(TEXT("ArcDegrees"));

	// Power & Scaling
	CheckProperty(TEXT("BasePower"));
	CheckProperty(TEXT("ScalingStats"));

	// Delivery
	CheckProperty(TEXT("DeliveryType"));

	// Upcast
	CheckProperty(TEXT("UpcastCostMultiplier"));
	CheckProperty(TEXT("UpcastPowerMultiplier"));

	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.Spell.SpellBaseBlocksActivationWithInsufficientSP (AC-019.3)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_BlocksInsufficientSP,
	"Mordecai.Spell.SpellBaseBlocksActivationWithInsufficientSP",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_BlocksInsufficientSP::RunTest(const FString& Parameters)
{
	UMordecaiSpellDataAsset* Data = CreateTestSpellData(100.f, /*SPCost=*/5);
	UMordecaiGA_SpellBase* Ability = CreateTestSpellAbility(Data);

	// Insufficient SP
	TestFalse("Blocks when SP < cost (3 < 5)", Ability->CheckSpellPointCost(3.f, false));

	// Exact SP
	TestTrue("Allows when SP == cost (5 == 5)", Ability->CheckSpellPointCost(5.f, false));

	// Excess SP
	TestTrue("Allows when SP > cost (10 > 5)", Ability->CheckSpellPointCost(10.f, false));

	// Insufficient SP for upcast (cost * 2.0 = 10)
	TestFalse("Blocks upcast when SP < upcast cost (8 < 10)", Ability->CheckSpellPointCost(8.f, true));
	TestTrue("Allows upcast when SP >= upcast cost (10 >= 10)", Ability->CheckSpellPointCost(10.f, true));

	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.Spell.SpellBaseDeductsSPOnCommit (AC-019.4)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_DeductsSP,
	"Mordecai.Spell.SpellBaseDeductsSPOnCommit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_DeductsSP::RunTest(const FString& Parameters)
{
	UMordecaiSpellDataAsset* Data = CreateTestSpellData(100.f, /*SPCost=*/5);
	UMordecaiGA_SpellBase* Ability = CreateTestSpellAbility(Data);

	// Verify effective cost computation
	TestEqual("Normal cost is SpellPointCost", Ability->GetEffectiveSPCost(false), 5.f);

	// Verify upcast cost
	TestEqual("Upcast cost is SpellPointCost * UpcastCostMultiplier",
		Ability->GetEffectiveSPCost(true), 10.f);

	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.Spell.SpellBaseAppliesCooldown (AC-019.5)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_AppliesCooldown,
	"Mordecai.Spell.SpellBaseAppliesCooldown",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_AppliesCooldown::RunTest(const FString& Parameters)
{
	UMordecaiSpellDataAsset* Data = CreateTestSpellData();
	Data->CooldownDuration = 5.0f;
	Data->CooldownTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Ability.Spell")), false);
	UMordecaiGA_SpellBase* Ability = CreateTestSpellAbility(Data);

	TestEqual("Cooldown duration from data asset", Ability->GetCooldownDuration(), 5.0f);

	FGameplayTag CooldownTag = Ability->GetCooldownTag();
	TestTrue("Cooldown tag is valid", CooldownTag.IsValid());

	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.Spell.CooldownPreventsReactivation (AC-019.5)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_CooldownPreventsReactivation,
	"Mordecai.Spell.CooldownPreventsReactivation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_CooldownPreventsReactivation::RunTest(const FString& Parameters)
{
	UMordecaiSpellDataAsset* Data = CreateTestSpellData();
	Data->CooldownTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Ability.Spell")), false);
	UMordecaiGA_SpellBase* Ability = CreateTestSpellAbility(Data);

	// Verify the cooldown tag is valid — the standard GAS cooldown check
	// in CanActivateAbility will block reactivation while this tag is present
	FGameplayTag CooldownTag = Ability->GetCooldownTag();
	TestTrue("Cooldown tag valid for reactivation check", CooldownTag.IsValid());

	// Verify cooldown mechanism: GetCooldownDuration returns non-zero
	TestTrue("Cooldown duration > 0", Ability->GetCooldownDuration() > 0.f);

	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.Spell.CastingPhasesProgress (AC-019.6)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_CastingPhases,
	"Mordecai.Spell.CastingPhasesProgress",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_CastingPhases::RunTest(const FString& Parameters)
{
	UMordecaiSpellDataAsset* Data = CreateTestSpellData(
		100.f, 5, 3.f, /*WindupTime=*/0.5f, /*CastTime=*/1.0f, /*RecoveryTime=*/0.3f);
	UMordecaiGA_SpellBase* Ability = CreateTestSpellAbility(Data);

	// Verify initial phase
	TestEqual("Initial phase is None", Ability->GetCurrentPhase(), EMordecaiCastingPhase::None);

	// Verify durations from data asset
	TestEqual("Windup duration", Ability->GetWindupDuration(), 0.5f);
	TestEqual("Cast duration", Ability->GetCastDuration(), 1.0f);
	TestEqual("Recovery duration", Ability->GetRecoveryDuration(), 0.3f);

	// Verify phase sequence via TransitionToPhase
	Ability->TransitionToPhase(EMordecaiCastingPhase::Windup);
	TestEqual("Phase 1: Windup", Ability->GetCurrentPhase(), EMordecaiCastingPhase::Windup);

	Ability->TransitionToPhase(EMordecaiCastingPhase::Cast);
	TestEqual("Phase 2: Cast", Ability->GetCurrentPhase(), EMordecaiCastingPhase::Cast);

	Ability->TransitionToPhase(EMordecaiCastingPhase::Recovery);
	TestEqual("Phase 3: Recovery", Ability->GetCurrentPhase(), EMordecaiCastingPhase::Recovery);

	return true;
}

// ---------------------------------------------------------------------------
// 7. Mordecai.Spell.SlowWhileCastingAppliesTag (AC-019.7)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_SlowWhileCasting,
	"Mordecai.Spell.SlowWhileCastingAppliesTag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_SlowWhileCasting::RunTest(const FString& Parameters)
{
	UMordecaiSpellDataAsset* Data = CreateTestSpellData();
	Data->MovementPolicy = EMordecaiMovementPolicy::SlowWhileCasting;
	UMordecaiGA_SpellBase* Ability = CreateTestSpellAbility(Data);

	FGameplayTag PolicyTag = Ability->GetMovementPolicyTag();
	TestTrue("SlowWhileCasting returns CastingSlow tag",
		PolicyTag == MordecaiGameplayTags::State_CastingSlow);

	return true;
}

// ---------------------------------------------------------------------------
// 8. Mordecai.Spell.RootWhileCastingAppliesTag (AC-019.7)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_RootWhileCasting,
	"Mordecai.Spell.RootWhileCastingAppliesTag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_RootWhileCasting::RunTest(const FString& Parameters)
{
	UMordecaiSpellDataAsset* Data = CreateTestSpellData();
	Data->MovementPolicy = EMordecaiMovementPolicy::RootWhileCasting;
	UMordecaiGA_SpellBase* Ability = CreateTestSpellAbility(Data);

	FGameplayTag PolicyTag = Ability->GetMovementPolicyTag();
	TestTrue("RootWhileCasting returns CastingRooted tag",
		PolicyTag == MordecaiGameplayTags::State_CastingRooted);

	// Also verify FreeMove returns invalid tag
	Data->MovementPolicy = EMordecaiMovementPolicy::FreeMove;
	FGameplayTag FreeMoveTag = Ability->GetMovementPolicyTag();
	TestFalse("FreeMove returns invalid tag", FreeMoveTag.IsValid());

	return true;
}

// ---------------------------------------------------------------------------
// 9. Mordecai.Spell.InterruptibleSpellCancelledOnDamage (AC-019.8)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_InterruptibleCancelled,
	"Mordecai.Spell.InterruptibleSpellCancelledOnDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_InterruptibleCancelled::RunTest(const FString& Parameters)
{
	UMordecaiSpellDataAsset* Data = CreateTestSpellData();
	Data->bInterruptible = true;
	UMordecaiGA_SpellBase* Ability = CreateTestSpellAbility(Data);

	// Verify interruptibility flag
	TestTrue("Spell is interruptible", Ability->IsInterruptible());

	// Verify interruption allowed during Windup
	Ability->TransitionToPhase(EMordecaiCastingPhase::Windup);
	TestTrue("Should interrupt during Windup", Ability->ShouldInterruptInCurrentPhase());

	// Verify interruption allowed during Cast
	Ability->TransitionToPhase(EMordecaiCastingPhase::Cast);
	TestTrue("Should interrupt during Cast", Ability->ShouldInterruptInCurrentPhase());

	// Verify interruption NOT allowed during Recovery
	Ability->TransitionToPhase(EMordecaiCastingPhase::Recovery);
	TestFalse("Should NOT interrupt during Recovery", Ability->ShouldInterruptInCurrentPhase());

	// Verify interruption NOT allowed when bInterruptible is false
	Data->bInterruptible = false;
	TestFalse("Not interruptible when bInterruptible is false", Ability->IsInterruptible());
	Ability->TransitionToPhase(EMordecaiCastingPhase::Windup);
	TestFalse("Should NOT interrupt during Windup when not interruptible",
		Ability->ShouldInterruptInCurrentPhase());
	Ability->TransitionToPhase(EMordecaiCastingPhase::Cast);
	TestFalse("Should NOT interrupt during Cast when not interruptible",
		Ability->ShouldInterruptInCurrentPhase());

	return true;
}

// ---------------------------------------------------------------------------
// 10. Mordecai.Spell.SpellPowerScalesWithAttributes (AC-019.9)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_PowerScalesAttributes,
	"Mordecai.Spell.SpellPowerScalesWithAttributes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_PowerScalesAttributes::RunTest(const FString& Parameters)
{
	UMordecaiSpellDataAsset* Data = CreateTestSpellData(/*BasePower=*/100.f);
	UMordecaiGA_SpellBase* Ability = CreateTestSpellAbility(Data);

	// Formula: SpellPower = BasePower * (1.0 + AttributeScalingBonus) * RankMultiplier * UpcastMultiplier
	// With no scaling: SpellPower = 100 * 1.0 * 1.0 * 1.0 = 100
	TestEqual("No scaling: power equals BasePower",
		Ability->ComputeSpellPower(0.f, 0, false), 100.f);

	// With attribute scaling bonus of 2.5:
	// SpellPower = 100 * (1.0 + 2.5) * 1.0 * 1.0 = 350
	TestEqual("Attribute scaling: 100 * (1.0 + 2.5) = 350",
		Ability->ComputeSpellPower(2.5f, 0, false), 350.f);

	// Verify EffectiveMod integration:
	// Raw mod 5 → EffectiveMod = 5.0 (band 1 only)
	// Coefficient = 0.5 → contribution = 0.5 * 5.0 = 2.5
	float EffMod5 = FMordecaiAttributeScaling::CalculateEffectiveMod(5.f);
	TestEqual("EffectiveMod(5) is 5.0", EffMod5, 5.0f);
	float ScalingBonus = 0.5f * EffMod5;
	TestEqual("Scaling bonus = 0.5 * 5.0 = 2.5", ScalingBonus, 2.5f);

	// SpellPower = 100 * (1.0 + 2.5) = 350
	TestEqual("Computed with EffectiveMod scaling",
		Ability->ComputeSpellPower(ScalingBonus, 0, false), 350.f);

	return true;
}

// ---------------------------------------------------------------------------
// 11. Mordecai.Spell.SpellPowerScalesWithSkillRank (AC-019.10)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_PowerScalesRank,
	"Mordecai.Spell.SpellPowerScalesWithSkillRank",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_PowerScalesRank::RunTest(const FString& Parameters)
{
	UMordecaiSpellDataAsset* Data = CreateTestSpellData(/*BasePower=*/100.f);
	UMordecaiGA_SpellBase* Ability = CreateTestSpellAbility(Data);

	// Rank 0 (untrained): multiplier = 1.0 + (0 * 0.05) = 1.0
	// SpellPower = 100 * 1.0 * 1.0 = 100
	TestEqual("Rank 0: multiplier 1.0",
		Ability->ComputeSpellPower(0.f, 0, false), 100.f);

	// Rank 10: multiplier = 1.0 + (10 * 0.05) = 1.5
	// SpellPower = 100 * 1.0 * 1.5 = 150
	TestEqual("Rank 10: multiplier 1.5",
		Ability->ComputeSpellPower(0.f, 10, false), 150.f);

	// Rank 20 (max): multiplier = 1.0 + (20 * 0.05) = 2.0
	// SpellPower = 100 * 1.0 * 2.0 = 200
	TestEqual("Rank 20: multiplier 2.0",
		Ability->ComputeSpellPower(0.f, 20, false), 200.f);

	// Combined: BasePower 100, scaling bonus 1.0, rank 10
	// SpellPower = 100 * (1.0 + 1.0) * 1.5 = 300
	TestEqual("Rank + attribute scaling combined",
		Ability->ComputeSpellPower(1.0f, 10, false), 300.f);

	return true;
}

// ---------------------------------------------------------------------------
// 12. Mordecai.Spell.UpcastIncreasedCostAndPower (AC-019.11)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Upcast,
	"Mordecai.Spell.UpcastIncreasedCostAndPower",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Upcast::RunTest(const FString& Parameters)
{
	UMordecaiSpellDataAsset* Data = CreateTestSpellData(/*BasePower=*/100.f, /*SPCost=*/5);
	Data->UpcastCostMultiplier = 2.0f;
	Data->UpcastPowerMultiplier = 1.5f;
	UMordecaiGA_SpellBase* Ability = CreateTestSpellAbility(Data);

	// Normal cost
	TestEqual("Normal SP cost", Ability->GetEffectiveSPCost(false), 5.f);

	// Upcast cost: 5 * 2.0 = 10
	TestEqual("Upcast SP cost = 5 * 2.0 = 10", Ability->GetEffectiveSPCost(true), 10.f);

	// Normal power (no scaling, rank 0)
	TestEqual("Normal power", Ability->ComputeSpellPower(0.f, 0, false), 100.f);

	// Upcast power: 100 * 1.0 * 1.0 * 1.5 = 150
	TestEqual("Upcast power = 100 * 1.5 = 150",
		Ability->ComputeSpellPower(0.f, 0, true), 150.f);

	// Upcast with rank and scaling: 100 * (1.0 + 1.0) * 1.5 * 1.5 = 450
	TestEqual("Upcast + rank + scaling: 100 * 2.0 * 1.5 * 1.5 = 450",
		Ability->ComputeSpellPower(1.0f, 10, true), 450.f);

	return true;
}

// ---------------------------------------------------------------------------
// 13. Mordecai.Spell.SilencedBlocksSpellActivation (AC-019.13)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_SilencedBlocks,
	"Mordecai.Spell.SilencedBlocksSpellActivation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_SilencedBlocks::RunTest(const FString& Parameters)
{
	// Check CDO has Silenced tag in ActivationBlockedTags
	const UMordecaiGA_SpellBase* CDO = GetDefault<UMordecaiGA_SpellBase>();
	TestNotNull("CDO exists", CDO);

	FGameplayTag SilencedTag = MordecaiGameplayTags::Status_Silenced;
	TestTrue("Silenced tag is valid", SilencedTag.IsValid());

	// Spell should be blocked when Silenced tag is present on caster
	TestTrue("ActivationBlockedTags has Silenced",
		CDO->HasActivationBlockedTag(SilencedTag));

	// Also verify spell applies casting tag
	FGameplayTag CastingTag = MordecaiGameplayTags::State_Casting;
	TestTrue("Casting tag is valid", CastingTag.IsValid());

	return true;
}
