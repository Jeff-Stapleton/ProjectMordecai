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
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Mordecai/MordecaiGameMode.h"
#include "PhysicsEngine/AggregateGeom.h"
#include "PhysicsEngine/BodySetup.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/SoftObjectPath.h"

namespace MordecaiVerdantWildsTests
{
	const TCHAR* const ManifestRelativePath = TEXT("Scripts/us120_vw_source_manifest.json");
	const TCHAR* const MapPackagePath = TEXT("/MordecaiCore/Game/Maps/L_VW_TileTest");
	const TCHAR* const MasterMaterialPath =
		TEXT("/MordecaiCore/World/Terrain/Shared/Materials/M_Terrain_Master");
	constexpr double BoundsToleranceCm = 1.0;
	constexpr double FootprintToleranceCm = 0.1;
	constexpr double HullExtentToleranceCm = 0.01;
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
		const FBox Expected(VectorField(*this, BoundsObject, TEXT("min")),
			VectorField(*this, BoundsObject, TEXT("max")));
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
	const TSharedPtr<FJsonObject> PondHullExtents =
		ObjectField(*this, PondContract, TEXT("actual_hull_extents_cm"));
	const TArray<TSharedPtr<FJsonValue>>* EdgeXY =
		ArrayField(*this, PondContract, TEXT("edge_xy_cm"));
	const TArray<TSharedPtr<FJsonValue>>* CornerXY =
		ArrayField(*this, PondContract, TEXT("corner_xy_cm"));
	if (!PondContract || !PondHullExtents || !EdgeXY || EdgeXY->Num() != 4
		|| !CornerXY || CornerXY->Num() != 4)
	{
		return false;
	}
	const double EdgeXTolerance = PondContract->GetNumberField(TEXT("edge_x_tolerance_cm"));
	const double PondMinimumZ = PondContract->GetNumberField(TEXT("min_z_cm"));
	const double ShallowPondMaximumZ = PondContract->GetNumberField(TEXT("shallow_max_z_cm"));
	const double DeepPondMaximumZ = PondContract->GetNumberField(TEXT("deep_wall_max_z_cm"));
	const double LandIntrusionLimit = PondContract->GetNumberField(TEXT("land_intrusion_limit_cm"));
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
		const TArray<TSharedPtr<FJsonValue>>* ExpectedHullValues = nullptr;
		if (bIsPond)
		{
			TestTrue(FString::Printf(TEXT("manifest records actual hull extents for %s"), *Id),
				PondHullExtents->TryGetArrayField(Id, ExpectedHullValues));
			if (!ExpectedHullValues)
			{
				continue;
			}
			TestEqual(FString::Printf(TEXT("%s extent count matches UCX hull count"), *Id),
				ExpectedHullValues->Num(), Mesh->GetBodySetup()->AggGeom.ConvexElems.Num());
		}
		int32 VertexIndex = 0;
		int32 HullIndex = 0;
		for (const FKConvexElem& Convex : Mesh->GetBodySetup()->AggGeom.ConvexElems)
		{
			const FTransform ConvexTransform = Convex.GetTransform();
			FBox HullBounds(EForceInit::ForceInit);
			bool bHullInXY = true;
			bool bDeepWallIntrudesLand = false;
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
				if (bIsDeepPond)
				{
					bDeepWallIntrudesLand |= bIsPondEdge
						? Vertex.Y > LandIntrusionLimit + FootprintToleranceCm
						: Vertex.X > LandIntrusionLimit + FootprintToleranceCm
							&& Vertex.Y > LandIntrusionLimit + FootprintToleranceCm;
				}
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
				if (ExpectedHullValues && ExpectedHullValues->IsValidIndex(HullIndex))
				{
					const TSharedPtr<FJsonObject> ExpectedHull = (*ExpectedHullValues)[HullIndex]->AsObject();
					const FVector ExpectedMin = VectorField(*this, ExpectedHull, TEXT("min"));
					const FVector ExpectedMax = VectorField(*this, ExpectedHull, TEXT("max"));
					TestTrue(FString::Printf(TEXT("%s hull %d measured min extent matches report"), *Id, HullIndex),
						HullBounds.Min.Equals(ExpectedMin, HullExtentToleranceCm));
					TestTrue(FString::Printf(TEXT("%s hull %d measured max extent matches report"), *Id, HullIndex),
						HullBounds.Max.Equals(ExpectedMax, HullExtentToleranceCm));
				}
				const bool bBlockingWall = bIsDeepPond
					&& HullBounds.Max.Z > ShallowPondMaximumZ + FootprintToleranceCm;
				if (bBlockingWall)
				{
					TestFalse(FString::Printf(
						TEXT("%s blocking-wall hull %d does not intrude into the land side"),
						*Id, HullIndex), bDeepWallIntrudesLand);
				}
				else if (!bIsDeepPond && HullBounds.Max.Z > ShallowPondMaximumZ + FootprintToleranceCm)
				{
					const bool bLandBody = bIsPondEdge
						? HullBounds.Max.Y > LandIntrusionLimit
						: HullBounds.Max.X > LandIntrusionLimit
							&& HullBounds.Max.Y > LandIntrusionLimit;
					TestTrue(FString::Printf(
						TEXT("%s shallow hull %d above +15 cm is the land body"),
						*Id, HullIndex), bLandBody);
				}
			}
			++HullIndex;
		}

		const double Overhang = Footprint->GetNumberField(TEXT("visual_overhang_downhill_cm"));
		const double Limit = Footprint->GetNumberField(TEXT("visual_overhang_limit_cm"));
		if (Overhang > Limit)
		{
			AddWarning(FString::Printf(
				TEXT("KNOWN ART DEFECT: %s visual overhang into local -Y is %.1f cm (> %.1f cm); collision remains independently checked"),
				*Mesh->GetName(), Overhang, Limit));
		}
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
	}

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
			TestEqual(FString::Printf(TEXT("%s slot %d resolves to %s"), *Mesh->GetName(), Index, *Slot),
				GetPathNameSafe(Actual[Index].MaterialInterface),
				ObjectPath(MaterialRoot / Slot));
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

	const TCHAR* RequiredLaneLabels[] = {
		TEXT("VW_Lane_Ramp2T"),
		TEXT("VW_Lane_Ramp3T"),
		TEXT("VW_Lane_Cliff"),
		TEXT("VW_Lane_ShallowPond"),
		TEXT("VW_Lane_DeepEdge"),
		TEXT("VW_Lane_DeepOuterCorner"),
		TEXT("VW_Lane_DeepInnerCorner"),
	};
	for (const TCHAR* Label : RequiredLaneLabels)
	{
		TestNotNull(FString::Printf(TEXT("tile map has dedicated lane actor %s"), Label),
			FindActorByLabel(World, Label));
	}
	return true;
}
