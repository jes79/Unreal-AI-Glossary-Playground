#include "Demo0523_ChromaticAberration.h"

#include "EffectsDemoKit.h"
#include "Engine/PostProcessVolume.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0523_ChromaticAberration::ADemo0523_ChromaticAberration()
{
	DemoId = TEXT("0523");
	Controls = TEXT("Space : 피격 (2초마다 자동)   C : Scene Fringe 5 고정(비교용)\n")
		TEXT("관찰: 맞는 순간 0.2초 동안 Scene Fringe Intensity 가 5 가 되어 화면 가장자리 물체의 윤곽이\n")
		TEXT("빨강·파랑으로 갈라진다(렌즈 색수차). 가운데보다 가장자리일수록 크게 벌어진다");
	Showcase = TEXT("피격 순간 0.2초 Scene Fringe Intensity 5 로 가장자리 색 번짐, C 키로 고정 비교");
	PlayerPawnClass = nullptr;
}

void ADemo0523_ChromaticAberration::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	// 흰 기둥과 검은 기둥을 번갈아 — 대비가 큰 윤곽에서 색수차가 잘 보인다
	for (int32 i = 0; i < 11; ++i)
	{
		const FLinearColor Color = i % 2 == 0 ? FLinearColor(0.95f, 0.95f, 0.95f) : Colors::Dark;
		SpawnBlock(World, FVector(400.f, -1000.f + i * 200.f, 0.f), FVector(120.f, 120.f, 500.f), Color);
	}
	Volume = EffectsKit::SpawnUnboundPostProcess(World);
	Volume->Settings.bOverride_SceneFringeIntensity = true;
	Volume->Settings.SceneFringeIntensity = 0.f;
	EffectsKit::SetupFixedCamera(this, FVector(-500.f, 0.f, 250.f), FVector(400.f, 0.f, 250.f), 90.f);
}

void ADemo0523_ChromaticAberration::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AutoTimer += DeltaSeconds;
	if (WasKeyPressed(EKeys::C)) bHold = !bHold;
	if (WasKeyPressed(EKeys::SpaceBar) || AutoTimer >= 2.f)
	{
		AutoTimer = 0.f;
		HitTimer = HitDuration;   // 0.2초 동안
	}
	HitTimer = FMath::Max(0.f, HitTimer - DeltaSeconds);
	// 핵심: 피격 중에는 5, 끝나면 0
	const float Fringe = (bHold || HitTimer > 0.f) ? HitFringeIntensity : 0.f;
	if (Volume)
	{
		Volume->Settings.SceneFringeIntensity = Fringe;
	}
	SetStatus(FString::Printf(TEXT("Scene Fringe Intensity %.1f %s"), Fringe,
		bHold ? TEXT("(C: 고정)") : HitTimer > 0.f ? TEXT("(피격 0.2초)") : TEXT("(평소)")));
}
