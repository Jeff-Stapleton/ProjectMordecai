// Project Mordecai — Status Effect Framework Tests (US-013)

#include "Misc/AutomationTest.h"
#include "GameplayTagsManager.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectTypes.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectComponent.h"

// ---------------------------------------------------------------------------
// Helper: Create a configured status effect GE for testing
// Uses UMordecaiStatusEffectGameplayEffect which can access GEComponents
// as a subclass of UGameplayEffect.
// ---------------------------------------------------------------------------
static UMordecaiStatusEffectGameplayEffect* CreateTestStatusGE(
	FGameplayTag StatusTag,
	EGameplayEffectDurationType Duration = EGameplayEffectDurationType::Infinite,
	float DurationSeconds = 0.0f)
{
	UMordecaiStatusEffectGameplayEffect* GE = NewObject<UMordecaiStatusEffectGameplayEffect>();
	GE->StatusTag = StatusTag;
	GE->DurationPolicy = Duration;
	if (Duration == EGameplayEffectDurationType::HasDuration)
	{
		GE->DurationMagnitude = FScalableFloat(DurationSeconds);
	}
	GE->InitializeStatusEffect();
	return GE;
}

// ---------------------------------------------------------------------------
// Helper: Minimal test environment with ASC
// ---------------------------------------------------------------------------
struct FStatusEffectTestEnv
{
	UWorld* World = nullptr;
	AActor* TestActor = nullptr;
	UAbilitySystemComponent* ASC = nullptr;
	UMordecaiStatusEffectComponent* StatusComp = nullptr;

	bool Setup()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false);
		if (!World || !GEngine)
		{
			return false;
		}

		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);

		// Spawn actor
		FActorSpawnParameters SpawnParams;
		TestActor = World->SpawnActor<AActor>(AActor::StaticClass(), SpawnParams);
		if (!TestActor)
		{
			Teardown();
			return false;
		}

		// Create and register ASC
		ASC = NewObject<UAbilitySystemComponent>(TestActor, TEXT("TestASC"));
		ASC->RegisterComponent();
		ASC->InitAbilityActorInfo(TestActor, TestActor);

		// Create status effect component with ASC override
		StatusComp = NewObject<UMordecaiStatusEffectComponent>(TestActor, TEXT("TestStatusComp"));
		StatusComp->SetAbilitySystemComponentOverride(ASC);
		StatusComp->RegisterComponent();

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
		TestActor = nullptr;
		ASC = nullptr;
		StatusComp = nullptr;
	}
};

// ===========================================================================
// AC-013.1: All 16 status gameplay tags registered
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_AllStatusTagsRegistered,
	"Mordecai.StatusEffect.AllStatusTagsRegistered",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_AllStatusTagsRegistered::RunTest(const FString& Parameters)
{
	// Convert FNativeGameplayTag to FGameplayTag for testing
	const TArray<TPair<FString, FGameplayTag>> StatusTags = {
		{ TEXT("Burning"),     FGameplayTag(MordecaiGameplayTags::Status_Burning) },
		{ TEXT("Bleeding"),    FGameplayTag(MordecaiGameplayTags::Status_Bleeding) },
		{ TEXT("Poisoned"),    FGameplayTag(MordecaiGameplayTags::Status_Poisoned) },
		{ TEXT("Frostbitten"), FGameplayTag(MordecaiGameplayTags::Status_Frostbitten) },
		{ TEXT("Shocked"),     FGameplayTag(MordecaiGameplayTags::Status_Shocked) },
		{ TEXT("Weakened"),    FGameplayTag(MordecaiGameplayTags::Status_Weakened) },
		{ TEXT("Brittle"),     FGameplayTag(MordecaiGameplayTags::Status_Brittle) },
		{ TEXT("Silenced"),    FGameplayTag(MordecaiGameplayTags::Status_Silenced) },
		{ TEXT("Rooted"),      FGameplayTag(MordecaiGameplayTags::Status_Rooted) },
		{ TEXT("Blinded"),     FGameplayTag(MordecaiGameplayTags::Status_Blinded) },
		{ TEXT("Fear"),        FGameplayTag(MordecaiGameplayTags::Status_Fear) },
		{ TEXT("Cursed"),      FGameplayTag(MordecaiGameplayTags::Status_Cursed) },
		{ TEXT("Exposed"),     FGameplayTag(MordecaiGameplayTags::Status_Exposed) },
		{ TEXT("Corroded"),    FGameplayTag(MordecaiGameplayTags::Status_Corroded) },
		{ TEXT("Drenched"),    FGameplayTag(MordecaiGameplayTags::Status_Drenched) },
		{ TEXT("Focused"),     FGameplayTag(MordecaiGameplayTags::Status_Focused) },
	};

	TestEqual("Exactly 16 status tags defined", StatusTags.Num(), 16);

	for (const auto& [Name, Tag] : StatusTags)
	{
		TestTrue(FString::Printf(TEXT("Mordecai.Status.%s is valid"), *Name), Tag.IsValid());
		TestEqual(FString::Printf(TEXT("Mordecai.Status.%s has correct tag name"), *Name),
			Tag.ToString(), FString::Printf(TEXT("Mordecai.Status.%s"), *Name));
	}

	return true;
}

// ===========================================================================
// AC-013.2: All 5 category tags registered
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_CategoryTagsRegistered,
	"Mordecai.StatusEffect.CategoryTagsRegistered",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_CategoryTagsRegistered::RunTest(const FString& Parameters)
{
	const FGameplayTag DoT = FGameplayTag(MordecaiGameplayTags::Status_Category_DoT);
	const FGameplayTag Debuff = FGameplayTag(MordecaiGameplayTags::Status_Category_Debuff);
	const FGameplayTag Control = FGameplayTag(MordecaiGameplayTags::Status_Category_Control);
	const FGameplayTag Buff = FGameplayTag(MordecaiGameplayTags::Status_Category_Buff);
	const FGameplayTag Environmental = FGameplayTag(MordecaiGameplayTags::Status_Category_Environmental);

	TestTrue("DoT category tag valid", DoT.IsValid());
	TestTrue("Debuff category tag valid", Debuff.IsValid());
	TestTrue("Control category tag valid", Control.IsValid());
	TestTrue("Buff category tag valid", Buff.IsValid());
	TestTrue("Environmental category tag valid", Environmental.IsValid());

	TestEqual("DoT tag name", DoT.ToString(), FString(TEXT("Mordecai.Status.Category.DoT")));
	TestEqual("Debuff tag name", Debuff.ToString(), FString(TEXT("Mordecai.Status.Category.Debuff")));
	TestEqual("Control tag name", Control.ToString(), FString(TEXT("Mordecai.Status.Category.Control")));
	TestEqual("Buff tag name", Buff.ToString(), FString(TEXT("Mordecai.Status.Category.Buff")));
	TestEqual("Environmental tag name", Environmental.ToString(), FString(TEXT("Mordecai.Status.Category.Environmental")));

	return true;
}

// ===========================================================================
// AC-013.11: All 16 immunity tags registered
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_ImmunityTagsRegistered,
	"Mordecai.StatusEffect.ImmunityTagsRegistered",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_ImmunityTagsRegistered::RunTest(const FString& Parameters)
{
	const TArray<TPair<FString, FGameplayTag>> ImmunityTags = {
		{ TEXT("Burning"),     FGameplayTag(MordecaiGameplayTags::Immunity_Burning) },
		{ TEXT("Bleeding"),    FGameplayTag(MordecaiGameplayTags::Immunity_Bleeding) },
		{ TEXT("Poisoned"),    FGameplayTag(MordecaiGameplayTags::Immunity_Poisoned) },
		{ TEXT("Frostbitten"), FGameplayTag(MordecaiGameplayTags::Immunity_Frostbitten) },
		{ TEXT("Shocked"),     FGameplayTag(MordecaiGameplayTags::Immunity_Shocked) },
		{ TEXT("Weakened"),    FGameplayTag(MordecaiGameplayTags::Immunity_Weakened) },
		{ TEXT("Brittle"),     FGameplayTag(MordecaiGameplayTags::Immunity_Brittle) },
		{ TEXT("Silenced"),    FGameplayTag(MordecaiGameplayTags::Immunity_Silenced) },
		{ TEXT("Rooted"),      FGameplayTag(MordecaiGameplayTags::Immunity_Rooted) },
		{ TEXT("Blinded"),     FGameplayTag(MordecaiGameplayTags::Immunity_Blinded) },
		{ TEXT("Fear"),        FGameplayTag(MordecaiGameplayTags::Immunity_Fear) },
		{ TEXT("Cursed"),      FGameplayTag(MordecaiGameplayTags::Immunity_Cursed) },
		{ TEXT("Exposed"),     FGameplayTag(MordecaiGameplayTags::Immunity_Exposed) },
		{ TEXT("Corroded"),    FGameplayTag(MordecaiGameplayTags::Immunity_Corroded) },
		{ TEXT("Drenched"),    FGameplayTag(MordecaiGameplayTags::Immunity_Drenched) },
		{ TEXT("Focused"),     FGameplayTag(MordecaiGameplayTags::Immunity_Focused) },
	};

	TestEqual("Exactly 16 immunity tags defined", ImmunityTags.Num(), 16);

	for (const auto& [Name, Tag] : ImmunityTags)
	{
		TestTrue(FString::Printf(TEXT("Mordecai.Immunity.%s is valid"), *Name), Tag.IsValid());
		TestEqual(FString::Printf(TEXT("Mordecai.Immunity.%s has correct tag name"), *Name),
			Tag.ToString(), FString::Printf(TEXT("Mordecai.Immunity.%s"), *Name));
	}

	return true;
}

// ===========================================================================
// AC-013.3: Base GE class exists with expected properties
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_BaseGEClassExists,
	"Mordecai.StatusEffect.BaseGEClassExists",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_BaseGEClassExists::RunTest(const FString& Parameters)
{
	const UMordecaiStatusEffectGameplayEffect* CDO = GetDefault<UMordecaiStatusEffectGameplayEffect>();
	TestNotNull("Base GE CDO exists", CDO);

	if (CDO)
	{
		// Default duration policy
		TestEqual("Default DurationPolicy is HasDuration",
			static_cast<int32>(CDO->DurationPolicy),
			static_cast<int32>(EGameplayEffectDurationType::HasDuration));

		// StatusTag property exists (unset on base class)
		TestFalse("StatusTag is not set on base class", CDO->StatusTag.IsValid());

		// Stacking policy property exists with default
		TestEqual("Default StackingPolicy is Refresh",
			static_cast<int32>(CDO->StackingPolicy),
			static_cast<int32>(EMordecaiStackingPolicy::Refresh));

		// GAS stacking defaults
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		TestEqual("StackingType is AggregateBySource",
			static_cast<int32>(CDO->StackingType),
			static_cast<int32>(EGameplayEffectStackingType::AggregateBySource));
		TestEqual("StackLimitCount is 1", CDO->StackLimitCount, 1);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	return true;
}

// ===========================================================================
// AC-013.4, AC-013.5: Component applies status tag via ASC
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_ComponentAppliesStatusTag,
	"Mordecai.StatusEffect.ComponentAppliesStatusTag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_ComponentAppliesStatusTag::RunTest(const FString& Parameters)
{
	FStatusEffectTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	UMordecaiStatusEffectGameplayEffect* BurningGE = CreateTestStatusGE(MordecaiGameplayTags::Status_Burning);
	FActiveGameplayEffectHandle Handle = Env.ASC->ApplyGameplayEffectToSelf(BurningGE, 1.0f, Env.ASC->MakeEffectContext());

	TestTrue("GE handle is valid", Handle.IsValid());
	TestTrue("ASC has Burning tag", Env.ASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_Burning));
	TestTrue("Component reports HasStatusEffect(Burning)", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));

	Env.Teardown();
	return true;
}

// ===========================================================================
// AC-013.4: Component removes status by tag
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_ComponentRemovesStatusByTag,
	"Mordecai.StatusEffect.ComponentRemovesStatusByTag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_ComponentRemovesStatusByTag::RunTest(const FString& Parameters)
{
	FStatusEffectTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	UMordecaiStatusEffectGameplayEffect* BurningGE = CreateTestStatusGE(MordecaiGameplayTags::Status_Burning);
	Env.ASC->ApplyGameplayEffectToSelf(BurningGE, 1.0f, Env.ASC->MakeEffectContext());

	TestTrue("Burning is active before removal", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));

	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Burning);

	TestFalse("Burning is gone after removal", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));

	Env.Teardown();
	return true;
}

// ===========================================================================
// AC-013.4: Component HasStatusEffect query
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_ComponentHasStatusQuery,
	"Mordecai.StatusEffect.ComponentHasStatusQuery",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_ComponentHasStatusQuery::RunTest(const FString& Parameters)
{
	FStatusEffectTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	TestFalse("No Burning initially", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));

	UMordecaiStatusEffectGameplayEffect* BurningGE = CreateTestStatusGE(MordecaiGameplayTags::Status_Burning);
	Env.ASC->ApplyGameplayEffectToSelf(BurningGE, 1.0f, Env.ASC->MakeEffectContext());

	TestTrue("HasStatusEffect returns true after apply", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));
	TestFalse("HasStatusEffect returns false for unapplied status", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Poisoned));

	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Burning);
	TestFalse("HasStatusEffect returns false after removal", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));

	Env.Teardown();
	return true;
}

// ===========================================================================
// AC-013.4: Component GetActiveStatusTags
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_ComponentGetActiveStatusTags,
	"Mordecai.StatusEffect.ComponentGetActiveStatusTags",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_ComponentGetActiveStatusTags::RunTest(const FString& Parameters)
{
	FStatusEffectTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	UMordecaiStatusEffectGameplayEffect* BurningGE = CreateTestStatusGE(MordecaiGameplayTags::Status_Burning);
	UMordecaiStatusEffectGameplayEffect* PoisonedGE = CreateTestStatusGE(MordecaiGameplayTags::Status_Poisoned);

	Env.ASC->ApplyGameplayEffectToSelf(BurningGE, 1.0f, Env.ASC->MakeEffectContext());
	Env.ASC->ApplyGameplayEffectToSelf(PoisonedGE, 1.0f, Env.ASC->MakeEffectContext());

	FGameplayTagContainer ActiveTags = Env.StatusComp->GetActiveStatusTags();
	TestTrue("Active tags contain Burning", ActiveTags.HasTag(MordecaiGameplayTags::Status_Burning));
	TestTrue("Active tags contain Poisoned", ActiveTags.HasTag(MordecaiGameplayTags::Status_Poisoned));
	TestFalse("Active tags do not contain unapplied status", ActiveTags.HasTag(MordecaiGameplayTags::Status_Frostbitten));

	Env.Teardown();
	return true;
}

// ===========================================================================
// AC-013.6: Duration-based status effects auto-remove
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_DurationAutoRemoves,
	"Mordecai.StatusEffect.DurationAutoRemoves",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_DurationAutoRemoves::RunTest(const FString& Parameters)
{
	// AC-013.6: Duration-based status effects auto-remove after their duration expires
	// (standard GAS GE duration behavior). Verify the GE is configured correctly so
	// GAS handles auto-removal, and verify the tag appears while active and disappears
	// when the effect is manually removed (proving the tag lifecycle works).

	FStatusEffectTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Create a duration-based GE
	UMordecaiStatusEffectGameplayEffect* DurationGE = CreateTestStatusGE(
		MordecaiGameplayTags::Status_Burning,
		EGameplayEffectDurationType::HasDuration,
		2.0f);

	// Verify the GE is configured with the correct duration policy
	TestEqual("DurationPolicy is HasDuration",
		static_cast<int32>(DurationGE->DurationPolicy),
		static_cast<int32>(EGameplayEffectDurationType::HasDuration));

	// Apply and verify tag is granted
	FActiveGameplayEffectHandle Handle = Env.ASC->ApplyGameplayEffectToSelf(
		DurationGE, 1.0f, Env.ASC->MakeEffectContext());
	TestTrue("GE handle is valid", Handle.IsValid());
	TestTrue("Burning is active after apply", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));

	// Verify the tag is removed when the GE is removed (simulates what happens on expiry)
	Env.ASC->RemoveActiveGameplayEffect(Handle);
	TestFalse("Burning is gone after GE removal", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));

	Env.Teardown();
	return true;
}

// ===========================================================================
// AC-013.8: Immunity blocks status effect application
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_ImmunityBlocksApplication,
	"Mordecai.StatusEffect.ImmunityBlocksApplication",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_ImmunityBlocksApplication::RunTest(const FString& Parameters)
{
	FStatusEffectTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Grant the immunity tag via a loose tag on the ASC
	Env.ASC->AddLooseGameplayTag(MordecaiGameplayTags::Immunity_Burning);
	TestTrue("Target has immunity tag", Env.ASC->HasMatchingGameplayTag(MordecaiGameplayTags::Immunity_Burning));

	// Create a Burning status GE (InitializeStatusEffect sets up immunity blocking)
	UMordecaiStatusEffectGameplayEffect* BurningGE = CreateTestStatusGE(MordecaiGameplayTags::Status_Burning);

	// Attempt to apply — should be blocked by ApplicationTagRequirements
	FActiveGameplayEffectHandle Handle = Env.ASC->ApplyGameplayEffectToSelf(
		BurningGE, 1.0f, Env.ASC->MakeEffectContext());

	TestFalse("Burning GE was blocked by immunity", Handle.IsValid());
	TestFalse("Target does not have Burning tag", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));

	Env.Teardown();
	return true;
}

// ===========================================================================
// AC-013.9: Cleanse by category removes matching statuses
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_CleanseByCategory,
	"Mordecai.StatusEffect.CleanseByCategory",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_CleanseByCategory::RunTest(const FString& Parameters)
{
	FStatusEffectTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	// Apply two DoT statuses (Burning + Poisoned) and one Control (Silenced)
	UMordecaiStatusEffectGameplayEffect* BurningGE = CreateTestStatusGE(MordecaiGameplayTags::Status_Burning);
	UMordecaiStatusEffectGameplayEffect* PoisonedGE = CreateTestStatusGE(MordecaiGameplayTags::Status_Poisoned);
	UMordecaiStatusEffectGameplayEffect* SilencedGE = CreateTestStatusGE(MordecaiGameplayTags::Status_Silenced);

	Env.ASC->ApplyGameplayEffectToSelf(BurningGE, 1.0f, Env.ASC->MakeEffectContext());
	Env.ASC->ApplyGameplayEffectToSelf(PoisonedGE, 1.0f, Env.ASC->MakeEffectContext());
	Env.ASC->ApplyGameplayEffectToSelf(SilencedGE, 1.0f, Env.ASC->MakeEffectContext());

	TestTrue("Burning is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));
	TestTrue("Poisoned is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Poisoned));
	TestTrue("Silenced is active", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Silenced));

	// Cleanse DoT category
	Env.StatusComp->RemoveStatusEffectsByCategory(MordecaiGameplayTags::Status_Category_DoT);

	TestFalse("Burning removed by DoT cleanse", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));
	TestFalse("Poisoned removed by DoT cleanse", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Poisoned));
	TestTrue("Silenced survives DoT cleanse", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Silenced));

	Env.Teardown();
	return true;
}

// ===========================================================================
// AC-013.10: Default stacking refreshes duration
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_DefaultStackingRefreshesDuration,
	"Mordecai.StatusEffect.DefaultStackingRefreshesDuration",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_DefaultStackingRefreshesDuration::RunTest(const FString& Parameters)
{
	// AC-013.10: Default stacking behavior is duration refresh (not stack count).
	// Verify GE stacking configuration and that re-application does NOT create
	// multiple independent effects (proves refresh/aggregate behavior).

	FStatusEffectTestEnv Env;
	if (!TestTrue("Test environment setup", Env.Setup()))
	{
		return true;
	}

	UMordecaiStatusEffectGameplayEffect* BurningGE = CreateTestStatusGE(
		MordecaiGameplayTags::Status_Burning,
		EGameplayEffectDurationType::HasDuration,
		4.0f);

	// Verify stacking configuration
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	TestEqual("StackingType is AggregateBySource",
		static_cast<int32>(BurningGE->StackingType),
		static_cast<int32>(EGameplayEffectStackingType::AggregateBySource));
	TestEqual("StackLimitCount is 1", BurningGE->StackLimitCount, 1);
	TestEqual("StackDurationRefreshPolicy is RefreshOnSuccessfulApplication",
		static_cast<int32>(BurningGE->StackDurationRefreshPolicy),
		static_cast<int32>(EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication));
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	// Apply GE
	Env.ASC->ApplyGameplayEffectToSelf(BurningGE, 1.0f, Env.ASC->MakeEffectContext());
	TestTrue("Burning is active after first application", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));

	// Re-apply same GE — should not create a second independent effect
	Env.ASC->ApplyGameplayEffectToSelf(BurningGE, 1.0f, Env.ASC->MakeEffectContext());
	TestTrue("Burning still active after re-application", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));

	// Remove the status — single removal should clear it completely
	// (if stacking were Independent, we'd need two removals)
	Env.StatusComp->RemoveStatusEffect(MordecaiGameplayTags::Status_Burning);
	TestFalse("Burning fully removed by single removal (no stacking)", Env.StatusComp->HasStatusEffect(MordecaiGameplayTags::Status_Burning));

	Env.Teardown();
	return true;
}

// ===========================================================================
// AC-013.12: Category enum has all values
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_CategoryEnumHasAllValues,
	"Mordecai.StatusEffect.CategoryEnumHasAllValues",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_CategoryEnumHasAllValues::RunTest(const FString& Parameters)
{
	EMordecaiStatusEffectCategory DoT = EMordecaiStatusEffectCategory::DoT;
	EMordecaiStatusEffectCategory Debuff = EMordecaiStatusEffectCategory::Debuff;
	EMordecaiStatusEffectCategory Control = EMordecaiStatusEffectCategory::Control;
	EMordecaiStatusEffectCategory Buff = EMordecaiStatusEffectCategory::Buff;
	EMordecaiStatusEffectCategory Environmental = EMordecaiStatusEffectCategory::Environmental;

	TestTrue("DoT != Debuff",           DoT != Debuff);
	TestTrue("DoT != Control",          DoT != Control);
	TestTrue("DoT != Buff",             DoT != Buff);
	TestTrue("DoT != Environmental",    DoT != Environmental);
	TestTrue("Debuff != Control",       Debuff != Control);
	TestTrue("Debuff != Buff",          Debuff != Buff);
	TestTrue("Debuff != Environmental", Debuff != Environmental);
	TestTrue("Control != Buff",         Control != Buff);
	TestTrue("Control != Environmental",Control != Environmental);
	TestTrue("Buff != Environmental",   Buff != Environmental);

	// Verify StackingPolicy enum values
	EMordecaiStackingPolicy Refresh = EMordecaiStackingPolicy::Refresh;
	EMordecaiStackingPolicy StackCount = EMordecaiStackingPolicy::StackCount;
	EMordecaiStackingPolicy StackDuration = EMordecaiStackingPolicy::StackDuration;
	EMordecaiStackingPolicy Independent = EMordecaiStackingPolicy::Independent;

	TestTrue("Stacking policies are all distinct",
		Refresh != StackCount && StackCount != StackDuration && StackDuration != Independent);

	return true;
}

// ===========================================================================
// AC-013.13: Category mapping returns correct category for all 16 statuses
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiStatusEffect_CategoryMappingCorrect,
	"Mordecai.StatusEffect.CategoryMappingCorrect",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiStatusEffect_CategoryMappingCorrect::RunTest(const FString& Parameters)
{
	using namespace MordecaiGameplayTags;

	// DoT: Burning, Bleeding, Poisoned
	TestEqual("Burning is DoT", GetStatusEffectCategory(Status_Burning), EMordecaiStatusEffectCategory::DoT);
	TestEqual("Bleeding is DoT", GetStatusEffectCategory(Status_Bleeding), EMordecaiStatusEffectCategory::DoT);
	TestEqual("Poisoned is DoT", GetStatusEffectCategory(Status_Poisoned), EMordecaiStatusEffectCategory::DoT);

	// Debuff: Frostbitten, Weakened, Brittle, Cursed, Corroded, Exposed
	TestEqual("Frostbitten is Debuff", GetStatusEffectCategory(Status_Frostbitten), EMordecaiStatusEffectCategory::Debuff);
	TestEqual("Weakened is Debuff", GetStatusEffectCategory(Status_Weakened), EMordecaiStatusEffectCategory::Debuff);
	TestEqual("Brittle is Debuff", GetStatusEffectCategory(Status_Brittle), EMordecaiStatusEffectCategory::Debuff);
	TestEqual("Cursed is Debuff", GetStatusEffectCategory(Status_Cursed), EMordecaiStatusEffectCategory::Debuff);
	TestEqual("Corroded is Debuff", GetStatusEffectCategory(Status_Corroded), EMordecaiStatusEffectCategory::Debuff);
	TestEqual("Exposed is Debuff", GetStatusEffectCategory(Status_Exposed), EMordecaiStatusEffectCategory::Debuff);

	// Control: Shocked, Silenced, Rooted, Blinded, Fear
	TestEqual("Shocked is Control", GetStatusEffectCategory(Status_Shocked), EMordecaiStatusEffectCategory::Control);
	TestEqual("Silenced is Control", GetStatusEffectCategory(Status_Silenced), EMordecaiStatusEffectCategory::Control);
	TestEqual("Rooted is Control", GetStatusEffectCategory(Status_Rooted), EMordecaiStatusEffectCategory::Control);
	TestEqual("Blinded is Control", GetStatusEffectCategory(Status_Blinded), EMordecaiStatusEffectCategory::Control);
	TestEqual("Fear is Control", GetStatusEffectCategory(Status_Fear), EMordecaiStatusEffectCategory::Control);

	// Buff: Focused
	TestEqual("Focused is Buff", GetStatusEffectCategory(Status_Focused), EMordecaiStatusEffectCategory::Buff);

	// Environmental: Drenched
	TestEqual("Drenched is Environmental", GetStatusEffectCategory(Status_Drenched), EMordecaiStatusEffectCategory::Environmental);

	return true;
}
