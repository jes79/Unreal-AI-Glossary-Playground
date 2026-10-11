#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "Engine/StaticMesh.h"
#include "GeometryCollection/GeometryCollection.h"
#include "GeometryCollection/GeometryCollectionClusteringUtility.h"
#include "GeometryCollection/GeometryCollectionEngineConversion.h"
#include "GeometryCollection/GeometryCollectionObject.h"
#include "GeometryCollection/GeometryCollectionUtility.h"
#include "GlossaryAssetKit.h"
#include "GlossaryKit.h"
#include "Materials/MaterialInstanceConstant.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PlanarCut.h"
#include "Voronoi/Voronoi.h"

DEFINE_LOG_CATEGORY_STATIC(LogGlossaryPhysicsAssets, Log, All);

namespace
{
	/** Physical Material 에셋 하나 (마찰·반발·결합 방식) */
	UPhysicalMaterial* BuildPhysMat(const FString& Path, float Friction, float Restitution, bool bFrictionMin, bool bRestitutionMax)
	{
		UPhysicalMaterial* PM = GlossaryAssetKit::CreateOrLoad<UPhysicalMaterial>(Path);
		if (!PM)
		{
			return nullptr;
		}
		PM->Friction = Friction;
		PM->StaticFriction = Friction;
		PM->Restitution = Restitution;
		// 두 표면의 값을 어떻게 합칠지: 얼음은 Min(어느 바닥에서든 미끄럽게), 고무공은 Max(어느 바닥에서든 튀게)
		PM->bOverrideFrictionCombineMode = bFrictionMin;
		PM->FrictionCombineMode = bFrictionMin ? EFrictionCombineMode::Min : EFrictionCombineMode::Average;
		PM->bOverrideRestitutionCombineMode = bRestitutionMax;
		PM->RestitutionCombineMode = bRestitutionMax ? EFrictionCombineMode::Max : EFrictionCombineMode::Average;
		GlossaryAssetKit::SaveAsset(PM);
		return PM;
	}

	/**
	 * 0211: Fracture 모드의 "New → Voronoi 20조각" 을 코드로 재현한다.
	 * 1) 큐브 스태틱 메시를 벽 크기(400×40×300)로 Geometry Collection 에 넣고
	 * 2) 벽 안에 무작위 사이트 20개로 Voronoi 다이어그램 → PlanarCells 로 자른다.
	 */
	void BuildFracturedWall(const FString& Path, int32 NumPieces, int32 Seed)
	{
		UGeometryCollection* GC = GlossaryAssetKit::CreateOrLoad<UGeometryCollection>(Path);
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (!GC || !Cube)
		{
			UE_LOG(LogGlossaryPhysicsAssets, Error, TEXT("[Glossary] GC_Wall 생성 실패 (GC=%p Cube=%p)"), GC, Cube);
			return;
		}
		GC->Reset();
		GC->EnableNanite = false;

		UMaterialInstanceConstant* WallMat = GlossaryAssetKit::CreateMaterialInstance(TEXT("/Game/Glossary/02_Physics/MI_DestructibleWall"),
			GlossaryKit::GetColorMaterial(), {}, {{TEXT("Color"), FLinearColor(0.75f, 0.42f, 0.25f)}});
		TArray<UMaterialInterface*> Materials;
		Materials.Add(WallMat ? static_cast<UMaterialInterface*>(WallMat) : GlossaryKit::GetColorMaterial());

		// 바닥 중심이 원점: 높이 300 → 중심 z=150. 엔진 큐브는 100cm 라서 스케일 4 × 0.4 × 3
		const FVector WallSize(400.f, 40.f, 300.f);
		const FTransform MeshTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, WallSize.Z * 0.5f), WallSize / 100.f);
		FGeometryCollectionEngineConversion::AppendStaticMesh(Cube, Materials, MeshTransform, GC,
			/*bReindexMaterials*/ false, /*bAddInternalMaterials*/ false, /*bSplitComponents*/ false, /*bSetInternalFromMaterialIndex*/ false);
		GC->InitializeMaterials(false);

		FGeometryCollection* Collection = GC->GetGeometryCollection().Get();
		if (!Collection || Collection->NumElements(FGeometryCollection::TransformGroup) == 0)
		{
			UE_LOG(LogGlossaryPhysicsAssets, Error, TEXT("[Glossary] GC_Wall: 메시를 Geometry Collection 으로 변환하지 못함"));
			return;
		}
		if (FGeometryCollectionClusteringUtility::ContainsMultipleRootBones(Collection))
		{
			FGeometryCollectionClusteringUtility::ClusterAllBonesUnderNewRoot(Collection);
		}

		// Voronoi 사이트: 벽 부피 안의 무작위 점 NumPieces 개 (시드 고정 → 매번 같은 조각)
		const FBox Bounds(FVector(-WallSize.X * 0.5f, -WallSize.Y * 0.5f, 0.f), FVector(WallSize.X * 0.5f, WallSize.Y * 0.5f, WallSize.Z));
		FRandomStream Random(Seed);
		TArray<FVector> Sites;
		for (int32 i = 0; i < NumPieces; ++i)
		{
			Sites.Add(FVector(Random.FRandRange(Bounds.Min.X, Bounds.Max.X), Random.FRandRange(Bounds.Min.Y, Bounds.Max.Y),
				Random.FRandRange(Bounds.Min.Z, Bounds.Max.Z)));
		}
		FVoronoiDiagram Voronoi(Sites, Bounds.ExpandBy(10.f), .1f);
		FPlanarCells Cells(Sites, Voronoi);

		// 잘 대상: 지오메트리가 있는 뼈(처음엔 큐브 하나)
		TArray<int32> Selection;
		const TManagedArray<int32>& TransformToGeometry = Collection->TransformToGeometryIndex;
		for (int32 Idx = 0; Idx < TransformToGeometry.Num(); ++Idx)
		{
			if (TransformToGeometry[Idx] != INDEX_NONE)
			{
				Selection.Add(Idx);
			}
		}
		const int32 FirstNew = CutMultipleWithPlanarCells(Cells, *Collection, Selection, /*Grout*/ 0.0, /*CollisionSampleSpacing*/ 50.0, Seed);

		FGeometryCollectionClusteringUtility::UpdateHierarchyLevelOfChildren(Collection, -1);
		::GeometryCollection::GenerateTemporaryGuids(Collection, 0, true);
		GC->InvalidateCollection();
		GC->RebuildRenderData();
		GC->CreateSimulationData();

		const int32 NumTransforms = Collection->NumElements(FGeometryCollection::TransformGroup);
		UE_LOG(LogGlossaryPhysicsAssets, Display, TEXT("[Glossary] GC_Wall: Voronoi %d 사이트 → 뼈 %d개 (첫 새 지오메트리 %d)"), NumPieces, NumTransforms, FirstNew);
		GlossaryAssetKit::SaveAsset(GC);
	}
}

/** 02_Physics 분야 데모가 쓰는 생성 에셋 (/Game/Glossary/02_Physics) */
static void BuildGlossaryPhysicsAssets()
{
	// 0206 Physical Material: 얼음(Friction 0.05) · 고무공(Restitution 0.9) · 비교용 기본
	BuildPhysMat(TEXT("/Game/Glossary/02_Physics/PM_Ice"), 0.05f, 0.1f, true, false);
	BuildPhysMat(TEXT("/Game/Glossary/02_Physics/PM_Rubber"), 0.8f, 0.9f, false, true);
	BuildPhysMat(TEXT("/Game/Glossary/02_Physics/PM_Default"), 0.7f, 0.1f, false, false);

	// 0211 Chaos Destruction: Voronoi 20조각 벽
	BuildFracturedWall(TEXT("/Game/Glossary/02_Physics/GC_Wall"), 20, 7);
}
#endif

class FGlossaryPhysicsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::RegisterBuilder(TEXT("02_Physics"), &BuildGlossaryPhysicsAssets);
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::UnregisterBuilder(TEXT("02_Physics"));
#endif
	}
};

IMPLEMENT_MODULE(FGlossaryPhysicsModule, GlossaryPhysics);
