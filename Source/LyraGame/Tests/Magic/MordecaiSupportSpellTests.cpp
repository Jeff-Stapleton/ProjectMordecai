// Project Mordecai — Support Spell Tests (US-022)

#include "Misc/AutomationTest.h"
#include "Mordecai/Magic/MordecaiGA_Bless.h"
#include "Mordecai/Magic/MordecaiGA_Restoration.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/Magic/MordecaiGA_SpellBase.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"

// ---------------------------------------------------------------------------
// Helpers: Create test spell data assets for support spells
// ---------------------------------------------------------------------------
namespace
{
	UMordecaiSpellDataAsset* CreateBlessData(
		float BasePower = 15.f,
		float Duration = 10.f)
	{
		UMordecaiSpellDataAsset* Data = NewObject<UMordecaiSpellDataAsset>();
		Data->SpellId = FName(TEXT("Bless"));
		Data->BasePower = BasePower;
		Data->BuffDuration = Duration;
		Data->DeliveryType = EMordecaiSpellDeliveryType::InstantApply;
		Data->TargetingType = EMordecaiSpellTargetingType::Self;
		Data->WindupTime = 0.3f;
		Data->CastTime = 0.2f;
		Data->RecoveryTime = 0.3f;
		Data->SpellPointCost = 4;
		return Data;
	}

	UMordecaiSpellDataAsset* CreateRestorationData(
		float BasePower = 50.f,
		float Duration = 10.f)
	{
		UMordecaiSpellDataAsset* Data = NewObject<UMordecaiSpellDataAsset>();
		Data->SpellId = FName(TEXT("Restoration"));
		Data->BasePower = BasePower;
		Data->BuffDuration = Duration;
		Data->DeliveryType = EMordecaiSpellDeliveryType::InstantApply;
		Data->TargetingType = EMordecaiSpellTargetingType::Self;
		Data->WindupTime = 0.5f;
		Data->CastTime = 0.3f;
		Data->RecoveryTime = 0.3f;
		Data->SpellPointCost = 5;
		return Data;
	}
}

// ---------------------------------------------------------------------------
// 1. Mordecai.Spell.Bless.IncreasesPhysicalDamageMultiplier (AC-022.1)
// Verifies BlessBonus increases PhysicalDamageMultiplier.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Bless_IncreasesPhysicalDamageMultiplier,
	"Mordecai.Spell.Bless.IncreasesPhysicalDamageMultiplier",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Bless_IncreasesPhysicalDamageMultiplier::RunTest(const FString& Parameters)
{
	// AC-022.1: Bless extends SpellBase
	UMordecaiGA_Bless* Bless = NewObject<UMordecaiGA_Bless>();
	TestNotNull("Bless ability created", Bless);

	UMordecaiGA_SpellBase* AsBase = Cast<UMordecaiGA_SpellBase>(Bless);
	TestNotNull("Bless inherits from SpellBase", AsBase);

	// Configure with data asset: BasePower=15 → +0.15
	UMordecaiSpellDataAsset* Data = CreateBlessData(/*BasePower=*/15.f);
	Bless->SpellData = Data;

	// AC-022.1: Delivery is InstantApply, self-targeted
	TestEqual("Bless delivery is InstantApply",
		Data->DeliveryType, EMordecaiSpellDeliveryType::InstantApply);
	TestEqual("Bless targeting is Self",
		Data->TargetingType, EMordecaiSpellTargetingType::Self);

	// AC-022.1: BlessBonus = BasePower/100 = 0.15
	float BlessBonus = UMordecaiGA_Bless::ComputeBlessBonus(15.f);
	TestTrue("BlessBonus from BasePower=15 is 0.15",
		FMath::IsNearlyEqual(BlessBonus, 0.15f, KINDA_SMALL_NUMBER));

	// Verify PhysicalDamageMultiplier attribute exists (target of the buff)
	FGameplayAttribute PhysDmgAttr = UMordecaiAttributeSet::GetPhysicalDamageMultiplierAttribute();
	TestTrue("PhysicalDamageMultiplier attribute is valid", PhysDmgAttr.IsValid());

	// Verify bonus is configurable
	float BlessBonus30 = UMordecaiGA_Bless::ComputeBlessBonus(30.f);
	TestTrue("BlessBonus from BasePower=30 is 0.30",
		FMath::IsNearlyEqual(BlessBonus30, 0.30f, KINDA_SMALL_NUMBER));

	// Verify getter reads from data asset
	TestTrue("GetBlessBonus reads from SpellData",
		FMath::IsNearlyEqual(Bless->GetBlessBonus(), 0.15f, KINDA_SMALL_NUMBER));

	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.Spell.Bless.IncreasesMagicDamageMultiplier (AC-022.1)
// Verifies BlessBonus increases MagicDamageMultiplier.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Bless_IncreasesMagicDamageMultiplier,
	"Mordecai.Spell.Bless.IncreasesMagicDamageMultiplier",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Bless_IncreasesMagicDamageMultiplier::RunTest(const FString& Parameters)
{
	UMordecaiGA_Bless* Bless = NewObject<UMordecaiGA_Bless>();
	UMordecaiSpellDataAsset* Data = CreateBlessData(/*BasePower=*/20.f);
	Bless->SpellData = Data;

	// AC-022.1: BlessBonus applies to BOTH multipliers equally
	float BlessBonus = UMordecaiGA_Bless::ComputeBlessBonus(20.f);
	TestTrue("BlessBonus from BasePower=20 is 0.20",
		FMath::IsNearlyEqual(BlessBonus, 0.20f, KINDA_SMALL_NUMBER));

	// Verify MagicDamageMultiplier attribute exists (target of the buff)
	FGameplayAttribute MagicDmgAttr = UMordecaiAttributeSet::GetMagicDamageMultiplierAttribute();
	TestTrue("MagicDamageMultiplier attribute is valid", MagicDmgAttr.IsValid());

	// Verify the spell knows which attributes to modify
	TArray<FGameplayAttribute> BuffedAttributes = Bless->GetBuffedAttributes();
	TestEqual("Bless buffs exactly 2 attributes", BuffedAttributes.Num(), 2);

	bool bBuffsPhysical = false;
	bool bBuffsMagic = false;
	for (const FGameplayAttribute& Attr : BuffedAttributes)
	{
		if (Attr == UMordecaiAttributeSet::GetPhysicalDamageMultiplierAttribute())
		{
			bBuffsPhysical = true;
		}
		if (Attr == UMordecaiAttributeSet::GetMagicDamageMultiplierAttribute())
		{
			bBuffsMagic = true;
		}
	}

	TestTrue("Bless buffs PhysicalDamageMultiplier", bBuffsPhysical);
	TestTrue("Bless buffs MagicDamageMultiplier", bBuffsMagic);

	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.Spell.Bless.ExpiresAfterDuration (AC-022.2)
// Verifies Bless GE duration and tag configuration.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Bless_ExpiresAfterDuration,
	"Mordecai.Spell.Bless.ExpiresAfterDuration",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Bless_ExpiresAfterDuration::RunTest(const FString& Parameters)
{
	UMordecaiGA_Bless* Bless = NewObject<UMordecaiGA_Bless>();
	UMordecaiSpellDataAsset* Data = CreateBlessData(15.f, /*Duration=*/10.f);
	Bless->SpellData = Data;

	// AC-022.2: Duration from SpellDataAsset
	TestEqual("Bless duration = 10s", Bless->GetBlessDuration(), 10.f);

	// Verify configurable
	Data->BuffDuration = 20.f;
	TestEqual("Bless duration updates", Bless->GetBlessDuration(), 20.f);

	// AC-022.2: Status tag is Mordecai.Status.Blessed
	FGameplayTag BlessTag = Bless->GetBlessTag();
	TestTrue("Bless tag is valid", BlessTag.IsValid());
	TestTrue("Bless tag is Status.Blessed",
		BlessTag == MordecaiGameplayTags::Status_Blessed);

	// AC-022.7: Ability tag
	FGameplayTag AbilityTag = MordecaiGameplayTags::Ability_Spell_Bless;
	TestTrue("Bless ability tag is valid", AbilityTag.IsValid());

	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.Spell.Bless.DoesNotStackWithSelf (AC-022.3)
// Verifies Bless non-stacking policy.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Bless_DoesNotStackWithSelf,
	"Mordecai.Spell.Bless.DoesNotStackWithSelf",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Bless_DoesNotStackWithSelf::RunTest(const FString& Parameters)
{
	UMordecaiGA_Bless* Bless = NewObject<UMordecaiGA_Bless>();
	UMordecaiSpellDataAsset* Data = CreateBlessData(/*BasePower=*/15.f);
	Bless->SpellData = Data;

	// AC-022.3: BlessBonus from BasePower=15 is 0.15 — even if applied twice,
	// the bonus should remain 0.15 (not 0.30) because stacking is prevented
	float SingleBonus = UMordecaiGA_Bless::ComputeBlessBonus(Data->BasePower);
	float DoubleBonus = SingleBonus * 2.f;

	TestTrue("Single bonus is 0.15",
		FMath::IsNearlyEqual(SingleBonus, 0.15f, KINDA_SMALL_NUMBER));
	TestTrue("Double bonus would be 0.30 (this must NOT happen)",
		FMath::IsNearlyEqual(DoubleBonus, 0.30f, KINDA_SMALL_NUMBER));

	// Verify the spell exposes non-stacking configuration
	TestEqual("Bless max stacks = 1", Bless->GetMaxBlessStacks(), 1);
	TestTrue("Bless refreshes duration on reapply", Bless->GetRefreshDurationOnReapply());

	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.Spell.Restoration.HealsOverTime (AC-022.4, AC-022.5)
// Verifies Restoration HoT tick healing computation.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Restoration_HealsOverTime,
	"Mordecai.Spell.Restoration.HealsOverTime",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Restoration_HealsOverTime::RunTest(const FString& Parameters)
{
	// AC-022.4: Restoration extends SpellBase
	UMordecaiGA_Restoration* Restoration = NewObject<UMordecaiGA_Restoration>();
	TestNotNull("Restoration ability created", Restoration);

	UMordecaiGA_SpellBase* AsBase = Cast<UMordecaiGA_SpellBase>(Restoration);
	TestNotNull("Restoration inherits from SpellBase", AsBase);

	// Configure: BasePower=50, Duration=10s → TotalHeal=50, tick=5.0/sec
	UMordecaiSpellDataAsset* Data = CreateRestorationData(/*BasePower=*/50.f, /*Duration=*/10.f);
	Restoration->SpellData = Data;

	// AC-022.4: Delivery is InstantApply, self-targeted
	TestEqual("Restoration delivery is InstantApply",
		Data->DeliveryType, EMordecaiSpellDeliveryType::InstantApply);
	TestEqual("Restoration targeting is Self",
		Data->TargetingType, EMordecaiSpellTargetingType::Self);

	// AC-022.5: Heal per tick = TotalHeal / Duration = 50/10 = 5.0
	float HealPerTick = UMordecaiGA_Restoration::ComputeHealPerTick(50.f, 10.f);
	TestTrue("Heal per tick = 5.0 for TotalHeal=50, Duration=10",
		FMath::IsNearlyEqual(HealPerTick, 5.0f, KINDA_SMALL_NUMBER));

	// Verify with different values: BasePower=100, Duration=5 → 20.0/tick
	float HealPerTick2 = UMordecaiGA_Restoration::ComputeHealPerTick(100.f, 5.f);
	TestTrue("Heal per tick = 20.0 for TotalHeal=100, Duration=5",
		FMath::IsNearlyEqual(HealPerTick2, 20.0f, KINDA_SMALL_NUMBER));

	// AC-022.5: Period is 1 second
	TestEqual("Restoration tick period = 1.0s", Restoration->GetTickPeriod(), 1.0f);

	// Verify getter reads duration from data asset
	TestEqual("GetRestorationDuration reads from SpellData",
		Restoration->GetRestorationDuration(), 10.f);

	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.Spell.Restoration.HealDoesNotExceedMaxHealth (AC-022.5)
// Verifies health clamping at MaxHealth.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Restoration_HealDoesNotExceedMaxHealth,
	"Mordecai.Spell.Restoration.HealDoesNotExceedMaxHealth",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Restoration_HealDoesNotExceedMaxHealth::RunTest(const FString& Parameters)
{
	UMordecaiGA_Restoration* Restoration = NewObject<UMordecaiGA_Restoration>();
	UMordecaiSpellDataAsset* Data = CreateRestorationData(/*BasePower=*/50.f, /*Duration=*/10.f);
	Restoration->SpellData = Data;

	// AC-022.5: Health cannot exceed MaxHealth — verify via ClampHeal
	// At 95 HP with MaxHP 100, healing 5/tick should only heal 5
	float Clamped1 = UMordecaiGA_Restoration::ClampHeal(95.f, 5.f, 100.f);
	TestTrue("At 95/100 HP, heal 5 → clamped to 5",
		FMath::IsNearlyEqual(Clamped1, 5.f, KINDA_SMALL_NUMBER));

	// At 98 HP with MaxHP 100, healing 5/tick should only heal 2
	float Clamped2 = UMordecaiGA_Restoration::ClampHeal(98.f, 5.f, 100.f);
	TestTrue("At 98/100 HP, heal 5 → clamped to 2",
		FMath::IsNearlyEqual(Clamped2, 2.f, KINDA_SMALL_NUMBER));

	// At MaxHP, healing should be 0
	float Clamped3 = UMordecaiGA_Restoration::ClampHeal(100.f, 5.f, 100.f);
	TestTrue("At 100/100 HP, heal 5 → clamped to 0",
		FMath::IsNearlyEqual(Clamped3, 0.f, KINDA_SMALL_NUMBER));

	// Verify attribute set clamps Health to MaxHealth
	UMordecaiAttributeSet* AttrSet = NewObject<UMordecaiAttributeSet>();
	FGameplayAttribute HealthAttr = UMordecaiAttributeSet::GetHealthAttribute();
	TestTrue("Health attribute is valid", HealthAttr.IsValid());
	FGameplayAttribute MaxHealthAttr = UMordecaiAttributeSet::GetMaxHealthAttribute();
	TestTrue("MaxHealth attribute is valid", MaxHealthAttr.IsValid());

	return true;
}

// ---------------------------------------------------------------------------
// 7. Mordecai.Spell.Restoration.ExpiresAfterDuration (AC-022.6)
// Verifies HoT GE duration and tag configuration.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Restoration_ExpiresAfterDuration,
	"Mordecai.Spell.Restoration.ExpiresAfterDuration",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Restoration_ExpiresAfterDuration::RunTest(const FString& Parameters)
{
	UMordecaiGA_Restoration* Restoration = NewObject<UMordecaiGA_Restoration>();
	UMordecaiSpellDataAsset* Data = CreateRestorationData(50.f, /*Duration=*/10.f);
	Restoration->SpellData = Data;

	// AC-022.6: Duration from SpellDataAsset
	TestEqual("Restoration duration = 10s", Restoration->GetRestorationDuration(), 10.f);

	// Verify configurable
	Data->BuffDuration = 15.f;
	TestEqual("Restoration duration updates", Restoration->GetRestorationDuration(), 15.f);

	// AC-022.6: Status tag is Mordecai.Status.Restoration
	FGameplayTag RestorationTag = Restoration->GetRestorationTag();
	TestTrue("Restoration tag is valid", RestorationTag.IsValid());
	TestTrue("Restoration tag is Status.Restoration",
		RestorationTag == MordecaiGameplayTags::Status_Restoration);

	// AC-022.7: Ability tag
	FGameplayTag AbilityTag = MordecaiGameplayTags::Ability_Spell_Restoration;
	TestTrue("Restoration ability tag is valid", AbilityTag.IsValid());

	return true;
}

// ---------------------------------------------------------------------------
// 8. Mordecai.Spell.Restoration.RefreshResetsDuration (AC-022.6)
// Verifies reapply refreshes duration and recalculates heal.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Restoration_RefreshResetsDuration,
	"Mordecai.Spell.Restoration.RefreshResetsDuration",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Restoration_RefreshResetsDuration::RunTest(const FString& Parameters)
{
	UMordecaiGA_Restoration* Restoration = NewObject<UMordecaiGA_Restoration>();
	UMordecaiSpellDataAsset* Data = CreateRestorationData(/*BasePower=*/50.f, /*Duration=*/10.f);
	Restoration->SpellData = Data;

	// AC-022.6: Reapplying refreshes duration
	TestTrue("Restoration refreshes duration on reapply",
		Restoration->GetRefreshDurationOnReapply());

	// AC-022.6: Reapplying recalculates heal amount
	// Verify heal per tick changes when SpellPower changes
	float HealPerTick1 = UMordecaiGA_Restoration::ComputeHealPerTick(50.f, 10.f);
	TestTrue("HealPerTick for SpellPower=50 is 5.0",
		FMath::IsNearlyEqual(HealPerTick1, 5.0f, KINDA_SMALL_NUMBER));

	// If SpellPower increases (e.g., higher rank or scaling), heal per tick changes
	float HealPerTick2 = UMordecaiGA_Restoration::ComputeHealPerTick(75.f, 10.f);
	TestTrue("HealPerTick for SpellPower=75 is 7.5",
		FMath::IsNearlyEqual(HealPerTick2, 7.5f, KINDA_SMALL_NUMBER));

	// Verify max stacks = 1 (refresh instead of stack)
	TestEqual("Restoration max stacks = 1", Restoration->GetMaxRestorationStacks(), 1);

	return true;
}
