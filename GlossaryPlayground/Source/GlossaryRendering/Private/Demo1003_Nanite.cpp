#include "Demo1003_Nanite.h"

#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GlossaryKit.h"
#include "RenderingAssetPaths.h"

using namespace GlossaryKit;

ADemo1003_Nanite::ADemo1003_Nanite()
{
	DemoId = TEXT("1003");
	Environment.bFixedExposure = false;   // 시각화 화면(Triangles·Clusters)도 자동 노출로 보이게
	Controls = TEXT("1 : 일반 화면   2 : Nanite Visualize Triangles   3 : Clusters\n")
		TEXT("4 : 같은 바위를 Nanite 끈 메시로 바꾸기(프레임 시간 비교)   WASD·QE·←→↑↓ 카메라\n")
		TEXT("관찰: 삼각형 33만 개짜리 바위 20개(650만)인데도 Nanite 는 화면 크기에 맞춰 클러스터 단위로 자동 LOD.\n")
		TEXT("가까이 가면 삼각형이 촘촘해지고 멀면 굵어진다.");
	Showcase = TEXT("33만 삼각형 바위 20개(Nanite), r.Nanite.Visualize Triangles·Clusters, Nanite 끈 메시와 GPU 시간 비교");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Day;
}

void ADemo1003_Nanite::BeginPlay()
{
	Super::BeginPlay();
	NaniteMesh = LoadAsset<UStaticMesh>(RenderingAssets::NaniteRock);
	NoNaniteMesh = LoadAsset<UStaticMesh>(RenderingAssets::NaniteRockOff);
	if (!NaniteMesh)
	{
		SetStatus(TEXT("SM_NaniteRock 이 없습니다. tools/glossary-build.sh -filter=10 으로 에셋을 만드세요."));
		return;
	}

	FRandomStream Rand(1003);
	for (int32 X = 0; X < RockGrid.X; ++X)
	{
		for (int32 Y = 0; Y < RockGrid.Y; ++Y)
		{
			UStaticMeshComponent* Rock = NewObject<UStaticMeshComponent>(this);
			Rock->SetStaticMesh(NaniteMesh);
			Rock->SetupAttachment(RootComponent);
			Rock->SetRelativeLocation(FVector(X * 450.f + Rand.FRandRange(-60.f, 60.f), (Y - (RockGrid.Y - 1) * 0.5f) * 450.f + Rand.FRandRange(-60.f, 60.f), 60.f));
			Rock->SetRelativeRotation(FRotator(0.f, Rand.FRandRange(0.f, 360.f), 0.f));
			Rock->SetRelativeScale3D(FVector(Rand.FRandRange(0.8f, 1.4f)));
			Rock->RegisterComponent();
			Rocks.Add(Rock);
		}
	}
	SpawnLabel(GetWorld(), TEXT("바위 1개 = 327,680 삼각형"), FVector(0.f, 0.f, 380.f), Colors::Item, 18.f);

	Camera = RenderKit::SpawnLookAtCamera(this, FVector(-500.f, -250.f, 420.f), FVector(700.f, 0.f, 0.f), 70.f);
	SetVisualize(TEXT(""));
}

void ADemo1003_Nanite::SetVisualize(const FString& Mode)
{
	VisualizeMode = Mode;
	// 콘솔 명령과 같다: r.Nanite.Visualize Triangles  (빈 값이면 끔)
	CVars.Set(TEXT("r.Nanite.Visualize"), Mode.IsEmpty() ? FString(TEXT("Off")) : Mode);
}

void ADemo1003_Nanite::SetUseNaniteMesh(bool bNanite)
{
	bUsingNaniteMesh = bNanite;
	for (UStaticMeshComponent* Rock : Rocks)
	{
		Rock->SetStaticMesh(bNanite ? NaniteMesh.Get() : NoNaniteMesh.Get());
	}
}

void ADemo1003_Nanite::EndPlay(const EEndPlayReason::Type Reason)
{
	CVars.RestoreAll();
	Super::EndPlay(Reason);
}

void ADemo1003_Nanite::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One))   { SetVisualize(TEXT("")); }
	if (WasKeyPressed(EKeys::Two))   { SetVisualize(TEXT("Triangles")); }
	if (WasKeyPressed(EKeys::Three)) { SetVisualize(TEXT("Clusters")); }
	if (WasKeyPressed(EKeys::Four) && NoNaniteMesh) { SetUseNaniteMesh(!bUsingNaniteMesh); }
	RenderKit::TickFlyCamera(this, Camera, DeltaSeconds, 500.f);

	Stats.Update(DeltaSeconds);
	// Nanite 메시의 GetNumTriangles 는 Fallback 메시 수라서, 같은 원본의 Nanite 끈 메시로 원본 삼각형 수를 센다
	const int32 Tris = NoNaniteMesh ? NoNaniteMesh->GetNumTriangles(0) : 0;
	SetStatus(FString::Printf(TEXT("메시: %s · Visualize: %s\n원본 삼각형 %d × %d = %.1fM\n%s"),
		bUsingNaniteMesh ? TEXT("Nanite 켬") : TEXT("Nanite 끔(일반 메시, LOD 없음)"),
		VisualizeMode.IsEmpty() ? TEXT("끔") : *VisualizeMode,
		Tris, Rocks.Num(), Tris * Rocks.Num() / 1.0e6f, *Stats.ToString()));
}
