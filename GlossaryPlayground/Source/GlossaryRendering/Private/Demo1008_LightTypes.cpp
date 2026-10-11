#include "Demo1008_LightTypes.h"

#include "Camera/CameraActor.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/PointLight.h"
#include "Engine/RectLight.h"
#include "Engine/SpotLight.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryKit.h"
#include "Materials/MaterialInterface.h"
#include "RenderingAssetPaths.h"

using namespace GlossaryKit;

ADemo1008_LightTypes::ADemo1008_LightTypes()
{
	DemoId = TEXT("1008");
	Controls = TEXT("1 : Point(전구)   2 : Spot(무대)   3 : Rect(형광등) 켜기/끄기   4 : Light Function(깜빡임) ON/OFF\n")
		TEXT("WASD·QE·←→↑↓ 카메라\n")
		TEXT("관찰: Point 는 사방으로, Spot 은 원뿔(안쪽 20° 선명 · 바깥 35° 까지 부드럽게), Rect 는 넓은 면에서 부드러운 그림자.\n")
		TEXT("Light Function 이 켜지면 형광등 하나와 무대 조명이 고장 난 것처럼 깜빡인다.");
	Showcase = TEXT("복도 Rect Light 형광등·무대 Spot(Inner 20°/Outer 35°)·Point 전구, Light Function 머티리얼로 깜빡임");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Indoor;
	Environment.bFog = false;
}

void ADemo1008_LightTypes::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	LightFunctionMaterial = LoadAsset<UMaterialInterface>(RenderingAssets::LightFunction);

	// 복도: X -1600~0, 폭 400, 높이 350.  무대: X 0~1000, 폭 1200, 높이 700
	const FLinearColor Wall(0.5f, 0.5f, 0.52f);
	SpawnShape(World, EShape::Cube, FVector(-800.f, -215.f, 175.f), FVector(1600.f, 30.f, 350.f), Wall);
	SpawnShape(World, EShape::Cube, FVector(-800.f, 215.f, 175.f), FVector(1600.f, 30.f, 350.f), Wall);
	SpawnShape(World, EShape::Cube, FVector(-800.f, 0.f, 365.f), FVector(1600.f, 460.f, 30.f), Wall);
	SpawnShape(World, EShape::Cube, FVector(500.f, -615.f, 350.f), FVector(1000.f, 30.f, 700.f), Wall);
	SpawnShape(World, EShape::Cube, FVector(500.f, 615.f, 350.f), FVector(1000.f, 30.f, 700.f), Wall);
	SpawnShape(World, EShape::Cube, FVector(1015.f, 0.f, 350.f), FVector(30.f, 1260.f, 700.f), FLinearColor(0.15f, 0.05f, 0.08f));
	SpawnShape(World, EShape::Cube, FVector(500.f, 0.f, 715.f), FVector(1000.f, 1260.f, 30.f), Wall);
	SpawnShape(World, EShape::Cube, FVector(0.f, -410.f, 525.f), FVector(30.f, 380.f, 350.f), Wall);   // 복도와 무대 사이 벽
	SpawnShape(World, EShape::Cube, FVector(0.f, 410.f, 525.f), FVector(30.f, 380.f, 350.f), Wall);
	SpawnShape(World, EShape::Cube, FVector(0.f, 0.f, 535.f), FVector(30.f, 440.f, 330.f), Wall);
	// 무대 단 + 배우(원기둥+구)
	SpawnBlock(World, FVector(650.f, 0.f, 0.f), FVector(600.f, 900.f, 40.f), FLinearColor(0.25f, 0.15f, 0.08f));
	SpawnShape(World, EShape::Cylinder, FVector(650.f, 0.f, 130.f), FVector(60.f, 60.f, 180.f), Colors::Neutral);
	SpawnShape(World, EShape::Sphere, FVector(650.f, 0.f, 250.f), FVector(55.f), Colors::Neutral);

	// --- Rect Light 형광등: 복도 천장에 4개, 아래를 향함
	for (int32 i = 0; i < 4; ++i)
	{
		const FVector Loc(-1400.f + i * 400.f, 0.f, 345.f);
		ARectLight* Rect = World->SpawnActor<ARectLight>(Loc, FRotator(-90.f, 0.f, 0.f));
		URectLightComponent* C = Rect->RectLightComponent;
		C->SetMobility(EComponentMobility::Movable);
		C->SetWorldRotation(FRotator(-90.f, 0.f, 0.f));   // 아래를 향하게 (스폰 회전이 라이트 액터에 안 먹는 경우가 있어 명시)
		C->SetIntensityUnits(ELightUnits::Candelas);
		C->SetIntensity(10.f);
		C->SetSourceWidth(150.f);
		C->SetSourceHeight(20.f);
		C->SetAttenuationRadius(1000.f);
		C->SetLightColor(FLinearColor(0.85f, 0.95f, 1.f));
		RectLights.Add(C);
		// 형광등 몸체 (발광)
		AStaticMeshActor* Tube = SpawnShape(World, EShape::Cube, Loc + FVector(0.f, 0.f, 5.f), FVector(20.f, 150.f, 6.f), FLinearColor::White);
		SetColor(Tube->GetStaticMeshComponent(), FLinearColor(0.85f, 0.95f, 1.f), 0.5f, 0.f, 8.f);
		Tube->GetStaticMeshComponent()->SetCastShadow(false);
		if (i == 2)
		{
			FlickerRect = C;
		}
	}

	// --- Point Light 전구: 복도 끝 근처에 매달린 따뜻한 전구
	{
		const FVector Loc(-200.f, 120.f, 250.f);
		APointLight* Bulb = World->SpawnActor<APointLight>(Loc, FRotator::ZeroRotator);
		UPointLightComponent* C = Bulb->PointLightComponent;
		C->SetMobility(EComponentMobility::Movable);
		C->SetIntensityUnits(ELightUnits::Candelas);
		C->SetIntensity(30.f);
		C->SetAttenuationRadius(800.f);
		C->SetLightColor(FLinearColor(1.f, 0.6f, 0.25f));
		C->SetSourceRadius(4.f);
		PointLights.Add(C);
		AStaticMeshActor* Glass = SpawnShape(World, EShape::Sphere, Loc, FVector(14.f), FLinearColor::White);
		SetColor(Glass->GetStaticMeshComponent(), FLinearColor(1.f, 0.6f, 0.25f), 0.5f, 0.f, 30.f);
		Glass->GetStaticMeshComponent()->SetCastShadow(false);
	}

	// --- Spot Light 무대 조명: Inner 20°, Outer 35°
	{
		const FVector Loc(150.f, 0.f, 680.f);
		ASpotLight* Spot = World->SpawnActor<ASpotLight>(Loc, (FVector(650.f, 0.f, 0.f) - Loc).Rotation());
		USpotLightComponent* C = Spot->SpotLightComponent;
		C->SetMobility(EComponentMobility::Movable);
		C->SetWorldRotation((FVector(650.f, 0.f, 0.f) - Loc).Rotation());
		C->SetIntensityUnits(ELightUnits::Candelas);
		C->SetIntensity(400.f);
		C->SetInnerConeAngle(SpotInnerConeAngle);
		C->SetOuterConeAngle(SpotOuterConeAngle);
		C->SetAttenuationRadius(2000.f);
		C->SetLightColor(FLinearColor(1.f, 0.95f, 0.85f));
		SpotLights.Add(C);
	}

	SpawnLabel(World, TEXT("Rect Light 형광등"), FVector(-1000.f, 0.f, 300.f), Colors::Cyan, 16.f);
	SpawnLabel(World, TEXT("Point Light"), FVector(-200.f, 120.f, 300.f), Colors::Accent, 16.f);
	SpawnLabel(World, TEXT("Spot Light (20°/35°)"), FVector(650.f, 0.f, 380.f), Colors::Item, 16.f);

	Camera = RenderKit::SpawnLookAtCamera(this, FVector(-1550.f, -120.f, 200.f), FVector(600.f, 0.f, 120.f), 75.f);
	ApplyLightFunction();
}

void ADemo1008_LightTypes::ApplyLightFunction()
{
	// Light Function: 라이트의 세기에 머티리얼(MD_LightFunction) 결과를 곱한다
	UMaterialInterface* Mat = bLightFunction ? LightFunctionMaterial.Get() : nullptr;
	if (FlickerRect)
	{
		FlickerRect->SetLightFunctionMaterial(Mat);
	}
	for (USpotLightComponent* S : SpotLights)
	{
		S->SetLightFunctionMaterial(Mat);
	}
}

void ADemo1008_LightTypes::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One))   { for (auto& L : PointLights) { L->SetVisibility(!L->IsVisible()); } }
	if (WasKeyPressed(EKeys::Two))   { for (auto& L : SpotLights)  { L->SetVisibility(!L->IsVisible()); } }
	if (WasKeyPressed(EKeys::Three)) { for (auto& L : RectLights)  { L->SetVisibility(!L->IsVisible()); } }
	if (WasKeyPressed(EKeys::Four))  { bLightFunction = !bLightFunction; ApplyLightFunction(); }
	RenderKit::TickFlyCamera(this, Camera, DeltaSeconds, 500.f);

	auto OnOff = [](bool b) { return b ? TEXT("ON") : TEXT("off"); };
	Stats.Update(DeltaSeconds);
	SetStatus(FString::Printf(TEXT("Point %s · Spot %s (Inner %.0f° / Outer %.0f°) · Rect %s ×%d\nLight Function %s%s\n%s"),
		OnOff(PointLights.Num() && PointLights[0]->IsVisible()), OnOff(SpotLights.Num() && SpotLights[0]->IsVisible()),
		SpotInnerConeAngle, SpotOuterConeAngle, OnOff(RectLights.Num() && RectLights[0]->IsVisible()), RectLights.Num(),
		OnOff(bLightFunction), LightFunctionMaterial ? TEXT("") : TEXT(" (M_LightFunction_Flicker 없음 — 에셋 빌드 필요)"),
		*Stats.ToString(false)));
}
