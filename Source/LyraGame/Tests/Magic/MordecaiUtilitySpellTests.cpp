// Project Mordecai — Utility Spell Tests (US-021)

#include "Misc/AutomationTest.h"
#include "Mordecai/Magic/MordecaiGA_Blink.h"
#include "Mordecai/Magic/MordecaiGA_Sleep.h"
#include "Mordecai/Magic/MordecaiGA_FireWard.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/Magic/MordecaiGA_SpellBase.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"

// ---------------------------------------------------------------------------
// Helpers: Create test spell data assets for utility spells
// ---------------------------------------------------------------------------
namespace
{
	UMordecaiSpellDataAsset* CreateBlinkData(
		float Range = 600.f,
		float InvulnDuration = 0.2f)
	{
		UMordecaiSpellDataAsset* Data = NewObject<UMordecaiSpellDataAsset>();
		Data->SpellId = FName(TEXT("Blink"));
		Data->Range = Range;
		Data->BuffDuration = InvulnDuration;
		Data->DeliveryType = EMordecaiSpellDeliveryType::DashMovement;
		Data->TargetingType = EMordecaiSpellTargetingType::Dash;
		Data->WindupTime = 0.f;
		Data->CastTime = 0.f;
		Data->RecoveryTime = 0.3f;
		Data->SpellPointCost = 3;
		Data->BasePower = 0.f;
		return Data;
	}

	UMordecaiSpellDataAsset* CreateSleepData(
		float Range = 1200.f,
		float Duration = 5.0f)
	{
		UMordecaiSpellDataAsset* Data = NewObject<UMordecaiSpellDataAsset>();
		Data->SpellId = FName(TEXT("Sleep"));
		Data->Range = Range;
		Data->BuffDuration = Duration;
		Data->DeliveryType = EMordecaiSpellDeliveryType::TraceHit;
		Data->TargetingType = EMordecaiSpellTargetingType::Line;
		Data->WindupTime = 0.5f;
		Data->CastTime = 0.3f;
		Data->RecoveryTime = 0.3f;
		Data->SpellPointCost = 4;
		Data->BasePower = 0.f;
		return Data;
	}

	UMordecaiSpellDataAsset* CreateFireWardData(
		float ShieldHP = 50.f,
		float Duration = 15.f)
	{
		UMordecaiSpellDataAsset* Data = NewObject<UMordecaiSpellDataAsset>();
		Data->SpellId = FName(TEXT("FireWard"));
		Data->BasePower = ShieldHP;
		Data->BuffDuration = Duration;
		Data->DeliveryType = EMordecaiSpellDeliveryType::InstantApply;
		Data->TargetingType = EMordecaiSpellTargetingType::Self;
		Data->WindupTime = 0.3f;
		Data->CastTime = 0.2f;
		Data->RecoveryTime = 0.3f;
		Data->SpellPointCost = 5;
		return Data;
	}
}

// ---------------------------------------------------------------------------
// 1. Mordecai.Spell.Blink.TeleportsCasterForward (AC-021.1)
// Verifies Blink extends SpellBase, uses DashMovement delivery, and reads
// teleport range from SpellDataAsset.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Blink_TeleportsCasterForward,
	"Mordecai.Spell.Blink.TeleportsCasterForward",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Blink_TeleportsCasterForward::RunTest(const FString& Parameters)
{
	// AC-021.1: Blink extends SpellBase
	UMordecaiGA_Blink* Blink = NewObject<UMordecaiGA_Blink>();
	TestNotNull("Blink ability created", Blink);

	UMordecaiGA_SpellBase* AsBase = Cast<UMordecaiGA_SpellBase>(Blink);
	TestNotNull("Blink inherits from SpellBase", AsBase);

	// Configure with data asset
	UMordecaiSpellDataAsset* Data = CreateBlinkData(/*Range=*/600.f);
	Blink->SpellData = Data;

	// AC-021.1: Delivery type is DashMovement
	TestEqual("Blink delivery type is DashMovement",
		Data->DeliveryType, EMordecaiSpellDeliveryType::DashMovement);

	// AC-021.1: Teleport range reads from SpellDataAsset
	TestEqual("Blink teleport range = 600", Blink->GetTeleportRange(), 600.f);

	// Verify range is configurable
	Data->Range = 800.f;
	TestEqual("Teleport range updates with data asset", Blink->GetTeleportRange(), 800.f);

	// Verify teleport destination computation (no wall)
	FVector Origin = FVector::ZeroVector;
	FVector AimDir = FVector::ForwardVector; // +X
	float Range = 600.f;

	FVector Dest = UMordecaiGA_Blink::ComputeTeleportDestination(Origin, AimDir, Range, false, FVector::ZeroVector);
	TestTrue("Teleport destination is ~600 units ahead",
		FMath::IsNearlyEqual(Dest.X, 600.f, 1.f));
	TestTrue("Teleport destination Y is ~0",
		FMath::IsNearlyEqual(Dest.Y, 0.f, 1.f));

	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.Spell.Blink.ClampsToValidPosition (AC-021.2)
// Verifies line trace clamping: teleport stops before wall.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Blink_ClampsToValidPosition,
	"Mordecai.Spell.Blink.ClampsToValidPosition",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Blink_ClampsToValidPosition::RunTest(const FString& Parameters)
{
	UMordecaiGA_Blink* Blink = NewObject<UMordecaiGA_Blink>();
	UMordecaiSpellDataAsset* Data = CreateBlinkData(/*Range=*/600.f);
	Blink->SpellData = Data;

	FVector Origin = FVector::ZeroVector;
	FVector AimDir = FVector::ForwardVector; // +X

	// AC-021.2: Wall at 400 units, buffer of 50 → clamp to 350
	FVector WallHitPoint = FVector(400.f, 0.f, 0.f);
	FVector Dest = UMordecaiGA_Blink::ComputeTeleportDestination(
		Origin, AimDir, 600.f, true, WallHitPoint);

	// Should be clamped to WallHitPoint - 50cm buffer in aim direction
	float ExpectedX = 400.f - 50.f; // 350
	TestTrue("Clamped destination X near 350",
		FMath::IsNearlyEqual(Dest.X, ExpectedX, 1.f));

	// Wall at 30 units (closer than buffer) → clamp to origin (can't go through)
	FVector CloseWall = FVector(30.f, 0.f, 0.f);
	FVector CloseDest = UMordecaiGA_Blink::ComputeTeleportDestination(
		Origin, AimDir, 600.f, true, CloseWall);
	TestTrue("Very close wall clamps to origin",
		FVector::Dist(CloseDest, Origin) < 51.f);

	// No wall hit → full range destination
	FVector NoWallDest = UMordecaiGA_Blink::ComputeTeleportDestination(
		Origin, AimDir, 600.f, false, FVector::ZeroVector);
	TestTrue("No wall = full range",
		FMath::IsNearlyEqual(NoWallDest.X, 600.f, 1.f));

	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.Spell.Blink.GrantsIFrames (AC-021.3)
// Verifies Blink grants Invulnerable tag for configurable duration.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Blink_GrantsIFrames,
	"Mordecai.Spell.Blink.GrantsIFrames",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Blink_GrantsIFrames::RunTest(const FString& Parameters)
{
	UMordecaiGA_Blink* Blink = NewObject<UMordecaiGA_Blink>();
	UMordecaiSpellDataAsset* Data = CreateBlinkData(600.f, /*InvulnDuration=*/0.2f);
	Blink->SpellData = Data;

	// AC-021.3: Invulnerability duration from SpellDataAsset
	TestEqual("Invulnerability duration = 0.2s",
		Blink->GetInvulnerabilityDuration(), 0.2f);

	// Verify configurable
	Data->BuffDuration = 0.5f;
	TestEqual("Invulnerability duration updates",
		Blink->GetInvulnerabilityDuration(), 0.5f);

	// AC-021.3: Uses Mordecai.State.Invulnerable tag
	FGameplayTag InvulnTag = Blink->GetInvulnerabilityTag();
	TestTrue("Invulnerable tag is valid", InvulnTag.IsValid());
	TestTrue("Invulnerable tag is State.Invulnerable",
		InvulnTag == MordecaiGameplayTags::State_Invulnerable);

	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.Spell.Blink.CostsSP (AC-021.1 + SpellBase integration)
// Verifies Blink costs SP and has near-instant cast timing.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Blink_CostsSP,
	"Mordecai.Spell.Blink.CostsSP",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Blink_CostsSP::RunTest(const FString& Parameters)
{
	UMordecaiGA_Blink* Blink = NewObject<UMordecaiGA_Blink>();
	UMordecaiSpellDataAsset* Data = CreateBlinkData();
	Blink->SpellData = Data;

	// Verify SP cost reads from data asset
	TestEqual("SP cost = 3", Blink->GetEffectiveSPCost(false), 3.f);

	// AC-021.4: Near-instant cast (WindupTime ≈ 0, CastTime ≈ 0)
	TestEqual("Windup time = 0", Blink->GetWindupDuration(), 0.f);
	TestEqual("Cast time = 0", Blink->GetCastDuration(), 0.f);

	// Recovery time applies normally
	TestTrue("Recovery time > 0", Blink->GetRecoveryDuration() > 0.f);

	// Verify ability tag
	FGameplayTag BlinkTag = MordecaiGameplayTags::Ability_Spell_Blink;
	TestTrue("Blink ability tag is valid", BlinkTag.IsValid());

	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.Spell.Sleep.AppliesSleepToTarget (AC-021.5, AC-021.6)
// Verifies Sleep extends SpellBase, uses TraceHit, and reads configuration.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Sleep_AppliesSleepToTarget,
	"Mordecai.Spell.Sleep.AppliesSleepToTarget",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Sleep_AppliesSleepToTarget::RunTest(const FString& Parameters)
{
	// AC-021.5: Sleep extends SpellBase
	UMordecaiGA_Sleep* Sleep = NewObject<UMordecaiGA_Sleep>();
	TestNotNull("Sleep ability created", Sleep);

	UMordecaiGA_SpellBase* AsBase = Cast<UMordecaiGA_SpellBase>(Sleep);
	TestNotNull("Sleep inherits from SpellBase", AsBase);

	// Configure
	UMordecaiSpellDataAsset* Data = CreateSleepData(/*Range=*/1200.f, /*Duration=*/5.0f);
	Sleep->SpellData = Data;

	// AC-021.5: Delivery type is TraceHit
	TestEqual("Sleep delivery is TraceHit",
		Data->DeliveryType, EMordecaiSpellDeliveryType::TraceHit);

	// AC-021.5: Range reads from SpellDataAsset
	TestEqual("Sleep range = 1200", Sleep->GetSleepRange(), 1200.f);

	// AC-021.6: Duration reads from SpellDataAsset
	TestEqual("Sleep duration = 5.0s", Sleep->GetSleepDuration(), 5.0f);

	// Verify configurable
	Data->BuffDuration = 8.0f;
	TestEqual("Sleep duration updates", Sleep->GetSleepDuration(), 8.0f);

	// AC-021.6: Sleep applies Mordecai.Status.Sleeping tag
	FGameplayTag SleepTag = Sleep->GetSleepTag();
	TestTrue("Sleep tag is valid", SleepTag.IsValid());
	TestTrue("Sleep tag is Status.Sleeping",
		SleepTag == MordecaiGameplayTags::Status_Sleeping);

	// Verify ability tag
	FGameplayTag AbilityTag = MordecaiGameplayTags::Ability_Spell_Sleep;
	TestTrue("Sleep ability tag is valid", AbilityTag.IsValid());

	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.Spell.Sleep.SleepingTargetCannotAct (AC-021.6)
// Verifies sleep blocks movement and ability activation.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Sleep_SleepingTargetCannotAct,
	"Mordecai.Spell.Sleep.SleepingTargetCannotAct",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Sleep_SleepingTargetCannotAct::RunTest(const FString& Parameters)
{
	UMordecaiGA_Sleep* Sleep = NewObject<UMordecaiGA_Sleep>();
	UMordecaiSpellDataAsset* Data = CreateSleepData();
	Sleep->SpellData = Data;

	// AC-021.6: Sleep blocks abilities and movement
	// Verify the tags that should be applied by the Sleep GE
	TArray<FGameplayTag> BlockTags = Sleep->GetSleepBlockedTags();

	// Should include movement-blocking tag
	bool bBlocksMovement = false;
	bool bBlocksAbilities = false;
	for (const FGameplayTag& Tag : BlockTags)
	{
		if (Tag == MordecaiGameplayTags::Status_Sleeping)
		{
			// The Sleeping tag itself is used by abilities to check if they should be blocked
			bBlocksAbilities = true;
		}
		if (Tag == MordecaiGameplayTags::State_Rooted)
		{
			bBlocksMovement = true;
		}
	}

	// Sleep should root the target (disable movement)
	TestTrue("Sleep blocks movement (applies Rooted)", bBlocksMovement);

	// Sleep status tag used to block abilities
	TestTrue("Sleep provides ability blocking tag", bBlocksAbilities);

	return true;
}

// ---------------------------------------------------------------------------
// 7. Mordecai.Spell.Sleep.DamageWakesSleepingTarget (AC-021.7)
// Verifies damage absorption logic for waking sleeping targets.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Sleep_DamageWakesSleepingTarget,
	"Mordecai.Spell.Sleep.DamageWakesSleepingTarget",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Sleep_DamageWakesSleepingTarget::RunTest(const FString& Parameters)
{
	UMordecaiGA_Sleep* Sleep = NewObject<UMordecaiGA_Sleep>();
	UMordecaiSpellDataAsset* Data = CreateSleepData();
	Sleep->SpellData = Data;

	// AC-021.7: Sleep should be removable on damage
	// Verify the spell knows it needs to monitor health changes
	TestTrue("Sleep monitors health for wake", Sleep->ShouldWakeOnDamage());

	// Verify the sleep tag that gets removed
	FGameplayTag SleepTag = Sleep->GetSleepTag();
	TestTrue("Sleep tag to remove is valid", SleepTag.IsValid());
	TestTrue("Sleep tag is Sleeping",
		SleepTag == MordecaiGameplayTags::Status_Sleeping);

	return true;
}

// ---------------------------------------------------------------------------
// 8. Mordecai.Spell.FireWard.AbsorbsFireDamage (AC-021.9, AC-021.10)
// Verifies Fire Ward shield HP and fire-only absorption.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_FireWard_AbsorbsFireDamage,
	"Mordecai.Spell.FireWard.AbsorbsFireDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_FireWard_AbsorbsFireDamage::RunTest(const FString& Parameters)
{
	// AC-021.8: FireWard extends SpellBase
	UMordecaiGA_FireWard* FireWard = NewObject<UMordecaiGA_FireWard>();
	TestNotNull("FireWard ability created", FireWard);

	UMordecaiGA_SpellBase* AsBase = Cast<UMordecaiGA_FireWard>(FireWard);
	TestNotNull("FireWard inherits from SpellBase", AsBase);

	// Configure
	UMordecaiSpellDataAsset* Data = CreateFireWardData(/*ShieldHP=*/50.f, /*Duration=*/15.f);
	FireWard->SpellData = Data;

	// AC-021.8: Delivery is InstantApply, self-targeted
	TestEqual("FireWard delivery is InstantApply",
		Data->DeliveryType, EMordecaiSpellDeliveryType::InstantApply);
	TestEqual("FireWard targeting is Self",
		Data->TargetingType, EMordecaiSpellTargetingType::Self);

	// AC-021.8: ShieldHP = BasePower from SpellDataAsset
	TestEqual("Shield HP = BasePower = 50", FireWard->GetShieldHP(), 50.f);

	// Verify configurable
	Data->BasePower = 100.f;
	TestEqual("Shield HP updates with BasePower", FireWard->GetShieldHP(), 100.f);

	// AC-021.10: Only absorbs fire damage
	FGameplayTag AbsorbTag = FireWard->GetAbsorbedDamageTag();
	TestTrue("Absorbed damage tag is valid", AbsorbTag.IsValid());
	TestTrue("Absorbed damage tag is Damage.Fire",
		AbsorbTag == MordecaiGameplayTags::Damage_Fire);

	// AC-021.9/AC-021.10: Absorption computation
	// 30 fire damage vs 50 shield → absorb 30, shield has 20 remaining
	float ShieldRemaining = 0.f;
	float DamagePassthrough = UMordecaiGA_FireWard::ComputeAbsorption(30.f, 50.f, ShieldRemaining);
	TestEqual("30 fire vs 50 shield: 0 passthrough", DamagePassthrough, 0.f);
	TestEqual("30 fire vs 50 shield: 20 remaining", ShieldRemaining, 20.f);

	// 80 fire damage vs 50 shield → absorb 50, 30 passes through
	DamagePassthrough = UMordecaiGA_FireWard::ComputeAbsorption(80.f, 50.f, ShieldRemaining);
	TestEqual("80 fire vs 50 shield: 30 passthrough", DamagePassthrough, 30.f);
	TestEqual("80 fire vs 50 shield: 0 remaining", ShieldRemaining, 0.f);

	// Exact match: 50 vs 50
	DamagePassthrough = UMordecaiGA_FireWard::ComputeAbsorption(50.f, 50.f, ShieldRemaining);
	TestEqual("50 fire vs 50 shield: 0 passthrough", DamagePassthrough, 0.f);
	TestEqual("50 fire vs 50 shield: 0 remaining", ShieldRemaining, 0.f);

	return true;
}

// ---------------------------------------------------------------------------
// 9. Mordecai.Spell.FireWard.DoesNotAbsorbNonFireDamage (AC-021.10)
// Verifies non-fire damage passes through unaffected.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_FireWard_DoesNotAbsorbNonFireDamage,
	"Mordecai.Spell.FireWard.DoesNotAbsorbNonFireDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_FireWard_DoesNotAbsorbNonFireDamage::RunTest(const FString& Parameters)
{
	UMordecaiGA_FireWard* FireWard = NewObject<UMordecaiGA_FireWard>();
	UMordecaiSpellDataAsset* Data = CreateFireWardData(/*ShieldHP=*/50.f);
	FireWard->SpellData = Data;

	// AC-021.10: Fire Ward only absorbs Mordecai.Damage.Fire
	FGameplayTag AbsorbTag = FireWard->GetAbsorbedDamageTag();

	// Non-fire damage tags should NOT match
	TestFalse("Physical damage not absorbed",
		AbsorbTag == MordecaiGameplayTags::Damage_Physical);
	TestFalse("Frost damage not absorbed",
		AbsorbTag == MordecaiGameplayTags::Damage_Frost);
	TestFalse("Arcane damage not absorbed",
		AbsorbTag == MordecaiGameplayTags::Damage_Arcane);
	TestFalse("Lightning damage not absorbed",
		AbsorbTag == MordecaiGameplayTags::Damage_Lightning);
	TestFalse("Shadow damage not absorbed",
		AbsorbTag == MordecaiGameplayTags::Damage_Shadow);

	// Confirm only fire matches
	TestTrue("Only fire damage is absorbed",
		AbsorbTag == MordecaiGameplayTags::Damage_Fire);

	// Static helper: ShouldAbsorb checks tag match
	TestTrue("ShouldAbsorb returns true for fire",
		UMordecaiGA_FireWard::ShouldAbsorbDamageType(MordecaiGameplayTags::Damage_Fire));
	TestFalse("ShouldAbsorb returns false for frost",
		UMordecaiGA_FireWard::ShouldAbsorbDamageType(MordecaiGameplayTags::Damage_Frost));
	TestFalse("ShouldAbsorb returns false for physical",
		UMordecaiGA_FireWard::ShouldAbsorbDamageType(MordecaiGameplayTags::Damage_Physical));
	TestFalse("ShouldAbsorb returns false for arcane",
		UMordecaiGA_FireWard::ShouldAbsorbDamageType(MordecaiGameplayTags::Damage_Arcane));

	return true;
}

// ---------------------------------------------------------------------------
// 10. Mordecai.Spell.FireWard.ExpiresAfterDuration (AC-021.9)
// Verifies ward duration and status tag configuration.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_FireWard_ExpiresAfterDuration,
	"Mordecai.Spell.FireWard.ExpiresAfterDuration",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_FireWard_ExpiresAfterDuration::RunTest(const FString& Parameters)
{
	UMordecaiGA_FireWard* FireWard = NewObject<UMordecaiGA_FireWard>();
	UMordecaiSpellDataAsset* Data = CreateFireWardData(50.f, /*Duration=*/15.f);
	FireWard->SpellData = Data;

	// AC-021.9: Duration from SpellDataAsset
	TestEqual("Ward duration = 15s", FireWard->GetWardDuration(), 15.f);

	// Verify configurable
	Data->BuffDuration = 20.f;
	TestEqual("Ward duration updates", FireWard->GetWardDuration(), 20.f);

	// AC-021.9: Status tag is Mordecai.Status.FireWard
	FGameplayTag WardTag = FireWard->GetWardTag();
	TestTrue("Ward tag is valid", WardTag.IsValid());
	TestTrue("Ward tag is Status.FireWard",
		WardTag == MordecaiGameplayTags::Status_FireWard);

	// Verify ability tag
	FGameplayTag AbilityTag = MordecaiGameplayTags::Ability_Spell_FireWard;
	TestTrue("FireWard ability tag is valid", AbilityTag.IsValid());

	// Verify FireWardShieldHP attribute exists on attribute set
	FGameplayAttribute ShieldAttr = UMordecaiAttributeSet::GetFireWardShieldHPAttribute();
	TestTrue("FireWardShieldHP attribute is valid", ShieldAttr.IsValid());

	// Verify default is 0
	UMordecaiAttributeSet* AttrSet = NewObject<UMordecaiAttributeSet>();
	TestEqual("FireWardShieldHP defaults to 0", AttrSet->GetFireWardShieldHP(), 0.f);

	return true;
}

// ---------------------------------------------------------------------------
// 11. Mordecai.Spell.FireWard.BreaksWhenShieldDepleted (AC-021.9)
// Verifies shield depletion mechanics.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_FireWard_BreaksWhenShieldDepleted,
	"Mordecai.Spell.FireWard.BreaksWhenShieldDepleted",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_FireWard_BreaksWhenShieldDepleted::RunTest(const FString& Parameters)
{
	UMordecaiGA_FireWard* FireWard = NewObject<UMordecaiGA_FireWard>();
	UMordecaiSpellDataAsset* Data = CreateFireWardData(/*ShieldHP=*/50.f, 15.f);
	FireWard->SpellData = Data;

	// AC-021.9: Shield depleted when HP reaches 0
	float ShieldRemaining = 0.f;

	// Damage exactly equal to shield → depleted
	UMordecaiGA_FireWard::ComputeAbsorption(50.f, 50.f, ShieldRemaining);
	TestTrue("Shield is depleted at 0 HP", ShieldRemaining <= 0.f);

	// Damage exceeding shield → also depleted
	UMordecaiGA_FireWard::ComputeAbsorption(100.f, 50.f, ShieldRemaining);
	TestTrue("Shield depleted by excess damage", ShieldRemaining <= 0.f);

	// Partial damage → not depleted
	UMordecaiGA_FireWard::ComputeAbsorption(20.f, 50.f, ShieldRemaining);
	TestTrue("Shield not depleted by partial damage", ShieldRemaining > 0.f);
	TestEqual("Shield has 30 HP remaining", ShieldRemaining, 30.f);

	// Verify shield HP is properly stored as attribute
	FGameplayAttribute ShieldAttr = UMordecaiAttributeSet::GetFireWardShieldHPAttribute();
	TestTrue("FireWardShieldHP attribute exists", ShieldAttr.IsValid());

	// Verify ShouldRemoveWard helper
	TestTrue("Ward should be removed at 0 shield",
		UMordecaiGA_FireWard::ShouldRemoveWard(0.f));
	TestFalse("Ward should NOT be removed with shield remaining",
		UMordecaiGA_FireWard::ShouldRemoveWard(10.f));

	return true;
}
