#include "Demo1018_PCG.h"

#include "Camera/CameraActor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryKit.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "RenderingAssetPaths.h"

using namespace GlossaryKit;

ADemo1018_PCG::ADemo1018_PCG()
{
	DemoId = TEXT("1018");
	Controls = TEXT("G : 새 시드로 다시 생성   P : 길 위치를 옮기고 다시 생성   C : 지우기(Cleanup)   WASD·QE·←→↑↓ 카메라\n")
		TEXT("관찰: 40m×40m 영역에 나무가 0.3개/㎡ 로 뿌려지고, 갈색 길(Spline) 양쪽 3.5m 는 Difference 로 비워진다.\n")
		TEXT("길을 옮기면 비워지는 곳도 따라 바뀐다 — 손으로 배치하지 않고 규칙(그래프)으로 배치.");
	Showcase = TEXT("코드로 만든 PCG 그래프: 영역 Spline 면에 Surface Sampler 0.3/㎡ 나무, 길 Spline 주변 Difference 로 비움, 실행 중 재생성");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Day;
}

USplineComponent* ADemo1018_PCG::SpawnSplineActor(FName Tag)
{
	// PCG 의 Get Spline Data 노드가 태그로 찾는 액터 (Spline 컴포넌트가 루트)
	AActor* Actor = GetWorld()->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	USplineComponent* Spline = NewObject<USplineComponent>(Actor, TEXT("Spline"));
	Actor->SetRootComponent(Spline);
	Spline->RegisterComponent();
	Actor->AddInstanceComponent(Spline);
	Actor->Tags.Add(Tag);
	return Spline;
}

void ADemo1018_PCG::BeginPlay()
{
	Super::BeginPlay();

	// 숲 영역: 닫힌 사각형 Spline
	AreaSpline = SpawnSplineActor(TEXT("PCG_Area"));
	AreaSpline->ClearSplinePoints(false);
	const float H = AreaSize * 0.5f;
	for (const FVector& P : {FVector(-H, -H, 0.f), FVector(H, -H, 0.f), FVector(H, H, 0.f), FVector(-H, H, 0.f)})
	{
		AreaSpline->AddSplinePoint(P, ESplineCoordinateSpace::World, false);
	}
	for (int32 i = 0; i < 4; ++i)
	{
		AreaSpline->SetSplinePointType(i, ESplinePointType::Linear, false);
	}
	AreaSpline->SetClosedLoop(true, false);
	AreaSpline->UpdateSpline();

	// 길: 영역을 가로지르는 S자 Spline
	PathSpline = SpawnSplineActor(TEXT("PCG_Path"));
	BuildPath();

	// PCG 컴포넌트에 그래프를 넣고 실행 중 생성
	PCG = NewObject<UPCGComponent>(this, TEXT("PCG"));
	PCG->GenerationTrigger = EPCGComponentGenerationTrigger::GenerateOnDemand;
	PCG->RegisterComponent();
	AddInstanceComponent(PCG);
	if (UPCGGraph* Graph = LoadAsset<UPCGGraph>(RenderingAssets::ForestGraph))
	{
		PCG->SetGraph(Graph);
		Generate();
	}
	else
	{
		SetStatus(TEXT("PCG_ForestScatter 가 없습니다. tools/glossary-build.sh -filter=10 으로 에셋을 만드세요."));
	}

	SpawnLabel(GetWorld(), TEXT("길 Spline 주변은 비움"), FVector(-H - 300.f, 0.f, 300.f), Colors::Item, 18.f);
	Camera = RenderKit::SpawnLookAtCamera(this, FVector(-H - 2200.f, -H - 1200.f, 2600.f), FVector(0.f, 0.f, 0.f), 70.f);
}

void ADemo1018_PCG::BuildPath()
{
	const float H = AreaSize * 0.5f;
	PathSpline->ClearSplinePoints(false);
	PathSpline->AddSplinePoint(FVector(-H - 400.f, -600.f + PathOffset, 0.f), ESplineCoordinateSpace::World, false);
	PathSpline->AddSplinePoint(FVector(-H * 0.3f, 500.f + PathOffset, 0.f), ESplineCoordinateSpace::World, false);
	PathSpline->AddSplinePoint(FVector(H * 0.3f, -500.f + PathOffset, 0.f), ESplineCoordinateSpace::World, false);
	PathSpline->AddSplinePoint(FVector(H + 400.f, 600.f + PathOffset, 0.f), ESplineCoordinateSpace::World, false);
	PathSpline->UpdateSpline();

	// 길 표시 (갈색 판)
	for (AStaticMeshActor* T : PathTiles) { T->Destroy(); }
	PathTiles.Reset();
	const float Length = PathSpline->GetSplineLength();
	for (float D = 0.f; D < Length; D += 150.f)
	{
		const FVector Loc = PathSpline->GetLocationAtDistanceAlongSpline(D, ESplineCoordinateSpace::World);
		const FRotator Rot = PathSpline->GetRotationAtDistanceAlongSpline(D, ESplineCoordinateSpace::World);
		PathTiles.Add(SpawnShape(GetWorld(), EShape::Cube, Loc + FVector(0.f, 0.f, 2.f), FVector(170.f, 300.f, 4.f), FLinearColor(0.35f, 0.22f, 0.1f), FRotator(0.f, Rot.Yaw, 0.f)));
	}
}

void ADemo1018_PCG::Generate()
{
	if (!PCG || !PCG->GetGraph())
	{
		return;
	}
	// 핵심: Generate (bForce=true 면 이전 결과를 지우고 다시)
	GenerateStart = FPlatformTime::Seconds();
	bWaiting = true;
	PCG->GenerateLocal(true);
}

int32 ADemo1018_PCG::CountInstances() const
{
	// PCG 의 Static Mesh Spawner 는 소유 액터에 (H)ISM 컴포넌트를 만들어 인스턴스로 그린다
	int32 Count = 0;
	TArray<UInstancedStaticMeshComponent*> ISMs;
	GetComponents(ISMs);
	for (const UInstancedStaticMeshComponent* ISM : ISMs)
	{
		Count += ISM->GetInstanceCount();
	}
	return Count;
}

void ADemo1018_PCG::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::G) && PCG) { PCG->Seed = FMath::Rand(); Generate(); }
	if (WasKeyPressed(EKeys::P) && PCG) { PathOffset = PathOffset > 0.f ? -900.f : PathOffset + 900.f; BuildPath(); Generate(); }
	if (WasKeyPressed(EKeys::C) && PCG) { PCG->CleanupLocal(true); }
	RenderKit::TickFlyCamera(this, Camera, DeltaSeconds, 1200.f);
	if (!PCG)
	{
		return;
	}
	if (bWaiting && !PCG->IsGenerating())
	{
		bWaiting = false;
		LastGenerateSeconds = float(FPlatformTime::Seconds() - GenerateStart);
	}

	Stats.Update(DeltaSeconds);
	const float AreaM2 = FMath::Square(AreaSize / 100.f);
	SetStatus(FString::Printf(TEXT("PCG Graph: PCG_ForestScatter · Seed %d · %s\n나무 인스턴스 %d (영역 %.0f㎡ × 0.3 = 최대 약 %.0f) · 생성 %.2f초\n%s"),
		PCG->Seed, PCG->IsGenerating() ? TEXT("생성 중…") : TEXT("생성 완료"), CountInstances(), AreaM2, AreaM2 * 0.3f, LastGenerateSeconds,
		*Stats.ToString(false)));
}
