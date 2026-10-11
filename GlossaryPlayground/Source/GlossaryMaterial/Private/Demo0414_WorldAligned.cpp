#include "Demo0414_WorldAligned.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"

using namespace GlossaryKit;

ADemo0414_WorldAligned::ADemo0414_WorldAligned()
{
	DemoId = TEXT("0414");
	Controls = TEXT("C : 앞줄을 World Aligned ↔ 메시 UV 로 바꾸기    [ / ] : TextureSize −/+ 50cm\n")
		TEXT("관찰: 뒷줄(UV)은 큰 바위일수록 벽돌이 늘어나고, 앞줄(World Aligned)은 크기와 상관없이 벽돌 크기가 같다");
	Showcase = TEXT("크기가 다른 바위에 메시 UV vs World Aligned Texture(트라이플래너) 비교, 늘어남 없음");
}

void ADemo0414_WorldAligned::BeginPlay()
{
	Super::BeginPlay();
	SetupFixedCamera(FVector(-1250.f, -40.f, 480.f), FVector(100.f, -40.f, 120.f), 60.f);

	UMaterialInterface* UVMat = LoadMaterial(TEXT("M_Rock_UV"));
	WorldAlignedMaterial = UMaterialInstanceDynamic::Create(LoadMaterial(TEXT("M_Rock_WorldAligned")), this);

	// 크기·비율이 제각각인 바위 5개 (같은 배치를 앞뒤 두 줄)
	struct FRock { EShape Shape; FVector Size; float Yaw; };
	const FRock Rocks[] = {
		{ EShape::Sphere, FVector(80.f, 80.f, 70.f), 0.f },
		{ EShape::Cube, FVector(120.f, 120.f, 160.f), 25.f },
		{ EShape::Sphere, FVector(220.f, 300.f, 150.f), 10.f },
		{ EShape::Cube, FVector(300.f, 140.f, 60.f), -20.f },
		{ EShape::Sphere, FVector(120.f, 120.f, 320.f), 0.f },
	};
	float Y = -350.f;
	for (const FRock& Rock : Rocks)
	{
		Y += Rock.Size.Y * 0.5f + 40.f;
		// 뒷줄: 메시 UV
		SpawnWithMaterial(Rock.Shape, FVector(450.f, Y + 150.f, Rock.Size.Z * 0.5f), Rock.Size, UVMat, FRotator(0.f, Rock.Yaw, 0.f));
		// 앞줄: World Aligned
		AStaticMeshActor* Front = SpawnWithMaterial(Rock.Shape, FVector(0.f, Y, Rock.Size.Z * 0.5f), Rock.Size, WorldAlignedMaterial, FRotator(0.f, Rock.Yaw, 0.f));
		FrontRocks.Add(Front->GetStaticMeshComponent());
		Y += Rock.Size.Y * 0.5f;
	}
	SpawnLabel(GetWorld(), TEXT("뒷줄: 메시 UV (M_Rock_UV) — 늘어남"), FVector(450.f, 250.f, 420.f), FLinearColor::White, 16.f);
	Apply();
}

void ADemo0414_WorldAligned::Apply()
{
	UMaterialInterface* UVMat = LoadMaterial(TEXT("M_Rock_UV"));
	WorldAlignedMaterial->SetVectorParameterValue(TEXT("TextureSize"), FLinearColor(TextureSize, TextureSize, TextureSize));
	for (UStaticMeshComponent* Mesh : FrontRocks)
	{
		Mesh->SetMaterial(0, bWorldAligned ? static_cast<UMaterialInterface*>(WorldAlignedMaterial) : UVMat);
	}
}

void ADemo0414_WorldAligned::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	bool bChanged = false;
	if (WasKeyPressed(EKeys::C)) { bWorldAligned = !bWorldAligned; bChanged = true; }
	if (WasKeyPressed(EKeys::LeftBracket)) { TextureSize = FMath::Max(50.f, TextureSize - 50.f); bChanged = true; }
	if (WasKeyPressed(EKeys::RightBracket)) { TextureSize += 50.f; bChanged = true; }
	if (bChanged)
	{
		Apply();
	}
	SetStatus(FString::Printf(TEXT("앞줄: %s · TextureSize %.0fcm (텍스처 한 장이 덮는 월드 크기)"),
		bWorldAligned ? TEXT("M_Rock_WorldAligned (WorldAlignedTexture 함수)") : TEXT("M_Rock_UV (메시 UV)"), TextureSize));
}
