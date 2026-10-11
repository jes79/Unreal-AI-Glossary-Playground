#include "Demo0529_PostProcessMaterial.h"

#include "EffectsDemoKit.h"
#include "Engine/PostProcessVolume.h"
#include "GlossaryKit.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

using namespace GlossaryKit;

ADemo0529_PostProcessMaterial::ADemo0529_PostProcessMaterial()
{
	DemoId = TEXT("0529");
	Controls = TEXT("WASD 이동   C : Post Process Material ON/OFF\n")
		TEXT("관찰: M_PP_GrayScanline(Material Domain = Post Process)이 PostProcessInput0(지금 화면)을 받아\n")
		TEXT("흑백으로 바꾸고 가로 스캔라인을 그린다. 볼륨의 Blendables 에 넣으면 화면 전체에 적용된다");
	Showcase = TEXT("Post Process 도메인 머티리얼(흑백 + 스캔라인)을 Blendables 에 넣어 화면 전체 적용, ON/OFF");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-300.f, 0.f, 100.f));
}

void ADemo0529_PostProcessMaterial::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	const FLinearColor Palette[] = { Colors::Enemy, Colors::Item, Colors::Ally, Colors::Cyan, Colors::Player, Colors::Purple };
	for (int32 i = 0; i < 6; ++i)
	{
		SpawnBlock(World, FVector(500.f + (i % 2) * 300.f, -750.f + i * 300.f, 0.f), FVector(160.f, 160.f, 200.f + i * 50.f), Palette[i]);
	}
	Volume = EffectsKit::SpawnUnboundPostProcess(World);
	if (UMaterialInterface* PPMat = LoadAsset<UMaterialInterface>(EffectsKit::AssetPath(TEXT("M_PP_GrayScanline"))))
	{
		ScanlineMID = UMaterialInstanceDynamic::Create(PPMat, this);
		// 핵심: Blendables 배열에 (가중치, 머티리얼) 추가
		Volume->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, ScanlineMID));
	}
}

void ADemo0529_PostProcessMaterial::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	if (WasKeyPressed(EKeys::C))
	{
		bOn = !bOn;
		if (Volume && Volume->Settings.WeightedBlendables.Array.Num() > 0)
		{
			Volume->Settings.WeightedBlendables.Array[0].Weight = bOn ? 1.f : 0.f;   // 가중치 0 = 끔
		}
	}
	if (ScanlineMID)
	{
		ScanlineMID->SetScalarParameterValue(TEXT("ScrollTime"), Time);   // 스캔라인이 천천히 흐른다
	}
	SetStatus(FString::Printf(TEXT("Post Process Material %s · Blendables[0] = M_PP_GrayScanline (Weight %.0f) · 스캔라인 270줄"),
		bOn ? TEXT("ON") : TEXT("OFF"), bOn ? 1.f : 0.f));
}
