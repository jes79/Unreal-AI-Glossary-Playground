#include "Demo0418_DepthFade.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"

using namespace GlossaryKit;

ADemo0418_DepthFade::ADemo0418_DepthFade()
{
	DemoId = TEXT("0418");
	Controls = TEXT("C : Depth Fade ON/OFF    [ / ] : FadeDistance −/+ 25cm\n")
		TEXT("관찰: 물이 바위·모래 경사·기둥과 닿는 경계 — ON 이면 얕은 곳부터 부드럽게 투명, OFF 면 칼로 자른 듯한 선");
	Showcase = TEXT("물 표면에 Depth Fade(100cm)로 지면·바위와 닿는 경계를 부드럽게, ON/OFF 비교");
	Environment.bClouds = true;
}

void ADemo0418_DepthFade::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	SetupFixedCamera(FVector(-700.f, -250.f, 420.f), FVector(150.f, -50.f, 50.f), 60.f);

	// 물 아래 지형: 모래 경사(물속으로 들어가는 해변), 바위, 기둥
	const FLinearColor Sand(0.75f, 0.65f, 0.45f);
	SpawnShape(World, EShape::Cube, FVector(0.f, -450.f, 110.f), FVector(900.f, 600.f, 40.f), Sand, FRotator(0.f, 0.f, 12.f));
	SpawnShape(World, EShape::Sphere, FVector(100.f, 50.f, 120.f), FVector(260.f, 220.f, 160.f), Colors::Wall);
	SpawnShape(World, EShape::Sphere, FVector(-150.f, 280.f, 130.f), FVector(160.f, 180.f, 120.f), Colors::Platform);
	SpawnShape(World, EShape::Cylinder, FVector(300.f, 350.f, 150.f), FVector(70.f, 70.f, 300.f), Colors::Accent);
	SpawnShape(World, EShape::Cube, FVector(350.f, -100.f, 100.f), FVector(120.f, 120.f, 200.f), Colors::Ground, FRotator(0.f, 30.f, 0.f));

	// 물 평면 (Translucent + DepthFade)
	AStaticMeshActor* Water = SpawnWithMaterial(EShape::Plane, FVector(150.f, 0.f, WaterLevel), FVector(1400.f, 1400.f, 1.f), LoadMaterial(TEXT("M_Water_DepthFade")));
	Water->SetActorEnableCollision(false);
	WaterMaterial = Water->GetStaticMeshComponent()->CreateAndSetMaterialInstanceDynamic(0);
	SpawnLabel(World, TEXT("M_Water_DepthFade : Opacity = DepthFade(0.85, FadeDistance)"), FVector(500.f, 0.f, 380.f), Colors::Cyan, 16.f);
}

void ADemo0418_DepthFade::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::C)) { bDepthFade = !bDepthFade; }
	if (WasKeyPressed(EKeys::LeftBracket)) { FadeDistance = FMath::Max(1.f, FadeDistance - 25.f); }
	if (WasKeyPressed(EKeys::RightBracket)) { FadeDistance += 25.f; }
	if (WaterMaterial)
	{
		// OFF = 페이드 거리를 거의 0 으로 → 경계에서 바로 불투명
		WaterMaterial->SetScalarParameterValue(TEXT("FadeDistance"), bDepthFade ? FadeDistance : 0.01f);
		WaterMaterial->SetScalarParameterValue(TEXT("WaterOpacity"), WaterOpacity);
	}
	SetStatus(FString::Printf(TEXT("Depth Fade %s · FadeDistance %.0fcm · 수면 Opacity %.2f"),
		bDepthFade ? TEXT("ON") : TEXT("OFF"), FadeDistance, WaterOpacity));
}
