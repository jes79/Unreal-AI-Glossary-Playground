#include "Demo0420_Refraction.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"

using namespace GlossaryKit;

ADemo0420_Refraction::ADemo0420_Refraction()
{
	DemoId = TEXT("0420");
	Controls = TEXT("C : 아지랑이 ON/OFF    [ / ] : HazeStrength −/+ 0.01\n")
		TEXT("관찰: 판 자체는 투명(Opacity 0) — Refraction 이 뒤 화면을 노이즈만큼 밀어서 줄무늬 벽이 위로 일렁인다");
	Showcase = TEXT("노이즈 Panner 로 Refraction(2D Offset)을 흔들어 모닥불·배기구 위 열 아지랑이, ON/OFF 비교");
	Environment.Sky = EGlossarySky::Sunset;
}

void ADemo0420_Refraction::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	SetupFixedCamera(FVector(-950.f, 30.f, 230.f), FVector(0.f, 30.f, 170.f), 60.f);

	// 뒤 배경: 세로 줄무늬 벽 (왜곡이 잘 보이게)
	for (int32 I = 0; I < 16; ++I)
	{
		SpawnBlock(World, FVector(350.f, -700.f + I * 90.f, 0.f), FVector(20.f, 45.f, 450.f), I % 2 ? Colors::Neutral : Colors::Dark);
	}

	UMaterialInterface* Haze = LoadMaterial(TEXT("M_HeatHaze"));
	const FRotator FaceCamera(90.f, 0.f, 0.f);
	auto AddHeatSource = [&](const FVector& Base, const FLinearColor& GlowColor, const FString& Label)
	{
		// 열원: 발광 원기둥 (Emissive)
		AStaticMeshActor* Source = SpawnShape(World, EShape::Cylinder, Base + FVector(0.f, 0.f, 25.f), FVector(110.f, 110.f, 50.f), GlowColor);
		SetColor(Source->GetStaticMeshComponent(), GlowColor, 0.5f, 0.f, 15.f);
		// 그 위 아지랑이 판 (Translucent + Refraction)
		AStaticMeshActor* Plane = SpawnWithMaterial(EShape::Plane, Base + FVector(0.f, 0.f, 210.f), FVector(320.f, 220.f, 1.f), Haze, FaceCamera);
		Plane->SetActorEnableCollision(false);
		Materials.Add(Plane->GetStaticMeshComponent()->CreateAndSetMaterialInstanceDynamic(0));
		SpawnLabel(World, Label, Base + FVector(0.f, 0.f, -10.f), FLinearColor::White, 14.f);
	};
	AddHeatSource(FVector(0.f, -60.f, 0.f), FLinearColor(1.f, 0.35f, 0.03f), TEXT("모닥불 + M_HeatHaze"));
	AddHeatSource(FVector(0.f, 330.f, 0.f), FLinearColor(1.f, 0.1f, 0.02f), TEXT("배기구 + M_HeatHaze"));
}

void ADemo0420_Refraction::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::C)) { bHaze = !bHaze; }
	if (WasKeyPressed(EKeys::LeftBracket)) { HazeStrength = FMath::Max(0.f, HazeStrength - 0.01f); }
	if (WasKeyPressed(EKeys::RightBracket)) { HazeStrength += 0.01f; }
	for (UMaterialInstanceDynamic* MID : Materials)
	{
		MID->SetScalarParameterValue(TEXT("HazeStrength"), bHaze ? HazeStrength : 0.f);
	}
	SetStatus(FString::Printf(TEXT("열 아지랑이 %s · HazeStrength %.2f (Refraction 2D Offset, 판 Opacity 0)"),
		bHaze ? TEXT("ON") : TEXT("OFF"), HazeStrength));
}
