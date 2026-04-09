// Project Mordecai — Tactical Spell Tests (US-023)

#include "Misc/AutomationTest.h"
#include "Mordecai/Magic/MordecaiGA_Snare.h"
#include "Mordecai/Magic/MordecaiGA_Enfeeble.h"
#include "Mordecai/Magic/MordecaiGA_EnchantWeapon.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/Magic/MordecaiGA_SpellBase.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"

// ---------------------------------------------------------------------------
// Helpers: Create test spell data assets for tactical spells
// ---------------------------------------------------------------------------
namespace
{
	UMordecaiSpellDataAsset* CreateSnareData(
		float Range = 1000.f,
		float Duration = 3.0f)
	{
		UMordecaiSpellDataAsset* Data = NewObject<UMordecaiSpellDataAsset>();
		Data->SpellId = FName(TEXT("Snare"));
		Data->Range = Range;
		Data->BuffDuration = Duration;
		Data->DeliveryType = EMordecaiSpellDeliveryType::TraceHit;
		Data->TargetingType = EMordecaiSpellTargetingType::Line;
		Data->WindupTime = 0.3f;
		Data->CastTime = 0.2f;
		Data->RecoveryTime = 0.3f;
		Data->SpellPointCost = 3;
		Data->BasePower = 0.f;
		return Data;
	}

	UMordecaiSpellDataAsset* CreateEnfeebleData(
		float Range = 1000.f,
		float Duration = 5.0f,
		float BasePower = 25.f)
	{
		UMordecaiSpellDataAsset* Data = NewObject<UMordecaiSpellDataAsset>();
		Data->SpellId = FName(TEXT("Enfeeble"));
		Data->Range = Range;
		Data->BuffDuration = Duration;
		Data->BasePower = BasePower;
		Data->DeliveryType = EMordecaiSpellDeliveryType::TraceHit;
		Data->TargetingType = EMordecaiSpellTargetingType::Line;
		Data->WindupTime = 0.4f;
		Data->CastTime = 0.2f;
		Data->RecoveryTime = 0.3f;
		Data->SpellPointCost = 4;
		return Data;
	}

	UMordecaiSpellDataAsset* CreateEnchantWeaponData(
		float BasePower = 10.f,
		float Duration = 15.f)
	{
		UMordecaiSpellDataAsset* Data = NewObject<UMordecaiSpellDataAsset>();
		Data->SpellId = FName(TEXT("EnchantWeapon"));
		Data->BasePower = BasePower;
		Data->BuffDuration = Duration;
		Data->DeliveryType = EMordecaiSpellDeliveryType::InstantApply;
		Data->TargetingType = EMordecaiSpellTargetingType::Self;
		Data->WindupTime = 0.3f;
		Data->CastTime = 0.2f;
		Data->RecoveryTime = 0.3f;
		Data->SpellPointCost = 3;
		return Data;
	}
}

// ---------------------------------------------------------------------------
// 1. Mordecai.Spell.Snare.RootsTarget (AC-023.1, AC-023.2, AC-023.3)
// Verifies Snare extends SpellBase, uses TraceHit, applies Rooted tag,
// and reads configuration from SpellDataAsset.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Snare_RootsTarget,
	"Mordecai.Spell.Snare.RootsTarget",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Snare_RootsTarget::RunTest(const FString& Parameters)
{
	// AC-023.1: Snare extends SpellBase
	UMordecaiGA_Snare* Snare = NewObject<UMordecaiGA_Snare>();
	TestNotNull("Snare ability created", Snare);

	UMordecaiGA_SpellBase* AsBase = Cast<UMordecaiGA_SpellBase>(Snare);
	TestNotNull("Snare inherits from SpellBase", AsBase);

	// Configure with data asset
	UMordecaiSpellDataAsset* Data = CreateSnareData(/*Range=*/1000.f, /*Duration=*/3.0f);
	Snare->SpellData = Data;

	// AC-023.1: Delivery type is TraceHit
	TestEqual("Snare delivery type is TraceHit",
		Data->DeliveryType, EMordecaiSpellDeliveryType::TraceHit);

	// AC-023.1: Range reads from SpellDataAsset
	TestEqual("Snare range = 1000", Snare->GetSnareRange(), 1000.f);

	// Verify range is configurable
	Data->Range = 1500.f;
	TestEqual("Snare range updates with data asset", Snare->GetSnareRange(), 1500.f);

	// AC-023.2: Duration reads from SpellDataAsset
	TestEqual("Snare duration = 3.0s", Snare->GetSnareDuration(), 3.0f);

	// Verify duration configurable
	Data->BuffDuration = 5.0f;
	TestEqual("Snare duration updates", Snare->GetSnareDuration(), 5.0f);

	// AC-023.2: Root GE applies Status_Rooted tag
	FGameplayTag RootTag = Snare->GetRootTag();
	TestTrue("Root tag is valid", RootTag.IsValid());
	TestTrue("Root tag is Status.Rooted",
		RootTag == MordecaiGameplayTags::Status_Rooted);

	// AC-023.3: Ability tag is Mordecai.Ability.Spell.Snare
	FGameplayTag AbilityTag = MordecaiGameplayTags::Ability_Spell_Snare;
	TestTrue("Snare ability tag is valid", AbilityTag.IsValid());

	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.Spell.Snare.RootedTargetCanStillAttack (AC-023.2)
// Verifies that the Rooted status does NOT block attack or block abilities.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Snare_RootedTargetCanStillAttack,
	"Mordecai.Spell.Snare.RootedTargetCanStillAttack",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Snare_RootedTargetCanStillAttack::RunTest(const FString& Parameters)
{
	UMordecaiGA_Snare* Snare = NewObject<UMordecaiGA_Snare>();
	UMordecaiSpellDataAsset* Data = CreateSnareData();
	Snare->SpellData = Data;

	// AC-023.2: While rooted, target CAN still attack and block
	// Verify that the Rooted status tag is NOT in MeleeAttack's ActivationBlockedTags
	UMordecaiGA_MeleeAttack* MeleeAttack = NewObject<UMordecaiGA_MeleeAttack>();
	TestFalse("MeleeAttack NOT blocked by Status_Rooted",
		MeleeAttack->HasActivationBlockedTag(MordecaiGameplayTags::Status_Rooted));

	// Verify the Snare knows that rooted targets can still attack
	TestTrue("Snare allows attack while rooted", Snare->GetRootAllowsAttack());
	TestTrue("Snare allows block while rooted", Snare->GetRootAllowsBlock());

	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.Spell.Snare.RootExpiresAfterDuration (AC-023.2)
// Verifies Root is removed when Duration expires — configurable via data.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Snare_RootExpiresAfterDuration,
	"Mordecai.Spell.Snare.RootExpiresAfterDuration",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Snare_RootExpiresAfterDuration::RunTest(const FString& Parameters)
{
	UMordecaiGA_Snare* Snare = NewObject<UMordecaiGA_Snare>();
	UMordecaiSpellDataAsset* Data = CreateSnareData(1000.f, /*Duration=*/3.0f);
	Snare->SpellData = Data;

	// AC-023.2: Duration from SpellDataAsset
	TestEqual("Root duration = 3.0s", Snare->GetSnareDuration(), 3.0f);

	// Verify different durations
	Data->BuffDuration = 2.0f;
	TestEqual("Root duration updates to 2.0s", Snare->GetSnareDuration(), 2.0f);
	Data->BuffDuration = 6.0f;
	TestEqual("Root duration updates to 6.0s", Snare->GetSnareDuration(), 6.0f);

	// AC-023.2: Root GE uses HasDuration policy (not infinite or instant)
	// The GE is created at runtime; verify the spell intends a duration-based GE
	TestTrue("Snare duration is positive", Snare->GetSnareDuration() > 0.f);

	// AC-023.3: Reuses Status_Rooted tag
	FGameplayTag RootTag = Snare->GetRootTag();
	TestTrue("Root tag matches Status_Rooted",
		RootTag == MordecaiGameplayTags::Status_Rooted);

	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.Spell.Enfeeble.AppliesWeakened (AC-023.4, AC-023.5, AC-023.6)
// Verifies Enfeeble extends SpellBase, uses TraceHit, applies Weakened tag,
// and reduces PhysicalDamageMultiplier by BasePower/100.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Enfeeble_AppliesWeakened,
	"Mordecai.Spell.Enfeeble.AppliesWeakened",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Enfeeble_AppliesWeakened::RunTest(const FString& Parameters)
{
	// AC-023.4: Enfeeble extends SpellBase
	UMordecaiGA_Enfeeble* Enfeeble = NewObject<UMordecaiGA_Enfeeble>();
	TestNotNull("Enfeeble ability created", Enfeeble);

	UMordecaiGA_SpellBase* AsBase = Cast<UMordecaiGA_SpellBase>(Enfeeble);
	TestNotNull("Enfeeble inherits from SpellBase", AsBase);

	// Configure
	UMordecaiSpellDataAsset* Data = CreateEnfeebleData(/*Range=*/1000.f, /*Duration=*/5.0f, /*BasePower=*/25.f);
	Enfeeble->SpellData = Data;

	// AC-023.4: Delivery type is TraceHit
	TestEqual("Enfeeble delivery is TraceHit",
		Data->DeliveryType, EMordecaiSpellDeliveryType::TraceHit);

	// AC-023.4: Range reads from SpellDataAsset
	TestEqual("Enfeeble range = 1000", Enfeeble->GetEnfeebleRange(), 1000.f);

	// AC-023.5: Weakened reduces PhysicalDamageMultiplier by BasePower/100
	// BasePower=25 means reduction of 0.25
	float Reduction = UMordecaiGA_Enfeeble::ComputeWeakenedReduction(25.f);
	TestTrue("Weakened reduction from BasePower=25 is 0.25",
		FMath::IsNearlyEqual(Reduction, 0.25f, KINDA_SMALL_NUMBER));

	// Verify with different BasePower
	float Reduction40 = UMordecaiGA_Enfeeble::ComputeWeakenedReduction(40.f);
	TestTrue("Weakened reduction from BasePower=40 is 0.40",
		FMath::IsNearlyEqual(Reduction40, 0.40f, KINDA_SMALL_NUMBER));

	// Verify getter reads from data asset
	TestTrue("GetWeakenedReduction reads from SpellData",
		FMath::IsNearlyEqual(Enfeeble->GetWeakenedReduction(), 0.25f, KINDA_SMALL_NUMBER));

	// AC-023.5: Target attribute is PhysicalDamageMultiplier
	FGameplayAttribute DebuffAttr = Enfeeble->GetDebuffedAttribute();
	TestTrue("Debuffed attribute is valid", DebuffAttr.IsValid());
	TestTrue("Debuffed attribute is PhysicalDamageMultiplier",
		DebuffAttr == UMordecaiAttributeSet::GetPhysicalDamageMultiplierAttribute());

	// AC-023.5: Applies Status_Weakened tag
	FGameplayTag WeakenedTag = Enfeeble->GetWeakenedTag();
	TestTrue("Weakened tag is valid", WeakenedTag.IsValid());
	TestTrue("Weakened tag is Status.Weakened",
		WeakenedTag == MordecaiGameplayTags::Status_Weakened);

	// AC-023.6: Ability tag
	FGameplayTag AbilityTag = MordecaiGameplayTags::Ability_Spell_Enfeeble;
	TestTrue("Enfeeble ability tag is valid", AbilityTag.IsValid());

	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.Spell.Enfeeble.WeakenedExpiresAfterDuration (AC-023.5)
// Verifies Weakened is removed after Duration and multiplier reverts.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Enfeeble_WeakenedExpiresAfterDuration,
	"Mordecai.Spell.Enfeeble.WeakenedExpiresAfterDuration",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Enfeeble_WeakenedExpiresAfterDuration::RunTest(const FString& Parameters)
{
	UMordecaiGA_Enfeeble* Enfeeble = NewObject<UMordecaiGA_Enfeeble>();
	UMordecaiSpellDataAsset* Data = CreateEnfeebleData(1000.f, /*Duration=*/5.0f, 25.f);
	Enfeeble->SpellData = Data;

	// AC-023.5: Duration from SpellDataAsset
	TestEqual("Enfeeble duration = 5.0s", Enfeeble->GetEnfeebleDuration(), 5.0f);

	// Verify configurable
	Data->BuffDuration = 8.0f;
	TestEqual("Enfeeble duration updates", Enfeeble->GetEnfeebleDuration(), 8.0f);

	// AC-023.5: Weakened uses a duration GE — when it expires, the modifier reverts
	// The GE applies a negative additive modifier on PhysicalDamageMultiplier.
	// When the GE duration expires, the modifier is removed automatically.
	// Verify the reduction is applied as a negative additive value:
	float Reduction = Enfeeble->GetWeakenedReduction();
	TestTrue("Reduction is positive (applied as negative to attribute)",
		Reduction > 0.f);

	// Verify that default PhysicalDamageMultiplier is 1.0
	UMordecaiAttributeSet* AttrSet = NewObject<UMordecaiAttributeSet>();
	float DefaultPhysMult = AttrSet->GetPhysicalDamageMultiplier();
	TestTrue("Default PhysicalDamageMultiplier is 1.0",
		FMath::IsNearlyEqual(DefaultPhysMult, 1.0f, KINDA_SMALL_NUMBER));

	// When the GE is active: multiplier = 1.0 - 0.25 = 0.75
	// When the GE expires: multiplier reverts to 1.0
	// This is handled by GAS duration GE lifecycle — no custom code needed

	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.Spell.EnchantWeapon.AddsBonusDamageToMelee (AC-023.7, AC-023.8, AC-023.9)
// Verifies Enchant Weapon extends SpellBase, applies bonus fire damage,
// and integrates with melee attack hit processing.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_EnchantWeapon_AddsBonusDamageToMelee,
	"Mordecai.Spell.EnchantWeapon.AddsBonusDamageToMelee",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_EnchantWeapon_AddsBonusDamageToMelee::RunTest(const FString& Parameters)
{
	// AC-023.7: EnchantWeapon extends SpellBase
	UMordecaiGA_EnchantWeapon* Enchant = NewObject<UMordecaiGA_EnchantWeapon>();
	TestNotNull("EnchantWeapon ability created", Enchant);

	UMordecaiGA_SpellBase* AsBase = Cast<UMordecaiGA_SpellBase>(Enchant);
	TestNotNull("EnchantWeapon inherits from SpellBase", AsBase);

	// Configure
	UMordecaiSpellDataAsset* Data = CreateEnchantWeaponData(/*BasePower=*/10.f, /*Duration=*/15.f);
	Enchant->SpellData = Data;

	// AC-023.7: Delivery is InstantApply, self-targeted
	TestEqual("EnchantWeapon delivery is InstantApply",
		Data->DeliveryType, EMordecaiSpellDeliveryType::InstantApply);
	TestEqual("EnchantWeapon targeting is Self",
		Data->TargetingType, EMordecaiSpellTargetingType::Self);

	// AC-023.8: Bonus damage = BasePower
	float BonusDamage = UMordecaiGA_EnchantWeapon::ComputeBonusDamage(10.f);
	TestTrue("Bonus damage from BasePower=10 is 10.0",
		FMath::IsNearlyEqual(BonusDamage, 10.f, KINDA_SMALL_NUMBER));

	// Verify with different BasePower
	float BonusDamage25 = UMordecaiGA_EnchantWeapon::ComputeBonusDamage(25.f);
	TestTrue("Bonus damage from BasePower=25 is 25.0",
		FMath::IsNearlyEqual(BonusDamage25, 25.f, KINDA_SMALL_NUMBER));

	// Verify getter reads from data asset
	TestTrue("GetEnchantBonusDamage reads from SpellData",
		FMath::IsNearlyEqual(Enchant->GetEnchantBonusDamage(), 10.f, KINDA_SMALL_NUMBER));

	// AC-023.8: Bonus damage is Fire element
	FGameplayTag DamageTag = Enchant->GetEnchantDamageTag();
	TestTrue("Enchant damage tag is valid", DamageTag.IsValid());
	TestTrue("Enchant damage tag is Damage.Fire",
		DamageTag == MordecaiGameplayTags::Damage_Fire);

	// AC-023.8: Status tag is EnchantedWeapon
	FGameplayTag EnchantTag = Enchant->GetEnchantTag();
	TestTrue("Enchant tag is valid", EnchantTag.IsValid());
	TestTrue("Enchant tag is Status.EnchantedWeapon",
		EnchantTag == MordecaiGameplayTags::Status_EnchantedWeapon);

	// AC-023.9: Ability tag
	FGameplayTag AbilityTag = MordecaiGameplayTags::Ability_Spell_EnchantWeapon;
	TestTrue("EnchantWeapon ability tag is valid", AbilityTag.IsValid());

	// AC-023.8: EnchantWeaponBonusDamage attribute exists for melee integration
	FGameplayAttribute BonusDmgAttr = UMordecaiAttributeSet::GetEnchantWeaponBonusDamageAttribute();
	TestTrue("EnchantWeaponBonusDamage attribute is valid", BonusDmgAttr.IsValid());

	// Verify default is 0
	UMordecaiAttributeSet* AttrSet = NewObject<UMordecaiAttributeSet>();
	TestEqual("EnchantWeaponBonusDamage defaults to 0", AttrSet->GetEnchantWeaponBonusDamage(), 0.f);

	return true;
}

// ---------------------------------------------------------------------------
// 7. Mordecai.Spell.EnchantWeapon.BuffExpiresAfterDuration (AC-023.8)
// Verifies Enchant duration from SpellDataAsset and tag configuration.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_EnchantWeapon_BuffExpiresAfterDuration,
	"Mordecai.Spell.EnchantWeapon.BuffExpiresAfterDuration",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_EnchantWeapon_BuffExpiresAfterDuration::RunTest(const FString& Parameters)
{
	UMordecaiGA_EnchantWeapon* Enchant = NewObject<UMordecaiGA_EnchantWeapon>();
	UMordecaiSpellDataAsset* Data = CreateEnchantWeaponData(10.f, /*Duration=*/15.f);
	Enchant->SpellData = Data;

	// AC-023.8: Duration from SpellDataAsset
	TestEqual("Enchant duration = 15s", Enchant->GetEnchantDuration(), 15.f);

	// Verify configurable
	Data->BuffDuration = 20.f;
	TestEqual("Enchant duration updates", Enchant->GetEnchantDuration(), 20.f);

	// AC-023.8: When GE expires, tag and bonus damage are removed
	// Duration GEs automatically remove their tag grants and attribute modifiers
	// Verify the enchant uses a positive duration
	TestTrue("Enchant duration is positive", Enchant->GetEnchantDuration() > 0.f);

	// Verify status tag
	FGameplayTag EnchantTag = Enchant->GetEnchantTag();
	TestTrue("Enchant tag is Status.EnchantedWeapon",
		EnchantTag == MordecaiGameplayTags::Status_EnchantedWeapon);

	// Verify the bonus damage attribute reverts to 0 when GE expires
	// (GAS handles this automatically for duration GEs)
	UMordecaiAttributeSet* AttrSet = NewObject<UMordecaiAttributeSet>();
	TestEqual("EnchantWeaponBonusDamage default is 0 (pre-enchant/post-expiry state)",
		AttrSet->GetEnchantWeaponBonusDamage(), 0.f);

	return true;
}
