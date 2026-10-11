#include "Demo0402_MaterialInstance.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstance.h"

using namespace GlossaryKit;

ADemo0402_MaterialInstance::ADemo0402_MaterialInstance()
{
	DemoId = TEXT("0402");
	Controls = TEXT("1~5 : 인스턴스 선택(상태 줄에 부모·파라미터 표시)\n")
		TEXT("C : 다섯 물체를 모두 부모 M_Base 로 되돌리기 / 다시 인스턴스로\n")
		TEXT("관찰: 셰이더는 M_Base 하나, MI_Base_* 다섯 개는 Color 값만 덮어쓴 복사본");
	Showcase = TEXT("부모 M_Base 하나에서 색만 바꾼 Material Instance 5개, C 로 부모와 비교");
}

void ADemo0402_MaterialInstance::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	SetupFixedCamera(FVector(-1000.f, -250.f, 420.f), FVector(0.f, -250.f, 170.f), 60.f);

	// 부모 머티리얼 견본 (왼쪽 아래)
	SpawnWithMaterial(EShape::Cube, FVector(0.f, -700.f, 80.f), FVector(140.f), LoadMaterial(TEXT("M_Base")));
	SpawnLabel(World, TEXT("부모 M_Base\n(Color · Roughness 노출)"), FVector(0.f, -700.f, 230.f), Colors::Item, 16.f);

	for (int32 I = 0; I < InstanceNames.Num(); ++I)
	{
		const FVector Location(0.f, -350.f + I * 230.f, 110.f);
		AStaticMeshActor* Actor = SpawnWithMaterial(EShape::Sphere, Location, FVector(180.f), nullptr);
		Meshes.Add(Actor->GetStaticMeshComponent());
		SpawnLabel(World, InstanceNames[I], Location + FVector(0.f, 0.f, 150.f), FLinearColor::White, 14.f);
	}
	Apply();
}

void ADemo0402_MaterialInstance::Apply()
{
	UMaterialInterface* Parent = LoadMaterial(TEXT("M_Base"));
	for (int32 I = 0; I < Meshes.Num(); ++I)
	{
		Meshes[I]->SetMaterial(0, bUseInstances ? LoadMaterial(InstanceNames[I]) : Parent);
	}
}

void ADemo0402_MaterialInstance::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const FKey Keys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five };
	for (int32 I = 0; I < 5; ++I)
	{
		if (WasKeyPressed(Keys[I])) { Selected = I; }
	}
	if (WasKeyPressed(EKeys::C))
	{
		bUseInstances = !bUseInstances;
		Apply();
	}

	FString Info = TEXT("-");
	if (Meshes.IsValidIndex(Selected))
	{
		UMaterialInterface* Current = Meshes[Selected]->GetMaterial(0);
		FLinearColor Color = FLinearColor::Black;
		float Roughness = 0.f;
		Current->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Color")), Color);
		Current->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Roughness")), Roughness);
		const UMaterialInstance* Instance = Cast<UMaterialInstance>(Current);
		Info = FString::Printf(TEXT("%d번 %s (부모 %s) · Color (%.2f, %.2f, %.2f) · Roughness %.2f"), Selected + 1, *Current->GetName(),
			Instance && Instance->Parent ? *Instance->Parent->GetName() : TEXT("없음 — 이것이 부모"), Color.R, Color.G, Color.B, Roughness);
	}
	SetStatus(FString::Printf(TEXT("%s\n%s"), bUseInstances ? TEXT("인스턴스 5개 사용 중 (셰이더 1개 공유)") : TEXT("모두 부모 M_Base (기본값 회색)"), *Info));
}
