#include "Demo0410_Toon.h"

#include "Animation/SkeletalMeshActor.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryEnvironment.h"
#include "Materials/MaterialInstanceDynamic.h"

using namespace GlossaryKit;

ADemo0410_Toon::ADemo0410_Toon()
{
	DemoId = TEXT("0410");
	Controls = TEXT("1 : 머티리얼 방식(M_Toon) ON/OFF    2 : 포스트 프로세스 방식 ON/OFF\n")
		TEXT("[ / ] : 해 방향 회전 (명암 경계가 따라 움직임)    , / . : 단계 수 −/+\n")
		TEXT("관찰: 왼쪽 = 툰 머티리얼(물체별), 오른쪽 = 일반 Lit. 포스트 프로세스는 화면 전체가 계단 명암");
	Showcase = TEXT("Dot(N,L) 3단계 툰 머티리얼 vs 일반 Lit 비교, 포스트 프로세스 툰(화면 전체)과 비교");

	ToonPostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("ToonPostProcess"));
	ToonPostProcess->SetupAttachment(Root);
	ToonPostProcess->bUnbound = true;
	ToonPostProcess->Priority = 10.f;
}

void ADemo0410_Toon::BeginPlay()
{
	Super::BeginPlay();
	SetupFixedCamera(FVector(-1350.f, 60.f, 420.f), FVector(0.f, 60.f, 90.f), 60.f);

	UMaterialInterface* Toon = LoadMaterial(TEXT("M_Toon"));
	const FLinearColor Palette[] = { FLinearColor(0.9f, 0.3f, 0.2f), FLinearColor(0.2f, 0.55f, 0.95f), FLinearColor(0.95f, 0.75f, 0.2f), FLinearColor(0.3f, 0.8f, 0.4f) };

	// 같은 모양 두 벌: 왼쪽(Y<0) 툰, 오른쪽(Y>0) 일반 Lit
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float BaseY = Side == 0 ? -200.f : 400.f;
		TArray<UPrimitiveComponent*> Created;
		Created.Add(SpawnWithMaterial(EShape::Sphere, FVector(0.f, BaseY - 130.f, 90.f), FVector(160.f), nullptr)->GetStaticMeshComponent());
		Created.Add(SpawnWithMaterial(EShape::Cylinder, FVector(-150.f, BaseY + 80.f, 70.f), FVector(110.f, 110.f, 140.f), nullptr)->GetStaticMeshComponent());
		Created.Add(SpawnWithMaterial(EShape::Cone, FVector(150.f, BaseY + 60.f, 80.f), FVector(130.f, 130.f, 160.f), nullptr)->GetStaticMeshComponent());
		Created.Add(SpawnMannequin(FVector(120.f, BaseY - 10.f, 0.f), FRotator(0.f, 60.f, 0.f))->GetSkeletalMeshComponent());
		for (int32 I = 0; I < Created.Num(); ++I)
		{
			UPrimitiveComponent* Comp = Created[I];
			// 도형은 같은 색의 Lit(M_Color) 로 칠한다. 마네킹은 원래 머티리얼(Lit) 그대로
			if (Cast<UStaticMeshComponent>(Comp))
			{
				SetColor(Comp, Palette[I], 0.6f);
			}
			if (Side == 1)
			{
				continue;
			}
			for (int32 Slot = 0; Slot < Comp->GetNumMaterials(); ++Slot)
			{
				UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Toon, this);
				MID->SetVectorParameterValue(TEXT("BaseColor"), Palette[I]);
				ToonMaterials.Add(MID);
				LitMaterials.Add(Comp->GetMaterial(Slot));
				ToonSlots.Add({ Comp, Slot });
			}
		}
	}
	SpawnLabel(GetWorld(), TEXT("툰 머티리얼 (M_Toon, Unlit)"), FVector(0.f, -200.f, 270.f), Colors::Item, 16.f);
	SpawnLabel(GetWorld(), TEXT("일반 Lit (비교)"), FVector(0.f, 400.f, 270.f), FLinearColor::White, 16.f);

	PostProcessMaterial = UMaterialInstanceDynamic::Create(LoadMaterial(TEXT("M_ToonPostProcess")), this);
	ToonPostProcess->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(0.f, PostProcessMaterial));
	ApplyMaterialMode();
}

void ADemo0410_Toon::ApplyMaterialMode()
{
	// 툰 줄: M_Toon MID ↔ 일반 Lit(M_Color) 을 바꿔 끼운다
	for (int32 I = 0; I < ToonSlots.Num(); ++I)
	{
		if (UPrimitiveComponent* Comp = ToonSlots[I].Component.Get())
		{
			Comp->SetMaterial(ToonSlots[I].Slot, bToonMaterial ? static_cast<UMaterialInterface*>(ToonMaterials[I]) : LitMaterials[I].Get());
		}
	}
	ToonPostProcess->Settings.WeightedBlendables.Array[0].Weight = bPostProcess ? 1.f : 0.f;
}

void ADemo0410_Toon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One)) { bToonMaterial = !bToonMaterial; ApplyMaterialMode(); }
	if (WasKeyPressed(EKeys::Two)) { bPostProcess = !bPostProcess; ApplyMaterialMode(); }
	if (WasKeyPressed(EKeys::LeftBracket)) { SunYaw -= 20.f; }
	if (WasKeyPressed(EKeys::RightBracket)) { SunYaw += 20.f; }
	if (WasKeyPressed(EKeys::Comma)) { ToonBands = FMath::Max(2, ToonBands - 1); }
	if (WasKeyPressed(EKeys::Period)) { ToonBands = FMath::Min(8, ToonBands + 1); }

	// 해 방향 → 라이트 벡터(물체에서 해를 향하는 방향) → M_Toon 의 LightDirection
	FVector LightDir(0.4f, -0.4f, 0.8f);
	if (AGlossaryEnvironment* Env = AGlossaryEnvironment::Find(this))
	{
		if (UDirectionalLightComponent* Sun = Env->GetSun())
		{
			Sun->SetWorldRotation(FRotator(-40.f, SunYaw, 0.f));
			LightDir = -Sun->GetForwardVector();
		}
	}
	for (UMaterialInstanceDynamic* MID : ToonMaterials)
	{
		MID->SetVectorParameterValue(TEXT("LightDirection"), FLinearColor(LightDir));
		MID->SetScalarParameterValue(TEXT("ToonBands"), ToonBands);
		MID->SetScalarParameterValue(TEXT("ShadowTone"), ShadowTone);
	}
	if (PostProcessMaterial)
	{
		PostProcessMaterial->SetScalarParameterValue(TEXT("ToonBands"), ToonBands + 1);
	}
	SetStatus(FString::Printf(TEXT("머티리얼 방식 %s · 포스트 프로세스 방식 %s · 단계 %d · 해 Yaw %.0f°\nLightDirection (%.2f, %.2f, %.2f)"),
		bToonMaterial ? TEXT("ON") : TEXT("OFF"), bPostProcess ? TEXT("ON") : TEXT("OFF"), ToonBands, SunYaw, LightDir.X, LightDir.Y, LightDir.Z));
}
