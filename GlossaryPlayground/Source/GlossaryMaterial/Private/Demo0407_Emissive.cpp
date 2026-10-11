#include "Demo0407_Emissive.h"

#include "Components/PostProcessComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"

using namespace GlossaryKit;

ADemo0407_Emissive::ADemo0407_Emissive()
{
	DemoId = TEXT("0407");
	Controls = TEXT("B : Bloom ON/OFF    [ / ] : Emissive 세기 ×0.5 / ×2\n")
		TEXT("관찰: 주황 × 20 은 화면 최대 밝기(1)를 훌쩍 넘는 HDR 값 → Bloom 으로 번지고, Lumen 이 바닥을 주황빛으로 비춘다");
	Showcase = TEXT("Emissive 주황×20 오브·막대가 Bloom 으로 번지고 바닥을 비춤, Bloom ON/OFF·세기 조절");
	Environment.Sky = EGlossarySky::Night;

	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("BloomPostProcess"));
	PostProcess->SetupAttachment(Root);
	PostProcess->bUnbound = true;
	PostProcess->Priority = 10.f;
}

void ADemo0407_Emissive::BeginPlay()
{
	Super::BeginPlay();
	SetupFixedCamera(FVector(-800.f, -250.f, 260.f), FVector(0.f, -250.f, 120.f), 60.f);

	UMaterialInterface* Emissive = LoadMaterial(TEXT("M_Emissive"));
	auto AddGlow = [&](EShape Shape, const FVector& Location, const FVector& Size, const FRotator& Rotation = FRotator::ZeroRotator)
	{
		AStaticMeshActor* Actor = SpawnWithMaterial(Shape, Location, Size, Emissive, Rotation);
		Materials.Add(Actor->GetStaticMeshComponent()->CreateAndSetMaterialInstanceDynamic(0));
	};
	// 받침대 위 발광 오브 + 바닥에 꽂힌 발광 막대들
	SpawnBlock(GetWorld(), FVector(0.f, -60.f, 0.f), FVector(120.f, 120.f, 90.f), Colors::Wall);
	AddGlow(EShape::Sphere, FVector(0.f, -60.f, 160.f), FVector(110.f));
	for (int32 I = 0; I < 4; ++I)
	{
		AddGlow(EShape::Cube, FVector(80.f, 180.f + I * 110.f, 90.f + I * 20.f), FVector(18.f, 18.f, 180.f + I * 40.f), FRotator(0.f, 0.f, -8.f + I * 6.f));
	}
	// 비교용: 같은 색이지만 Emissive 0 인 구
	AStaticMeshActor* Plain = SpawnShape(GetWorld(), EShape::Sphere, FVector(0.f, -330.f, 70.f), FVector(110.f), EmissiveColor);
	SpawnLabel(GetWorld(), TEXT("Emissive 0 (같은 주황 Base Color)"), Plain->GetActorLocation() + FVector(0.f, 0.f, 100.f), FLinearColor::White, 14.f);
	SpawnLabel(GetWorld(), TEXT("Emissive 주황 × 20"), FVector(0.f, -60.f, 270.f), Colors::Accent, 16.f);
	Apply();
}

void ADemo0407_Emissive::Apply()
{
	for (UMaterialInstanceDynamic* MID : Materials)
	{
		MID->SetVectorParameterValue(TEXT("EmissiveColor"), EmissiveColor);
		MID->SetScalarParameterValue(TEXT("EmissiveStrength"), EmissiveStrength);
	}
	PostProcess->Settings.bOverride_BloomIntensity = true;
	PostProcess->Settings.BloomIntensity = bBloom ? BloomIntensity : 0.f;
}

void ADemo0407_Emissive::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	bool bChanged = false;
	if (WasKeyPressed(EKeys::B)) { bBloom = !bBloom; bChanged = true; }
	if (WasKeyPressed(EKeys::LeftBracket)) { EmissiveStrength = FMath::Max(0.25f, EmissiveStrength * 0.5f); bChanged = true; }
	if (WasKeyPressed(EKeys::RightBracket)) { EmissiveStrength = FMath::Min(640.f, EmissiveStrength * 2.f); bChanged = true; }
	if (bChanged)
	{
		Apply();
	}
	SetStatus(FString::Printf(TEXT("Emissive = 주황 × %.2g  (채널 최대값 %.1f)\nBloom %s"),
		EmissiveStrength, EmissiveStrength * EmissiveColor.GetMax(), bBloom ? TEXT("ON") : TEXT("OFF")));
}
