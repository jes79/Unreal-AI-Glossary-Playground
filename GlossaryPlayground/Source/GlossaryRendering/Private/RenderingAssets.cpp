// 10_Rendering 에셋 빌더 (에디터 전용).
// 데모가 쓰는 메시(Nanite 바위·LOD 바위·나무·HLOD 프록시), Light Function 머티리얼, PCG 그래프를 코드로 만든다.
#if WITH_EDITOR

#include "RenderingAssets.h"

#include "Engine/StaticMesh.h"
#include "GlossaryAssetKit.h"
#include "GlossaryKit.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialInstanceConstant.h"
#include "MeshDescription.h"
#include "PCGGraph.h"
#include "PCGNode.h"
#include "PCGPin.h"
#include "PCGCommon.h"
#include "Elements/PCGBoundsModifier.h"
#include "Elements/PCGCreateSurfaceFromSpline.h"
#include "Elements/PCGDifferenceElement.h"
#include "Elements/PCGSplineSampler.h"
#include "Elements/PCGStaticMeshSpawner.h"
#include "Elements/PCGSurfaceSampler.h"
#include "Elements/PCGTransformPoints.h"
#include "Elements/PCGTypedGetter.h"
#include "MeshSelectors/PCGMeshSelectorWeighted.h"
#include "RenderingAssetPaths.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshCompiler.h"

DEFINE_LOG_CATEGORY_STATIC(LogGlossaryRendering, Log, All);

namespace
{
	// ------------------------------------------------------------------ 간단한 메시 조립기
	/** 삼각형 목록 메시. 정점은 공유(부드러운 면) 또는 면마다 따로(각진 면) 넣는다 */
	struct FSimpleMesh
	{
		TArray<FVector3f> Positions;
		TArray<FVector3f> Normals;
		TArray<FVector2f> UVs;
		TArray<int32> Indices;    // 3개씩 삼각형. 규칙: cross(B-A, C-A) 가 바깥을 향한다 (ToMeshDescription 이 언리얼 순서로 바꿈)
		TArray<int32> TriGroup;   // 삼각형마다 머티리얼 슬롯 번호

		int32 AddVertex(const FVector3f& P, const FVector3f& N, const FVector2f& UV)
		{
			Positions.Add(P);
			Normals.Add(N);
			UVs.Add(UV);
			return Positions.Num() - 1;
		}
		void AddTri(int32 A, int32 B, int32 C, int32 Group = 0)
		{
			Indices.Append({A, B, C});
			TriGroup.Add(Group);
		}
		/** 사각형 하나(각진 면). Corner 는 바깥에서 봤을 때 반시계 순서 */
		void AddQuad(const FVector3f& P0, const FVector3f& P1, const FVector3f& P2, const FVector3f& P3, int32 Group = 0)
		{
			const FVector3f N = FVector3f::CrossProduct(P1 - P0, P3 - P0).GetSafeNormal();
			const int32 A = AddVertex(P0, N, FVector2f(0, 0));
			const int32 B = AddVertex(P1, N, FVector2f(1, 0));
			const int32 C = AddVertex(P2, N, FVector2f(1, 1));
			const int32 D = AddVertex(P3, N, FVector2f(0, 1));
			AddTri(A, B, C, Group);
			AddTri(A, C, D, Group);
		}
		/** 바닥 중심 Base, 크기 Size 상자 */
		void AddBox(const FVector3f& Base, const FVector3f& Size, int32 Group = 0)
		{
			const FVector3f H(Size.X * 0.5f, Size.Y * 0.5f, 0.f);
			const FVector3f Lo = Base - H;
			const FVector3f Hi = Base + H + FVector3f(0, 0, Size.Z);
			auto P = [&](int32 X, int32 Y, int32 Z) { return FVector3f(X ? Hi.X : Lo.X, Y ? Hi.Y : Lo.Y, Z ? Hi.Z : Lo.Z); };
			AddQuad(P(1, 0, 0), P(1, 1, 0), P(1, 1, 1), P(1, 0, 1), Group); // +X
			AddQuad(P(0, 1, 0), P(0, 0, 0), P(0, 0, 1), P(0, 1, 1), Group); // -X
			AddQuad(P(1, 1, 0), P(0, 1, 0), P(0, 1, 1), P(1, 1, 1), Group); // +Y
			AddQuad(P(0, 0, 0), P(1, 0, 0), P(1, 0, 1), P(0, 0, 1), Group); // -Y
			AddQuad(P(0, 0, 1), P(1, 0, 1), P(1, 1, 1), P(0, 1, 1), Group); // +Z
			AddQuad(P(0, 1, 0), P(1, 1, 0), P(1, 0, 0), P(0, 0, 0), Group); // -Z
		}
		/** 원뿔대(아래 반지름 R0, 위 반지름 R1). R1=0 이면 원뿔 */
		void AddFrustum(const FVector3f& Base, float R0, float R1, float Height, int32 Sides, int32 Group = 0)
		{
			const float Slope = (R0 - R1) / Height;
			for (int32 i = 0; i < Sides; ++i)
			{
				const float A0 = 2.f * PI * i / Sides, A1 = 2.f * PI * (i + 1) / Sides;
				const FVector3f D0(FMath::Cos(A0), FMath::Sin(A0), 0.f), D1(FMath::Cos(A1), FMath::Sin(A1), 0.f);
				const FVector3f N0 = (D0 + FVector3f(0, 0, Slope)).GetSafeNormal(), N1 = (D1 + FVector3f(0, 0, Slope)).GetSafeNormal();
				const int32 B0 = AddVertex(Base + D0 * R0, N0, FVector2f(float(i) / Sides, 1));
				const int32 B1 = AddVertex(Base + D1 * R0, N1, FVector2f(float(i + 1) / Sides, 1));
				const int32 T0 = AddVertex(Base + D0 * R1 + FVector3f(0, 0, Height), N0, FVector2f(float(i) / Sides, 0));
				const int32 T1 = AddVertex(Base + D1 * R1 + FVector3f(0, 0, Height), N1, FVector2f(float(i + 1) / Sides, 0));
				AddTri(B0, T1, T0, Group);
				AddTri(B0, B1, T1, Group);
				// 바닥 뚜껑
				const int32 C = AddVertex(Base, FVector3f(0, 0, -1), FVector2f(0.5f, 0.5f));
				const int32 C0 = AddVertex(Base + D0 * R0, FVector3f(0, 0, -1), FVector2f(0, 0));
				const int32 C1 = AddVertex(Base + D1 * R0, FVector3f(0, 0, -1), FVector2f(1, 0));
				AddTri(C, C1, C0, Group);
			}
		}
		/** 정이십면체를 Subdiv 번 쪼갠 구. Displace(방향) 로 반지름을 바꿔 바위 모양을 만든다 */
		void AddIcoSphere(float Radius, int32 Subdiv, TFunctionRef<float(const FVector3f&)> Displace, int32 Group = 0)
		{
			const float T = (1.f + FMath::Sqrt(5.f)) * 0.5f;
			TArray<FVector3f> V = {
				{-1, T, 0}, {1, T, 0}, {-1, -T, 0}, {1, -T, 0}, {0, -1, T}, {0, 1, T},
				{0, -1, -T}, {0, 1, -T}, {T, 0, -1}, {T, 0, 1}, {-T, 0, -1}, {-T, 0, 1}};
			for (FVector3f& P : V) { P.Normalize(); }
			TArray<FIntVector> F = {
				{0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11}, {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
				{3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9}, {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}};
			for (int32 Level = 0; Level < Subdiv; ++Level)
			{
				TMap<uint64, int32> Mid;
				auto GetMid = [&](int32 A, int32 B)
				{
					const uint64 Key = (uint64(FMath::Min(A, B)) << 32) | uint64(FMath::Max(A, B));
					if (const int32* Found = Mid.Find(Key)) { return *Found; }
					const int32 Index = V.Add(((V[A] + V[B]) * 0.5f).GetSafeNormal());
					Mid.Add(Key, Index);
					return Index;
				};
				TArray<FIntVector> NF;
				NF.Reserve(F.Num() * 4);
				for (const FIntVector& Tri : F)
				{
					const int32 A = GetMid(Tri.X, Tri.Y), B = GetMid(Tri.Y, Tri.Z), C = GetMid(Tri.Z, Tri.X);
					NF.Add({Tri.X, A, C}); NF.Add({Tri.Y, B, A}); NF.Add({Tri.Z, C, B}); NF.Add({A, B, C});
				}
				F = MoveTemp(NF);
			}
			// 변위 후 면 법선을 정점에 모아 부드러운 법선을 만든다
			const int32 Start = Positions.Num();
			TArray<FVector3f> Acc;
			Acc.SetNumZeroed(V.Num());
			TArray<FVector3f> P;
			P.SetNumUninitialized(V.Num());
			for (int32 i = 0; i < V.Num(); ++i) { P[i] = V[i] * Radius * Displace(V[i]); }
			for (const FIntVector& Tri : F)
			{
				const FVector3f N = FVector3f::CrossProduct(P[Tri.Y] - P[Tri.X], P[Tri.Z] - P[Tri.X]);
				Acc[Tri.X] += N; Acc[Tri.Y] += N; Acc[Tri.Z] += N;
			}
			for (int32 i = 0; i < V.Num(); ++i)
			{
				// 바깥 방향(V) 쪽으로 법선 부호를 맞춘다
				FVector3f N = Acc[i].GetSafeNormal();
				if (FVector3f::DotProduct(N, V[i]) < 0.f) { N = -N; }
				AddVertex(P[i], N, FVector2f(FMath::Atan2(V[i].Y, V[i].X) / (2.f * PI) + 0.5f, V[i].Z * 0.5f + 0.5f));
			}
			for (const FIntVector& Tri : F)
			{
				AddTri(Start + Tri.X, Start + Tri.Y, Start + Tri.Z, Group);
			}
		}
	};

	FMeshDescription ToMeshDescription(const FSimpleMesh& Mesh, int32 NumGroups)
	{
		FMeshDescription MD;
		FStaticMeshAttributes Attr(MD);
		Attr.Register();
		TVertexAttributesRef<FVector3f> Pos = Attr.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> Nrm = Attr.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector2f> UV = Attr.GetVertexInstanceUVs();
		TPolygonGroupAttributesRef<FName> Slots = Attr.GetPolygonGroupMaterialSlotNames();

		TArray<FPolygonGroupID> Groups;
		for (int32 g = 0; g < NumGroups; ++g)
		{
			const FPolygonGroupID G = MD.CreatePolygonGroup();
			Slots[G] = FName(*FString::Printf(TEXT("Slot%d"), g));
			Groups.Add(G);
		}
		MD.ReserveNewVertices(Mesh.Positions.Num());
		MD.ReserveNewVertexInstances(Mesh.Positions.Num());
		MD.ReserveNewTriangles(Mesh.TriGroup.Num());
		TArray<FVertexInstanceID> Instances;
		Instances.Reserve(Mesh.Positions.Num());
		for (int32 i = 0; i < Mesh.Positions.Num(); ++i)
		{
			const FVertexID V = MD.CreateVertex();
			Pos[V] = Mesh.Positions[i];
			const FVertexInstanceID VI = MD.CreateVertexInstance(V);
			Nrm[VI] = Mesh.Normals[i];
			UV[VI] = Mesh.UVs[i];
			Instances.Add(VI);
		}
		for (int32 t = 0; t < Mesh.TriGroup.Num(); ++t)
		{
			// 언리얼은 왼손 좌표계 + 반시계 감김이라 앞면 법선이 cross(C-A, B-A) 다 → B 와 C 를 바꿔 넣는다
			const FVertexInstanceID Tri[3] = {Instances[Mesh.Indices[t * 3]], Instances[Mesh.Indices[t * 3 + 2]], Instances[Mesh.Indices[t * 3 + 1]]};
			MD.CreateTriangle(Groups[Mesh.TriGroup[t]], Tri);
		}
		return MD;
	}

	struct FMeshBuildOptions
	{
		bool bNanite = false;
		/** LOD1~ 의 남길 삼각형 비율 (예: 0.5, 0.25, 0.1) */
		TArray<float> LODPercents;
		/** LOD0~ 의 Screen Size */
		TArray<float> LODScreenSizes;
	};

	UStaticMesh* BuildStaticMesh(const FString& Path, const FSimpleMesh& Mesh, const TArray<UMaterialInterface*>& Materials, const FMeshBuildOptions& Options)
	{
		UStaticMesh* SM = GlossaryAssetKit::CreateOrLoad<UStaticMesh>(Path);
		SM->PreEditChange(nullptr);

		SM->GetStaticMaterials().Reset();
		for (int32 g = 0; g < Materials.Num(); ++g)
		{
			const FName Slot(*FString::Printf(TEXT("Slot%d"), g));
			SM->GetStaticMaterials().Add(FStaticMaterial(Materials[g], Slot, Slot));
		}

		const int32 NumLODs = 1 + Options.LODPercents.Num();
		SM->SetNumSourceModels(NumLODs);
		for (int32 i = 0; i < NumLODs; ++i)
		{
			FStaticMeshSourceModel& Src = SM->GetSourceModel(i);
			Src.BuildSettings.bRecomputeNormals = false;   // 직접 넣은 법선을 쓴다
			Src.BuildSettings.bRecomputeTangents = true;
			Src.BuildSettings.bGenerateLightmapUVs = true;
			Src.BuildSettings.bBuildReversedIndexBuffer = false;
			if (i > 0)
			{
				// LOD1~ 은 LOD0 을 자동 축소(Reduction)해서 만든다
				Src.ReductionSettings.PercentTriangles = Options.LODPercents[i - 1];
				Src.ReductionSettings.BaseLODModel = 0;
			}
			if (Options.LODScreenSizes.IsValidIndex(i))
			{
				Src.ScreenSize.Default = Options.LODScreenSizes[i];
			}
		}
		SM->SetAutoComputeLODScreenSize(Options.LODScreenSizes.Num() == 0);

		SM->CreateMeshDescription(0, ToMeshDescription(Mesh, FMath::Max(1, Materials.Num())));
		SM->CommitMeshDescription(0);
		for (int32 i = 1; i < NumLODs; ++i)
		{
			// 축소 LOD 는 원본 메시 설명이 없어야 Reduction 으로 생성된다
			SM->ClearMeshDescription(i);
		}

		FMeshNaniteSettings& Nanite = SM->GetNaniteSettings();
		Nanite.bEnabled = Options.bNanite;

		SM->SetLightMapCoordinateIndex(1);
		SM->SetLightMapResolution(64);
		SM->Build(true);
		SM->PostEditChange();
		FStaticMeshCompilingManager::Get().FinishCompilation({SM});

		UE_LOG(LogGlossaryRendering, Display, TEXT("[Glossary] 메시 %s: LOD %d, LOD0 삼각형 %d, Nanite %s"),
			*Path, SM->GetNumLODs(), SM->GetNumTriangles(0), Options.bNanite ? TEXT("켬") : TEXT("끔"));
		GlossaryAssetKit::SaveAsset(SM);
		return SM;
	}

	/** 바위 모양 변위: 저주파 덩어리 + 고주파 거친 표면 */
	float RockDisplace(const FVector3f& Dir, float Seed)
	{
		const FVector D(Dir);
		float H = 1.f;
		H += 0.25f * FMath::PerlinNoise3D(D * 1.3f + FVector(Seed));
		H += 0.10f * FMath::PerlinNoise3D(D * 4.f + FVector(Seed * 2.f));
		H += 0.04f * FMath::PerlinNoise3D(D * 12.f + FVector(Seed * 3.f));
		H += 0.015f * FMath::PerlinNoise3D(D * 40.f);
		// 바닥은 납작하게
		if (Dir.Z < -0.3f) { H *= FMath::Lerp(1.f, 0.7f, (-Dir.Z - 0.3f) / 0.7f); }
		return H;
	}

	void BuildMeshes()
	{
		using namespace GlossaryAssetKit;
		UMaterialInterface* ColorMat = GlossaryKit::GetColorMaterial();
		UMaterialInstanceConstant* RockMat = CreateMaterialInstance(RenderingAssets::RockMaterial, ColorMat,
			{{TEXT("Roughness"), 0.85f}}, {{TEXT("Color"), FLinearColor(0.42f, 0.38f, 0.33f)}});
		// 나무는 HISM·PCG(인스턴스)로도 그려지므로 Used with Instanced Static Meshes 를 켠 전용 머티리얼을 쓴다
		UMaterial* Foliage = CreateMaterial(TEXT("/Game/Glossary/10_Rendering/M_Foliage"));
		Foliage->bUsedWithInstancedStaticMeshes = true;
		{
			auto* Color = AddVectorParam(Foliage, TEXT("Color"), FLinearColor(0.1f, 0.4f, 0.1f));
			auto* Rough = AddScalarParam(Foliage, TEXT("Roughness"), 0.8f, -600, 200);
			ConnectToPin(Color, MP_BaseColor);
			ConnectToPin(Rough, MP_Roughness);
			FinishMaterial(Foliage);
		}
		UMaterialInstanceConstant* Trunk = CreateMaterialInstance(TEXT("/Game/Glossary/10_Rendering/MI_TreeTrunk"), Foliage,
			{{TEXT("Roughness"), 0.9f}}, {{TEXT("Color"), FLinearColor(0.25f, 0.13f, 0.06f)}});
		UMaterialInstanceConstant* Leaves = CreateMaterialInstance(TEXT("/Game/Glossary/10_Rendering/MI_TreeLeaves"), Foliage,
			{{TEXT("Roughness"), 0.8f}}, {{TEXT("Color"), FLinearColor(0.08f, 0.35f, 0.07f)}});
		UMaterialInstanceConstant* Proxy = CreateMaterialInstance(TEXT("/Game/Glossary/10_Rendering/MI_HLODProxy"), ColorMat,
			{{TEXT("Roughness"), 0.7f}}, {{TEXT("Color"), FLinearColor(0.75f, 0.55f, 0.35f)}});

		// 1003 Nanite 바위: 이십면체 7번 분할 = 327,680 삼각형
		{
			FSimpleMesh Rock;
			Rock.AddIcoSphere(150.f, 7, [](const FVector3f& D) { return RockDisplace(D, 0.37f); });
			FMeshBuildOptions Opt;
			Opt.bNanite = true;
			BuildStaticMesh(RenderingAssets::NaniteRock, Rock, {RockMat}, Opt);
			Opt.bNanite = false;
			BuildStaticMesh(RenderingAssets::NaniteRockOff, Rock, {RockMat}, Opt);
		}
		// 1010 LOD 바위: 5번 분할 = 20,480 삼각형, LOD 3단계 자동 생성
		{
			FSimpleMesh Rock;
			Rock.AddIcoSphere(100.f, 5, [](const FVector3f& D) { return RockDisplace(D, 1.7f); });
			FMeshBuildOptions Opt;
			Opt.LODPercents = {0.5f, 0.25f, 0.1f};
			Opt.LODScreenSizes = {1.0f, 0.5f, 0.25f, 0.1f};
			BuildStaticMesh(RenderingAssets::LODRock, Rock, {RockMat}, Opt);
		}
		// 나무: 줄기(슬롯0) + 원뿔 잎 2단(슬롯1)
		{
			FSimpleMesh Tree;
			Tree.AddFrustum(FVector3f(0, 0, 0), 18.f, 12.f, 220.f, 8, 0);
			Tree.AddFrustum(FVector3f(0, 0, 150), 130.f, 0.f, 260.f, 10, 1);
			Tree.AddFrustum(FVector3f(0, 0, 300), 95.f, 0.f, 220.f, 10, 1);
			BuildStaticMesh(RenderingAssets::Tree, Tree, {Trunk, Leaves}, FMeshBuildOptions());
		}
		// 1011 HLOD 프록시: 마을 건물 25채를 지붕·창 없이 상자 하나씩으로 단순화해 한 메시·한 머티리얼로 합친다
		{
			FSimpleMesh Town;
			for (const RenderingAssets::FTownBuilding& B : RenderingAssets::GetTownBuildings())
			{
				Town.AddBox(FVector3f(B.Base), FVector3f(B.Size));
			}
			BuildStaticMesh(RenderingAssets::TownProxy, Town, {Proxy}, FMeshBuildOptions());
		}
	}

	void BuildLightFunction()
	{
		using namespace GlossaryAssetKit;
		// 1008: Light Function 은 MD_LightFunction 도메인 머티리얼의 Emissive(흑백)로 빛 세기를 곱한다
		UMaterial* M = CreateMaterial(RenderingAssets::LightFunction);
		M->MaterialDomain = MD_LightFunction;
		UMaterialExpressionTime* Time = AddExpression<UMaterialExpressionTime>(M, -700, 0);
		auto* Speed = AddScalarParam(M, TEXT("FlickerSpeed"), 12.f, -700, 120);
		// 형광등이 고장 난 것처럼: 대부분 켜져 있다가 가끔 불규칙하게 꺼진다
		UMaterialExpressionCustom* Code = AddCustom(M,
			TEXT("float t = floor(Time * Speed);\n")
			TEXT("float r = frac(sin(t * 12.9898) * 43758.5453);\n")
			TEXT("float burst = step(0.75, frac(sin(floor(Time * 0.7) * 78.233) * 43758.5453));\n")
			TEXT("return lerp(1.0, r > 0.45 ? 1.0 : 0.05, burst);"),
			{TEXT("Time"), TEXT("Speed")}, CMOT_Float1, TEXT("Flicker"));
		Connect(Time, TEXT(""), Code, TEXT("Time"));
		Connect(Speed, TEXT(""), Code, TEXT("Speed"));
		ConnectToPin(Code, MP_EmissiveColor);
		FinishMaterial(M);
	}

	template <typename T>
	T* AddPCGNode(UPCGGraph* Graph, UPCGNode*& OutNode, int32 X, int32 Y)
	{
		T* Settings = nullptr;
		OutNode = Graph->AddNodeOfType<T>(Settings);
#if WITH_EDITORONLY_DATA
		OutNode->PositionX = X;
		OutNode->PositionY = Y;
#endif
		return Settings;
	}

	void BuildForestGraph()
	{
		// 1018: 영역 스플라인 안을 Surface Sampler 로 0.3 개/㎡ 뿌리고, 길 스플라인 주변(±350cm)은 Difference 로 비운다
		UPCGGraph* Graph = GlossaryAssetKit::CreateOrLoad<UPCGGraph>(RenderingAssets::ForestGraph);
		{
			TArray<UPCGNode*> Old;
			for (UPCGNode* Node : Graph->GetNodes()) { Old.Add(Node); }
			Graph->RemoveNodes(Old);
		}

		UPCGNode *AreaNode, *SurfaceNode, *SamplerNode, *PathNode, *PathSampleNode, *BoundsNode, *DiffNode, *TransformNode, *SpawnNode;

		// 영역: 태그 "PCG_Area" 액터의 닫힌 스플라인 → 면(Surface)
		UPCGGetSplineSettings* Area = AddPCGNode<UPCGGetSplineSettings>(Graph, AreaNode, -900, -100);
		Area->ActorSelector.ActorFilter = EPCGActorFilter::AllWorldActors;
		Area->ActorSelector.ActorSelection = EPCGActorSelection::ByTag;
		Area->ActorSelector.ActorSelectionTag = TEXT("PCG_Area");
		AddPCGNode<UPCGCreateSurfaceFromSplineSettings>(Graph, SurfaceNode, -650, -100);

		UPCGSurfaceSamplerSettings* Sampler = AddPCGNode<UPCGSurfaceSamplerSettings>(Graph, SamplerNode, -400, -100);
		Sampler->PointsPerSquaredMeter = 0.3f;   // 요청 예문의 "밀도 0.3"
		Sampler->PointExtents = FVector(60.f);
		Sampler->Looseness = 1.f;
		Sampler->bUnbounded = true;               // 액터 경계 대신 영역 면 전체

		// 길: 태그 "PCG_Path" 액터의 스플라인 → 100cm 마다 점 → 점 크기를 ±350 으로 키워 '비울 띠'를 만든다
		UPCGGetSplineSettings* Path = AddPCGNode<UPCGGetSplineSettings>(Graph, PathNode, -900, 200);
		Path->ActorSelector.ActorFilter = EPCGActorFilter::AllWorldActors;
		Path->ActorSelector.ActorSelection = EPCGActorSelection::ByTag;
		Path->ActorSelector.ActorSelectionTag = TEXT("PCG_Path");
		UPCGSplineSamplerSettings* PathSampler = AddPCGNode<UPCGSplineSamplerSettings>(Graph, PathSampleNode, -650, 200);
		PathSampler->SamplerParams.Mode = EPCGSplineSamplingMode::Distance;
		PathSampler->SamplerParams.DistanceIncrement = 100.f;
		PathSampler->SamplerParams.bUnbounded = true;
		UPCGBoundsModifierSettings* Bounds = AddPCGNode<UPCGBoundsModifierSettings>(Graph, BoundsNode, -400, 200);
		Bounds->Mode = EPCGBoundsModifierMode::Set;
		Bounds->BoundsMin = FVector(-350.f, -350.f, -500.f);
		Bounds->BoundsMax = FVector(350.f, 350.f, 500.f);

		UPCGDifferenceSettings* Diff = AddPCGNode<UPCGDifferenceSettings>(Graph, DiffNode, -150, 0);
		Diff->DensityFunction = EPCGDifferenceDensityFunction::Binary;
		Diff->Mode = EPCGDifferenceMode::Discrete;

		UPCGTransformPointsSettings* Transform = AddPCGNode<UPCGTransformPointsSettings>(Graph, TransformNode, 100, 0);
		Transform->RotationMin = FRotator(0.f, 0.f, 0.f);
		Transform->RotationMax = FRotator(0.f, 360.f, 0.f);
		Transform->ScaleMin = FVector(0.7f);
		Transform->ScaleMax = FVector(1.3f);

		UPCGStaticMeshSpawnerSettings* Spawner = AddPCGNode<UPCGStaticMeshSpawnerSettings>(Graph, SpawnNode, 350, 0);
		Spawner->SetMeshSelectorType(UPCGMeshSelectorWeighted::StaticClass());
		if (UPCGMeshSelectorWeighted* Selector = Cast<UPCGMeshSelectorWeighted>(Spawner->MeshSelectorParameters))
		{
			Selector->MeshEntries.Reset();
			Selector->MeshEntries.Emplace(TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(FString(RenderingAssets::Tree) + TEXT(".SM_Tree"))), 1);
		}

		const FName In = PCGPinConstants::DefaultInputLabel;
		const FName Out = PCGPinConstants::DefaultOutputLabel;
		Graph->AddEdge(AreaNode, Out, SurfaceNode, In);
		Graph->AddEdge(SurfaceNode, Out, SamplerNode, TEXT("Surface"));
		Graph->AddEdge(PathNode, Out, PathSampleNode, TEXT("Spline"));
		Graph->AddEdge(PathSampleNode, Out, BoundsNode, In);
		Graph->AddEdge(SamplerNode, Out, DiffNode, TEXT("Source"));
		Graph->AddEdge(BoundsNode, Out, DiffNode, TEXT("Differences"));
		Graph->AddEdge(DiffNode, Out, TransformNode, In);
		Graph->AddEdge(TransformNode, Out, SpawnNode, In);

		UE_LOG(LogGlossaryRendering, Display, TEXT("[Glossary] PCG 그래프 %s: 노드 %d"), RenderingAssets::ForestGraph, Graph->GetNodes().Num());
		GlossaryAssetKit::SaveAsset(Graph);
	}
}

void BuildGlossaryRenderingAssets()
{
	BuildMeshes();
	BuildLightFunction();
	BuildForestGraph();
}

#endif // WITH_EDITOR
