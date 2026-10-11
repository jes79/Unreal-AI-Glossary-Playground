#include "Demo1004_VirtualShadowMaps.h"

#include "Camera/CameraActor.h"
#include "Components/DirectionalLightComponent.h"
#include "GlossaryEnvironment.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo1004_VirtualShadowMaps::ADemo1004_VirtualShadowMaps()
{
	DemoId = TEXT("1004");
	Controls = TEXT("1 : Virtual Shadow Maps   2 : 기존 Shadow Maps(CSM)   [ ] : CSM 해상도 ÷2 / ×2\n")
		TEXT("WASD·QE·←→↑↓ 카메라 (멀리 있는 울타리 그림자도 확인)\n")
		TEXT("관찰: VSM 은 2cm 철사 그림자까지 선명하고 멀리서도 유지된다.\n")
		TEXT("Shadow Maps 는 가는 그림자가 뭉개지거나 끊기고 경계가 계단처럼 흐려진다.");
	Showcase = TEXT("2cm 철망·울타리·격자 지붕 그림자, r.Shadow.Virtual.Enable 1/0 으로 VSM vs CSM 해상도 비교");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Day;
}

void ADemo1004_VirtualShadowMaps::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 해를 낮게 눕혀 그림자를 길게
	if (AGlossaryEnvironment* Env = AGlossaryEnvironment::Find(this))
	{
		Env->GetSun()->SetWorldRotation(FRotator(-32.f, 60.f, 0.f));
	}

	// 1) 철망: 2cm 굵기 막대로 만든 격자 (가로 30 × 세로 15)
	const FVector MeshOrigin(300.f, -300.f, 0.f);
	for (int32 i = 0; i <= 30; ++i)
	{
		SpawnShape(World, EShape::Cylinder, MeshOrigin + FVector(0.f, i * 20.f, 150.f), FVector(2.f, 2.f, 300.f), Colors::Neutral);
	}
	for (int32 j = 0; j <= 15; ++j)
	{
		SpawnShape(World, EShape::Cylinder, MeshOrigin + FVector(0.f, 300.f, j * 20.f), FVector(2.f, 2.f, 600.f), Colors::Neutral, FRotator(0.f, 0.f, 90.f));
	}

	// 2) 격자 지붕(퍼걸러): 5cm 각목 사이로 빛이 새어 바닥에 줄무늬
	const FVector Pergola(500.f, 500.f, 0.f);
	for (int32 i = 0; i < 4; ++i)
	{
		SpawnShape(World, EShape::Cube, Pergola + FVector((i % 2) * 400.f, (i / 2) * 400.f, 150.f), FVector(12.f, 12.f, 300.f), Colors::Wall);
	}
	for (int32 i = 0; i <= 20; ++i)
	{
		SpawnShape(World, EShape::Cube, Pergola + FVector(i * 20.f, 200.f, 300.f), FVector(5.f, 440.f, 8.f), Colors::Accent);
	}

	// 3) 멀리까지 이어지는 울타리 (거리에 따른 그림자 품질)
	for (int32 i = 0; i < 60; ++i)
	{
		SpawnShape(World, EShape::Cube, FVector(1200.f + i * 100.f, -600.f, 60.f), FVector(6.f, 6.f, 120.f), Colors::Platform);
	}
	SpawnShape(World, EShape::Cube, FVector(1200.f + 2950.f, -600.f, 100.f), FVector(6000.f, 4.f, 4.f), Colors::Platform);

	// 4) 작은 캐릭터 대용 구·원기둥 (접지 그림자)
	SpawnShape(World, EShape::Sphere, FVector(100.f, 300.f, 25.f), FVector(50.f), Colors::Player);
	SpawnShape(World, EShape::Cylinder, FVector(150.f, 400.f, 40.f), FVector(10.f, 10.f, 80.f), Colors::Enemy);

	SpawnLabel(World, TEXT("2cm 철망"), MeshOrigin + FVector(0.f, 300.f, 340.f), Colors::Item, 18.f);
	SpawnLabel(World, TEXT("격자 지붕"), Pergola + FVector(200.f, 200.f, 360.f), Colors::Item, 18.f);

	Camera = RenderKit::SpawnLookAtCamera(this, FVector(-250.f, 150.f, 380.f), FVector(500.f, 150.f, 0.f), 70.f);
	Apply();
}

void ADemo1004_VirtualShadowMaps::Apply()
{
	// 핵심: Shadow Map Method 전환. 실행 중에 바꿔도 다음 프레임부터 반영된다
	CVars.Set(TEXT("r.Shadow.Virtual.Enable"), bVirtualShadowMaps ? 1 : 0);
	CVars.Set(TEXT("r.Shadow.MaxCSMResolution"), ShadowMapResolution);
}

void ADemo1004_VirtualShadowMaps::EndPlay(const EEndPlayReason::Type Reason)
{
	CVars.RestoreAll();
	Super::EndPlay(Reason);
}

void ADemo1004_VirtualShadowMaps::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One)) { bVirtualShadowMaps = true; Apply(); }
	if (WasKeyPressed(EKeys::Two)) { bVirtualShadowMaps = false; Apply(); }
	if (WasKeyPressed(EKeys::LeftBracket))  { ShadowMapResolution = FMath::Max(256, ShadowMapResolution / 2); Apply(); }
	if (WasKeyPressed(EKeys::RightBracket)) { ShadowMapResolution = FMath::Min(8192, ShadowMapResolution * 2); Apply(); }
	RenderKit::TickFlyCamera(this, Camera, DeltaSeconds, 500.f);

	Stats.Update(DeltaSeconds);
	SetStatus(FString::Printf(TEXT("Shadow Map Method: %s  (r.Shadow.Virtual.Enable %s)\n%s\n%s"),
		bVirtualShadowMaps ? TEXT("Virtual Shadow Maps (16K 가상 페이지)") : *FString::Printf(TEXT("Shadow Maps (CSM %d)"), ShadowMapResolution),
		*RenderKit::GetCVar(TEXT("r.Shadow.Virtual.Enable")), *Stats.ToString(false),
		bVirtualShadowMaps ? TEXT("") : TEXT("[ ] 로 CSM 해상도를 바꿔 봐도 철망 그림자는 VSM 만큼 선명해지지 않는다")));
}
