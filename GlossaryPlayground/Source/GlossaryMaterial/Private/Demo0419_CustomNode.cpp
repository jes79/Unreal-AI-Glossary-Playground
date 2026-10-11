#include "Demo0419_CustomNode.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"

using namespace GlossaryKit;

ADemo0419_CustomNode::ADemo0419_CustomNode()
{
	DemoId = TEXT("0419");
	Controls = TEXT("[ / ] : CellCount −/+ 2 (Custom 노드의 Cells 입력)    C : 색 바꾸기\n")
		TEXT("관찰: 노드 그래프 대신 HLSL 세 줄 — float2 Cell = floor(UV * Cells); return fmod(Cell.x + Cell.y, 2.0);");
	Showcase = TEXT("Custom 노드 HLSL 체커보드 함수, 칸 수 파라미터를 키로 조절");
}

void ADemo0419_CustomNode::BeginPlay()
{
	Super::BeginPlay();
	SetupFixedCamera(FVector(-800.f, -200.f, 330.f), FVector(0.f, -150.f, 110.f), 60.f);

	UMaterialInterface* Checker = LoadMaterial(TEXT("M_CustomChecker"));
	TArray<AStaticMeshActor*> Actors;
	Actors.Add(SpawnWithMaterial(EShape::Plane, FVector(0.f, -150.f, 2.f), FVector(700.f, 700.f, 1.f), Checker));
	Actors.Add(SpawnWithMaterial(EShape::Cube, FVector(0.f, -350.f, 100.f), FVector(160.f), Checker, FRotator(0.f, 30.f, 0.f)));
	Actors.Add(SpawnWithMaterial(EShape::Sphere, FVector(0.f, 50.f, 110.f), FVector(200.f), Checker));
	Actors.Add(SpawnWithMaterial(EShape::Cylinder, FVector(100.f, 300.f, 110.f), FVector(140.f, 140.f, 220.f), Checker));
	for (AStaticMeshActor* Actor : Actors)
	{
		Materials.Add(Actor->GetStaticMeshComponent()->CreateAndSetMaterialInstanceDynamic(0));
	}
	SpawnLabel(GetWorld(), TEXT("Custom 노드 \"Checkerboard\" (HLSL)"), FVector(0.f, -150.f, 300.f), Colors::Item, 18.f);
}

void ADemo0419_CustomNode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::LeftBracket)) { CellCount = FMath::Max(1.f, CellCount - 2.f); }
	if (WasKeyPressed(EKeys::RightBracket)) { CellCount = FMath::Min(64.f, CellCount + 2.f); }
	if (WasKeyPressed(EKeys::C)) { PaletteIndex = (PaletteIndex + 1) % 3; }
	const FLinearColor A[] = { FLinearColor(0.9f, 0.9f, 0.88f), FLinearColor(0.95f, 0.75f, 0.1f), FLinearColor(0.9f, 0.2f, 0.25f) };
	const FLinearColor B[] = { FLinearColor(0.05f, 0.08f, 0.15f), FLinearColor(0.1f, 0.3f, 0.8f), FLinearColor(0.1f, 0.1f, 0.1f) };
	for (UMaterialInstanceDynamic* MID : Materials)
	{
		MID->SetScalarParameterValue(TEXT("CellCount"), CellCount);
		MID->SetVectorParameterValue(TEXT("ColorA"), A[PaletteIndex]);
		MID->SetVectorParameterValue(TEXT("ColorB"), B[PaletteIndex]);
	}
	SetStatus(FString::Printf(TEXT("CellCount %.0f → UV 0~1 을 %.0f×%.0f 칸으로\nHLSL: float2 Cell = floor(UV * Cells); return fmod(Cell.x + Cell.y, 2.0);"),
		CellCount, CellCount, CellCount));
}
