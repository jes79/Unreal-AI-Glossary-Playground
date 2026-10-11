#include "Demo0521_Bloom.h"

#include "Components/StaticMeshComponent.h"
#include "EffectsDemoKit.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Texture2D.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0521_Bloom::ADemo0521_Bloom()
{
	DemoId = TEXT("0521");
	Controls = TEXT("1 : 표준 Bloom (Intensity 1.5, Threshold -1)   2 : Convolution Bloom (FFT)   3 : Bloom 끔\n")
		TEXT("관찰: 발광 세기 1·5·20·100 의 구. Threshold -1 은 문턱 없이 모든 밝기가 번진다.\n")
		TEXT("Convolution 은 커널 이미지(별 모양 빛 갈래)로 번지게 해 더 물리적인 렌즈 느낌 — 대신 더 비싸다");
	Showcase = TEXT("발광 세기가 다른 구들에 Bloom Intensity 1.5 · Threshold -1 표준 Bloom ↔ Convolution Bloom ↔ 끔 비교");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Night;
	Environment.ExposureEV100 = 3.f;
}

void ADemo0521_Bloom::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	const float Emissive[] = { 1.f, 5.f, 20.f, 100.f };
	for (int32 i = 0; i < 4; ++i)
	{
		const FVector Loc(0.f, -450.f + i * 300.f, 180.f);
		AStaticMeshActor* Ball = SpawnShape(World, EShape::Sphere, Loc, FVector(110.f), FLinearColor::White);
		SetColor(Ball->GetStaticMeshComponent(), FLinearColor(1.f, 0.55f, 0.2f), 0.5f, 0.f, Emissive[i]);
		SpawnLabel(World, FString::Printf(TEXT("Emissive ×%.0f"), Emissive[i]), Loc + FVector(0.f, 0.f, 140.f), FLinearColor::White, 18.f);
	}
	// 작은 흰 점광원 (Convolution 의 별 모양 갈래가 잘 보인다)
	AStaticMeshActor* Star = SpawnShape(World, EShape::Sphere, FVector(0.f, 0.f, 420.f), FVector(15.f), FLinearColor::White);
	SetColor(Star->GetStaticMeshComponent(), FLinearColor::White, 0.5f, 0.f, 400.f);

	Volume = EffectsKit::SpawnUnboundPostProcess(World);
	EffectsKit::SetupFixedCamera(this, FVector(-1000.f, 0.f, 260.f), FVector(0.f, 0.f, 240.f), 60.f);
	ApplyMode();
}

void ADemo0521_Bloom::ApplyMode()
{
	if (!Volume)
	{
		return;
	}
	FPostProcessSettings& S = Volume->Settings;
	S.bOverride_BloomMethod = true;
	S.bOverride_BloomIntensity = true;
	S.bOverride_BloomThreshold = true;
	S.BloomThreshold = BloomThreshold;   // -1 = 문턱 없음(모든 밝기가 번짐)
	if (Mode == 1)
	{
		S.BloomMethod = BM_SOG;           // 표준: 가우시안 합(Sum of Gaussians)
		S.BloomIntensity = BloomIntensity;
	}
	else if (Mode == 2)
	{
		S.BloomMethod = BM_FFT;           // Convolution: 커널 이미지로 FFT 컨볼루션
		S.BloomIntensity = BloomIntensity;
		S.bOverride_BloomConvolutionTexture = true;
		S.BloomConvolutionTexture = LoadAsset<UTexture2D>(TEXT("/Engine/EngineMaterials/DefaultBloomKernel"));
		S.bOverride_BloomConvolutionScatterDispersion = true;
		S.BloomConvolutionScatterDispersion = 1.f;
	}
	else
	{
		S.BloomMethod = BM_SOG;
		S.BloomIntensity = 0.f;
	}
}

void ADemo0521_Bloom::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One))   { Mode = 1; ApplyMode(); }
	if (WasKeyPressed(EKeys::Two))   { Mode = 2; ApplyMode(); }
	if (WasKeyPressed(EKeys::Three)) { Mode = 3; ApplyMode(); }
	const TCHAR* Names[] = { TEXT(""), TEXT("표준 Bloom (Sum of Gaussians)"), TEXT("Convolution Bloom (FFT, DefaultBloomKernel)"), TEXT("Bloom 끔 (Intensity 0)") };
	SetStatus(FString::Printf(TEXT("%s · Intensity %.1f · Threshold %.0f"), Names[Mode], Mode == 3 ? 0.f : BloomIntensity, BloomThreshold));
}
