#include "Demo0405_MaterialFunction.h"

#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"

using namespace GlossaryKit;

ADemo0405_MaterialFunction::ADemo0405_MaterialFunction()
{
	DemoId = TEXT("0405");
	Controls = TEXT("C : 림 라이트 ON/OFF (세 머티리얼의 RimIntensity 를 한꺼번에)    [ / ] : 림 세기 ×0.5 / ×2\n")
		TEXT("관찰: 금속(주황)·돌(초록)·유령(파랑) 모두 같은 MF_RimLight 를 호출 — 색·지수만 다르게 넘긴다");
	Showcase = TEXT("MF_RimLight 하나를 금속·돌·유령 머티리얼 3개가 Material Function Call 로 재사용");
	Environment.Sky = EGlossarySky::Sunset;
}

void ADemo0405_MaterialFunction::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	SetupFixedCamera(FVector(-900.f, -200.f, 260.f), FVector(0.f, -200.f, 130.f), 60.f);

	const FString Labels[] = { TEXT("M_Rim_Metal\n금속 + 주황 림"), TEXT("M_Rim_Stone\n돌 + 초록 림"), TEXT("M_Rim_Ghost\n유령 + 파랑 림") };
	for (int32 I = 0; I < CallerMaterials.Num(); ++I)
	{
		UMaterialInterface* Material = LoadMaterial(CallerMaterials[I]);
		const float Y = -180.f + I * 300.f;
		UPrimitiveComponent* Target = nullptr;
		if (I == 2)
		{
			// 유령은 마네킹에
			ASkeletalMeshActor* Ghost = SpawnMannequin(FVector(0.f, Y, 0.f), FRotator(0.f, 90.f, 0.f), Material);
			Target = Ghost->GetSkeletalMeshComponent();
		}
		else
		{
			AStaticMeshActor* Actor = SpawnWithMaterial(I == 0 ? EShape::Sphere : EShape::Cube, FVector(0.f, Y, 110.f), FVector(190.f), Material,
				I == 1 ? FRotator(20.f, 35.f, 10.f) : FRotator::ZeroRotator);
			Target = Actor->GetStaticMeshComponent();
		}
		for (int32 Slot = 0; Slot < Target->GetNumMaterials(); ++Slot)
		{
			UMaterialInstanceDynamic* MID = Target->CreateDynamicMaterialInstance(Slot, Material);
			float Intensity = 1.f;
			MID->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("RimIntensity")), Intensity);
			Materials.Add(MID);
			DefaultIntensity.Add(Intensity);
		}
		SpawnLabel(World, Labels[I], FVector(0.f, Y, 270.f), FLinearColor::White, 15.f);
	}
	SpawnLabel(World, TEXT("MF_RimLight (Fresnel × RimColor × RimIntensity)"), FVector(0.f, 120.f, 360.f), Colors::Item, 18.f);
}

void ADemo0405_MaterialFunction::ApplyRim()
{
	for (int32 I = 0; I < Materials.Num(); ++I)
	{
		Materials[I]->SetScalarParameterValue(TEXT("RimIntensity"), bRimOn ? DefaultIntensity[I] * RimScale : 0.f);
	}
}

void ADemo0405_MaterialFunction::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	bool bChanged = false;
	if (WasKeyPressed(EKeys::C)) { bRimOn = !bRimOn; bChanged = true; }
	if (WasKeyPressed(EKeys::LeftBracket)) { RimScale *= 0.5f; bChanged = true; }
	if (WasKeyPressed(EKeys::RightBracket)) { RimScale *= 2.f; bChanged = true; }
	if (bChanged)
	{
		ApplyRim();
	}
	SetStatus(FString::Printf(TEXT("림 라이트 %s · 세기 ×%.2f\nMF_RimLight 호출: M_Rim_Metal, M_Rim_Stone, M_Rim_Ghost"),
		bRimOn ? TEXT("ON") : TEXT("OFF"), RimScale));
}
