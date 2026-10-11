#include "Demo0508_CurlNoise.h"

#include "EffectsDemoKit.h"
#include "GlossaryKit.h"
#include "NiagaraComponent.h"

using namespace GlossaryKit;

ADemo0508_CurlNoise::ADemo0508_CurlNoise()
{
	DemoId = TEXT("0508");
	Controls = TEXT("C : 비교 보기 ↔ Curl Noise 만 보기\n")
		TEXT("관찰: 오른쪽 반딧불은 Curl Noise Force 가 만드는 소용돌이 흐름을 따라 곡선으로 떠다니고,\n")
		TEXT("왼쪽은 같은 입자에 처음 속도만 있어 직선으로 흩어진다");
	Showcase = TEXT("Curl Noise Force 로 곡선을 그리며 떠다니는 반딧불 vs 힘 없는 직선 이동");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Night;
	Environment.ExposureEV100 = 12.f;   // 환경 노출 보정 = EV100-10 이라 값이 클수록 밝다 (밤 장면을 조금 밝게)
}

void ADemo0508_CurlNoise::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	WithoutCurl = EffectsKit::AddNiagara(this, TEXT("NS_FirefliesNoCurl"), FVector(0.f, -380.f, 200.f));
	WithCurl = EffectsKit::AddNiagara(this, TEXT("NS_FirefliesCurl"), FVector(0.f, 380.f, 200.f));
	SpawnLabel(World, TEXT("힘 없음 — 직선"), FVector(0.f, -380.f, 450.f), FLinearColor::White, 20.f);
	SpawnLabel(World, TEXT("Curl Noise Force — 소용돌이"), FVector(0.f, 380.f, 450.f), Colors::Item, 20.f);
	// 풀숲 느낌의 낮은 덤불
	for (int32 i = 0; i < 12; ++i)
	{
		const float Y = -900.f + i * 160.f;
		SpawnShape(World, EShape::Sphere, FVector(250.f + (i % 3) * 60.f, Y, 30.f), FVector(140.f, 140.f, 70.f), FLinearColor(0.03f, 0.12f, 0.04f));
	}
	EffectsKit::SetupFixedCamera(this, FVector(-1100.f, -420.f, 260.f), FVector(0.f, -260.f, 220.f), 70.f);
}

void ADemo0508_CurlNoise::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::C))
	{
		bShowBoth = !bShowBoth;
		if (WithoutCurl) WithoutCurl->SetVisibility(bShowBoth);
	}
	SetStatus(TEXT("Curl Noise Force: 3D 노이즈 장의 회전(curl) = 발산이 없는 흐름 → 입자가 뭉치거나 흩어지지 않고 부드럽게 휘돈다"));
}
