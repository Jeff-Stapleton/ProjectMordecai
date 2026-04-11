// Project Mordecai — Status-Applying Attack Profile Tests (US-056)

#include "Misc/AutomationTest.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Mordecai/Combat/MordecaiCombatTypes.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiProjectile.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Burning.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Poisoned.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Weakened.h"

// ---------------------------------------------------------------------------
// Helper: Minimal test environment with Source + Target ASCs
// ---------------------------------------------------------------------------
namespace
{
	struct FStatusOnHitTestEnv
	{
		UWorld* World = nullptr;
		AActor* SourceActor = nullptr;
		AActor* TargetActor = nullptr;
		UAbilitySystemComponent* SourceASC = nullptr;
		UAbilitySystemComponent* TargetASC = nullptr;

		bool Setup()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (!World || !GEngine)
			{
				return false;
			}

			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);

			FActorSpawnParameters SpawnParams;

			// Source actor (attacker)
			SourceActor = World->SpawnActor<AActor>(AActor::StaticClass(), SpawnParams);
			if (!SourceActor) { Teardown(); return false; }

			SourceASC = NewObject<UAbilitySystemComponent>(SourceActor, TEXT("SourceASC"));
			SourceASC->RegisterComponent();
			SourceASC->InitAbilityActorInfo(SourceActor, SourceActor);

			// Target actor (victim)
			TargetActor = World->SpawnActor<AActor>(AActor::StaticClass(), SpawnParams);
			if (!TargetActor) { Teardown(); return false; }

			TargetASC = NewObject<UAbilitySystemComponent>(TargetActor, TEXT("TargetASC"));
			TargetASC->RegisterComponent();
			TargetASC->InitAbilityActorInfo(TargetActor, TargetActor);

			return true;
		}

		void Teardown()
		{
			if (World && GEngine)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
			World = nullptr;
			SourceActor = nullptr;
			TargetActor = nullptr;
			SourceASC = nullptr;
			TargetASC = nullptr;
		}
	};
}

// ===========================================================================
// 1. Mordecai.Combat.StatusOnHit.AppliesStatusOnMeleeHit (AC-056.3)
// Melee attack with 100% chance applies the status GE to the target.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StatusOnHit_AppliesOnMelee,
	"Mordecai.Combat.StatusOnHit.AppliesStatusOnMeleeHit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StatusOnHit_AppliesOnMelee::RunTest(const FString& Parameters)
{
	FStatusOnHitTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Build StatusEffectsOnHit array with 100% chance using real GE class
	TArray<FMordecaiStatusOnHitEntry> Entries;
	FMordecaiStatusOnHitEntry Entry;
	Entry.StatusEffectTag = FGameplayTag(MordecaiGameplayTags::Status_Burning);
	Entry.ApplicationChance = 1.0f;
	Entry.StatusEffectGEClass = UMordecaiGE_Burning::StaticClass();
	Entries.Add(Entry);

	// Verify target does NOT have the status before
	TestFalse("Target does not have Burning before hit",
		Env.TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_Burning));

	// Apply status effects using the static utility (same logic called by melee ApplyDamageToTarget)
	UMordecaiGA_MeleeAttack::ApplyStatusEffectsFromEntries(Entries, Env.SourceASC, Env.TargetASC, nullptr);

	// Verify target now has the status
	TestTrue("Target has Burning after melee hit",
		Env.TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_Burning));

	Env.Teardown();
	return true;
}

// ===========================================================================
// 2. Mordecai.Combat.StatusOnHit.RespectsProbability (AC-056.3, AC-056.6)
// 0% chance never applies; 100% always applies.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StatusOnHit_RespectsProbability,
	"Mordecai.Combat.StatusOnHit.RespectsProbability",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StatusOnHit_RespectsProbability::RunTest(const FString& Parameters)
{
	FStatusOnHitTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// --- 0% chance: should NEVER apply ---
	{
		TArray<FMordecaiStatusOnHitEntry> Entries;
		FMordecaiStatusOnHitEntry Entry;
		Entry.StatusEffectTag = FGameplayTag(MordecaiGameplayTags::Status_Burning);
		Entry.ApplicationChance = 0.0f;
		Entry.StatusEffectGEClass = UMordecaiGE_Burning::StaticClass();
		Entries.Add(Entry);

		// Apply multiple times to be confident
		for (int32 i = 0; i < 10; ++i)
		{
			UMordecaiGA_MeleeAttack::ApplyStatusEffectsFromEntries(Entries, Env.SourceASC, Env.TargetASC, nullptr);
		}

		TestFalse("0% chance never applies Burning",
			Env.TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_Burning));
	}

	// --- 100% chance: should ALWAYS apply ---
	{
		TArray<FMordecaiStatusOnHitEntry> Entries;
		FMordecaiStatusOnHitEntry Entry;
		Entry.StatusEffectTag = FGameplayTag(MordecaiGameplayTags::Status_Burning);
		Entry.ApplicationChance = 1.0f;
		Entry.StatusEffectGEClass = UMordecaiGE_Burning::StaticClass();
		Entries.Add(Entry);

		UMordecaiGA_MeleeAttack::ApplyStatusEffectsFromEntries(Entries, Env.SourceASC, Env.TargetASC, nullptr);

		TestTrue("100% chance always applies Burning",
			Env.TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_Burning));
	}

	Env.Teardown();
	return true;
}

// ===========================================================================
// 3. Mordecai.Combat.StatusOnHit.RespectImmunity (AC-056.4)
// Target with immunity tag is not affected even when roll succeeds.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StatusOnHit_RespectsImmunity,
	"Mordecai.Combat.StatusOnHit.RespectImmunity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StatusOnHit_RespectsImmunity::RunTest(const FString& Parameters)
{
	FStatusOnHitTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Grant immunity to Burning on the target
	Env.TargetASC->AddLooseGameplayTag(MordecaiGameplayTags::Immunity_Burning);
	TestTrue("Target has Burning immunity",
		Env.TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::Immunity_Burning));

	// Build entry with 100% chance using real GE class
	TArray<FMordecaiStatusOnHitEntry> Entries;
	FMordecaiStatusOnHitEntry Entry;
	Entry.StatusEffectTag = FGameplayTag(MordecaiGameplayTags::Status_Burning);
	Entry.ApplicationChance = 1.0f;
	Entry.StatusEffectGEClass = UMordecaiGE_Burning::StaticClass();
	Entries.Add(Entry);

	// Apply — should be blocked by immunity
	UMordecaiGA_MeleeAttack::ApplyStatusEffectsFromEntries(Entries, Env.SourceASC, Env.TargetASC, nullptr);

	TestFalse("Burning is NOT applied to immune target",
		Env.TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_Burning));

	Env.Teardown();
	return true;
}

// ===========================================================================
// 4. Mordecai.Combat.StatusOnHit.AppliesStatusOnProjectileHit (AC-056.5)
// Projectile with StatusEffectsOnHit applies status to target.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StatusOnHit_AppliesOnProjectile,
	"Mordecai.Combat.StatusOnHit.AppliesStatusOnProjectileHit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StatusOnHit_AppliesOnProjectile::RunTest(const FString& Parameters)
{
	FStatusOnHitTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Create entries using real GE class
	TArray<FMordecaiStatusOnHitEntry> Entries;
	FMordecaiStatusOnHitEntry Entry;
	Entry.StatusEffectTag = FGameplayTag(MordecaiGameplayTags::Status_Poisoned);
	Entry.ApplicationChance = 1.0f;
	Entry.StatusEffectGEClass = UMordecaiGE_Poisoned::StaticClass();
	Entries.Add(Entry);

	// Spawn a projectile and set its status entries
	FActorSpawnParameters SpawnParams;
	AMordecaiProjectile* Projectile = Env.World->SpawnActor<AMordecaiProjectile>(SpawnParams);
	TestNotNull("Projectile spawned", Projectile);
	if (!Projectile) { Env.Teardown(); return true; }

	Projectile->SetStatusEffectsOnHit(Entries);
	TestEqual("Projectile has 1 status entry", Projectile->GetStatusEffectsOnHit().Num(), 1);

	// Use the same static utility to verify the application logic works
	// (same function that the projectile's ApplyDamageToTarget will call)
	UMordecaiGA_MeleeAttack::ApplyStatusEffectsFromEntries(
		Projectile->GetStatusEffectsOnHit(), Env.SourceASC, Env.TargetASC, Projectile);

	TestTrue("Target has Poisoned after projectile hit",
		Env.TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_Poisoned));

	Env.Teardown();
	return true;
}

// ===========================================================================
// 5. Mordecai.Combat.StatusOnHit.MultipleStatusesResolveIndependently (AC-056.7)
// Attack with 2 status entries rolls each independently.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StatusOnHit_MultipleIndependent,
	"Mordecai.Combat.StatusOnHit.MultipleStatusesResolveIndependently",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StatusOnHit_MultipleIndependent::RunTest(const FString& Parameters)
{
	FStatusOnHitTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Build entries: Burning at 100%, Weakened at 100%
	TArray<FMordecaiStatusOnHitEntry> Entries;

	FMordecaiStatusOnHitEntry BurnEntry;
	BurnEntry.StatusEffectTag = FGameplayTag(MordecaiGameplayTags::Status_Burning);
	BurnEntry.ApplicationChance = 1.0f;
	BurnEntry.StatusEffectGEClass = UMordecaiGE_Burning::StaticClass();
	Entries.Add(BurnEntry);

	FMordecaiStatusOnHitEntry WeakenEntry;
	WeakenEntry.StatusEffectTag = FGameplayTag(MordecaiGameplayTags::Status_Weakened);
	WeakenEntry.ApplicationChance = 1.0f;
	WeakenEntry.StatusEffectGEClass = UMordecaiGE_Weakened::StaticClass();
	Entries.Add(WeakenEntry);

	// Apply
	UMordecaiGA_MeleeAttack::ApplyStatusEffectsFromEntries(Entries, Env.SourceASC, Env.TargetASC, nullptr);

	// Both should be present
	TestTrue("Target has Burning",
		Env.TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_Burning));
	TestTrue("Target has Weakened",
		Env.TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_Weakened));

	Env.Teardown();
	return true;
}

// ===========================================================================
// 6. Mordecai.Combat.StatusOnHit.EmptyArrayNoEffect (AC-056.2)
// Attack with empty StatusEffectsOnHit applies no statuses.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StatusOnHit_EmptyArrayNoEffect,
	"Mordecai.Combat.StatusOnHit.EmptyArrayNoEffect",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StatusOnHit_EmptyArrayNoEffect::RunTest(const FString& Parameters)
{
	FStatusOnHitTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Default attack profile — StatusEffectsOnHit should be empty
	UMordecaiAttackProfileDataAsset* Profile = NewObject<UMordecaiAttackProfileDataAsset>();
	TestEqual("Default StatusEffectsOnHit is empty", Profile->StatusEffectsOnHit.Num(), 0);

	// Apply with empty array — should do nothing
	UMordecaiGA_MeleeAttack::ApplyStatusEffectsFromEntries(
		Profile->StatusEffectsOnHit, Env.SourceASC, Env.TargetASC, nullptr);

	// Verify no status tags on target
	TestFalse("No Burning", Env.TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_Burning));
	TestFalse("No Poisoned", Env.TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_Poisoned));
	TestFalse("No Weakened", Env.TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_Weakened));

	Env.Teardown();
	return true;
}
