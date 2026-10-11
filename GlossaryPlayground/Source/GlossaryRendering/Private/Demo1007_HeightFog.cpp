#include "Demo1007_HeightFog.h"

#include "Camera/CameraActor.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "GlossaryEnvironment.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo1007_HeightFog::ADemo1007_HeightFog()
{
	DemoId = TEXT("1007");
	Controls = TEXT("1 : Volumetric Fog ON/OFF   2 : Height Fog 전체 ON/OFF   [ / ] : 안개 밀도   , / . : 산란 방향\n")
		TEXT("WASD·QE·←→↑↓ 카메라\n")
		TEXT("관찰: Volumetric Fog 를 켜면 창살 사이로 들어온 햇빛이 공기 중에 빛줄기로 보인다. 끄면 바닥의 빛 무늬만 남는다.");
	Showcase = TEXT("창문 있는 어두운 방, Exponential Height Fog + Volumetric Fog 로 창살 사이 God Ray, ON/OFF·밀도 조절");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Day;
	Environment.bFog = true;
}

void ADemo1007_HeightFog::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 방: X -800~800, Y -500~500, 높이 600. 해가 들어오는 -X 벽에 창문(Y -250~250, Z 180~470)
	const FLinearColor Wall(0.55f, 0.52f, 0.48f);
	SpawnShape(World, EShape::Cube, FVector(0.f, 0.f, 615.f), FVector(1640.f, 1040.f, 30.f), Wall);        // 지붕
	SpawnShape(World, EShape::Cube, FVector(815.f, 0.f, 300.f), FVector(30.f, 1040.f, 600.f), Wall);       // +X 벽
	SpawnShape(World, EShape::Cube, FVector(0.f, -515.f, 300.f), FVector(1640.f, 30.f, 600.f), Wall);      // -Y 벽
	SpawnShape(World, EShape::Cube, FVector(0.f, 515.f, 300.f), FVector(1640.f, 30.f, 600.f), Wall);       // +Y 벽
	// 창문이 있는 -X 벽: 창 구멍을 남기고 4조각
	SpawnShape(World, EShape::Cube, FVector(-815.f, -390.f, 300.f), FVector(30.f, 280.f, 600.f), Wall);
	SpawnShape(World, EShape::Cube, FVector(-815.f, 390.f, 300.f), FVector(30.f, 280.f, 600.f), Wall);
	SpawnShape(World, EShape::Cube, FVector(-815.f, 0.f, 90.f), FVector(30.f, 500.f, 180.f), Wall);
	SpawnShape(World, EShape::Cube, FVector(-815.f, 0.f, 535.f), FVector(30.f, 500.f, 130.f), Wall);
	// 창살: 세로 4개 + 가로 1개
	for (int32 i = 1; i <= 4; ++i)
	{
		SpawnShape(World, EShape::Cube, FVector(-815.f, -250.f + i * 100.f, 325.f), FVector(20.f, 12.f, 290.f), Colors::Dark);
	}
	SpawnShape(World, EShape::Cube, FVector(-815.f, 0.f, 325.f), FVector(20.f, 500.f, 12.f), Colors::Dark);

	// 방 안 소품
	SpawnBlock(World, FVector(300.f, -250.f, 0.f), FVector(200.f, 200.f, 90.f), FLinearColor(0.3f, 0.18f, 0.1f));
	SpawnShape(World, EShape::Sphere, FVector(100.f, 200.f, 60.f), FVector(120.f), Colors::Neutral);

	Camera = RenderKit::SpawnLookAtCamera(this, FVector(700.f, 380.f, 330.f), FVector(-500.f, -50.f, 200.f), 80.f);
	ApplyFog();
}

void ADemo1007_HeightFog::ApplyFog()
{
	AGlossaryEnvironment* Env = AGlossaryEnvironment::Find(this);
	if (!Env)
	{
		return;
	}
	// 해: 창문 쪽(-X)에서 낮게 들어오도록
	UDirectionalLightComponent* Sun = Env->GetSun();
	Sun->SetWorldRotation(FRotator(-28.f, 5.f, 0.f));
	Sun->SetVolumetricScatteringIntensity(SunScatteringIntensity);

	// 핵심: Exponential Height Fog 의 Volumetric Fog 옵션
	UExponentialHeightFogComponent* Fog = Env->GetFog();
	Fog->SetVisibility(bFogEnabled);
	Fog->SetFogDensity(FogDensity);
	Fog->SetFogHeightFalloff(0.05f);
	Fog->SetVolumetricFog(bVolumetricFog);
	Fog->SetVolumetricFogScatteringDistribution(ScatteringDistribution);
	Fog->SetVolumetricFogExtinctionScale(1.f);
	Fog->SetVolumetricFogDistance(3000.f);
}

void ADemo1007_HeightFog::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	bool bChanged = !bApplied;   // 첫 Tick 에 한 번 더 (환경 액터 BeginPlay 순서와 무관하게)
	bApplied = true;
	if (WasKeyPressed(EKeys::One)) { bVolumetricFog = !bVolumetricFog; bChanged = true; }
	if (WasKeyPressed(EKeys::Two)) { bFogEnabled = !bFogEnabled; bChanged = true; }
	if (WasKeyPressed(EKeys::LeftBracket))  { FogDensity = FMath::Max(0.005f, FogDensity * 0.5f); bChanged = true; }
	if (WasKeyPressed(EKeys::RightBracket)) { FogDensity = FMath::Min(0.5f, FogDensity * 2.f); bChanged = true; }
	if (WasKeyPressed(EKeys::Comma))  { ScatteringDistribution = FMath::Max(0.f, ScatteringDistribution - 0.2f); bChanged = true; }
	if (WasKeyPressed(EKeys::Period)) { ScatteringDistribution = FMath::Min(0.9f, ScatteringDistribution + 0.2f); bChanged = true; }
	if (bChanged)
	{
		ApplyFog();
	}
	RenderKit::TickFlyCamera(this, Camera, DeltaSeconds, 400.f);

	Stats.Update(DeltaSeconds);
	SetStatus(FString::Printf(TEXT("Height Fog %s · Volumetric Fog %s\nFog Density %.3f · Scattering Distribution %.1f · Sun Scattering %.1f\n%s"),
		bFogEnabled ? TEXT("ON") : TEXT("OFF"), bVolumetricFog ? TEXT("ON (빛줄기)") : TEXT("OFF"),
		FogDensity, ScatteringDistribution, SunScatteringIntensity, *Stats.ToString(false)));
}
