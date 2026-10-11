#include "Demo0517_Weather.h"

#include "Camera/PlayerCameraManager.h"
#include "EffectsDemoKit.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "NiagaraComponent.h"

using namespace GlossaryKit;

ADemo0517_Weather::ADemo0517_Weather()
{
	DemoId = TEXT("0517");
	Controls = TEXT("WASD 이동 · 마우스 시점   R : 비 ON/OFF\n")
		TEXT("관찰: 비 시스템은 카메라 XY 를 매 프레임 따라가므로 어디로 가도 주변 40m 에만 비가 내린다(맵 전체에 뿌리지 않음).\n")
		TEXT("빗줄기는 GPU Sim 이미터, 바닥에는 물방울이 튀는 Splash 이미터");
	Showcase = TEXT("카메라를 따라다니는 GPU 비 이미터(초당 6000) + 바닥에 튀는 물방울, 이동해도 항상 주변에만 비");
	Environment.Sky = EGlossarySky::Sunset;
	Environment.bClouds = true;
	Environment.ExposureEV100 = 8.f;
}

void ADemo0517_Weather::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	// 비 시스템은 데모 액터에 붙이되 Absolute 위치로 두고 매 프레임 카메라 아래로 옮긴다
	Rain = EffectsKit::AddNiagara(this, TEXT("NS_Rain"), FVector::ZeroVector);
	if (Rain)
	{
		Rain->SetUsingAbsoluteLocation(true);
		Rain->SetVariableFloat(TEXT("RainRate"), RainRate);
	}
	for (int32 i = 0; i < 8; ++i)
	{
		SpawnBlock(World, FVector(400.f + (i % 4) * 500.f, -800.f + (i / 4) * 1600.f, 0.f), FVector(200.f, 200.f, 150.f + i * 40.f), Colors::Wall);
	}
}

void ADemo0517_Weather::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::R))
	{
		bRainOn = !bRainOn;
		if (Rain) { bRainOn ? Rain->Activate(true) : Rain->Deactivate(); }
	}
	FVector CamLoc = FVector::ZeroVector;
	if (APlayerController* PC = GetPC())
	{
		CamLoc = PC->PlayerCameraManager->GetCameraLocation();
	}
	if (Rain)
	{
		// 카메라를 따라간다: XY 만 따라가고 높이는 바닥(z=0) 기준 — 물방울 Splash 가 바닥 높이에 생기도록
		Rain->SetWorldLocation(FVector(CamLoc.X, CamLoc.Y, 0.f));
	}
	SetStatus(FString::Printf(TEXT("비 %s · Spawn Rate %.0f /초 (GPU) · 비 중심 = 카메라 (%.0f, %.0f)"),
		bRainOn ? TEXT("ON") : TEXT("OFF"), RainRate, CamLoc.X, CamLoc.Y));
}
