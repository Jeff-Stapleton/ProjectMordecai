// Project Mordecai — US-090 spell-point regeneration and respawn recovery tests

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeScaling.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/MordecaiPlayerState.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Cursed.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "UObject/UnrealType.h"

namespace MordecaiRecoverySpellPointTest
{
	using namespace MordecaiRecoveryTest;

	constexpr float BaseRegenPerSecond = 0.25f;
	constexpr float PeriodSeconds = 1.0f;
	constexpr float Tolerance = 0.01f;

	void Tick(FRecoveryTestEnv& Env, float Seconds)
	{
		constexpr float StepSeconds = 0.05f;
		float Remaining = Seconds;
		while (Remaining > 0.f)
		{
			const float Step = FMath::Min(StepSeconds, Remaining);
			++GFrameCounter;
			Env.World->Tick(LEVELTICK_All, Step);
			Remaining -= Step;
		}
	}

	float SpellPoints(const FRecoveryTestEnv& Env)
	{
		const UAbilitySystemComponent* ASC = Env.GetPlayerASC();
		return ASC
			? ASC->GetNumericAttribute(UMordecaiAttributeSet::GetSpellPointsAttribute())
			: -1.f;
	}

	bool IsSpellPointRegenEffect(const FActiveGameplayEffect* Active)
	{
		if (!Active || !Active->Spec.Def
			|| Active->Spec.Def->DurationPolicy != EGameplayEffectDurationType::Infinite
			|| !FMath::IsNearlyEqual(Active->Spec.Def->Period.GetValue(), PeriodSeconds)
			|| Active->Spec.Def->bExecutePeriodicEffectOnApplication)
		{
			return false;
		}

		return Active->Spec.Def->Modifiers.ContainsByPredicate(
			[](const FGameplayModifierInfo& Modifier)
			{
				return Modifier.Attribute == UMordecaiAttributeSet::GetSpellPointsAttribute();
			});
	}

	TArray<FActiveGameplayEffectHandle> SpellPointRegenEffects(
		const UAbilitySystemComponent* ASC)
	{
		TArray<FActiveGameplayEffectHandle> Matches;
		if (!ASC)
		{
			return Matches;
		}

		for (const FActiveGameplayEffectHandle Handle :
			ASC->GetActiveEffects(FGameplayEffectQuery()))
		{
			if (IsSpellPointRegenEffect(ASC->GetActiveGameplayEffect(Handle)))
			{
				Matches.Add(Handle);
			}
		}
		return Matches;
	}

	bool IsReplicatedAttribute(const TCHAR* PropertyName)
	{
		const FProperty* Property =
			FindFProperty<FProperty>(UMordecaiAttributeSet::StaticClass(), PropertyName);
		return Property && Property->HasAnyPropertyFlags(CPF_Net);
	}

	void ApplyInstantAttributeDelta(
		UAbilitySystemComponent* ASC,
		const FGameplayAttribute& Attribute,
		float Delta)
	{
		UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage());
		Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
		FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude =
			FGameplayEffectModifierMagnitude(FScalableFloat(Delta));
		ASC->ApplyGameplayEffectToSelf(Effect, 1.f, ASC->MakeEffectContext());
	}

	void ApplyInstantSpellPointDelta(UAbilitySystemComponent* ASC, float Delta)
	{
		ApplyInstantAttributeDelta(
			ASC,
			UMordecaiAttributeSet::GetSpellPointsAttribute(),
			Delta);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_SpellPoints_RegenRatePeriodAndClamp,
	"Mordecai.Recovery.SpellPoints.RegenRatePeriodAndClamp",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_SpellPoints_RegenRatePeriodAndClamp::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiRecoverySpellPointTest;
	FRecoveryTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!TestTrue("authority player world", Env.Setup(FVector::ZeroVector)))
	{
		return true;
	}

	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	if (!TestNotNull("PlayerState ASC", ASC))
	{
		return true;
	}
	TestEqual("fresh MaxSP remains 10",
		ASC->GetNumericAttribute(UMordecaiAttributeSet::GetMaxSpellPointsAttribute()), 10.f);

	ASC->SetNumericAttributeBase(
		UMordecaiAttributeSet::GetSpellPointsRegenMultiplierAttribute(), 1.f);
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetSpellPointsAttribute(), 0.f);
	Tick(Env, 0.95f);
	TestTrue("no regeneration before the one-second period",
		FMath::IsNearlyEqual(SpellPoints(Env), 0.f, Tolerance));
	// The first world tick initializes the active-effect clock, so leave one
	// automation-frame of scheduling tolerance beyond the exact one-second period.
	Tick(Env, 0.20f);
	TestTrue("first period restores exactly 0.25 SP",
		FMath::IsNearlyEqual(SpellPoints(Env), BaseRegenPerSecond, Tolerance));

	ASC->SetNumericAttributeBase(
		UMordecaiAttributeSet::GetSpellPointsRegenMultiplierAttribute(), 2.f);
	Tick(Env, PeriodSeconds + 0.10f);
	TestTrue("rate is multiplied by SpellPointsRegenMultiplier",
		FMath::IsNearlyEqual(SpellPoints(Env), 0.75f, Tolerance));

	const float MaxSP =
		ASC->GetNumericAttribute(UMordecaiAttributeSet::GetMaxSpellPointsAttribute());
	ASC->SetNumericAttributeBase(
		UMordecaiAttributeSet::GetSpellPointsAttribute(), MaxSP - 0.1f);
	Tick(Env, PeriodSeconds + 0.10f);
	TestTrue("periodic regeneration clamps at MaxSP",
		FMath::IsNearlyEqual(SpellPoints(Env), MaxSP, Tolerance));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_SpellPoints_DeadSilencedAndCursed,
	"Mordecai.Recovery.SpellPoints.DeadSilencedAndCursed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_SpellPoints_DeadSilencedAndCursed::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiRecoverySpellPointTest;

	auto CheckBlockedTag = [this](const TCHAR* Label, const FGameplayTag& Tag)
	{
		FRecoveryTestEnv Env;
		ON_SCOPE_EXIT { Env.Teardown(); };
		if (!TestTrue(*FString::Printf(TEXT("%s authority world"), Label),
				Env.Setup(FVector::ZeroVector)))
		{
			return;
		}
		UAbilitySystemComponent* ASC = Env.GetPlayerASC();
		ASC->SetNumericAttributeBase(
			UMordecaiAttributeSet::GetSpellPointsRegenMultiplierAttribute(), 1.f);
		ASC->SetNumericAttributeBase(
			UMordecaiAttributeSet::GetSpellPointsAttribute(), 0.f);
		ASC->AddLooseGameplayTag(Tag);
		Tick(Env, 1.1f);
		TestTrue(*FString::Printf(TEXT("%s inhibits regeneration"), Label),
			FMath::IsNearlyEqual(SpellPoints(Env), 0.f, Tolerance));
		ASC->RemoveLooseGameplayTag(Tag);
		Tick(Env, 1.1f);
		TestTrue(*FString::Printf(TEXT("regeneration resumes after %s"), Label),
			FMath::IsNearlyEqual(SpellPoints(Env), BaseRegenPerSecond, Tolerance));
	};

	CheckBlockedTag(TEXT("Dead"), MordecaiGameplayTags::State_Dead);
	CheckBlockedTag(TEXT("Silenced"), MordecaiGameplayTags::Status_Silenced);

	FRecoveryTestEnv CursedEnv;
	ON_SCOPE_EXIT { CursedEnv.Teardown(); };
	if (!TestTrue("Cursed authority world", CursedEnv.Setup(FVector::ZeroVector)))
	{
		return true;
	}
	UAbilitySystemComponent* ASC = CursedEnv.GetPlayerASC();
	ASC->SetNumericAttributeBase(
		UMordecaiAttributeSet::GetSpellPointsRegenMultiplierAttribute(), 1.f);
	ASC->SetNumericAttributeBase(
		UMordecaiAttributeSet::GetSpellPointsAttribute(), 0.f);
	const FActiveGameplayEffectHandle CursedHandle = ASC->ApplyGameplayEffectToSelf(
		GetDefault<UMordecaiGE_Cursed>(), 1.f, ASC->MakeEffectContext());
	TestTrue("Cursed GE applies", CursedHandle.IsValid());
	TestTrue("Cursed keeps its 0.5 regen multiplier",
		FMath::IsNearlyEqual(
			ASC->GetNumericAttribute(
				UMordecaiAttributeSet::GetSpellPointsRegenMultiplierAttribute()),
			0.5f, Tolerance));
	Tick(CursedEnv, 1.1f);
	TestTrue("Cursed regenerates at half speed",
		FMath::IsNearlyEqual(SpellPoints(CursedEnv), BaseRegenPerSecond * 0.5f, Tolerance));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_SpellPoints_AppliedOnceAndSurvivesRespawn,
	"Mordecai.Recovery.SpellPoints.AppliedOnceAndSurvivesRespawn",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_SpellPoints_AppliedOnceAndSurvivesRespawn::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiRecoverySpellPointTest;
	FRecoveryTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!TestTrue("authority player world", Env.Setup(FVector::ZeroVector)))
	{
		return true;
	}

	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	const TArray<FActiveGameplayEffectHandle> Before =
		SpellPointRegenEffects(ASC);
	TestEqual("exactly one periodic SP regen effect is applied per PlayerState",
		Before.Num(), 1);
	TestEqual("the stable regen GE asset tag resolves exactly one live effect",
		Env.PlayerState->GetSpellPointRegenEffectCount(), 1);

	Env.Player->HandlePlayerRespawn();
	const TArray<FActiveGameplayEffectHandle> After =
		SpellPointRegenEffects(ASC);
	TestEqual("respawn does not add another regen effect", After.Num(), 1);
	if (Before.Num() == 1 && After.Num() == 1)
	{
		TestEqual("the same active effect survives the pawn respawn",
			After[0], Before[0]);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_SpellPoints_RespawnRefillsToMax,
	"Mordecai.Recovery.SpellPoints.RespawnRefillsToMax",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_SpellPoints_RespawnRefillsToMax::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiRecoverySpellPointTest;
	FRecoveryTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!TestTrue("authority player world", Env.Setup(FVector::ZeroVector)))
	{
		return true;
	}

	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	ASC->SetNumericAttributeBase(
		UMordecaiAttributeSet::GetSpellPointsAttribute(), 1.f);
	Env.Player->HandlePlayerRespawn();
	TestEqual("respawn immediately restores SP to MaxSP",
		SpellPoints(Env),
		ASC->GetNumericAttribute(
			UMordecaiAttributeSet::GetMaxSpellPointsAttribute()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_SpellPoints_ReplicatedAttributes,
	"Mordecai.Recovery.SpellPoints.ReplicatedAttributes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_SpellPoints_ReplicatedAttributes::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiRecoverySpellPointTest;
	TestTrue("SpellPoints replicates to clients",
		IsReplicatedAttribute(TEXT("SpellPoints")));
	TestTrue("MaxSpellPoints replicates to clients",
		IsReplicatedAttribute(TEXT("MaxSpellPoints")));
	TestTrue("SpellPointsRegenMultiplier replicates to clients",
		IsReplicatedAttribute(TEXT("SpellPointsRegenMultiplier")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_SpellPoints_NonAuthorityHasNoRegenEffect,
	"Mordecai.Recovery.SpellPoints.NonAuthorityHasNoRegenEffect",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_SpellPoints_NonAuthorityHasNoRegenEffect::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiRecoverySpellPointTest;
	FRecoveryTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!TestTrue("test world", Env.Setup(FVector::ZeroVector)))
	{
		return true;
	}

	AMordecaiPlayerState* ClientCopy =
		Env.World->SpawnActorDeferred<AMordecaiPlayerState>(
			AMordecaiPlayerState::StaticClass(),
			FTransform::Identity);
	if (!TestNotNull("deferred non-authority PlayerState", ClientCopy))
	{
		return true;
	}
	ClientCopy->SetRole(ROLE_SimulatedProxy);
	ClientCopy->FinishSpawning(FTransform::Identity);

	TestEqual(
		"non-authority PlayerState applies zero spell-point regen effects",
		SpellPointRegenEffects(ClientCopy->GetAbilitySystemComponent()).Num(),
		0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_SpellPoints_LiveIntelligenceUpdatesPeriodicMagnitude,
	"Mordecai.Recovery.SpellPoints.LiveIntelligenceUpdatesPeriodicMagnitude",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_SpellPoints_LiveIntelligenceUpdatesPeriodicMagnitude::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiRecoverySpellPointTest;
	FRecoveryTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!TestTrue("authority player world", Env.Setup(FVector::ZeroVector)))
	{
		return true;
	}

	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetSpellPointsAttribute(), 0.f);
	ApplyInstantAttributeDelta(
		ASC,
		UMordecaiAttributeSet::GetIntelligenceAttribute(),
		2.f);

	const float EffectiveInt =
		FMordecaiAttributeScaling::CalculateEffectiveMod(
			ASC->GetNumericAttribute(UMordecaiAttributeSet::GetIntelligenceAttribute()));
	const float ExpectedMultiplier = 1.f + 0.02f * EffectiveInt;
	TestTrue(
		"INT GameplayEffect refreshes the live regen multiplier through the ASC",
		FMath::IsNearlyEqual(
			ASC->GetNumericAttribute(
				UMordecaiAttributeSet::GetSpellPointsRegenMultiplierAttribute()),
			ExpectedMultiplier,
			Tolerance));
	Tick(Env, PeriodSeconds + 0.10f);
	TestTrue(
		"next periodic tick uses 0.25 times the effective-INT multiplier",
		FMath::IsNearlyEqual(
			SpellPoints(Env),
			BaseRegenPerSecond * ExpectedMultiplier,
			Tolerance));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_SpellPoints_RegenMultiplierClampsNonNegative,
	"Mordecai.Recovery.SpellPoints.RegenMultiplierClampsNonNegative",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_SpellPoints_RegenMultiplierClampsNonNegative::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiRecoverySpellPointTest;
	FRecoveryTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!TestTrue("authority player world", Env.Setup(FVector::ZeroVector)))
	{
		return true;
	}

	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	ASC->SetNumericAttributeBase(
		UMordecaiAttributeSet::GetSpellPointsRegenMultiplierAttribute(),
		-1.f);
	TestEqual(
		"SpellPointsRegenMultiplier clamps at zero",
		ASC->GetNumericAttribute(
			UMordecaiAttributeSet::GetSpellPointsRegenMultiplierAttribute()),
		0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_SpellPoints_ExactSpendBeforeFirstPeriod,
	"Mordecai.Recovery.SpellPoints.ExactSpendBeforeFirstPeriod",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_SpellPoints_ExactSpendBeforeFirstPeriod::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiRecoverySpellPointTest;
	FRecoveryTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!TestTrue("authority player world", Env.Setup(FVector::ZeroVector)))
	{
		return true;
	}

	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	ASC->SetNumericAttributeBase(
		UMordecaiAttributeSet::GetSpellPointsRegenMultiplierAttribute(), 1.f);
	ASC->SetNumericAttributeBase(
		UMordecaiAttributeSet::GetSpellPointsAttribute(), 10.f);
	ApplyInstantSpellPointDelta(ASC, -3.f);
	TestTrue("the cast spends exactly 3 SP immediately",
		FMath::IsNearlyEqual(SpellPoints(Env), 7.f, Tolerance));
	Tick(Env, 0.95f);
	TestTrue("short exact-spend test ticks stay deterministic",
		FMath::IsNearlyEqual(SpellPoints(Env), 7.f, Tolerance));
	return true;
}
