#include "Demo0406_Fresnel.h"

#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"

using namespace GlossaryKit;

ADemo0406_Fresnel::ADemo0406_Fresnel()
{
	DemoId = TEXT("0406");
	Controls = TEXT("C : Fresnel 림 라이트 ON/OFF    [ / ] : Exponent −/+ 0.5\n")
		TEXT("관찰: 카메라와 비스듬한 면(가장자리)일수록 Fresnel 값이 1 → 파랗게 빛남. Exponent↑ = 테두리 얇게");
	Showcase = TEXT("Fresnel(Exponent 3) 파란 림 라이트 ON/OFF, Exponent 조절로 테두리 두께 비교");
	Environment.Sky = EGlossarySky::Night;
}

void ADemo0406_Fresnel::BeginPlay()
{
	Super::BeginPlay();
	SetupFixedCamera(FVector(-1150.f, 60.f, 260.f), FVector(0.f, 60.f, 120.f), 60.f);

	UMaterialInterface* Fresnel = LoadMaterial(TEXT("M_Fresnel"));
	TArray<UPrimitiveComponent*> Targets;
	Targets.Add(SpawnWithMaterial(EShape::Sphere, FVector(0.f, -150.f, 120.f), FVector(220.f), Fresnel)->GetStaticMeshComponent());
	Targets.Add(SpawnMannequin(FVector(0.f, 170.f, 0.f), FRotator(0.f, 60.f, 0.f), Fresnel)->GetSkeletalMeshComponent());
	Targets.Add(SpawnWithMaterial(EShape::Cylinder, FVector(0.f, 470.f, 100.f), FVector(150.f, 150.f, 200.f), Fresnel)->GetStaticMeshComponent());
	for (UPrimitiveComponent* Target : Targets)
	{
		for (int32 Slot = 0; Slot < Target->GetNumMaterials(); ++Slot)
		{
			Materials.Add(Target->CreateDynamicMaterialInstance(Slot, Fresnel));
		}
	}
	SpawnLabel(GetWorld(), TEXT("M_Fresnel — Fresnel → Emissive"), FVector(0.f, 170.f, 330.f), Colors::Cyan, 18.f);
	Apply();
}

void ADemo0406_Fresnel::Apply()
{
	for (UMaterialInstanceDynamic* MID : Materials)
	{
		MID->SetScalarParameterValue(TEXT("FresnelExponent"), FresnelExponent);
		MID->SetVectorParameterValue(TEXT("RimColor"), RimColor);
		MID->SetScalarParameterValue(TEXT("RimIntensity"), bRimOn ? RimIntensity : 0.f);
	}
}

void ADemo0406_Fresnel::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	bool bChanged = false;
	if (WasKeyPressed(EKeys::C)) { bRimOn = !bRimOn; bChanged = true; }
	if (WasKeyPressed(EKeys::LeftBracket)) { FresnelExponent = FMath::Max(0.5f, FresnelExponent - 0.5f); bChanged = true; }
	if (WasKeyPressed(EKeys::RightBracket)) { FresnelExponent = FMath::Min(12.f, FresnelExponent + 0.5f); bChanged = true; }
	if (bChanged)
	{
		Apply();
	}
	SetStatus(FString::Printf(TEXT("Fresnel 림 %s · Exponent %.1f · RimIntensity %.0f"), bRimOn ? TEXT("ON") : TEXT("OFF"), FresnelExponent, RimIntensity));
}
