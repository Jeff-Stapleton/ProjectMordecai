// Project Mordecai — Level: the playtest test maps (US-100, issue #46 / T1)
//
// MovementTestMap (no enemies, no pickups) and CombatTestMap (a TestDummy that never moves or aggroes, and a
// TestAttacker that holds its spot and swings at a known cadence) take the playtests off DevTestMap, whose
// Frontliner swarm killed the player mid-script (N14). Scripts/us100_01_create_test_enemies.py and
// Scripts/us100_02_create_test_maps.py create and save them. These tests load the saved maps and Blueprints
// (no PIE, no play) and check what a packaged build and the playtest scripts rely on: the Mordecai experience
// route, one .umap per map, one spawn at the origin, the geometry and its grid material, Movable lights only, the
// enemies' bodies, and where the enemies stand against their aggro range.

#include "Misc/AutomationTest.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/LightComponentBase.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "GameModes/LyraWorldSettings.h"
#include "Materials/MaterialInstance.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Enemy/MordecaiEnemyAIController.h"
#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"
#include "Mordecai/Items/MordecaiItemPickup.h"
#include "Mordecai/MordecaiGameMode.h"
#include "Mordecai/UI/MordecaiEnemyHealthBarWidget.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UnrealType.h"

namespace MordecaiTestMapTests
{
	const TCHAR* const MovementTestMapPath = TEXT("/MordecaiCore/Game/Maps/MovementTestMap.MovementTestMap");
	const TCHAR* const CombatTestMapPath = TEXT("/MordecaiCore/Game/Maps/CombatTestMap.CombatTestMap");
	const TCHAR* const TestDummyClassPath = TEXT("/MordecaiCore/Enemies/BP_MordecaiEnemy_TestDummy.BP_MordecaiEnemy_TestDummy_C");
	const TCHAR* const TestAttackerClassPath = TEXT("/MordecaiCore/Enemies/BP_MordecaiEnemy_TestAttacker.BP_MordecaiEnemy_TestAttacker_C");
	const TCHAR* const BasicSlashPath = TEXT("/MordecaiCore/Combat/Enemies/DA_EnemyAttack_BasicSlash.DA_EnemyAttack_BasicSlash");
	// Every T1_ box: a measuring grid that is always cooked
	const TCHAR* const GridMaterialPath = TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial");

	// The enemy bodies (Scripts/us100_01_create_test_enemies.py): BasicShapeMaterial instances whose "Color" is set
	const TCHAR* const TestDummyMaterialPath = TEXT("/MordecaiCore/Enemies/Materials/MI_Enemy_TestDummy.MI_Enemy_TestDummy");
	const TCHAR* const TestAttackerMaterialPath = TEXT("/MordecaiCore/Enemies/Materials/MI_Enemy_TestAttacker.MI_Enemy_TestAttacker");
	const TCHAR* const BodyColorParameter = TEXT("Color");
	const FLinearColor TestDummyColor(0.25f, 0.35f, 0.75f);    // blue-grey
	const FLinearColor TestAttackerColor(1.0f, 0.45f, 0.05f);  // orange
	constexpr float ColorTolerance = 0.01f;

	const TCHAR* const TestDummyClassName = TEXT("BP_MordecaiEnemy_TestDummy_C");
	const TCHAR* const TestAttackerClassName = TEXT("BP_MordecaiEnemy_TestAttacker_C");
	const TCHAR* const FrontlinerClassName = TEXT("BP_MordecaiEnemy_Frontliner_C");

	// Both maps: one PlayerStart at the origin, 100 cm over a floor whose top is z 0 (the scripts' SPAWN (0, 0))
	const FVector PlayerStartLocation(0.0, 0.0, 100.0);
	constexpr double SpotToleranceCm = 5.0;
	constexpr double GeometryToleranceCm = 2.0;
	constexpr double YawToleranceDeg = 0.5;

	struct FBoxSpec
	{
		const TCHAR* Label;
		FBox Box;
	};

	struct FMapBoxes
	{
		const TCHAR* MapPath;
		TArray<FBoxSpec> Boxes;
	};

	/**
	 * Each map's floor (a 100 cm slab, top z 0) and boundary walls (z 0..100, 50 cm thick, inner faces on the floor
	 * edges, 50 cm past the corners): the story's boxes, as (x0, x1, y0, y1, z0, z1).
	 */
	TArray<FMapBoxes> FloorAndWallBoxes()
	{
		auto Box = [](double X0, double X1, double Y0, double Y1, double Z0, double Z1)
		{
			return FBox(FVector(X0, Y0, Z0), FVector(X1, Y1, Z1));
		};
		return {
			{ MovementTestMapPath, {
				{ TEXT("T1_Floor"),  Box(-2000.0, 2000.0, -2000.0, 2000.0, -100.0, 0.0) },
				{ TEXT("T1_Wall_N"), Box(-2050.0, 2050.0, 2000.0, 2050.0, 0.0, 100.0) },
				{ TEXT("T1_Wall_S"), Box(-2050.0, 2050.0, -2050.0, -2000.0, 0.0, 100.0) },
				{ TEXT("T1_Wall_E"), Box(2000.0, 2050.0, -2050.0, 2050.0, 0.0, 100.0) },
				{ TEXT("T1_Wall_W"), Box(-2050.0, -2000.0, -2050.0, 2050.0, 0.0, 100.0) },
			} },
			{ CombatTestMapPath, {
				{ TEXT("T1_Floor"),  Box(-1500.0, 2700.0, -1600.0, 1600.0, -100.0, 0.0) },
				{ TEXT("T1_Wall_N"), Box(-1550.0, 2750.0, 1600.0, 1650.0, 0.0, 100.0) },
				{ TEXT("T1_Wall_S"), Box(-1550.0, 2750.0, -1650.0, -1600.0, 0.0, 100.0) },
				{ TEXT("T1_Wall_E"), Box(2700.0, 2750.0, -1650.0, 1650.0, 0.0, 100.0) },
				{ TEXT("T1_Wall_W"), Box(-1550.0, -1500.0, -1650.0, 1650.0, 0.0, 100.0) },
			} },
		};
	}

	// CombatTestMap spots (Scripts/tests/fake_bridge.py TEST_DUMMY_SPEC, TEST_ATTACKER_SPEC, COMBAT_*_SPOT)
	const FVector2D TestDummySpot(600.0, 600.0);
	const FVector2D TestAttackerSpot(2100.0, 0.0);
	const FVector2D DummyMeleeSpot(440.0, 600.0);
	const FVector2D DummyShotSpot(300.0, 600.0);
	const FVector2D AttackerRouteCorner(440.0, 0.0);
	// 100 cm -X of the attacker: inside its 150 cm swing even when walk_to stops 25 cm off the spot
	const FVector2D AttackerFightSpot(2000.0, 0.0);
	const FVector2D BlinkLaneStart(0.0, -600.0);
	constexpr double BlinkLaneLengthCm = 1100.0;

	// A pawn standing on the floor: capsule half height 90 (the scripts read z ~92). The AI measures AggroRange in 3D
	// between actor centres, so on the flat floor every pawn centre sits at this height.
	constexpr double PawnCentreZ = 92.0;
	// walk_to stops within 50 cm of a spot and the player drifts: keep the spots this far beyond AggroRange
	constexpr double AggroMarginCm = 100.0;
	// Two capsules of radius 40 cannot stand closer than this
	constexpr double MinPawnSpacingCm = 80.0;

	/** Loads a saved map as a UWorld (no PIE, no play: the world is never initialized). */
	UWorld* LoadMap(FAutomationTestBase& Test, const TCHAR* ObjectPath)
	{
		UWorld* World = Cast<UWorld>(FSoftObjectPath(ObjectPath).TryLoad());
		if (!Test.TestNotNull(FString::Printf(TEXT("%s loads as a UWorld"), ObjectPath), World))
		{
			return nullptr;
		}
		if (!Test.TestNotNull(FString::Printf(TEXT("%s has a persistent level"), ObjectPath), World->PersistentLevel.Get()))
		{
			return nullptr;
		}
		return World;
	}

	/** The persistent level's actors, without the empty slots a loaded level keeps. */
	TArray<AActor*> LevelActors(const UWorld* World)
	{
		TArray<AActor*> Actors;
		for (AActor* Actor : World->PersistentLevel->Actors)
		{
			if (IsValid(Actor))
			{
				Actors.Add(Actor);
			}
		}
		return Actors;
	}

	template <typename ActorType>
	TArray<ActorType*> LevelActorsOf(const UWorld* World)
	{
		TArray<ActorType*> Found;
		for (AActor* Actor : LevelActors(World))
		{
			if (ActorType* Typed = Cast<ActorType>(Actor))
			{
				Found.Add(Typed);
			}
		}
		return Found;
	}

	/** True when Class or one of its parents is named ClassName (Blueprint classes, and classes LyraGame does not link). */
	bool IsChildOfClassNamed(const UClass* Class, const TCHAR* ClassName)
	{
		const FName Name(ClassName);
		for (const UClass* It = Class; It; It = It->GetSuperClass())
		{
			if (It->GetFName() == Name)
			{
				return true;
			}
		}
		return false;
	}

	TArray<AActor*> LevelActorsOfClassNamed(const UWorld* World, const TCHAR* ClassName)
	{
		return LevelActors(World).FilterByPredicate([ClassName](const AActor* Actor)
		{
			return IsChildOfClassNamed(Actor->GetClass(), ClassName);
		});
	}

	/** The actor the map script labelled Label (labels are saved with the level in editor builds). */
	AActor* FindActorByLabel(const UWorld* World, const TCHAR* Label)
	{
		for (AActor* Actor : LevelActors(World))
		{
			if (Actor->GetActorNameOrLabel() == Label)
			{
				return Actor;
			}
		}
		return nullptr;
	}

	/**
	 * A component's world transform, composed from the saved relative transforms. A loaded, never-initialized world
	 * registers no components, so their cached ComponentToWorld and Bounds are not set. (Sockets and absolute
	 * location/rotation/scale are not modelled: the map scripts use neither.)
	 */
	FTransform WorldTransformOf(const USceneComponent* Component)
	{
		FTransform Transform = Component->GetRelativeTransform();
		for (const USceneComponent* Parent = Component->GetAttachParent(); Parent; Parent = Parent->GetAttachParent())
		{
			Transform = Transform * Parent->GetRelativeTransform();
		}
		return Transform;
	}

	FVector ActorLocationOf(const AActor* Actor)
	{
		const USceneComponent* Root = Actor->GetRootComponent();
		return Root ? WorldTransformOf(Root).GetLocation() : FVector::ZeroVector;
	}

	FVector2D ActorSpotOf(const AActor* Actor)
	{
		const FVector Location = ActorLocationOf(Actor);
		return FVector2D(Location.X, Location.Y);
	}

	/** The components of Actor that stop a walking player (collision on, blocking the Pawn channel). */
	TArray<const UPrimitiveComponent*> PawnBlockingPrimitives(const AActor* Actor)
	{
		TArray<const UPrimitiveComponent*> Blocking;
		TInlineComponentArray<UPrimitiveComponent*> Primitives(Actor);
		for (const UPrimitiveComponent* Primitive : Primitives)
		{
			if (Primitive && !Primitive->IsEditorOnly() && Primitive->IsCollisionEnabled()
				&& Primitive->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block)
			{
				Blocking.Add(Primitive);
			}
		}
		return Blocking;
	}

	FBox WorldBoxOf(const UPrimitiveComponent* Primitive)
	{
		return Primitive->CalcBounds(WorldTransformOf(Primitive)).GetBox();
	}

	/** The world box of everything in Actor that blocks pawns; invalid (IsValid == 0) when nothing does. */
	FBox PawnBlockingBoxOf(const AActor* Actor)
	{
		FBox Box(ForceInit);
		for (const UPrimitiveComponent* Primitive : PawnBlockingPrimitives(Actor))
		{
			Box += WorldBoxOf(Primitive);
		}
		return Box;
	}

	bool BoxesNearlyEqual(const FBox& A, const FBox& B, double Tolerance)
	{
		return A.IsValid && B.IsValid && A.Min.Equals(B.Min, Tolerance) && A.Max.Equals(B.Max, Tolerance);
	}

	/** The material a box actor shows: its static mesh's slot 0, override first (what the map script set). */
	const UMaterialInterface* BoxMaterialOf(const AActor* Actor)
	{
		const UStaticMeshComponent* Mesh = Actor->FindComponentByClass<UStaticMeshComponent>();
		return Mesh ? Mesh->GetMaterial(0) : nullptr;
	}

	/** The enemy's BodyMesh, the body players see. Private with no accessor, so read through reflection. */
	const UStaticMeshComponent* BodyMeshOf(const AMordecaiEnemyCharacter* Enemy)
	{
		const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(AMordecaiEnemyCharacter::StaticClass(), TEXT("BodyMesh"));
		return Property ? Cast<UStaticMeshComponent>(Property->GetObjectPropertyValue_InContainer(Enemy)) : nullptr;
	}

	/** The body shows MaterialPath, an instance that sets its own Color (the one us100_01 writes) to about Expected. */
	void TestBodyMaterial(FAutomationTestBase& Test, const TCHAR* Who, const UStaticMeshComponent* Body, const TCHAR* MaterialPath, const FLinearColor& Expected)
	{
		const UMaterialInterface* Material = Body ? Body->GetMaterial(0) : nullptr;
		Test.TestEqual(FString::Printf(TEXT("%s's BodyMesh material is %s"), Who, MaterialPath), *GetPathNameSafe(Material), MaterialPath);
		const UMaterialInstance* Instance = Cast<UMaterialInstance>(Material);
		if (!Test.TestNotNull(FString::Printf(TEXT("%s's BodyMesh material is a material instance"), Who), Instance))
		{
			return;
		}
		FLinearColor Color(ForceInit);
		if (!Test.TestTrue(FString::Printf(TEXT("%s's body material sets its own %s parameter"), Who, BodyColorParameter),
			Instance->GetVectorParameterValue(FHashedMaterialParameterInfo(BodyColorParameter), Color, /*bOveriddenOnly*/ true)))
		{
			return;
		}
		Test.TestTrue(FString::Printf(TEXT("%s's body %s is about %s (is %s)"), Who, BodyColorParameter, *Expected.ToString(), *Color.ToString()),
			Color.Equals(Expected, ColorTolerance));
	}

	const AMordecaiEnemyAIController* AIDefaultsOf(const AMordecaiEnemyCharacter* Enemy)
	{
		const UClass* AIClass = Enemy->AIControllerClass.Get();
		return AIClass ? Cast<AMordecaiEnemyAIController>(AIClass->GetDefaultObject()) : nullptr;
	}

	bool GrantsMeleeAttack(const AMordecaiEnemyCharacter* Enemy)
	{
		for (const TSubclassOf<UGameplayAbility>& Ability : Enemy->DefaultAbilities)
		{
			if (Ability && Ability->IsChildOf(UMordecaiGA_MeleeAttack::StaticClass()))
			{
				return true;
			}
		}
		return false;
	}

	/** How far a profile's hit shape reaches from the attacker (mirrors UMordecaiGA_MeleeAttack::GetAttackReach). */
	double ProfileReach(const UMordecaiAttackProfileDataAsset* Profile)
	{
		if (!Profile)
		{
			return 0.0;
		}
		const FMordecaiHitShapeParams& Shape = Profile->HitShapeParams;
		switch (Profile->HitShapeType)
		{
		case EMordecaiHitShapeType::ArcSector: return Shape.Radius;
		case EMordecaiHitShapeType::Circle:    return Shape.Length + Shape.Radius;
		case EMordecaiHitShapeType::Capsule:
		case EMordecaiHitShapeType::Box:       return Shape.Length;
		default:                               return 0.0;
		}
	}

	/** The range the AI attacks from: AttackRange, shortened to the swing's reach (AMordecaiEnemyAIController::GetEffectiveAttackRange). */
	double EffectiveAttackRange(const AMordecaiEnemyCharacter* Enemy, const AMordecaiEnemyAIController* AI)
	{
		const double Reach = Enemy->EnemyAttackProfiles.Num() > 0 ? ProfileReach(Enemy->EnemyAttackProfiles[0]) : 0.0;
		return Reach > 0.0 ? FMath::Min(static_cast<double>(AI->AttackRange), Reach) : AI->AttackRange;
	}

	double PawnDistance(const FVector2D& A, const FVector2D& B)
	{
		return FVector::Dist(FVector(A.X, A.Y, PawnCentreZ), FVector(B.X, B.Y, PawnCentreZ));
	}

	/** The test enemy Blueprint's class defaults; nullptr (and a test error) when the Blueprint is missing. */
	const AMordecaiEnemyCharacter* LoadEnemyDefaults(FAutomationTestBase& Test, const TCHAR* ClassPath)
	{
		UClass* EnemyClass = FSoftClassPath(ClassPath).TryLoadClass<AMordecaiEnemyCharacter>();
		if (!Test.TestNotNull(FString::Printf(TEXT("%s loads as an AMordecaiEnemyCharacter class"), ClassPath), EnemyClass))
		{
			return nullptr;
		}
		return GetDefault<AMordecaiEnemyCharacter>(EnemyClass);
	}

	bool AutoPossessedWhenPlaced(const APawn* Pawn)
	{
		return Pawn->AutoPossessAI == EAutoPossessAI::PlacedInWorld || Pawn->AutoPossessAI == EAutoPossessAI::PlacedInWorldOrSpawned;
	}
}

// ---------------------------------------------------------------------------
// 1. Mordecai.Level.TestMaps.MovementTestMapExists
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Level_TestMaps_MovementTestMapExists,
	"Mordecai.Level.TestMaps.MovementTestMapExists",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Level_TestMaps_MovementTestMapExists::RunTest(const FString& Parameters)
{
	using namespace MordecaiTestMapTests;

	LoadMap(*this, MovementTestMapPath);
	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.Level.TestMaps.CombatTestMapExists
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Level_TestMaps_CombatTestMapExists,
	"Mordecai.Level.TestMaps.CombatTestMapExists",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Level_TestMaps_CombatTestMapExists::RunTest(const FString& Parameters)
{
	using namespace MordecaiTestMapTests;

	LoadMap(*this, CombatTestMapPath);
	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.Level.TestMaps.MapsUseMordecaiGameModeWithoutExperience
// The packaged route to the Mordecai experience: GlobalDefaultGameMode is B_LyraGameMode, so each map's World
// Settings GameMode Override must be AMordecaiGameMode, whose InitGame appends B_MordecaiArenaExperience only when
// the World Settings name no experience. PIE's ExperienceOverride hides a miss; a package boots Lyra's shooter.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Level_TestMaps_MapsUseMordecaiGameMode,
	"Mordecai.Level.TestMaps.MapsUseMordecaiGameModeWithoutExperience",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Level_TestMaps_MapsUseMordecaiGameMode::RunTest(const FString& Parameters)
{
	using namespace MordecaiTestMapTests;

	const FSoftClassProperty* ExperienceProperty = FindFProperty<FSoftClassProperty>(ALyraWorldSettings::StaticClass(), TEXT("DefaultGameplayExperience"));
	TestNotNull("ALyraWorldSettings has a DefaultGameplayExperience property", ExperienceProperty);

	for (const TCHAR* MapPath : { MovementTestMapPath, CombatTestMapPath })
	{
		const UWorld* World = LoadMap(*this, MapPath);
		if (!World)
		{
			continue;
		}

		const AWorldSettings* Settings = World->PersistentLevel->GetWorldSettings(false);
		if (!TestNotNull(FString::Printf(TEXT("%s has World Settings"), MapPath), Settings))
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s GameMode Override is AMordecaiGameMode (is %s)"), MapPath, *GetNameSafe(Settings->DefaultGameMode.Get())),
			Settings->DefaultGameMode && Settings->DefaultGameMode->IsChildOf(AMordecaiGameMode::StaticClass()));

		const ALyraWorldSettings* LyraSettings = Cast<ALyraWorldSettings>(Settings);
		if (!TestNotNull(FString::Printf(TEXT("%s World Settings are ALyraWorldSettings"), MapPath), LyraSettings))
		{
			continue;
		}
		if (ExperienceProperty)
		{
			const FSoftObjectPtr Experience = ExperienceProperty->GetPropertyValue_InContainer(LyraSettings);
			TestTrue(FString::Printf(TEXT("%s World Settings name no experience (DefaultGameplayExperience is '%s')"), MapPath, *Experience.ToString()),
				Experience.IsNull());
		}
		TestFalse(FString::Printf(TEXT("%s World Settings resolve no experience"), MapPath),
			LyraSettings->GetDefaultGameplayExperience().IsValid());
	}
	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.Level.TestMaps.MapsHaveOnePlayerStartAtOrigin
// Spawn and respawn both go to AGameModeBase::ChoosePlayerStart: with two starts the spawn is random. The scripts'
// SPAWN is (0, 0) and the respawn check wants the player within 150 cm of it.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Level_TestMaps_OnePlayerStartAtOrigin,
	"Mordecai.Level.TestMaps.MapsHaveOnePlayerStartAtOrigin",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Level_TestMaps_OnePlayerStartAtOrigin::RunTest(const FString& Parameters)
{
	using namespace MordecaiTestMapTests;

	for (const TCHAR* MapPath : { MovementTestMapPath, CombatTestMapPath })
	{
		const UWorld* World = LoadMap(*this, MapPath);
		if (!World)
		{
			continue;
		}

		const TArray<APlayerStart*> Starts = LevelActorsOf<APlayerStart>(World);
		if (!TestEqual(FString::Printf(TEXT("%s has exactly one PlayerStart"), MapPath), Starts.Num(), 1))
		{
			continue;
		}
		const FVector Location = ActorLocationOf(Starts[0]);
		TestNearlyEqual(FString::Printf(TEXT("%s PlayerStart is at (0, 0, 100) (is %s)"), MapPath, *Location.ToCompactString()),
			Location, PlayerStartLocation, static_cast<float>(SpotToleranceCm));
		// Yaw 0, as on DevTestMap: the player spawns facing +X
		const USceneComponent* Root = Starts[0]->GetRootComponent();
		const FRotator Rotation = Root ? WorldTransformOf(Root).Rotator() : FRotator::ZeroRotator;
		TestTrue(FString::Printf(TEXT("%s PlayerStart has yaw 0 (is %s)"), MapPath, *Rotation.ToCompactString()),
			Root && FMath::IsNearlyZero(FRotator::NormalizeAxis(Rotation.Yaw), YawToleranceDeg));
	}
	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.Level.TestMaps.MovementTestMapHasNoEnemiesOrPickups
// The movement map exists so us001, us083 and us069 play without an enemy in reach (N14).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Level_TestMaps_MovementNoEnemiesOrPickups,
	"Mordecai.Level.TestMaps.MovementTestMapHasNoEnemiesOrPickups",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Level_TestMaps_MovementNoEnemiesOrPickups::RunTest(const FString& Parameters)
{
	using namespace MordecaiTestMapTests;

	const UWorld* World = LoadMap(*this, MovementTestMapPath);
	if (!World)
	{
		return true;
	}
	TestEqual("MovementTestMap has no enemies", LevelActorsOf<AMordecaiEnemyCharacter>(World).Num(), 0);
	TestEqual("MovementTestMap has no item pickups", LevelActorsOf<AMordecaiItemPickup>(World).Num(), 0);
	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.Level.TestMaps.MovementTestMapHasStepsAndClearLane
// The floor (top z 0) under the whole square; a 30 cm step a player walks up (MaxStepHeight 45) and a 60 cm step
// only a jump gets on (apex ~105 cm), off every scripted route at +Y; and lane L1 (y -800, x -1000..1600), where
// us001 measures walking and sprinting, with nothing on it but the floor.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Level_TestMaps_MovementStepsAndLane,
	"Mordecai.Level.TestMaps.MovementTestMapHasStepsAndClearLane",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Level_TestMaps_MovementStepsAndLane::RunTest(const FString& Parameters)
{
	using namespace MordecaiTestMapTests;

	const UWorld* World = LoadMap(*this, MovementTestMapPath);
	if (!World)
	{
		return true;
	}

	const AActor* Floor = FindActorByLabel(World, TEXT("T1_Floor"));
	if (TestNotNull("MovementTestMap has T1_Floor", Floor))
	{
		const FBox FloorBox = PawnBlockingBoxOf(Floor);
		if (TestTrue("T1_Floor blocks pawns", FloorBox.IsValid != 0))
		{
			TestNearlyEqual("T1_Floor top is at z 0", FloorBox.Max.Z, 0.0, GeometryToleranceCm);
			TestTrue(FString::Printf(TEXT("T1_Floor covers x -2000..2000, y -2000..2000 (is %s)"), *FloorBox.ToString()),
				FloorBox.Min.X <= -2000.0 + GeometryToleranceCm && FloorBox.Max.X >= 2000.0 - GeometryToleranceCm
				&& FloorBox.Min.Y <= -2000.0 + GeometryToleranceCm && FloorBox.Max.Y >= 2000.0 - GeometryToleranceCm);
		}
	}

	struct FStepSpec
	{
		const TCHAR* Label;
		FBox Box;
	};
	const FStepSpec Steps[] = {
		{ TEXT("T1_Step_Walk30"), FBox(FVector(600.0, 800.0, 0.0), FVector(1000.0, 1200.0, 30.0)) },
		{ TEXT("T1_Step_Jump60"), FBox(FVector(1200.0, 800.0, 0.0), FVector(1600.0, 1200.0, 60.0)) },
	};
	for (const FStepSpec& Step : Steps)
	{
		const AActor* StepActor = FindActorByLabel(World, Step.Label);
		if (!TestNotNull(FString::Printf(TEXT("MovementTestMap has %s"), Step.Label), StepActor))
		{
			continue;
		}
		const FBox Box = PawnBlockingBoxOf(StepActor);
		if (!TestTrue(FString::Printf(TEXT("%s blocks pawns"), Step.Label), Box.IsValid != 0))
		{
			continue;
		}
		TestEqual(FString::Printf(TEXT("%s shows WorldGridMaterial"), Step.Label), *GetPathNameSafe(BoxMaterialOf(StepActor)), GridMaterialPath);
		TestNearlyEqual(FString::Printf(TEXT("%s top is at z %.0f"), Step.Label, Step.Box.Max.Z), Box.Max.Z, Step.Box.Max.Z, GeometryToleranceCm);
		TestNearlyEqual(FString::Printf(TEXT("%s sits on the floor (bottom z 0)"), Step.Label), Box.Min.Z, 0.0, GeometryToleranceCm);
		TestTrue(FString::Printf(TEXT("%s spans x %.0f..%.0f, y %.0f..%.0f (is %s)"), Step.Label,
				Step.Box.Min.X, Step.Box.Max.X, Step.Box.Min.Y, Step.Box.Max.Y, *Box.ToString()),
			FMath::IsNearlyEqual(Box.Min.X, Step.Box.Min.X, GeometryToleranceCm) && FMath::IsNearlyEqual(Box.Max.X, Step.Box.Max.X, GeometryToleranceCm)
			&& FMath::IsNearlyEqual(Box.Min.Y, Step.Box.Min.Y, GeometryToleranceCm) && FMath::IsNearlyEqual(Box.Max.Y, Step.Box.Max.Y, GeometryToleranceCm));
	}

	// Lane L1: from 1 cm over the floor to above a standing player's head (z 200; the capsule top is ~180), 50 cm
	// either side of y -800
	const FBox Lane(FVector(-1000.0, -850.0, 1.0), FVector(1600.0, -750.0, 200.0));
	TArray<FString> Blockers;
	for (const AActor* Actor : LevelActors(World))
	{
		if (Actor == Floor)
		{
			continue;
		}
		for (const UPrimitiveComponent* Primitive : PawnBlockingPrimitives(Actor))
		{
			const FBox Box = WorldBoxOf(Primitive);
			if (Box.Intersect(Lane))
			{
				Blockers.Add(FString::Printf(TEXT("%s.%s %s"), *Actor->GetActorNameOrLabel(), *Primitive->GetName(), *Box.ToString()));
			}
		}
	}
	TestEqual(FString::Printf(TEXT("nothing that blocks pawns is on lane L1 (x -1000..1600, y -850..-750) [%s]"), *FString::Join(Blockers, TEXT("; "))),
		Blockers.Num(), 0);
	return true;
}

// ---------------------------------------------------------------------------
// 7. Mordecai.Level.TestMaps.CombatTestMapHasDummyAndAttackerAtTheirSpots
// Exactly the two test enemies, where the scripts walk to them, with their Blueprints' stats (an instance override
// is the N13 trap: ValidateContent logs it), and none of DevTestMap's Frontliners or pickups.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Level_TestMaps_CombatEnemiesAtSpots,
	"Mordecai.Level.TestMaps.CombatTestMapHasDummyAndAttackerAtTheirSpots",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Level_TestMaps_CombatEnemiesAtSpots::RunTest(const FString& Parameters)
{
	using namespace MordecaiTestMapTests;

	const UWorld* World = LoadMap(*this, CombatTestMapPath);
	if (!World)
	{
		return true;
	}

	struct FEnemySpec
	{
		const TCHAR* ClassName;
		FVector2D Spot;
	};
	const FEnemySpec Enemies[] = {
		{ TestDummyClassName, TestDummySpot },
		{ TestAttackerClassName, TestAttackerSpot },
	};
	for (const FEnemySpec& Spec : Enemies)
	{
		const TArray<AActor*> Placed = LevelActors(World).FilterByPredicate([&Spec](const AActor* Actor)
		{
			return Actor->GetClass()->GetFName() == FName(Spec.ClassName);
		});
		if (!TestEqual(FString::Printf(TEXT("CombatTestMap has exactly one %s"), Spec.ClassName), Placed.Num(), 1))
		{
			continue;
		}
		const FVector2D Spot = ActorSpotOf(Placed[0]);
		TestTrue(FString::Printf(TEXT("%s stands at (%.0f, %.0f) (is at (%.1f, %.1f))"), Spec.ClassName, Spec.Spot.X, Spec.Spot.Y, Spot.X, Spot.Y),
			FVector2D::Distance(Spot, Spec.Spot) <= SpotToleranceCm);

		const AMordecaiEnemyCharacter* Enemy = Cast<AMordecaiEnemyCharacter>(Placed[0]);
		if (!TestNotNull(FString::Printf(TEXT("%s is an AMordecaiEnemyCharacter"), Spec.ClassName), Enemy))
		{
			continue;
		}
		const AMordecaiEnemyCharacter* ClassDefaults = GetDefault<AMordecaiEnemyCharacter>(Enemy->GetClass());
		TestEqual(FString::Printf(TEXT("the placed %s keeps its class DefaultMaxHealth"), Spec.ClassName), Enemy->DefaultMaxHealth, ClassDefaults->DefaultMaxHealth);
		TestEqual(FString::Printf(TEXT("the placed %s keeps its class DefaultMaxPosture"), Spec.ClassName), Enemy->DefaultMaxPosture, ClassDefaults->DefaultMaxPosture);
		TestEqual(FString::Printf(TEXT("the placed %s keeps its class DefaultMoveSpeed"), Spec.ClassName), Enemy->DefaultMoveSpeed, ClassDefaults->DefaultMoveSpeed);
		TestTrue(FString::Printf(TEXT("the placed %s keeps its class AIControllerClass"), Spec.ClassName),
			Enemy->AIControllerClass.Get() == ClassDefaults->AIControllerClass.Get());
	}

	TestEqual("CombatTestMap has no Frontliners", LevelActorsOfClassNamed(World, FrontlinerClassName).Num(), 0);
	TestEqual("CombatTestMap has only the two test enemies", LevelActorsOf<AMordecaiEnemyCharacter>(World).Num(), 2);
	TestEqual("CombatTestMap has no item pickups", LevelActorsOf<AMordecaiItemPickup>(World).Num(), 0);
	return true;
}

// ---------------------------------------------------------------------------
// 8. Mordecai.Level.TestMaps.CombatTestMapAttackerOutsideAggroRangeOfScriptedSpots
// The attacker engages only when a script walks the player to its fight spot. PlayerStart (spawn and every
// respawn: an arena reset puts it back in Idle at its spawn), the dummy, the melee and shot spots, the route corner
// and the whole Blink lane are beyond its AggroRange; the fight spot is inside the range it swings from.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Level_TestMaps_CombatAttackerAggro,
	"Mordecai.Level.TestMaps.CombatTestMapAttackerOutsideAggroRangeOfScriptedSpots",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Level_TestMaps_CombatAttackerAggro::RunTest(const FString& Parameters)
{
	using namespace MordecaiTestMapTests;

	const UWorld* World = LoadMap(*this, CombatTestMapPath);
	if (!World)
	{
		return true;
	}

	const TArray<AActor*> Attackers = LevelActorsOfClassNamed(World, TestAttackerClassName);
	const TArray<AActor*> Dummies = LevelActorsOfClassNamed(World, TestDummyClassName);
	const TArray<APlayerStart*> Starts = LevelActorsOf<APlayerStart>(World);
	const bool bOneAttacker = TestEqual("CombatTestMap has one TestAttacker", Attackers.Num(), 1);
	const bool bOneDummy = TestEqual("CombatTestMap has one TestDummy", Dummies.Num(), 1);
	const bool bOneStart = TestEqual("CombatTestMap has one PlayerStart", Starts.Num(), 1);
	if (!bOneAttacker || !bOneDummy || !bOneStart)
	{
		return true;
	}

	const AMordecaiEnemyCharacter* Attacker = Cast<AMordecaiEnemyCharacter>(Attackers[0]);
	const AMordecaiEnemyAIController* AI = Attacker ? AIDefaultsOf(Attacker) : nullptr;
	if (!TestNotNull("TestAttacker is an AMordecaiEnemyCharacter", Attacker)
		|| !TestNotNull("TestAttacker's AIControllerClass is an AMordecaiEnemyAIController", AI))
	{
		return true;
	}
	const double AggroRange = AI->AggroRange;
	TestTrue(FString::Printf(TEXT("TestAttacker aggroes at all (AggroRange %.0f)"), AggroRange), AggroRange > 0.0);

	const FVector2D AttackerSpot = ActorSpotOf(Attacker);
	struct FScriptedSpot
	{
		FString Name;
		FVector2D Spot;
	};
	TArray<FScriptedSpot> OutOfAggro = {
		{ TEXT("PlayerStart"), ActorSpotOf(Starts[0]) },
		{ TEXT("the TestDummy"), ActorSpotOf(Dummies[0]) },
		{ TEXT("the dummy melee spot (440, 600)"), DummyMeleeSpot },
		{ TEXT("the Fireball shot spot (300, 600)"), DummyShotSpot },
		{ TEXT("the route corner (440, 0)"), AttackerRouteCorner },
	};
	for (double X = 0.0; X <= BlinkLaneLengthCm; X += 100.0)
	{
		const FVector2D Point(BlinkLaneStart.X + X, BlinkLaneStart.Y);
		OutOfAggro.Add(FScriptedSpot{ FString::Printf(TEXT("the Blink lane at (%.0f, %.0f)"), Point.X, Point.Y), Point });
	}
	for (const FScriptedSpot& Scripted : OutOfAggro)
	{
		const double Distance = PawnDistance(AttackerSpot, Scripted.Spot);
		TestTrue(FString::Printf(TEXT("TestAttacker is beyond its AggroRange %.0f (+%.0f margin) of %s (%.0f cm)"), AggroRange, AggroMarginCm, *Scripted.Name, Distance),
			Distance > AggroRange + AggroMarginCm);
	}

	const double AttackRange = EffectiveAttackRange(Attacker, AI);
	const double FightDistance = PawnDistance(AttackerSpot, AttackerFightSpot);
	TestTrue(FString::Printf(TEXT("the fight spot (%.0f, %.0f) is inside TestAttacker's attack range %.0f (%.0f cm)"),
			AttackerFightSpot.X, AttackerFightSpot.Y, AttackRange, FightDistance),
		FightDistance <= AttackRange);
	TestTrue(FString::Printf(TEXT("a player fits at the fight spot (%.0f cm from TestAttacker, capsules need %.0f)"), FightDistance, MinPawnSpacingCm),
		FightDistance >= MinPawnSpacingCm);
	return true;
}

// ---------------------------------------------------------------------------
// 9. Mordecai.Level.TestMaps.CombatTestMapHasNavMesh
// The attacker never moves (speed 0), but its AI calls MoveToActor while it approaches; the navigation is static,
// so the map ships a built navmesh (RecastNavMesh) inside a NavMeshBoundsVolume that covers the scripted spots.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Level_TestMaps_CombatNavMesh,
	"Mordecai.Level.TestMaps.CombatTestMapHasNavMesh",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Level_TestMaps_CombatNavMesh::RunTest(const FString& Parameters)
{
	using namespace MordecaiTestMapTests;

	const UWorld* World = LoadMap(*this, CombatTestMapPath);
	if (!World)
	{
		return true;
	}

	const TArray<AActor*> Volumes = LevelActorsOfClassNamed(World, TEXT("NavMeshBoundsVolume"));
	TestTrue("CombatTestMap has a NavMeshBoundsVolume", Volumes.Num() > 0);
	TestTrue("CombatTestMap has a RecastNavMesh (a built navmesh is saved in the level)", LevelActorsOfClassNamed(World, TEXT("RecastNavMesh")).Num() > 0);

	FBox NavBounds(ForceInit);
	for (const AActor* Volume : Volumes)
	{
		TInlineComponentArray<UPrimitiveComponent*> Primitives(Volume);
		for (const UPrimitiveComponent* Primitive : Primitives)
		{
			NavBounds += WorldBoxOf(Primitive);
		}
	}
	if (Volumes.Num() == 0)
	{
		return true;
	}
	const FVector2D Spots[] = { FVector2D::ZeroVector, TestDummySpot, DummyMeleeSpot, DummyShotSpot, AttackerRouteCorner, AttackerFightSpot, TestAttackerSpot, BlinkLaneStart };
	for (const FVector2D& Spot : Spots)
	{
		TestTrue(FString::Printf(TEXT("the NavMeshBoundsVolume covers (%.0f, %.0f) at pawn height (bounds %s)"), Spot.X, Spot.Y, *NavBounds.ToString()),
			NavBounds.IsValid != 0 && NavBounds.IsInsideOrOn(FVector(Spot.X, Spot.Y, PawnCentreZ)));
	}
	return true;
}

// ---------------------------------------------------------------------------
// 10. Mordecai.Level.TestMaps.TestDummyNeverMovesOrAggroes
// A punching bag: speed 0, 200 HP, an AI that never leaves Idle (AggroRange 0) so it never approaches or swings,
// yet still a full enemy (melee ability and a profile, so ValidateContent logs nothing; a blue-grey body
// (MI_Enemy_TestDummy) and a health bar).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Level_TestMaps_TestDummy,
	"Mordecai.Level.TestMaps.TestDummyNeverMovesOrAggroes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Level_TestMaps_TestDummy::RunTest(const FString& Parameters)
{
	using namespace MordecaiTestMapTests;

	const AMordecaiEnemyCharacter* Dummy = LoadEnemyDefaults(*this, TestDummyClassPath);
	if (!Dummy)
	{
		return true;
	}

	TestEqual("TestDummy DefaultMoveSpeed is 0 (never moves)", Dummy->DefaultMoveSpeed, 0.f);
	TestEqual("TestDummy DefaultMaxHealth is 200", Dummy->DefaultMaxHealth, 200.f);
	TestTrue("TestDummy is possessed by its AI when placed (the bridge reads ai_state Idle)", AutoPossessedWhenPlaced(Dummy));

	const UClass* AIClass = Dummy->AIControllerClass.Get();
	TestTrue(FString::Printf(TEXT("TestDummy's AIControllerClass is a child of AMordecaiEnemyAIController (is %s)"), *GetNameSafe(AIClass)),
		AIClass && AIClass->IsChildOf(AMordecaiEnemyAIController::StaticClass()));
	if (const AMordecaiEnemyAIController* AI = AIDefaultsOf(Dummy))
	{
		TestEqual("TestDummy's AI AggroRange is 0 (never leaves Idle)", AI->AggroRange, 0.f);
	}

	TestTrue("TestDummy keeps the melee ability in DefaultAbilities", GrantsMeleeAttack(Dummy));
	TestTrue("TestDummy keeps an attack profile (ValidateContent logs none missing)",
		Dummy->EnemyAttackProfiles.Num() >= 1 && Dummy->EnemyAttackProfiles[0] != nullptr);
	TestNotNull("TestDummy has a health bar widget class", Dummy->EnemyHealthBarWidgetClass.Get());

	const UStaticMeshComponent* Body = BodyMeshOf(Dummy);
	TestTrue("TestDummy's BodyMesh has a static mesh (players can see it)", Body && Body->GetStaticMesh());
	TestBodyMaterial(*this, TEXT("TestDummy"), Body, TestDummyMaterialPath, TestDummyColor);
	return true;
}

// ---------------------------------------------------------------------------
// 11. Mordecai.Level.TestMaps.TestAttackerHoldsItsSpotAndSwings
// Speed 0 holds its spot; the native AI aggroes (900 today) and swings BasicSlash on a known cadence: windup 500 ms,
// active 200, recovery 400, then AttackCooldownMs 1500, so about one 15-damage hit every 2.6 s. An orange body
// (MI_Enemy_TestAttacker) tells it from the dummy on screen.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Level_TestMaps_TestAttacker,
	"Mordecai.Level.TestMaps.TestAttackerHoldsItsSpotAndSwings",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Level_TestMaps_TestAttacker::RunTest(const FString& Parameters)
{
	using namespace MordecaiTestMapTests;

	const AMordecaiEnemyCharacter* Attacker = LoadEnemyDefaults(*this, TestAttackerClassPath);
	if (!Attacker)
	{
		return true;
	}

	TestEqual("TestAttacker DefaultMoveSpeed is 0 (holds its spot)", Attacker->DefaultMoveSpeed, 0.f);
	TestEqual("TestAttacker DefaultMaxHealth is 200", Attacker->DefaultMaxHealth, 200.f);
	TestTrue("TestAttacker is possessed by its AI when placed", AutoPossessedWhenPlaced(Attacker));

	const UClass* AIClass = Attacker->AIControllerClass.Get();
	TestTrue(FString::Printf(TEXT("TestAttacker's AIControllerClass is a child of AMordecaiEnemyAIController (is %s)"), *GetNameSafe(AIClass)),
		AIClass && AIClass->IsChildOf(AMordecaiEnemyAIController::StaticClass()));
	if (const AMordecaiEnemyAIController* AI = AIDefaultsOf(Attacker))
	{
		TestTrue(FString::Printf(TEXT("TestAttacker's AI aggroes (AggroRange %.0f > 0)"), AI->AggroRange), AI->AggroRange > 0.f);
		TestEqual("TestAttacker's AI AttackCooldownMs is 1500 (the known cadence)", AI->AttackCooldownMs, 1500.f);
		TestTrue("TestAttacker's AI swings the melee ability",
			AI->MeleeAttackAbilityClass && AI->MeleeAttackAbilityClass->IsChildOf(UMordecaiGA_MeleeAttack::StaticClass()));
	}

	TestTrue("TestAttacker has the melee ability in DefaultAbilities", GrantsMeleeAttack(Attacker));
	if (TestEqual("TestAttacker has one attack profile (the AI only swings profile 0, N24)", Attacker->EnemyAttackProfiles.Num(), 1))
	{
		TestEqual("TestAttacker's attack profile is DA_EnemyAttack_BasicSlash",
			*GetPathNameSafe(Attacker->EnemyAttackProfiles[0]), BasicSlashPath);
	}
	TestNotNull("TestAttacker has a health bar widget class", Attacker->EnemyHealthBarWidgetClass.Get());

	const UStaticMeshComponent* Body = BodyMeshOf(Attacker);
	TestTrue("TestAttacker's BodyMesh has a static mesh (players can see it)", Body && Body->GetStaticMesh());
	TestBodyMaterial(*this, TEXT("TestAttacker"), Body, TestAttackerMaterialPath, TestAttackerColor);
	return true;
}

// ---------------------------------------------------------------------------
// 12. Mordecai.Level.TestMaps.MapsAreOneUmapNotWorldPartition
// new_level(path, is_partitioned_world=False): every actor lives in the one .umap. A World Partition map keeps its
// actors in __ExternalActors__ files, which a commit or a cook can miss without anyone noticing.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Level_TestMaps_OneUmap,
	"Mordecai.Level.TestMaps.MapsAreOneUmapNotWorldPartition",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Level_TestMaps_OneUmap::RunTest(const FString& Parameters)
{
	using namespace MordecaiTestMapTests;

	for (const TCHAR* MapPath : { MovementTestMapPath, CombatTestMapPath })
	{
		const UWorld* World = LoadMap(*this, MapPath);
		if (!World)
		{
			continue;
		}
		const AWorldSettings* Settings = World->PersistentLevel->GetWorldSettings(false);
		if (!TestNotNull(FString::Printf(TEXT("%s has World Settings"), MapPath), Settings))
		{
			continue;
		}
		TestNull(FString::Printf(TEXT("%s is not World Partition (its World Settings hold no UWorldPartition)"), MapPath), Settings->GetWorldPartition());
#if WITH_EDITOR
		// ULevel::IsUsingExternalActors exists only in editor builds; the packaged game target compiles this file too
		TestFalse(FString::Printf(TEXT("%s keeps its actors in the .umap (no external actors)"), MapPath), World->PersistentLevel->IsUsingExternalActors());
#endif
	}
	return true;
}

// ---------------------------------------------------------------------------
// 13. Mordecai.Level.TestMaps.MapsHaveGridFloorAndBoundaryWalls
// Each map: T1_Floor (top z 0) under the whole play area and the four 100 cm boundary walls, on exactly the story's
// boxes, all blocking pawns and showing WorldGridMaterial (a measuring grid in every screenshot, always cooked).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Level_TestMaps_FloorAndWalls,
	"Mordecai.Level.TestMaps.MapsHaveGridFloorAndBoundaryWalls",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Level_TestMaps_FloorAndWalls::RunTest(const FString& Parameters)
{
	using namespace MordecaiTestMapTests;

	for (const FMapBoxes& Map : FloorAndWallBoxes())
	{
		const UWorld* World = LoadMap(*this, Map.MapPath);
		if (!World)
		{
			continue;
		}
		for (const FBoxSpec& Spec : Map.Boxes)
		{
			const AActor* Actor = FindActorByLabel(World, Spec.Label);
			if (!TestNotNull(FString::Printf(TEXT("%s has %s"), Map.MapPath, Spec.Label), Actor))
			{
				continue;
			}
			const FBox Box = PawnBlockingBoxOf(Actor);
			if (TestTrue(FString::Printf(TEXT("%s %s blocks pawns"), Map.MapPath, Spec.Label), Box.IsValid != 0))
			{
				TestTrue(FString::Printf(TEXT("%s %s spans %s (is %s)"), Map.MapPath, Spec.Label, *Spec.Box.ToString(), *Box.ToString()),
					BoxesNearlyEqual(Box, Spec.Box, GeometryToleranceCm));
			}
			TestEqual(FString::Printf(TEXT("%s %s shows WorldGridMaterial"), Map.MapPath, Spec.Label),
				*GetPathNameSafe(BoxMaterialOf(Actor)), GridMaterialPath);
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
// 14. Mordecai.Level.TestMaps.MapsHaveOnlyMovableLights
// The project lights with Lumen: Movable lights only, so there is no lighting to build and no _BuiltData (gitignored:
// it would reach the cook from the working tree, never the commit), and no reflection captures.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Level_TestMaps_MovableLights,
	"Mordecai.Level.TestMaps.MapsHaveOnlyMovableLights",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Level_TestMaps_MovableLights::RunTest(const FString& Parameters)
{
	using namespace MordecaiTestMapTests;

	for (const TCHAR* MapPath : { MovementTestMapPath, CombatTestMapPath })
	{
		const UWorld* World = LoadMap(*this, MapPath);
		if (!World)
		{
			continue;
		}
		int32 DirectionalLights = 0;
		int32 SkyLights = 0;
		TArray<FString> NotMovable;
		for (const AActor* Actor : LevelActors(World))
		{
			TInlineComponentArray<ULightComponentBase*> Lights(Actor);
			for (const ULightComponentBase* Light : Lights)
			{
				DirectionalLights += Light->IsA<UDirectionalLightComponent>() ? 1 : 0;
				SkyLights += Light->IsA<USkyLightComponent>() ? 1 : 0;
				if (Light->Mobility.GetValue() != EComponentMobility::Movable)
				{
					NotMovable.Add(FString::Printf(TEXT("%s.%s (mobility %d)"), *Actor->GetActorNameOrLabel(), *Light->GetName(), static_cast<int32>(Light->Mobility.GetValue())));
				}
			}
		}
		TestTrue(FString::Printf(TEXT("%s has a directional light (T1_Sun)"), MapPath), DirectionalLights >= 1);
		TestTrue(FString::Printf(TEXT("%s has a sky light (T1_SkyLight)"), MapPath), SkyLights >= 1);
		TestEqual(FString::Printf(TEXT("every light in %s is Movable [%s]"), MapPath, *FString::Join(NotMovable, TEXT("; "))), NotMovable.Num(), 0);
		TestEqual(FString::Printf(TEXT("%s has no reflection captures"), MapPath), LevelActorsOfClassNamed(World, TEXT("ReflectionCapture")).Num(), 0);
	}
	return true;
}
