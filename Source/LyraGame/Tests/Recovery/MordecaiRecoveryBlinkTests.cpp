// Project Mordecai -- US-087 Blink recovery regression tests (issue #25 / N20)

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "MordecaiRecoveryTestEnv.h"

#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"
#include "Mordecai/Magic/MordecaiGA_Blink.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"

namespace MordecaiRecoveryBlinkTest
{
	using MordecaiRecoveryTest::FRecoveryTestEnv;

	constexpr float PlayerZ = 90.f;
	constexpr float EnemyZ = 96.f;
	constexpr float BlinkRange = 500.f;
	constexpr float InitialSP = 10.f;
	constexpr float BlinkCost = 2.f;

	AActor* SpawnCollisionBox(
		UWorld* World,
		const FVector& Location,
		const FVector& Extent,
		const FName Profile)
	{
		AActor* Actor = World ? World->SpawnActor<AActor>(
			AActor::StaticClass(), Location, FRotator::ZeroRotator, FRecoveryTestEnv::AlwaysSpawn()) : nullptr;
		if (!Actor)
		{
			return nullptr;
		}
		UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
		Actor->AddInstanceComponent(Box);
		Actor->SetRootComponent(Box);
		Box->SetBoxExtent(Extent);
		Box->SetCollisionProfileName(Profile);
		Box->RegisterComponent();
		Actor->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
		return Actor;
	}

	AMordecaiEnemyCharacter* SpawnTraceBlockingPawn(FRecoveryTestEnv& Env, const FVector& Location)
	{
		AMordecaiEnemyCharacter* Enemy = Env.SpawnEnemy(Location, {});
		if (Enemy && Enemy->GetCapsuleComponent())
		{
			// CombatTestMap's Pawn-object enemy capsules block the existing ECC_WorldStatic trace.
			Enemy->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		}
		return Enemy;
	}

	AMordecaiEnemyCharacter* SpawnEndpointPawn(FRecoveryTestEnv& Env, const FVector& Location)
	{
		AMordecaiEnemyCharacter* Enemy = Env.SpawnEnemy(Location, {});
		if (Enemy && Enemy->GetCapsuleComponent())
		{
			// Isolate endpoint resolution: this Pawn cannot stop the path trace, but its real capsule still blocks Pawn.
			Enemy->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
		}
		return Enemy;
	}

	struct FSpellCastEventCounter
	{
		int32 Count = 0;
		FDelegateHandle Handle;
		UAbilitySystemComponent* ASC = nullptr;

		void Start(UAbilitySystemComponent* InASC)
		{
			ASC = InASC;
			Handle = ASC->GenericGameplayEventCallbacks.FindOrAdd(MordecaiGameplayTags::Event_SpellCast)
				.AddLambda([this](const FGameplayEventData*) { ++Count; });
		}

		void Stop()
		{
			if (ASC)
			{
				ASC->GenericGameplayEventCallbacks.FindOrAdd(MordecaiGameplayTags::Event_SpellCast).Remove(Handle);
			}
			ASC = nullptr;
		}
	};

	bool SetupBlink(FRecoveryTestEnv& Env)
	{
		if (!Env.Setup(FVector(0.f, 0.f, PlayerZ)))
		{
			return false;
		}
		// Real Lyra capsule half-height is 90: this floor's top at Z=0 is an exact floor touch.
		return SpawnCollisionBox(
			Env.World, FVector(0.f, 0.f, -50.f), FVector(2000.f, 2000.f, 50.f), TEXT("BlockAll")) != nullptr;
	}

	FGameplayAbilitySpecHandle GrantBlink(FRecoveryTestEnv& Env)
	{
		UAbilitySystemComponent* ASC = Env.GetPlayerASC();
		ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetSpellPointsAttribute(), InitialSP);
		UMordecaiSpellDataAsset* Data = NewObject<UMordecaiSpellDataAsset>(ASC);
		Data->SpellId = FName(TEXT("Blink"));
		Data->SpellPointCost = static_cast<int32>(BlinkCost);
		Data->Range = BlinkRange;
		Data->BuffDuration = 0.3f;
		Data->DeliveryType = EMordecaiSpellDeliveryType::DashMovement;
		Data->TargetingType = EMordecaiSpellTargetingType::Dash;
		Data->WindupTime = 0.f;
		Data->CastTime = 0.f;
		Data->RecoveryTime = 0.2f;
		Data->CooldownDuration = 1.f;
		Data->CooldownTag = MordecaiGameplayTags::Ability_Spell_Blink;
		return ASC->GiveAbility(
			FGameplayAbilitySpec(UMordecaiGA_Blink::StaticClass(), 1, INDEX_NONE, Data));
	}

	float SpellPoints(const FRecoveryTestEnv& Env)
	{
		return Env.GetPlayerASC()->GetNumericAttribute(UMordecaiAttributeSet::GetSpellPointsAttribute());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Blink_EnemyAheadDoesNotBlock,
	"Mordecai.Recovery.Blink.EnemyAheadDoesNotBlock",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Blink_EnemyAheadDoesNotBlock::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryBlinkTest;
	FRecoveryTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!TestTrue("real player, ASC and touching floor set up", SetupBlink(Env)))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy = SpawnTraceBlockingPawn(Env, FVector(90.f, 0.f, EnemyZ));
	const FVector EnemyBefore = Enemy ? Enemy->GetActorLocation() : FVector::ZeroVector;
	if (!TestNotNull("real enemy Pawn capsule about 90 cm ahead", Enemy))
	{
		return true;
	}

	const FVector Origin = Env.Player->GetActorLocation();
	const FGameplayAbilitySpecHandle Blink = GrantBlink(Env);
	TestTrue("real Blink activates through the PlayerState ASC", Env.GetPlayerASC()->TryActivateAbility(Blink));
	const FVector Delta = Env.Player->GetActorLocation() - Origin;

	TestTrue("enemy Pawn does not consume the Blink", Delta.X > 300.f);
	TestTrue("Blink stays on the +X line", FMath::Abs(Delta.Y) <= 1.f && FMath::Abs(Delta.Z) <= 1.f);
	TestEqual("successful Blink spends exactly 2 SP", SpellPoints(Env), InitialSP - BlinkCost);
	TestEqual("the enemy was not moved", Enemy->GetActorLocation(), EnemyBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Blink_NoMoveNoCost,
	"Mordecai.Recovery.Blink.NoMoveNoCost",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Blink_NoMoveNoCost::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryBlinkTest;
	FRecoveryTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!TestTrue("real player, ASC and touching floor set up", SetupBlink(Env))
		|| !TestNotNull("blocking wall starts 45 cm ahead",
			SpawnCollisionBox(Env.World, FVector(50.f, 0.f, PlayerZ), FVector(5.f, 100.f, 100.f), TEXT("BlockAll"))))
	{
		return true;
	}

	const FVector Origin = Env.Player->GetActorLocation();
	const FGameplayAbilitySpecHandle Blink = GrantBlink(Env);
	FSpellCastEventCounter CastEvents;
	CastEvents.Start(Env.GetPlayerASC());
	ON_SCOPE_EXIT { CastEvents.Stop(); };
	TestTrue("real Blink activation is attempted", Env.GetPlayerASC()->TryActivateAbility(Blink));
	const FVector Delta = Env.Player->GetActorLocation() - Origin;

	TestTrue("wall buffer leaves numerically zero movement", Delta.IsNearlyZero(UE_KINDA_SMALL_NUMBER));
	TestEqual("zero-movement Blink leaves the character in place", Env.Player->GetActorLocation(), Origin);
	TestEqual("zero-movement Blink does not spend SP", SpellPoints(Env), InitialSP);
	TestEqual("zero-movement Blink does not broadcast its cast event", CastEvents.Count, 0);
	TestFalse("zero-movement Blink does not start cooldown",
		Env.GetPlayerASC()->HasMatchingGameplayTag(MordecaiGameplayTags::Ability_Spell_Blink));
	TestFalse("zero-movement Blink does not grant invulnerability",
		Env.GetPlayerASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_Invulnerable));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Blink_DestinationPawnAdjustedToFreeSpot,
	"Mordecai.Recovery.Blink.DestinationPawnAdjustedToFreeSpot",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Blink_DestinationPawnAdjustedToFreeSpot::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryBlinkTest;
	FRecoveryTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!TestTrue("real player, ASC and touching floor set up", SetupBlink(Env))
		|| !TestNotNull("path-nonblocking destination Pawn spawned",
			SpawnEndpointPawn(Env, FVector(BlinkRange, 0.f, EnemyZ))))
	{
		return true;
	}

	const FVector Origin = Env.Player->GetActorLocation();
	const FGameplayAbilitySpecHandle Blink = GrantBlink(Env);
	TestTrue("real Blink activates", Env.GetPlayerASC()->TryActivateAbility(Blink));
	const FVector Destination = Env.Player->GetActorLocation();
	const float Moved = Destination.X - Origin.X;

	TestTrue("endpoint-only fixture still moves >300 cm", Moved > 300.f);
	TestTrue("occupied exact endpoint is adjusted back", Moved < BlinkRange - UE_KINDA_SMALL_NUMBER);
	TestFalse("actual character capsule is free at the adjusted destination",
		Env.World->EncroachingBlockingGeometry(Env.Player, Destination, Env.Player->GetActorRotation()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Blink_PreservesGeometryAndFloorSemantics,
	"Mordecai.Recovery.Blink.PreservesGeometryAndFloorSemantics",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Blink_PreservesGeometryAndFloorSemantics::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryBlinkTest;
	auto RunCase = [this](
		const TCHAR* Label, const FName Profile, const FVector& BoxLocation,
		const FVector& BoxExtent, const float ExpectedX)
	{
		FRecoveryTestEnv Env;
		if (!TestTrue(*FString::Printf(TEXT("%s: setup"), Label), SetupBlink(Env)))
		{
			Env.Teardown();
			return;
		}
		if (!Profile.IsNone())
		{
			TestNotNull(*FString::Printf(TEXT("%s: box"), Label),
				SpawnCollisionBox(Env.World, BoxLocation, BoxExtent, Profile));
		}
		const FGameplayAbilitySpecHandle Blink = GrantBlink(Env);
		TestTrue(*FString::Printf(TEXT("%s: activates"), Label), Env.GetPlayerASC()->TryActivateAbility(Blink));
		TestTrue(*FString::Printf(TEXT("%s: destination X %.2f"), Label, ExpectedX),
			FMath::IsNearlyEqual(Env.Player->GetActorLocation().X, ExpectedX, 2.f));
		TestTrue(*FString::Printf(TEXT("%s: remains on floor"), Label),
			FMath::IsNearlyEqual(Env.Player->GetActorLocation().Z, PlayerZ, 1.f));
		Env.Teardown();
	};

	RunCase(TEXT("floor touch does not block"), NAME_None, FVector::ZeroVector, FVector::ZeroVector, 500.f);
	RunCase(TEXT("WorldStatic wall keeps 50 cm clamp"), TEXT("BlockAll"),
		FVector(305.f, 0.f, PlayerZ), FVector(5.f, 100.f, 100.f), 250.f);
	RunCase(TEXT("blocking WorldDynamic still clamps"), TEXT("BlockAllDynamic"),
		FVector(305.f, 0.f, PlayerZ), FVector(5.f, 100.f, 100.f), 250.f);
	RunCase(TEXT("overlap trigger at endpoint is ignored"), TEXT("Trigger"),
		FVector(500.f, 0.f, PlayerZ), FVector(20.f, 100.f, 100.f), 500.f);
	return true;
}

#endif // WITH_AUTOMATION_TESTS
