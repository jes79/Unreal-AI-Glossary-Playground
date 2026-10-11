#include "Demo0408_Dissolve.h"

#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"

using namespace GlossaryKit;

ADemo0408_Dissolve::ADemo0408_Dissolve()
{
	DemoId = TEXT("0408");
	Controls = TEXT("Space : 1초 디졸브 (사라짐 ↔ 나타남)    A : 자동 반복 ON/OFF\n")
		TEXT("[ / ] : DissolveAmount ±0.1 · 1 : 0.5 에서 멈춤 (자동 반복 멈춤)\n")
		TEXT("관찰: 노이즈 값이 낮은 곳부터 구멍이 나고, 잘린 경계는 주황 Emissive 로 타들어간다");
	Showcase = TEXT("노이즈 + Opacity Mask 로 1초 디졸브, 경계선 주황 Emissive, 자동 반복·수동 조절");
	Environment.Sky = EGlossarySky::Sunset;
}

void ADemo0408_Dissolve::BeginPlay()
{
	Super::BeginPlay();
	SetupFixedCamera(FVector(-1050.f, 30.f, 260.f), FVector(0.f, 30.f, 110.f), 60.f);

	UMaterialInterface* Dissolve = LoadMaterial(TEXT("M_Dissolve"));
	TArray<UPrimitiveComponent*> Targets;
	Targets.Add(SpawnWithMaterial(EShape::Sphere, FVector(0.f, -120.f, 120.f), FVector(200.f), Dissolve)->GetStaticMeshComponent());
	Targets.Add(SpawnMannequin(FVector(0.f, 170.f, 0.f), FRotator(0.f, 70.f, 0.f), Dissolve)->GetSkeletalMeshComponent());
	Targets.Add(SpawnWithMaterial(EShape::Cube, FVector(0.f, 440.f, 90.f), FVector(150.f), Dissolve, FRotator(0.f, 30.f, 0.f))->GetStaticMeshComponent());
	for (UPrimitiveComponent* Target : Targets)
	{
		for (int32 Slot = 0; Slot < Target->GetNumMaterials(); ++Slot)
		{
			UMaterialInstanceDynamic* MID = Target->CreateDynamicMaterialInstance(Slot, Dissolve);
			MID->SetScalarParameterValue(TEXT("EdgeWidth"), EdgeWidth);
			MID->SetVectorParameterValue(TEXT("EdgeColor"), EdgeColor);
			Materials.Add(MID);
		}
	}
	Direction = 1.f;
}

void ADemo0408_Dissolve::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::SpaceBar))
	{
		Direction = Amount < 0.5f ? 1.f : -1.f;
	}
	if (WasKeyPressed(EKeys::A)) { bAutoLoop = !bAutoLoop; }
	if (WasKeyPressed(EKeys::One)) { bAutoLoop = false; Direction = 0.f; Amount = 0.5f; }   // 절반에서 멈춰 경계선 관찰
	if (WasKeyPressed(EKeys::LeftBracket))  { bAutoLoop = false; Direction = 0.f; Amount = FMath::Clamp(Amount - 0.1f, 0.f, 1.f); }
	if (WasKeyPressed(EKeys::RightBracket)) { bAutoLoop = false; Direction = 0.f; Amount = FMath::Clamp(Amount + 0.1f, 0.f, 1.f); }

	if (Direction != 0.f)
	{
		// 1초(DissolveDuration) 동안 0 → 1
		Amount = FMath::Clamp(Amount + Direction * DeltaSeconds / DissolveDuration, 0.f, 1.f);
		if (Amount <= 0.f || Amount >= 1.f)
		{
			Direction = 0.f;
			HoldTimer = 0.f;
		}
	}
	else if (bAutoLoop)
	{
		HoldTimer += DeltaSeconds;
		if (HoldTimer > 0.8f)
		{
			Direction = Amount < 0.5f ? 1.f : -1.f;
		}
	}

	for (UMaterialInstanceDynamic* MID : Materials)
	{
		MID->SetScalarParameterValue(TEXT("DissolveAmount"), Amount);
	}
	SetStatus(FString::Printf(TEXT("DissolveAmount %.2f %s · 재생 시간 %.1f초%s"), Amount,
		Direction > 0.f ? TEXT("(사라지는 중)") : Direction < 0.f ? TEXT("(나타나는 중)") : TEXT(""),
		DissolveDuration, bAutoLoop ? TEXT(" · 자동 반복") : TEXT("")));
}
