#include "Demo0501_NiagaraSystem.h"

#include "Components/PointLightComponent.h"
#include "EffectsDemoKit.h"
#include "GlossaryKit.h"
#include "NiagaraComponent.h"

using namespace GlossaryKit;

ADemo0501_NiagaraSystem::ADemo0501_NiagaraSystem()
{
	DemoId = TEXT("0501");
	Controls = TEXT("1 : Flames 이미터 ON/OFF   2 : Embers 이미터 ON/OFF   3 : Smoke 이미터 ON/OFF\n")
		TEXT("관찰: 하나의 Niagara System(NS_Campfire) 안에 불꽃·불씨·연기 이미터 3개가 들어 있다.\n")
		TEXT("이미터를 하나씩 끄면 모닥불이 어떤 층으로 만들어졌는지 보인다");
	Showcase = TEXT("불꽃·불씨·연기 3개 이미터로 된 Niagara 모닥불 + 깜빡이는 Point Light, 이미터별 ON/OFF");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Night;
	Environment.ExposureEV100 = 10.f;
	PrimaryActorTick.bCanEverTick = true;
}

void ADemo0501_NiagaraSystem::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 장작: 엇갈린 원기둥 4개 + 둘레 돌
	for (int32 i = 0; i < 4; ++i)
	{
		const float Yaw = i * 45.f;
		SpawnShape(World, EShape::Cylinder, FVector(0.f, 0.f, 12.f), FVector(16.f, 16.f, 120.f), FLinearColor(0.18f, 0.09f, 0.04f), FRotator(90.f, Yaw, 0.f));
	}
	for (int32 i = 0; i < 10; ++i)
	{
		const float A = i * 36.f * PI / 180.f;
		SpawnShape(World, EShape::Sphere, FVector(FMath::Cos(A) * 85.f, FMath::Sin(A) * 85.f, 8.f), FVector(34.f, 34.f, 24.f), Colors::Wall);
	}

	// 핵심: Niagara System 하나를 컴포넌트로 붙이면 안의 이미터 3개가 같이 재생된다
	Campfire = EffectsKit::AddNiagara(this, TEXT("NS_Campfire"), FVector(0.f, 0.f, 10.f));
	FireLight = EffectsKit::AddPointLight(this, FVector(0.f, 0.f, 80.f), FLinearColor(1.f, 0.55f, 0.2f), 60.f, 1200.f);
	ApplyRates();

	SpawnLabel(World, TEXT("NS_Campfire = Flames + Embers + Smoke"), FVector(0.f, 0.f, 330.f), Colors::Item, 20.f);
	EffectsKit::SetupFixedCamera(this, FVector(-520.f, -120.f, 200.f), FVector(0.f, 0.f, 130.f), 60.f);
}

void ADemo0501_NiagaraSystem::ApplyRates()
{
	if (!Campfire)
	{
		return;
	}
	// User 파라미터 이름은 "User." 접두사 없이 쓴다
	Campfire->SetVariableFloat(TEXT("FlameRate"), bEnabled[0] ? FlameRate : 0.f);
	Campfire->SetVariableFloat(TEXT("EmberRate"), bEnabled[1] ? EmberRate : 0.f);
	Campfire->SetVariableFloat(TEXT("SmokeRate"), bEnabled[2] ? SmokeRate : 0.f);
}

void ADemo0501_NiagaraSystem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;

	const FKey Keys[3] = { EKeys::One, EKeys::Two, EKeys::Three };
	for (int32 i = 0; i < 3; ++i)
	{
		if (WasKeyPressed(Keys[i]))
		{
			bEnabled[i] = !bEnabled[i];
			ApplyRates();
		}
	}

	// 불빛 깜빡임: 서로 다른 주기의 사인 두 개를 섞어 불규칙하게
	if (FireLight)
	{
		const float Flicker = 0.75f + 0.15f * FMath::Sin(Time * 13.f) + 0.1f * FMath::Sin(Time * 29.f + 1.3f);
		FireLight->SetIntensity(bEnabled[0] ? 60.f * Flicker : 4.f);
	}

	SetStatus(FString::Printf(TEXT("Flames %s (%.0f/s) · Embers %s (%.0f/s) · Smoke %s (%.0f/s)"),
		bEnabled[0] ? TEXT("ON") : TEXT("OFF"), FlameRate, bEnabled[1] ? TEXT("ON") : TEXT("OFF"), EmberRate,
		bEnabled[2] ? TEXT("ON") : TEXT("OFF"), SmokeRate));
}
