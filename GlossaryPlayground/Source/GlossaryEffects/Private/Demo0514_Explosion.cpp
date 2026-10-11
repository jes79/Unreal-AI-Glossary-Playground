#include "Demo0514_Explosion.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/PointLightComponent.h"
#include "EffectsDemoKit.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "NiagaraComponent.h"

using namespace GlossaryKit;

ADemo0514_Explosion::ADemo0514_Explosion()
{
	DemoId = TEXT("0514");
	Controls = TEXT("Space : 폭발   K : 카메라 쉐이크 ON/OFF (4초마다 자동 폭발)\n")
		TEXT("관찰: 한 시스템 안의 4개 이미터가 시간차로 터진다 — 0초 섬광(Flash) · 0초 파편(Debris)\n")
		TEXT("· 0.05초 충격파 링(Shockwave) · 0.1초 연기(Smoke). 동시에 섬광 라이트와 카메라 쉐이크");
	Showcase = TEXT("섬광·파편·충격파 링·연기 4개 이미터가 시간차로 터지는 폭발 + 섬광 라이트 + 카메라 쉐이크");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Sunset;
}

void ADemo0514_Explosion::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	Explosion = EffectsKit::AddNiagara(this, TEXT("NS_Explosion"), FVector(0.f, 0.f, 30.f), nullptr, false);
	FlashLight = EffectsKit::AddPointLight(this, FVector(0.f, 0.f, 150.f), FLinearColor(1.f, 0.6f, 0.3f), 0.f, 2500.f);
	// 주변 소품 (빛을 받는 상자들)
	for (int32 i = 0; i < 6; ++i)
	{
		const float A = i * 60.f * PI / 180.f;
		SpawnBlock(World, FVector(FMath::Cos(A) * 600.f, FMath::Sin(A) * 600.f, 0.f), FVector(120.f, 120.f, 120.f + (i % 3) * 60.f), Colors::Wall);
	}
	EffectsKit::SetupFixedCamera(this, FVector(-1300.f, -300.f, 380.f), FVector(0.f, 0.f, 220.f), 70.f);
	Timer = AutoInterval - 0.8f;   // 0.8초, 4.8초, 8.8초… 에 폭발
}

void ADemo0514_Explosion::Explode()
{
	if (Explosion)
	{
		Explosion->ResetSystem();   // 섬광·파편·충격파·연기 이미터가 각자의 Spawn Time 에 터진다
	}
	if (bShake)
	{
		if (APlayerController* PC = GetPC())
		{
			PC->PlayerCameraManager->StartCameraShake(UGlossaryExplosionShake::StaticClass(), 1.f);
		}
	}
	SinceExplode = 0.f;
	Timer = 0.f;
}

void ADemo0514_Explosion::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Timer += DeltaSeconds;
	SinceExplode += DeltaSeconds;
	if (WasKeyPressed(EKeys::K)) { bShake = !bShake; }
	if (WasKeyPressed(EKeys::SpaceBar) || Timer >= AutoInterval)
	{
		Explode();
	}
	// 섬광 라이트: 0.25초 동안 빠르게 꺼진다
	if (FlashLight)
	{
		const float Light = FMath::Clamp(1.f - SinceExplode / 0.25f, 0.f, 1.f);
		FlashLight->SetIntensity(Light * Light * 20000.f);
	}
	const TCHAR* Phase = SinceExplode < 0.05f ? TEXT("섬광 + 파편") : SinceExplode < 0.1f ? TEXT("충격파 링") : SinceExplode < 3.f ? TEXT("연기") : TEXT("대기");
	SetStatus(FString::Printf(TEXT("폭발 후 %.2f초 · 단계: %s · 카메라 쉐이크 %s"), FMath::Min(SinceExplode, 99.f), Phase, bShake ? TEXT("ON") : TEXT("OFF")));
}
