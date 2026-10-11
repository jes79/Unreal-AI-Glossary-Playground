#include "Demo0415_BlendMode.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"

using namespace GlossaryKit;

ADemo0415_BlendMode::ADemo0415_BlendMode()
{
	DemoId = TEXT("0415");
	Controls = TEXT("[ / ] : 유리 Opacity −/+ 0.1    , / . : 불빛 세기 −/+\n")
		TEXT("관찰: Opaque = 뒤 안 보임 · Masked = 잎 모양 밖은 완전히 잘림(Clip 0.33) · Translucent = 반쯤 비침 · Additive = 뒤 색에 더해 밝아짐");
	Showcase = TEXT("줄무늬 벽 앞 Opaque 큐브·Masked 잎(Clip 0.33)·Translucent 유리·Additive 불빛 나란히 비교");
}

void ADemo0415_BlendMode::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	SetupFixedCamera(FVector(-850.f, -150.f, 260.f), FVector(0.f, -150.f, 160.f), 60.f);

	// 뒤 줄무늬 벽: 투명도를 판단하는 배경
	for (int32 I = 0; I < 14; ++I)
	{
		SpawnBlock(World, FVector(220.f, -650.f + I * 90.f, 0.f), FVector(20.f, 45.f, 420.f), I % 2 ? Colors::Player : Colors::Item);
	}

	const FRotator FaceCamera(90.f, 0.f, 0.f);   // Plane(+Z 노멀)을 세워 카메라(-X)를 보게
	SpawnWithMaterial(EShape::Cube, FVector(0.f, -450.f, 110.f), FVector(150.f), LoadMaterial(TEXT("M_Blend_Opaque")), FRotator(0.f, 20.f, 0.f));
	SpawnWithMaterial(EShape::Plane, FVector(0.f, -170.f, 160.f), FVector(220.f, 220.f, 1.f), LoadMaterial(TEXT("M_Blend_Masked")), FaceCamera);
	AStaticMeshActor* Glass = SpawnWithMaterial(EShape::Sphere, FVector(0.f, 120.f, 130.f), FVector(200.f), LoadMaterial(TEXT("M_Blend_Translucent")));
	GlassMaterial = Glass->GetStaticMeshComponent()->CreateAndSetMaterialInstanceDynamic(0);
	AStaticMeshActor* Glow = SpawnWithMaterial(EShape::Plane, FVector(0.f, 420.f, 150.f), FVector(260.f, 260.f, 1.f), LoadMaterial(TEXT("M_Blend_Additive")), FaceCamera);
	GlowMaterial = Glow->GetStaticMeshComponent()->CreateAndSetMaterialInstanceDynamic(0);
	Glow->SetActorEnableCollision(false);

	SpawnLabel(World, TEXT("Opaque"), FVector(0.f, -450.f, 30.f), FLinearColor::White, 18.f);
	SpawnLabel(World, TEXT("Masked (Clip 0.33)"), FVector(0.f, -170.f, 30.f), FLinearColor::White, 18.f);
	SpawnLabel(World, TEXT("Translucent"), FVector(0.f, 120.f, 10.f), FLinearColor::White, 18.f);
	SpawnLabel(World, TEXT("Additive"), FVector(0.f, 420.f, 10.f), FLinearColor::White, 18.f);
}

void ADemo0415_BlendMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::LeftBracket)) { GlassOpacity = FMath::Clamp(GlassOpacity - 0.1f, 0.f, 1.f); }
	if (WasKeyPressed(EKeys::RightBracket)) { GlassOpacity = FMath::Clamp(GlassOpacity + 0.1f, 0.f, 1.f); }
	if (WasKeyPressed(EKeys::Comma)) { GlowIntensity = FMath::Max(0.f, GlowIntensity - 2.f); }
	if (WasKeyPressed(EKeys::Period)) { GlowIntensity += 2.f; }
	if (GlassMaterial) { GlassMaterial->SetScalarParameterValue(TEXT("GlassOpacity"), GlassOpacity); }
	if (GlowMaterial) { GlowMaterial->SetScalarParameterValue(TEXT("GlowIntensity"), GlowIntensity); }
	SetStatus(FString::Printf(TEXT("Masked Opacity Mask Clip 0.33 · 유리 Opacity %.1f · Additive 세기 %.0f"), GlassOpacity, GlowIntensity));
}
