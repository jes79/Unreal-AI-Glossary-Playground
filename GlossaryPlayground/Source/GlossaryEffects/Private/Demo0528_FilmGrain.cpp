#include "Demo0528_FilmGrain.h"

#include "Components/PointLightComponent.h"
#include "EffectsDemoKit.h"
#include "Engine/PostProcessVolume.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0528_FilmGrain::ADemo0528_FilmGrain()
{
	DemoId = TEXT("0528");
	Controls = TEXT("G : Film Grain ON/OFF\n")
		TEXT("관찰: Film Grain Intensity 0.3 — 화면 전체에 매 프레임 바뀌는 미세한 입자 노이즈가 깔려\n")
		TEXT("어두운 복도가 오래된 필름·감시 카메라처럼 거칠어진다. 어두운 부분일수록 잘 보인다");
	Showcase = TEXT("깜빡이는 전구 하나뿐인 어두운 복도에 Film Grain Intensity 0.3, ON/OFF 비교");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Indoor;
	Environment.ExposureEV100 = 3.f;
}

void ADemo0528_FilmGrain::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	// 복도: 양쪽 벽 + 천장, 문틀처럼 기둥
	const FLinearColor Wall(0.35f, 0.33f, 0.3f);
	SpawnBlock(World, FVector(1000.f, -250.f, 0.f), FVector(2400.f, 40.f, 350.f), Wall);
	SpawnBlock(World, FVector(1000.f, 250.f, 0.f), FVector(2400.f, 40.f, 350.f), Wall);
	SpawnBlock(World, FVector(1000.f, 0.f, 350.f), FVector(2400.f, 540.f, 30.f), Wall);
	for (int32 i = 0; i < 6; ++i)
	{
		SpawnBlock(World, FVector(200.f + i * 400.f, -220.f, 0.f), FVector(40.f, 40.f, 350.f), Colors::Dark);
		SpawnBlock(World, FVector(200.f + i * 400.f, 220.f, 0.f), FVector(40.f, 40.f, 350.f), Colors::Dark);
	}
	SpawnShape(World, EShape::Cube, FVector(1200.f, 120.f, 60.f), FVector(80.f, 80.f, 120.f), FLinearColor(0.4f, 0.05f, 0.05f));
	Bulb = EffectsKit::AddPointLight(this, FVector(700.f, 0.f, 320.f), FLinearColor(1.f, 0.85f, 0.6f), 30.f, 1500.f);

	Volume = EffectsKit::SpawnUnboundPostProcess(World);
	Volume->Settings.bOverride_FilmGrainIntensity = true;
	Volume->Settings.FilmGrainIntensity = FilmGrainIntensity;
	EffectsKit::SetupFixedCamera(this, FVector(-200.f, 0.f, 170.f), FVector(1500.f, 0.f, 150.f), 75.f);
}

void ADemo0528_FilmGrain::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	if (WasKeyPressed(EKeys::G))
	{
		bGrain = !bGrain;
		if (Volume) Volume->Settings.FilmGrainIntensity = bGrain ? FilmGrainIntensity : 0.f;
	}
	// 불안하게 깜빡이는 전구
	if (Bulb)
	{
		const bool bOff = FMath::Fmod(Time, 3.1f) > 2.8f || FMath::Fmod(Time, 1.7f) > 1.62f;
		Bulb->SetIntensity(bOff ? 2.f : 30.f);
	}
	SetStatus(FString::Printf(TEXT("Film Grain %s · Intensity %.1f"), bGrain ? TEXT("ON") : TEXT("OFF"), bGrain ? FilmGrainIntensity : 0.f));
}
