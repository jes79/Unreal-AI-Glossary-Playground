#include "Demo0401_PBR.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"

using namespace GlossaryKit;

ADemo0401_PBR::ADemo0401_PBR()
{
	DemoId = TEXT("0401");
	Controls = TEXT("1 / 2 / 3 : 격자 Base Color 흰색·빨강·금색\n")
		TEXT("관찰: 오른쪽으로 갈수록 Metallic↑(주변을 비추는 금속), 아래로 갈수록 Roughness↑(반사가 뿌옇게)\n")
		TEXT("왼쪽 큰 구 = 요청 예문 M_PBR_RedMetal (상수 노드: 빨강 · Metallic 1 · Roughness 0.2)");
	Showcase = TEXT("Metallic×Roughness 5×5 구 격자와 빨강·Metallic 1·Roughness 0.2 금속 구");
	Environment.bClouds = true;
}

void ADemo0401_PBR::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	SetupFixedCamera(FVector(-1150.f, -150.f, 330.f), FVector(0.f, -150.f, 300.f), 60.f);

	// 반사에 비칠 거리: 뒤쪽 색 기둥
	for (int32 I = 0; I < 6; ++I)
	{
		const FLinearColor Colors[] = { Colors::Enemy, Colors::Item, Colors::Ally, Colors::Player, Colors::Purple, Colors::Accent };
		SpawnBlock(World, FVector(-1400.f + (I % 2) * 100.f, -900.f + I * 360.f, 0.f), FVector(80.f, 80.f, 500.f), Colors[I]);
	}

	// 5×5 격자: 열 = Metallic, 행 = Roughness. 모두 같은 M_PBR 의 MID 이고 값만 다르다
	UMaterialInterface* PBR = LoadMaterial(TEXT("M_PBR"));
	constexpr int32 N = 5;
	constexpr float Spacing = 115.f;
	const FVector Origin(0.f, 30.f, 560.f);
	for (int32 Row = 0; Row < N; ++Row)
	{
		for (int32 Col = 0; Col < N; ++Col)
		{
			const float Metallic = Col / float(N - 1);
			const float Roughness = Row / float(N - 1);
			AStaticMeshActor* Sphere = SpawnWithMaterial(EShape::Sphere, Origin + FVector(0.f, Col * Spacing, -Row * Spacing), FVector(95.f), PBR);
			UMaterialInstanceDynamic* MID = Sphere->GetStaticMeshComponent()->CreateAndSetMaterialInstanceDynamic(0);
			MID->SetScalarParameterValue(TEXT("Metallic"), Metallic);
			MID->SetScalarParameterValue(TEXT("Roughness"), Roughness);
			GridMaterials.Add(MID);
		}
	}
	for (int32 I = 0; I < N; ++I)
	{
		const float V = I / float(N - 1);
		SpawnLabel(World, FString::Printf(TEXT("M %.2g"), V), Origin + FVector(0.f, I * Spacing, 85.f), FLinearColor::White, 14.f);
		SpawnLabel(World, FString::Printf(TEXT("R %.2g"), V), Origin + FVector(0.f, N * Spacing - 25.f, -I * Spacing), FLinearColor::White, 14.f);
	}
	SpawnLabel(World, TEXT("Metallic →"), Origin + FVector(0.f, 2 * Spacing, 140.f), Colors::Item, 18.f);
	SpawnLabel(World, TEXT("Roughness ↓"), Origin + FVector(0.f, N * Spacing + 40.f, -5.f * Spacing + 30.f), Colors::Item, 18.f);

	// 요청 예문: 상수 노드로 만든 빨간 금속 (Metallic 1, Roughness 0.2)
	SpawnWithMaterial(EShape::Sphere, FVector(0.f, -560.f, 140.f), FVector(240.f), LoadMaterial(TEXT("M_PBR_RedMetal")));
	SpawnLabel(World, TEXT("M_PBR_RedMetal\n빨강 · Metallic 1 · Roughness 0.2"), FVector(0.f, -560.f, 300.f), Colors::Item, 16.f);

	ApplyBaseColor();
}

void ADemo0401_PBR::ApplyBaseColor()
{
	for (UMaterialInstanceDynamic* MID : GridMaterials)
	{
		MID->SetVectorParameterValue(TEXT("BaseColor"), BaseColor);
	}
}

void ADemo0401_PBR::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One))   { BaseColor = FLinearColor(0.9f, 0.9f, 0.9f); ColorName = TEXT("흰색"); ApplyBaseColor(); }
	if (WasKeyPressed(EKeys::Two))   { BaseColor = FLinearColor(0.9f, 0.04f, 0.03f); ColorName = TEXT("빨강"); ApplyBaseColor(); }
	if (WasKeyPressed(EKeys::Three)) { BaseColor = FLinearColor(1.f, 0.7f, 0.25f); ColorName = TEXT("금색"); ApplyBaseColor(); }
	SetStatus(FString::Printf(TEXT("격자 Base Color %s · Metallic 0~1 (가로) × Roughness 0~1 (세로)\n요청 구: 상수 노드 빨강 · Metallic 1 · Roughness 0.2"),
		*ColorName));
}
