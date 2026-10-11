#include "Demo0409_Hologram.h"

#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"

using namespace GlossaryKit;

ADemo0409_Hologram::ADemo0409_Hologram()
{
	DemoId = TEXT("0409");
	Controls = TEXT("C : 홀로그램 ↔ 원래 머티리얼    [ / ] : 스캔 속도 −/+    F : 깜빡임 ON/OFF\n")
		TEXT("관찰: 반투명(Translucent)·Unlit 이라 뒤가 비치고 조명과 무관하게 빛남. 줄무늬는 월드 높이 기준으로 위로 흐른다");
	Showcase = TEXT("Translucent·Unlit 홀로그램: 위로 흐르는 스캔라인 + Fresnel 테두리 + 깜빡임, 원래 머티리얼과 비교");
	Environment.Sky = EGlossarySky::Night;
}

void ADemo0409_Hologram::BeginPlay()
{
	Super::BeginPlay();
	SetupFixedCamera(FVector(-820.f, 20.f, 210.f), FVector(0.f, 20.f, 120.f), 60.f);

	// 투사기 받침대 + 발광 링
	SpawnShape(GetWorld(), EShape::Cylinder, FVector(0.f, 0.f, 10.f), FVector(170.f, 170.f, 20.f), Colors::Dark);
	AStaticMeshActor* Ring = SpawnShape(GetWorld(), EShape::Cylinder, FVector(0.f, 0.f, 22.f), FVector(150.f, 150.f, 6.f), HoloColor);
	SetColor(Ring->GetStaticMeshComponent(), HoloColor, 0.5f, 0.f, 4.f);

	UMaterialInterface* Hologram = LoadMaterial(TEXT("M_Hologram"));
	Figure = SpawnMannequin(FVector(0.f, 0.f, 25.f), FRotator(0.f, 70.f, 0.f));
	USkeletalMeshComponent* Mesh = Figure->GetSkeletalMeshComponent();
	for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
	{
		OriginalMaterials.Add(Mesh->GetMaterial(Slot));
		Materials.Add(UMaterialInstanceDynamic::Create(Hologram, this));
	}
	// 홀로그램 구 (뒤 배경이 비치는지 보기 쉽게)
	AStaticMeshActor* Orb = SpawnWithMaterial(EShape::Sphere, FVector(0.f, 330.f, 110.f), FVector(150.f), nullptr);
	OriginalMaterials.Add(GetColorMaterial());
	Materials.Add(UMaterialInstanceDynamic::Create(Hologram, this));
	SpawnShape(GetWorld(), EShape::Cylinder, FVector(0.f, 330.f, 10.f), FVector(120.f, 120.f, 20.f), Colors::Dark);
	// 뒤쪽 벽: 반투명이 비치는 것을 보여 줄 줄무늬
	for (int32 I = 0; I < 8; ++I)
	{
		SpawnBlock(GetWorld(), FVector(300.f, -250.f + I * 110.f, 0.f), FVector(20.f, 55.f, 380.f), I % 2 ? Colors::Item : Colors::Purple);
	}
	OrbMesh = Orb->GetStaticMeshComponent();
	Apply();
}

void ADemo0409_Hologram::Apply()
{
	USkeletalMeshComponent* Mesh = Figure ? Figure->GetSkeletalMeshComponent() : nullptr;
	const int32 FigureSlots = Mesh ? Mesh->GetNumMaterials() : 0;
	for (int32 I = 0; I < Materials.Num(); ++I)
	{
		UMaterialInstanceDynamic* MID = Materials[I];
		MID->SetScalarParameterValue(TEXT("ScanSpeed"), ScanSpeed);
		MID->SetScalarParameterValue(TEXT("FlickerAmount"), FlickerAmount);
		MID->SetVectorParameterValue(TEXT("HoloColor"), HoloColor);
		UMaterialInterface* Use = bHologram ? static_cast<UMaterialInterface*>(MID) : OriginalMaterials[I].Get();
		if (I < FigureSlots)
		{
			Mesh->SetMaterial(I, Use);
		}
		else if (OrbMesh)
		{
			OrbMesh->SetMaterial(0, Use);
		}
	}
}

void ADemo0409_Hologram::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	bool bChanged = false;
	if (WasKeyPressed(EKeys::C)) { bHologram = !bHologram; bChanged = true; }
	if (WasKeyPressed(EKeys::LeftBracket)) { ScanSpeed = FMath::Max(0.f, ScanSpeed - 0.4f); bChanged = true; }
	if (WasKeyPressed(EKeys::RightBracket)) { ScanSpeed += 0.4f; bChanged = true; }
	if (WasKeyPressed(EKeys::F)) { FlickerAmount = FlickerAmount > 0.f ? 0.f : 0.15f; bChanged = true; }
	if (bChanged)
	{
		Apply();
	}
	SetStatus(FString::Printf(TEXT("%s · ScanSpeed %.1f · 깜빡임 %.2f"),
		bHologram ? TEXT("M_Hologram (Translucent · Unlit)") : TEXT("원래 머티리얼"), ScanSpeed, FlickerAmount));
}
