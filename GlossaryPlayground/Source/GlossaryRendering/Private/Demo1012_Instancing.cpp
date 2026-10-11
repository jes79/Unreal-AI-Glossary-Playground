#include "Demo1012_Instancing.h"

#include "Camera/CameraActor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryKit.h"
#include "RenderingAssetPaths.h"

using namespace GlossaryKit;

ADemo1012_Instancing::ADemo1012_Instancing()
{
	DemoId = TEXT("1012");
	Controls = TEXT("1 : HISM 인스턴스 5000개   2 : StaticMeshActor 5000개   WASD·QE·←→↑↓ 카메라\n")
		TEXT("관찰: 같은 숲이지만 액터 방식은 컴포넌트 5000개를 렌더 스레드가 하나씩 처리해 Draw(ms)·DrawCalls 가 커진다.\n")
		TEXT("HISM 은 컴포넌트 1개가 인스턴스를 묶어 그리고, 클러스터 단위로 컬링·LOD 를 한다.");
	Showcase = TEXT("나무 5000그루를 HISM 1개 vs StaticMeshActor 5000개로 전환, DrawCalls·Game/Draw/GPU ms·생성 시간 비교");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Day;
	Environment.FloorSize = 30000.f;
}

void ADemo1012_Instancing::BeginPlay()
{
	Super::BeginPlay();
	TreeMesh = LoadAsset<UStaticMesh>(RenderingAssets::Tree);
	if (!TreeMesh)
	{
		SetStatus(TEXT("SM_Tree 가 없습니다. tools/glossary-build.sh -filter=10 으로 에셋을 만드세요."));
		return;
	}
	// 100 × 50 격자에 무작위로 흩뜨린 5000그루 (두 방식이 같은 위치를 쓴다)
	FRandomStream Rand(1012);
	const int32 Columns = 100;
	for (int32 i = 0; i < TreeCount; ++i)
	{
		const int32 X = i % Columns, Y = i / Columns;
		const FVector Loc(X * 220.f + Rand.FRandRange(-80.f, 80.f), (Y - TreeCount / Columns * 0.5f) * 220.f + Rand.FRandRange(-80.f, 80.f), 0.f);
		TreeTransforms.Add(FTransform(FRotator(0.f, Rand.FRandRange(0.f, 360.f), 0.f), Loc, FVector(Rand.FRandRange(0.7f, 1.3f))));
	}
	Camera = RenderKit::SpawnViewCamera(this, FVector(-1500.f, 0.f, 2200.f), FRotator(-25.f, 0.f, 0.f), 80.f);
	Rebuild(true);
}

void ADemo1012_Instancing::Rebuild(bool bHISM)
{
	const double Start = FPlatformTime::Seconds();
	bUseHISM = bHISM;
	// 이전 방식 정리
	if (HISM)
	{
		HISM->DestroyComponent();
		HISM = nullptr;
	}
	for (AStaticMeshActor* A : TreeActors)
	{
		A->Destroy();
	}
	TreeActors.Reset();

	if (bHISM)
	{
		// 핵심: 컴포넌트 하나 + AddInstances 한 번
		HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
		HISM->SetStaticMesh(TreeMesh);
		HISM->SetupAttachment(RootComponent);
		HISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HISM->RegisterComponent();
		HISM->AddInstances(TreeTransforms, false, true);
	}
	else
	{
		// 비교: 나무마다 액터 하나 (흔한 실수)
		TreeActors.Reserve(TreeTransforms.Num());
		for (const FTransform& T : TreeTransforms)
		{
			AStaticMeshActor* A = GetWorld()->SpawnActor<AStaticMeshActor>(T.GetLocation(), T.Rotator());
			A->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
			A->SetActorScale3D(T.GetScale3D());
			A->GetStaticMeshComponent()->SetStaticMesh(TreeMesh);
			A->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			TreeActors.Add(A);
		}
	}
	LastBuildMs = (FPlatformTime::Seconds() - Start) * 1000.0;
}

void ADemo1012_Instancing::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!TreeMesh)
	{
		return;
	}
	if (WasKeyPressed(EKeys::One) && !bUseHISM) { Rebuild(true); }
	if (WasKeyPressed(EKeys::Two) && bUseHISM)  { Rebuild(false); }
	RenderKit::TickFlyCamera(this, Camera, DeltaSeconds, 1500.f);

	Stats.Update(DeltaSeconds);
	SetStatus(FString::Printf(TEXT("방식: %s · 나무 %d그루 · 생성 %.0fms\n%s\n병목: %s"),
		bUseHISM ? TEXT("HISM 컴포넌트 1개 (인스턴스)") : TEXT("StaticMeshActor 액터마다 1개"),
		TreeTransforms.Num(), LastBuildMs, *Stats.ToString(), *Stats.Bottleneck()));
}
