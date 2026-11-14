// Project Mordecai -- US-120: deterministic Verdant Wilds v3 terrain-kit content contract.

#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/BlockingVolume.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Mordecai/MordecaiGameMode.h"
#include "PhysicsEngine/AggregateGeom.h"
#include "PhysicsEngine/BodySetup.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "StaticMeshResources.h"
#include "UObject/SoftObjectPath.h"

namespace MordecaiVerdantWildsTests
{
	const TCHAR* const ManifestRelativePath = TEXT("Scripts/us120_vw_source_manifest.json");
	const TCHAR* const MapPackagePath = TEXT("/MordecaiCore/Game/Maps/L_VW_TileTest");
	const TCHAR* const MasterMaterialPath =
		TEXT("/MordecaiCore/World/Terrain/Shared/Materials/M_Terrain_Master");
	constexpr double BoundsToleranceCm = 1.0;
	constexpr double FootprintToleranceCm = 0.1;
	constexpr double GridToleranceCm = 0.1;

	TSharedPtr<FJsonObject> LoadManifest(FAutomationTestBase& Test)
	{
		const FString Path = FPaths::Combine(FPaths::ProjectDir(), ManifestRelativePath);
		FString Text;
		if (!Test.TestTrue(FString::Printf(TEXT("manifest exists at %s"), *Path),
			FFileHelper::LoadFileToString(Text, *Path)))
		{
			return nullptr;
		}

		TSharedPtr<FJsonObject> Root;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
		Test.TestTrue(TEXT("manifest is valid JSON"), FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid());
		return Root;
	}

	FString StringField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		FString Result;
		return Object.IsValid() && Object->TryGetStringField(Field, Result) ? Result : FString();
	}

	const TArray<TSharedPtr<FJsonValue>>* ArrayField(
		FAutomationTestBase& Test, const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		const TArray<TSharedPtr<FJsonValue>>* Result = nullptr;
		Test.TestTrue(FString::Printf(TEXT("manifest has array '%s'"), Field),
			Object.IsValid() && Object->TryGetArrayField(Field, Result));
		return Result;
	}

	TSharedPtr<FJsonObject> ObjectField(
		FAutomationTestBase& Test, const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		const TSharedPtr<FJsonObject>* Result = nullptr;
		Test.TestTrue(FString::Printf(TEXT("manifest has object '%s'"), Field),
			Object.IsValid() && Object->TryGetObjectField(Field, Result));
		return Result ? *Result : nullptr;
	}

	FVector VectorField(
		FAutomationTestBase& Test, const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = ArrayField(Test, Object, Field);
		if (!Values || !Test.TestEqual(FString::Printf(TEXT("%s has three numbers"), Field), Values->Num(), 3))
		{
			return FVector::ZeroVector;
		}
		return FVector((*Values)[0]->AsNumber(), (*Values)[1]->AsNumber(), (*Values)[2]->AsNumber());
	}

	FString ObjectPath(const FString& PackagePath)
	{
		return PackagePath + TEXT(".") + FPackageName::GetShortName(PackagePath);
	}

	template <typename AssetType>
	AssetType* LoadAsset(const FString& PackagePath)
	{
		return Cast<AssetType>(FSoftObjectPath(ObjectPath(PackagePath)).TryLoad());
	}

	FString AssetRoot(
		FAutomationTestBase& Test, const TSharedPtr<FJsonObject>& Manifest, const TCHAR* Field)
	{
		return StringField(ObjectField(Test, Manifest, TEXT("import_contract")), Field);
	}

	const TArray<TSharedPtr<FJsonValue>>* MeshEntries(
		FAutomationTestBase& Test, const TSharedPtr<FJsonObject>& Manifest)
	{
		return ArrayField(Test, Manifest, TEXT("meshes"));
	}

	UStaticMesh* LoadMesh(
		FAutomationTestBase& Test, const FString& MeshRoot, const TSharedPtr<FJsonObject>& Entry)
	{
		const FString Id = StringField(Entry, TEXT("id"));
		const FString Path = MeshRoot / Id;
		UStaticMesh* Mesh = LoadAsset<UStaticMesh>(Path);
		Test.TestNotNull(FString::Printf(TEXT("%s loads as UStaticMesh"), *Path), Mesh);
		return Mesh;
	}

	TArray<AActor*> LevelActors(const UWorld* World)
	{
		TArray<AActor*> Result;
		if (!World || !World->PersistentLevel)
		{
			return Result;
		}
		for (AActor* Actor : World->PersistentLevel->Actors)
		{
			if (IsValid(Actor))
			{
				Result.Add(Actor);
			}
		}
		return Result;
	}

	AActor* FindActorByLabel(const UWorld* World, const FString& Label)
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

	FTransform SavedWorldTransform(const USceneComponent* Component)
	{
		FTransform Transform = Component ? Component->GetRelativeTransform() : FTransform::Identity;
		for (const USceneComponent* Parent = Component ? Component->GetAttachParent() : nullptr;
			Parent; Parent = Parent->GetAttachParent())
		{
			Transform = Transform * Parent->GetRelativeTransform();
		}
		return Transform;
	}

	bool IsOnGrid(double Value)
	{
		return FMath::IsNearlyEqual(Value, FMath::GridSnap(Value, 200.0), GridToleranceCm);
	}

	double Cross2D(const FVector2D& Origin, const FVector2D& A, const FVector2D& B)
	{
		return (A.X - Origin.X) * (B.Y - Origin.Y)
			- (A.Y - Origin.Y) * (B.X - Origin.X);
	}

	TArray<FVector2D> ConvexOutline(const FKConvexElem& Convex)
	{
		TArray<FVector2D> Points;
		const FTransform Transform = Convex.GetTransform();
		for (const FVector& LocalVertex : Convex.VertexData)
		{
			const FVector Vertex = Transform.TransformPosition(LocalVertex);
			Points.AddUnique(FVector2D(Vertex.X, Vertex.Y));
		}
		Points.Sort([](const FVector2D& A, const FVector2D& B)
		{
			return A.X < B.X || (FMath::IsNearlyEqual(A.X, B.X) && A.Y < B.Y);
		});
		if (Points.Num() < 3)
		{
			return Points;
		}
		TArray<FVector2D> Lower;
		for (const FVector2D& Point : Points)
		{
			while (Lower.Num() >= 2
				&& Cross2D(Lower[Lower.Num() - 2], Lower.Last(), Point) <= 0.0)
			{
				Lower.Pop(EAllowShrinking::No);
			}
			Lower.Add(Point);
		}
		TArray<FVector2D> Upper;
		for (int32 Index = Points.Num() - 1; Index >= 0; --Index)
		{
			const FVector2D& Point = Points[Index];
			while (Upper.Num() >= 2
				&& Cross2D(Upper[Upper.Num() - 2], Upper.Last(), Point) <= 0.0)
			{
				Upper.Pop(EAllowShrinking::No);
			}
			Upper.Add(Point);
		}
		Lower.Pop(EAllowShrinking::No);
		Upper.Pop(EAllowShrinking::No);
		Lower.Append(Upper);
		return Lower;
	}

	TArray<FVector2D> ClipHalfPlane(
		const TArray<FVector2D>& Polygon, bool bClipX, double Limit, bool bKeepLess)
	{
		TArray<FVector2D> Result;
		if (Polygon.IsEmpty())
		{
			return Result;
		}
		auto Coordinate = [bClipX](const FVector2D& Point)
		{
			return bClipX ? Point.X : Point.Y;
		};
		FVector2D Previous = Polygon.Last();
		bool bPreviousInside = bKeepLess
			? Coordinate(Previous) < Limit : Coordinate(Previous) > Limit;
		for (const FVector2D& Current : Polygon)
		{
			const bool bCurrentInside = bKeepLess
				? Coordinate(Current) < Limit : Coordinate(Current) > Limit;
			if (bCurrentInside != bPreviousInside)
			{
				const double PreviousCoordinate = Coordinate(Previous);
				const double Denominator = Coordinate(Current) - PreviousCoordinate;
				const double Alpha = FMath::IsNearlyZero(Denominator)
					? 0.0 : (Limit - PreviousCoordinate) / Denominator;
				Result.Add(Previous + (Current - Previous) * Alpha);
			}
			if (bCurrentInside)
			{
				Result.Add(Current);
			}
			Previous = Current;
			bPreviousInside = bCurrentInside;
		}
		return Result;
	}

	bool ConvexOverlapsLandRegion(
		FAutomationTestBase& Test,
		const FKConvexElem& Convex,
		const TSharedPtr<FJsonObject>& Region)
	{
		if (!Region.IsValid())
		{
			Test.AddError(TEXT("missing corner_land_region"));
			return true;
		}
		const FString Operator = StringField(Region, TEXT("operator"));
		TArray<FVector2D> Outline = ConvexOutline(Convex);
		if (Operator == TEXT("half_plane"))
		{
			return !ClipHalfPlane(Outline, false,
				Region->GetNumberField(TEXT("y_lt_cm")) - FootprintToleranceCm, true).IsEmpty();
		}
		if (Operator == TEXT("and"))
		{
			Outline = ClipHalfPlane(Outline, true,
				Region->GetNumberField(TEXT("x_gt_cm")) + FootprintToleranceCm, false);
			Outline = ClipHalfPlane(Outline, false,
				Region->GetNumberField(TEXT("y_lt_cm")) - FootprintToleranceCm, true);
			return !Outline.IsEmpty();
		}
		if (Operator == TEXT("or"))
		{
			return !ClipHalfPlane(Outline, true,
					Region->GetNumberField(TEXT("x_gt_cm")) + FootprintToleranceCm, false).IsEmpty()
				|| !ClipHalfPlane(Outline, false,
					Region->GetNumberField(TEXT("y_lt_cm")) - FootprintToleranceCm, true).IsEmpty();
		}
		Test.AddError(FString::Printf(TEXT("unsupported corner_land_region operator '%s'"), *Operator));
		return true;
	}

	UWorld* LoadTileMap(FAutomationTestBase& Test)
	{
		UWorld* World = LoadAsset<UWorld>(MapPackagePath);
		Test.TestNotNull(FString::Printf(TEXT("%s loads as UWorld"), MapPackagePath), World);
		return World;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_VW_AllReadyMeshesLoad,
	"Mordecai.World.Terrain.VerdantWilds.AllReadyMeshesLoad",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_VW_AllReadyMeshesLoad::RunTest(const FString& Parameters)
{
	using namespace MordecaiVerdantWildsTests;
	const TSharedPtr<FJsonObject> Manifest = LoadManifest(*this);
	const TArray<TSharedPtr<FJsonValue>>* Entries = MeshEntries(*this, Manifest);
	if (!Manifest || !Entries)
	{
		return false;
	}
	TestEqual(TEXT("manifest contains exactly 17 ready meshes"), Entries->Num(), 17);

	const FString MeshRoot = AssetRoot(*this, Manifest, TEXT("mesh_root"));
	TSet<FName> ExpectedPackages;
	for (const TSharedPtr<FJsonValue>& Value : *Entries)
	{
		const TSharedPtr<FJsonObject> Entry = Value->AsObject();
		TestEqual(FString::Printf(TEXT("%s is ready"), *StringField(Entry, TEXT("id"))),
			StringField(Entry, TEXT("status")), FString(TEXT("ready")));
		const FString PackagePath = MeshRoot / StringField(Entry, TEXT("id"));
		ExpectedPackages.Add(FName(PackagePath));
		LoadMesh(*this, MeshRoot, Entry);
	}

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(MeshRoot));
	Filter.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = false;
	TArray<FAssetData> Assets;
	FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"))
		.Get().GetAssets(Filter, Assets);
	TSet<FName> ActualPackages;
	for (const FAssetData& Asset : Assets)
	{
		ActualPackages.Add(Asset.PackageName);
	}
	TestEqual(TEXT("mesh folder has no unexpected UStaticMesh assets"),
		ActualPackages.Difference(ExpectedPackages).Num(), 0);
	TestEqual(TEXT("mesh folder contains every expected package"),
		ExpectedPackages.Difference(ActualPackages).Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_VW_ImportContract,
	"Mordecai.World.Terrain.VerdantWilds.ImportContract",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_VW_ImportContract::RunTest(const FString& Parameters)
{
	using namespace MordecaiVerdantWildsTests;
	const TSharedPtr<FJsonObject> Manifest = LoadManifest(*this);
	const TArray<TSharedPtr<FJsonValue>>* Entries = MeshEntries(*this, Manifest);
	if (!Manifest || !Entries)
	{
		return false;
	}
	const FString MeshRoot = AssetRoot(*this, Manifest, TEXT("mesh_root"));
	for (const TSharedPtr<FJsonValue>& Value : *Entries)
	{
		const TSharedPtr<FJsonObject> Entry = Value->AsObject();
		UStaticMesh* Mesh = LoadMesh(*this, MeshRoot, Entry);
		if (!Mesh)
		{
			continue;
		}
		const TSharedPtr<FJsonObject> BoundsObject = ObjectField(*this, Entry, TEXT("bounds_cm"));
		const TSharedPtr<FJsonObject> SourceBoundsObject =
			ObjectField(*this, Entry, TEXT("source_bounds_cm"));
		const FBox Expected(VectorField(*this, BoundsObject, TEXT("min")),
			VectorField(*this, BoundsObject, TEXT("max")));
		const FBox SourceBounds(VectorField(*this, SourceBoundsObject, TEXT("min")),
			VectorField(*this, SourceBoundsObject, TEXT("max")));
		const FBox ConvertedSourceBounds(
			FVector(SourceBounds.Min.X, -SourceBounds.Max.Y, SourceBounds.Min.Z),
			FVector(SourceBounds.Max.X, -SourceBounds.Min.Y, SourceBounds.Max.Z));
		TestTrue(FString::Printf(TEXT("%s manifest converts source bounds by negating Y"),
				*Mesh->GetName()), Expected.Equals(ConvertedSourceBounds, KINDA_SMALL_NUMBER));
		const FBox Actual = Mesh->GetBoundingBox();
		TestTrue(FString::Printf(TEXT("%s min bounds match catalog within 1 cm: expected %s, actual %s"),
				*Mesh->GetName(), *Expected.Min.ToString(), *Actual.Min.ToString()),
			Expected.Min.Equals(Actual.Min, BoundsToleranceCm));
		TestTrue(FString::Printf(TEXT("%s max bounds match catalog within 1 cm: expected %s, actual %s"),
				*Mesh->GetName(), *Expected.Max.ToString(), *Actual.Max.ToString()),
			Expected.Max.Equals(Actual.Max, BoundsToleranceCm));

		const TSharedPtr<FJsonObject> Units = ObjectField(*this, Entry, TEXT("units"));
		TestEqual(FString::Printf(TEXT("%s uses centimeters"), *Mesh->GetName()),
			StringField(Units, TEXT("system")), FString(TEXT("cm")));
		TestEqual(FString::Printf(TEXT("%s catalog grid is 200 cm"), *Mesh->GetName()),
			static_cast<int32>(Units->GetNumberField(TEXT("grid_cm"))), 200);

		const TSharedPtr<FJsonObject> Footprint = ObjectField(*this, Entry, TEXT("footprint"));
		const FVector Pivot = VectorField(*this, Footprint, TEXT("pivot_local_cm"));
		TestTrue(FString::Printf(TEXT("%s pivot is the declared local grid origin"), *Mesh->GetName()),
			Pivot.IsNearlyZero(KINDA_SMALL_NUMBER));
#if WITH_EDITOR
		const FMeshBuildSettings& BuildSettings = Mesh->GetSourceModel(0).BuildSettings;
		TestTrue(FString::Printf(TEXT("%s BuildScale3D is unit"), *Mesh->GetName()),
			BuildSettings.BuildScale3D.Equals(FVector::OneVector, KINDA_SMALL_NUMBER));
		TestFalse(FString::Printf(TEXT("%s does not generate lightmap UVs during import"),
			*Mesh->GetName()), BuildSettings.bGenerateLightmapUVs);
#endif
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_VW_NaniteAndSimpleCollision,
	"Mordecai.World.Terrain.VerdantWilds.NaniteAndSimpleCollision",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_VW_NaniteAndSimpleCollision::RunTest(const FString& Parameters)
{
	using namespace MordecaiVerdantWildsTests;
	const TSharedPtr<FJsonObject> Manifest = LoadManifest(*this);
	const TArray<TSharedPtr<FJsonValue>>* Entries = MeshEntries(*this, Manifest);
	if (!Manifest || !Entries)
	{
		return false;
	}
	const FString MeshRoot = AssetRoot(*this, Manifest, TEXT("mesh_root"));
	for (const TSharedPtr<FJsonValue>& Value : *Entries)
	{
		const TSharedPtr<FJsonObject> Entry = Value->AsObject();
		UStaticMesh* Mesh = LoadMesh(*this, MeshRoot, Entry);
		if (!Mesh)
		{
			continue;
		}
#if WITH_EDITOR
		TestTrue(FString::Printf(TEXT("%s has Nanite enabled"), *Mesh->GetName()),
			Mesh->GetNaniteSettings().bEnabled);
#endif
		const UBodySetup* Body = Mesh->GetBodySetup();
		if (!TestNotNull(FString::Printf(TEXT("%s has a BodySetup"), *Mesh->GetName()), Body))
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s does not use complex-as-simple"), *Mesh->GetName()),
			Body->CollisionTraceFlag != CTF_UseComplexAsSimple);
		const TSharedPtr<FJsonObject> Collision = ObjectField(*this, Entry, TEXT("collision"));
		const int32 ExpectedHullCount = static_cast<int32>(Collision->GetNumberField(TEXT("count")));
		TestEqual(FString::Printf(TEXT("%s preserves the catalog UCX hull count"), *Mesh->GetName()),
			Body->AggGeom.ConvexElems.Num(), ExpectedHullCount);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_VW_CollisionInsideFootprint,
	"Mordecai.World.Terrain.VerdantWilds.CollisionInsideFootprint",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_VW_CollisionInsideFootprint::RunTest(const FString& Parameters)
{
	using namespace MordecaiVerdantWildsTests;
	const TSharedPtr<FJsonObject> Manifest = LoadManifest(*this);
	const TArray<TSharedPtr<FJsonValue>>* Entries = MeshEntries(*this, Manifest);
	if (!Manifest || !Entries)
	{
		return false;
	}
	const FString MeshRoot = AssetRoot(*this, Manifest, TEXT("mesh_root"));
	const TSharedPtr<FJsonObject> PondContract =
		ObjectField(*this, Manifest, TEXT("pond_collision_contracts"));
	const TArray<TSharedPtr<FJsonValue>>* EdgeXY =
		ArrayField(*this, PondContract, TEXT("edge_xy_cm"));
	const TArray<TSharedPtr<FJsonValue>>* CornerXY =
		ArrayField(*this, PondContract, TEXT("corner_xy_cm"));
	if (!PondContract || !EdgeXY || EdgeXY->Num() != 4
		|| !CornerXY || CornerXY->Num() != 4)
	{
		return false;
	}
	const double EdgeXTolerance = PondContract->GetNumberField(TEXT("edge_x_tolerance_cm"));
	const double PondMinimumZ = PondContract->GetNumberField(TEXT("min_z_cm"));
	const double ShallowPondMaximumZ = PondContract->GetNumberField(TEXT("shallow_max_z_cm"));
	const double DeepPondMaximumZ = PondContract->GetNumberField(TEXT("deep_wall_max_z_cm"));
	for (const TSharedPtr<FJsonValue>& Value : *Entries)
	{
		const TSharedPtr<FJsonObject> Entry = Value->AsObject();
		UStaticMesh* Mesh = LoadMesh(*this, MeshRoot, Entry);
		if (!Mesh || !Mesh->GetBodySetup())
		{
			continue;
		}
		const TSharedPtr<FJsonObject> Footprint = ObjectField(*this, Entry, TEXT("footprint"));
		const FString Id = StringField(Entry, TEXT("id"));
		const FString Kind = StringField(Entry, TEXT("kind"));
		const bool bIsPond = Kind.StartsWith(TEXT("pond_bank"));
		const bool bIsDeepPond = Kind == TEXT("pond_bank_deep");
		const bool bIsPondEdge = bIsPond && Id.Contains(TEXT("_Edge_"));
		const TSharedPtr<FJsonObject> Collision =
			ObjectField(*this, Entry, TEXT("collision"));
		const TSharedPtr<FJsonObject> LandRegion = bIsPond
			? ObjectField(*this, Collision, TEXT("corner_land_region"))
			: nullptr;
		const TArray<TSharedPtr<FJsonValue>>* RectValues = ArrayField(*this, Footprint, TEXT("rects_cm"));
		const TArray<TSharedPtr<FJsonValue>>* ZValues = ArrayField(*this, Footprint, TEXT("z_cm"));
		if (!RectValues || !ZValues || ZValues->Num() != 2)
		{
			continue;
		}
		const double ZMin = bIsPond
			? PondMinimumZ - FootprintToleranceCm
			: (*ZValues)[0]->AsNumber() - FootprintToleranceCm;
		const double ZMax = bIsPond
			? (bIsDeepPond ? DeepPondMaximumZ : ShallowPondMaximumZ) + FootprintToleranceCm
			: (*ZValues)[1]->AsNumber() + FootprintToleranceCm;
		int32 VertexIndex = 0;
		int32 HullIndex = 0;
		for (const FKConvexElem& Convex : Mesh->GetBodySetup()->AggGeom.ConvexElems)
		{
			TestTrue(FString::Printf(TEXT("%s hull %d has at least four source vertices"),
				*Id, HullIndex), Convex.VertexData.Num() >= 4);
			const FTransform ConvexTransform = Convex.GetTransform();
			FBox HullBounds(EForceInit::ForceInit);
			bool bHullInXY = true;
			for (const FVector& LocalVertex : Convex.VertexData)
			{
				const FVector Vertex = ConvexTransform.TransformPosition(LocalVertex);
				HullBounds += Vertex;
				bool bInXY = bIsPond
					? (bIsPondEdge
						? Vertex.X >= (*EdgeXY)[0]->AsNumber() - EdgeXTolerance
							&& Vertex.X <= (*EdgeXY)[1]->AsNumber() + EdgeXTolerance
							&& Vertex.Y >= (*EdgeXY)[2]->AsNumber() - FootprintToleranceCm
							&& Vertex.Y <= (*EdgeXY)[3]->AsNumber() + FootprintToleranceCm
						: Vertex.X >= (*CornerXY)[0]->AsNumber() - FootprintToleranceCm
							&& Vertex.X <= (*CornerXY)[1]->AsNumber() + FootprintToleranceCm
							&& Vertex.Y >= (*CornerXY)[2]->AsNumber() - FootprintToleranceCm
							&& Vertex.Y <= (*CornerXY)[3]->AsNumber() + FootprintToleranceCm)
					: false;
				for (const TSharedPtr<FJsonValue>& RectValue : *RectValues)
				{
					const TArray<TSharedPtr<FJsonValue>>& Rect = RectValue->AsArray();
					if (!bIsPond && Rect.Num() == 4
						&& Vertex.X >= Rect[0]->AsNumber() - FootprintToleranceCm
						&& Vertex.X <= Rect[1]->AsNumber() + FootprintToleranceCm
						&& Vertex.Y >= Rect[2]->AsNumber() - FootprintToleranceCm
						&& Vertex.Y <= Rect[3]->AsNumber() + FootprintToleranceCm)
					{
						bInXY = true;
						break;
					}
				}
				bHullInXY &= bInXY;
				TestTrue(FString::Printf(TEXT("%s collision vertex %d %s is inside the declared XY footprint"),
						*Mesh->GetName(), VertexIndex, *Vertex.ToString()), bInXY);
				TestTrue(FString::Printf(TEXT("%s collision vertex %d Z %.2f is inside %.2f..%.2f"),
						*Mesh->GetName(), VertexIndex, Vertex.Z, ZMin, ZMax),
					Vertex.Z >= ZMin && Vertex.Z <= ZMax);
				++VertexIndex;
			}
			if (bIsPond)
			{
				AddInfo(FString::Printf(
					TEXT("POND_HULL_EXTENT|%s|%d|%s|min=%.3f,%.3f,%.3f|max=%.3f,%.3f,%.3f"),
					*Id, HullIndex, *Convex.GetName().ToString(),
					HullBounds.Min.X, HullBounds.Min.Y, HullBounds.Min.Z,
					HullBounds.Max.X, HullBounds.Max.Y, HullBounds.Max.Z));
				TestTrue(FString::Printf(TEXT("%s hull %d stays inside revised pond XY"), *Id, HullIndex),
					bHullInXY);
				const bool bBlockingWall = bIsDeepPond
					&& HullBounds.Max.Z > ShallowPondMaximumZ + FootprintToleranceCm;
				if (bBlockingWall)
				{
					TestFalse(FString::Printf(
						TEXT("%s blocking-wall hull %d does not intrude into the land side"),
						*Id, HullIndex), ConvexOverlapsLandRegion(*this, Convex, LandRegion));
				}
				else if (!bIsDeepPond && HullBounds.Max.Z > ShallowPondMaximumZ + FootprintToleranceCm)
				{
					TestTrue(FString::Printf(
						TEXT("%s shallow hull %d above +15 cm is the land body"),
						*Id, HullIndex), ConvexOverlapsLandRegion(*this, Convex, LandRegion));
				}
			}
			++HullIndex;
		}
		TestTrue(FString::Printf(TEXT("%s checks at least one collision vertex"),
			*Mesh->GetName()), VertexIndex > 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_VW_MaterialsAndTexturesLoad,
	"Mordecai.World.Terrain.VerdantWilds.MaterialsAndTexturesLoad",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_VW_MaterialsAndTexturesLoad::RunTest(const FString& Parameters)
{
	using namespace MordecaiVerdantWildsTests;
	const TSharedPtr<FJsonObject> Manifest = LoadManifest(*this);
	const TArray<TSharedPtr<FJsonValue>>* Meshes = MeshEntries(*this, Manifest);
	const TArray<TSharedPtr<FJsonValue>>* Textures = ArrayField(*this, Manifest, TEXT("textures"));
	if (!Manifest || !Meshes || !Textures)
	{
		return false;
	}
	const FString MeshRoot = AssetRoot(*this, Manifest, TEXT("mesh_root"));
	const FString TextureRoot = AssetRoot(*this, Manifest, TEXT("texture_root"));
	const FString MaterialRoot = AssetRoot(*this, Manifest, TEXT("material_root"));
	UMaterial* Master = LoadAsset<UMaterial>(MasterMaterialPath);
	TestNotNull(FString::Printf(TEXT("%s loads as UMaterial"), MasterMaterialPath), Master);
	if (Master)
	{
		TestEqual(TEXT("terrain master is opaque"), Master->GetBlendMode(), BLEND_Opaque);
		TestTrue(TEXT("terrain master is Default Lit"),
			Master->GetShadingModels().HasShadingModel(MSM_DefaultLit));
		TestTrue(TEXT("terrain master explicitly supports Nanite"), Master->bUsedWithNanite);
	}

	const TMap<FString, TextureCompressionSettings> CompressionByName = {
		{ TEXT("Default"), TC_Default },
		{ TEXT("Normalmap"), TC_Normalmap },
		{ TEXT("Masks"), TC_Masks },
		{ TEXT("Grayscale"), TC_Grayscale },
	};
	for (const TSharedPtr<FJsonValue>& Value : *Textures)
	{
		const TSharedPtr<FJsonObject> Entry = Value->AsObject();
		const FString Id = StringField(Entry, TEXT("id"));
		const FString Path = TextureRoot / Id;
		const UTexture2D* Texture = LoadAsset<UTexture2D>(Path);
		if (!TestNotNull(FString::Printf(TEXT("%s loads as UTexture2D"), *Path), Texture))
		{
			continue;
		}
		TestEqual(FString::Printf(TEXT("%s sRGB matches the manifest"), *Id),
			Texture->SRGB, Entry->GetBoolField(TEXT("srgb")));
		const FString Compression = StringField(Entry, TEXT("compression"));
		const TextureCompressionSettings* Expected = CompressionByName.Find(Compression);
		TestNotNull(FString::Printf(TEXT("%s names a supported compression '%s'"), *Id, *Compression), Expected);
		if (Expected)
		{
			TestEqual(FString::Printf(TEXT("%s compression matches the manifest"), *Id),
				Texture->CompressionSettings, *Expected);
		}
		const FString LODGroup = StringField(Entry, TEXT("lod_group"));
		const TextureGroup ExpectedLODGroup =
			LODGroup == TEXT("WorldNormalMap") ? TEXTUREGROUP_WorldNormalMap : TEXTUREGROUP_World;
		TestEqual(FString::Printf(TEXT("%s LOD group matches role"), *Id),
			Texture->LODGroup, ExpectedLODGroup);
	}

	const TSharedPtr<FJsonObject> MaterialTextures =
		ObjectField(*this, Manifest, TEXT("material_textures"));
	TSet<FString> RequiredInstances;
	for (const TSharedPtr<FJsonValue>& Value : *Meshes)
	{
		const TSharedPtr<FJsonObject> Entry = Value->AsObject();
		UStaticMesh* Mesh = LoadMesh(*this, MeshRoot, Entry);
		const TArray<TSharedPtr<FJsonValue>>* Slots = ArrayField(*this, Entry, TEXT("material_slots"));
		if (!Mesh || !Slots)
		{
			continue;
		}
		const TArray<FStaticMaterial>& Actual = Mesh->GetStaticMaterials();
		TestEqual(FString::Printf(TEXT("%s has the manifest slot count"), *Mesh->GetName()),
			Actual.Num(), Slots->Num());
		for (int32 Index = 0; Index < Slots->Num() && Index < Actual.Num(); ++Index)
		{
			const FString Slot = (*Slots)[Index]->AsString();
			RequiredInstances.Add(Slot);
			TestEqual(FString::Printf(TEXT("%s slot %d keeps exact catalog name"), *Mesh->GetName(), Index),
				Actual[Index].MaterialSlotName.ToString(), Slot);
#if WITH_EDITORONLY_DATA
			TestEqual(FString::Printf(TEXT("%s slot %d keeps its imported FBX name"),
				*Mesh->GetName(), Index), Actual[Index].ImportedMaterialSlotName.ToString(), Slot);
#endif
			TestEqual(FString::Printf(TEXT("%s slot %d resolves to %s"), *Mesh->GetName(), Index, *Slot),
				GetPathNameSafe(Actual[Index].MaterialInterface),
				ObjectPath(MaterialRoot / Slot));
		}
		TSet<int32> UsedMaterialIndices;
		const FStaticMeshRenderData* RenderData = Mesh->GetRenderData();
		const FStaticMeshLODResources* LOD0 =
			RenderData && RenderData->LODResources.Num() > 0 ? &RenderData->LODResources[0] : nullptr;
		for (int32 SectionIndex = 0; LOD0 && SectionIndex < LOD0->Sections.Num(); ++SectionIndex)
		{
			UsedMaterialIndices.Add(LOD0->Sections[SectionIndex].MaterialIndex);
		}
		for (int32 MaterialIndex = 0; MaterialIndex < Actual.Num(); ++MaterialIndex)
		{
			TestTrue(FString::Printf(TEXT("%s material slot %d is used by a section"),
				*Mesh->GetName(), MaterialIndex), UsedMaterialIndices.Contains(MaterialIndex));
		}
	}
	for (const FString& Name : RequiredInstances)
	{
		const FString Path = MaterialRoot / Name;
		const UMaterialInstance* Instance = LoadAsset<UMaterialInstance>(Path);
		if (TestNotNull(FString::Printf(TEXT("%s loads as a material instance"), *Path), Instance))
		{
			TestEqual(FString::Printf(TEXT("%s uses the terrain master"), *Name),
				GetPathNameSafe(Instance->Parent), ObjectPath(MasterMaterialPath));
			const TSharedPtr<FJsonObject> TextureContract =
				ObjectField(*this, MaterialTextures, *Name);
			TestEqual(FString::Printf(TEXT("%s has exactly three sampled texture parameters"), *Name),
				TextureContract.IsValid() ? TextureContract->Values.Num() : 0, 3);
			if (!TextureContract.IsValid())
			{
				continue;
			}
			for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : TextureContract->Values)
			{
				UTexture* ActualTexture = nullptr;
				const bool bFound = Instance->GetTextureParameterValue(
					FHashedMaterialParameterInfo(FName(*Pair.Key)), ActualTexture, true);
				TestTrue(FString::Printf(TEXT("%s overrides texture parameter %s"),
					*Name, *Pair.Key), bFound);
				TestEqual(FString::Printf(TEXT("%s parameter %s uses the manifest texture"),
					*Name, *Pair.Key), GetPathNameSafe(ActualTexture),
					ObjectPath(TextureRoot / Pair.Value->AsString()));
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_VW_TileTestMapContract,
	"Mordecai.World.Terrain.VerdantWilds.TileTestMapContract",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_VW_TileTestMapContract::RunTest(const FString& Parameters)
{
	using namespace MordecaiVerdantWildsTests;
	const TSharedPtr<FJsonObject> Manifest = LoadManifest(*this);
	const TArray<TSharedPtr<FJsonValue>>* Entries = MeshEntries(*this, Manifest);
	UWorld* World = LoadTileMap(*this);
	if (!Manifest || !Entries || !World || !World->PersistentLevel)
	{
		return false;
	}
	const FString MeshRoot = AssetRoot(*this, Manifest, TEXT("mesh_root"));
	const AWorldSettings* Settings = World->PersistentLevel->GetWorldSettings(false);
	TestTrue(TEXT("tile map uses AMordecaiGameMode"),
		Settings && Settings->DefaultGameMode
		&& Settings->DefaultGameMode->IsChildOf(AMordecaiGameMode::StaticClass()));
	if (Settings)
	{
		TestNull(TEXT("tile map is not World Partition"), Settings->GetWorldPartition());
	}
#if WITH_EDITOR
	TestFalse(TEXT("tile map keeps actors in its one .umap"),
		World->PersistentLevel->IsUsingExternalActors());
#endif

	int32 Starts = 0;
	int32 BlockingVolumes = 0;
	for (AActor* Actor : LevelActors(World))
	{
		Starts += Actor->IsA<APlayerStart>() ? 1 : 0;
		BlockingVolumes += Actor->IsA<ABlockingVolume>() ? 1 : 0;
	}
	TestEqual(TEXT("tile map has exactly one PlayerStart"), Starts, 1);
	TestEqual(TEXT("tile map has no supplemental BlockingVolume"), BlockingVolumes, 0);

	for (const TSharedPtr<FJsonValue>& Value : *Entries)
	{
		const TSharedPtr<FJsonObject> Entry = Value->AsObject();
		const FString Id = StringField(Entry, TEXT("id"));
		const AActor* Actor = FindActorByLabel(World, Id);
		if (!TestNotNull(FString::Printf(TEXT("gallery has labelled actor %s"), *Id), Actor))
		{
			continue;
		}
		const UStaticMeshComponent* Component = Actor->FindComponentByClass<UStaticMeshComponent>();
		if (!TestNotNull(FString::Printf(TEXT("%s has a StaticMeshComponent"), *Id), Component))
		{
			continue;
		}
		TestEqual(FString::Printf(TEXT("%s uses its exact manifest mesh"), *Id),
			GetPathNameSafe(Component->GetStaticMesh()), ObjectPath(MeshRoot / Id));
		const FTransform Transform = SavedWorldTransform(Component);
		TestTrue(FString::Printf(TEXT("%s is unit scale"), *Id),
			Transform.GetScale3D().Equals(FVector::OneVector, KINDA_SMALL_NUMBER));
		const FVector Location = Transform.GetLocation();
		TestTrue(FString::Printf(TEXT("%s X %.2f is on the 200 cm grid"), *Id, Location.X),
			IsOnGrid(Location.X));
		TestTrue(FString::Printf(TEXT("%s Y %.2f is on the 200 cm grid"), *Id, Location.Y),
			IsOnGrid(Location.Y));
	}

	const TSharedPtr<FJsonObject> Fixture =
		ObjectField(*this, Manifest, TEXT("test_fixture"));
	const TCHAR* FixtureTables[] = {
		TEXT("gallery"), TEXT("lanes"), TEXT("shallow_bank_ring")
	};
	for (const TCHAR* TableName : FixtureTables)
	{
		const TArray<TSharedPtr<FJsonValue>>* Actors =
			ArrayField(*this, Fixture, TableName);
		if (!Actors)
		{
			continue;
		}
		for (const TSharedPtr<FJsonValue>& ActorValue : *Actors)
		{
			const TSharedPtr<FJsonObject> ExpectedActor = ActorValue->AsObject();
			const FString Label = StringField(ExpectedActor, TEXT("label"));
			const FString MeshId = StringField(ExpectedActor, TEXT("mesh"));
			const AActor* Actor = FindActorByLabel(World, Label);
			if (!TestNotNull(FString::Printf(TEXT("%s actor %s exists"), TableName, *Label), Actor))
			{
				continue;
			}
			const UStaticMeshComponent* Component =
				Actor->FindComponentByClass<UStaticMeshComponent>();
			if (!TestNotNull(FString::Printf(TEXT("%s has a StaticMeshComponent"), *Label), Component))
			{
				continue;
			}
			const FTransform Transform = SavedWorldTransform(Component);
			const FVector ExpectedLocation =
				VectorField(*this, ExpectedActor, TEXT("location_cm"));
			const FVector ExpectedScale =
				VectorField(*this, ExpectedActor, TEXT("scale"));
			const double ExpectedYaw = ExpectedActor->GetNumberField(TEXT("yaw_deg"));
			TestEqual(FString::Printf(TEXT("%s uses its exact shared-table mesh"), *Label),
				GetPathNameSafe(Component->GetStaticMesh()), ObjectPath(MeshRoot / MeshId));
			TestTrue(FString::Printf(TEXT("%s exact location %s"), *Label,
				*ExpectedLocation.ToString()),
				Transform.GetLocation().Equals(ExpectedLocation, GridToleranceCm));
			TestTrue(FString::Printf(TEXT("%s exact unit scale"), *Label),
				Transform.GetScale3D().Equals(ExpectedScale, KINDA_SMALL_NUMBER));
			TestTrue(FString::Printf(TEXT("%s exact yaw %.1f"), *Label, ExpectedYaw),
				FMath::IsNearlyEqual(
					FRotator::NormalizeAxis(Transform.Rotator().Yaw),
					FRotator::NormalizeAxis(ExpectedYaw), GridToleranceCm));
		}
	}

	const FBox CliffCollisionBand(
		FVector(1300.0, -100.0, 1.0),
		FVector(1500.0, 100.0, 250.0));
	for (AActor* Actor : LevelActors(World))
	{
		const UStaticMeshComponent* Component =
			Actor->FindComponentByClass<UStaticMeshComponent>();
		if (!Component || Component->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
		{
			continue;
		}
		const FBox Bounds = Component->CalcBounds(SavedWorldTransform(Component)).GetBox();
		const FString MeshPath = GetPathNameSafe(Component->GetStaticMesh());
		const bool bImportedTile = MeshPath.StartsWith(MeshRoot + TEXT("/"));
		TestFalse(FString::Printf(
			TEXT("non-tile collision actor %s does not enter the cliff collision band"),
			*Actor->GetActorNameOrLabel()),
			!bImportedTile && Bounds.Intersect(CliffCollisionBand));
	}

	TestNull(TEXT("tile map removes the old basin-covering monolithic floor"),
		FindActorByLabel(World, TEXT("VW_Floor")));

	struct FExpectedPondSurface
	{
		const TCHAR* Label;
		double TopZ;
		bool bNoCollision;
	};
	const FExpectedPondSurface ExpectedSurfaces[] = {
		{ TEXT("VW_Shallow_Bed"), -38.0, false },
		{ TEXT("VW_Shallow_Water"), -8.0, true },
		{ TEXT("VW_DeepEdge_Bed"), -141.0, false },
		{ TEXT("VW_DeepEdge_Water"), -8.0, true },
		{ TEXT("VW_DeepOuterCorner_Bed"), -141.0, false },
		{ TEXT("VW_DeepOuterCorner_Water"), -8.0, true },
		{ TEXT("VW_DeepInnerCorner_Bed"), -141.0, false },
		{ TEXT("VW_DeepInnerCorner_Water"), -8.0, true },
	};
	for (const FExpectedPondSurface& Expected : ExpectedSurfaces)
	{
		const AActor* Actor = FindActorByLabel(World, Expected.Label);
		if (!TestNotNull(FString::Printf(TEXT("tile map has pond surface %s"), Expected.Label), Actor))
		{
			continue;
		}
		const UStaticMeshComponent* Component = Actor->FindComponentByClass<UStaticMeshComponent>();
		if (!TestNotNull(FString::Printf(TEXT("%s has a StaticMeshComponent"), Expected.Label), Component))
		{
			continue;
		}
		const FBox Bounds = Component->CalcBounds(SavedWorldTransform(Component)).GetBox();
		TestTrue(FString::Printf(TEXT("%s top Z is %.0f cm"), Expected.Label, Expected.TopZ),
			FMath::IsNearlyEqual(Bounds.Max.Z, Expected.TopZ, GridToleranceCm));
		TestEqual(FString::Printf(TEXT("%s collision matches fixture contract"), Expected.Label),
			Component->GetCollisionEnabled(),
			Expected.bNoCollision ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	}

	const TCHAR* RequiredLandSupports[] = {
		TEXT("VW_DeepEdge_Land"),
		TEXT("VW_DeepOuterCorner_Land"),
		TEXT("VW_DeepInnerCorner_LandEast"),
		TEXT("VW_DeepInnerCorner_LandSouth"),
	};
	for (const TCHAR* Label : RequiredLandSupports)
	{
		TestNotNull(FString::Printf(TEXT("tile map has intended land support %s"), Label),
			FindActorByLabel(World, Label));
	}

	struct FPondHole
	{
		double MinX;
		double MaxX;
		double MinY;
		double MaxY;
	};
	const FPondHole PondHoles[] = {
		{ 2200.0, 2800.0, -300.0, 300.0 },
		{ 3500.0, 4100.0, -1300.0, -700.0 },
		{ 3500.0, 4100.0, -300.0, 300.0 },
		{ 3500.0, 4100.0, 700.0, 1300.0 },
	};
	int32 FloorSegmentCount = 0;
	for (AActor* Actor : LevelActors(World))
	{
		if (!Actor->GetActorNameOrLabel().StartsWith(TEXT("VW_Floor_")))
		{
			continue;
		}
		++FloorSegmentCount;
		const UStaticMeshComponent* Component = Actor->FindComponentByClass<UStaticMeshComponent>();
		if (!TestNotNull(FString::Printf(TEXT("%s has a StaticMeshComponent"),
			*Actor->GetActorNameOrLabel()), Component))
		{
			continue;
		}
		const FBox Bounds = Component->CalcBounds(SavedWorldTransform(Component)).GetBox();
		for (int32 HoleIndex = 0; HoleIndex < UE_ARRAY_COUNT(PondHoles); ++HoleIndex)
		{
			const FPondHole& Hole = PondHoles[HoleIndex];
			const double OverlapX = FMath::Min(Bounds.Max.X, Hole.MaxX)
				- FMath::Max(Bounds.Min.X, Hole.MinX);
			const double OverlapY = FMath::Min(Bounds.Max.Y, Hole.MaxY)
				- FMath::Max(Bounds.Min.Y, Hole.MinY);
			TestFalse(FString::Printf(TEXT("%s does not cover pond hole %d"),
				*Actor->GetActorNameOrLabel(), HoleIndex),
				OverlapX > GridToleranceCm && OverlapY > GridToleranceCm);
		}
	}
	TestTrue(TEXT("tile map floor is segmented around pond holes"), FloorSegmentCount > 1);
	return true;
}
