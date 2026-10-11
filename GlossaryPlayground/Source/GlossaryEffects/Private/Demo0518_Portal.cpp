#include "Demo0518_Portal.h"

#include "Components/PointLightComponent.h"
#include "EffectsDemoKit.h"
#include "GlossaryKit.h"
#include "NiagaraComponent.h"

using namespace GlossaryKit;

ADemo0518_Portal::ADemo0518_Portal()
{
	DemoId = TEXT("0518");
	Controls = TEXT("관찰: 링 둘레에서 태어난 입자가 Vortex Velocity 로 포탈 축을 따라 돌면서\n")
		TEXT("Point Attraction Force 로 중심에 빨려 들어가 소용돌이가 된다. 중심에 가까울수록 빠르고 작아진다");
	Showcase = TEXT("Vortex Velocity + Point Attraction 으로 입자가 돌면서 중심으로 빨려 드는 소용돌이 포탈");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Night;
	Environment.ExposureEV100 = 12.f;   // 환경 노출 보정 = EV100-10 이라 값이 클수록 밝다 (밤 장면을 조금 밝게)
}

void ADemo0518_Portal::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	const FVector Center(0.f, 0.f, 260.f);
	// 돌 아치 (구 18개로 둥근 틀)
	for (int32 i = 0; i < 18; ++i)
	{
		const float A = i * 20.f * PI / 180.f;
		SpawnShape(World, EShape::Sphere, Center + FVector(0.f, FMath::Cos(A) * 250.f, FMath::Sin(A) * 250.f), FVector(70.f), Colors::Wall);
	}
	// 포탈 축 = 액터 로컬 X (카메라 쪽). 시스템은 로컬 공간 시뮬레이션
	Portal = EffectsKit::AddNiagara(this, TEXT("NS_Portal"), Center, nullptr, true, FRotator(90.f, 0.f, 0.f));   // Pitch 90: 로컬 Z(소용돌이 축) → 월드 -X(카메라 쪽)
	Glow = EffectsKit::AddPointLight(this, Center + FVector(-60.f, 0.f, 0.f), FLinearColor(0.5f, 0.2f, 1.f), 80.f, 900.f);
	EffectsKit::SetupFixedCamera(this, FVector(-900.f, -250.f, 300.f), Center, 60.f);
}

void ADemo0518_Portal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	if (Glow)
	{
		Glow->SetIntensity(80.f * (0.85f + 0.15f * FMath::Sin(Time * 3.f)));
	}
	SetStatus(TEXT("Vortex Velocity (축 X, 회전) + Point Attraction Force (중심으로 당김) + Scale Size (작아짐)"));
}
