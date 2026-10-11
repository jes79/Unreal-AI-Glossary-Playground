#include "Demo0413_Panner.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"

using namespace GlossaryKit;

ADemo0413_Panner::ADemo0413_Panner()
{
	DemoId = TEXT("0413");
	Controls = TEXT("C : 흐름 ON/OFF (PanSpeed 0)    [ / ] : PanSpeed −/+ 0.05\n")
		TEXT("관찰: 텍스처 자체는 그대로이고 UV 만 매초 PanSpeed 만큼 밀린다 → 용암이 흐르는 것처럼 보임");
	Showcase = TEXT("Panner 로 용암 텍스처가 초당 0.1 씩 흐르는 용암 강, 멈춘 판과 비교·속도 조절");
	Environment.Sky = EGlossarySky::Sunset;
}

void ADemo0413_Panner::BeginPlay()
{
	Super::BeginPlay();
	SetupFixedCamera(FVector(-750.f, -250.f, 520.f), FVector(150.f, -150.f, 0.f), 60.f);

	UMaterialInterface* Lava = LoadMaterial(TEXT("M_Lava"));
	// 용암 강: 길쭉한 판 (UV 0~1 이 판 전체 → 타일링 2)
	AStaticMeshActor* River = SpawnWithMaterial(EShape::Plane, FVector(200.f, 50.f, 2.f), FVector(400.f, 1400.f, 1.f), Lava);
	LavaMaterial = River->GetStaticMeshComponent()->CreateAndSetMaterialInstanceDynamic(0);
	// 강둑 바위
	for (int32 I = 0; I < 7; ++I)
	{
		SpawnShape(GetWorld(), EShape::Sphere, FVector(-30.f + (I % 2) * 460.f, -600.f + I * 200.f, 20.f), FVector(140.f, 160.f, 90.f), Colors::Dark, FRotator(0.f, I * 40.f, 0.f));
	}
	// 비교: 같은 머티리얼, PanSpeed 0 (오른쪽 앞)
	AStaticMeshActor* Still = SpawnWithMaterial(EShape::Plane, FVector(-250.f, -700.f, 2.f), FVector(250.f, 250.f, 1.f), Lava);
	Still->GetStaticMeshComponent()->CreateAndSetMaterialInstanceDynamic(0)->SetScalarParameterValue(TEXT("PanSpeed"), 0.f);
	SpawnLabel(GetWorld(), TEXT("PanSpeed 0 (멈춤)"), FVector(-250.f, -700.f, 80.f), FLinearColor::White, 14.f);
	SpawnLabel(GetWorld(), TEXT("Panner → 흐르는 용암"), FVector(200.f, 400.f, 120.f), Colors::Accent, 16.f);
}

void ADemo0413_Panner::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::C)) { bPanning = !bPanning; }
	if (WasKeyPressed(EKeys::LeftBracket)) { PanSpeed -= 0.05f; }
	if (WasKeyPressed(EKeys::RightBracket)) { PanSpeed += 0.05f; }
	const float Speed = bPanning ? PanSpeed : 0.f;
	if (LavaMaterial)
	{
		LavaMaterial->SetScalarParameterValue(TEXT("PanSpeed"), Speed);
	}
	SetStatus(FString::Printf(TEXT("PanSpeed %.2f UV/초 %s · 1 타일(UV 1) 흐르는 데 %.1f초"),
		Speed, bPanning ? TEXT("") : TEXT("(멈춤)"), FMath::Abs(Speed) > KINDA_SMALL_NUMBER ? 1.f / FMath::Abs(Speed) : 0.f));
}
