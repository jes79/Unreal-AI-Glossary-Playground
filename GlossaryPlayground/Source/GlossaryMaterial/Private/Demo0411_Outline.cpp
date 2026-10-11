#include "Demo0411_Outline.h"

#include "Animation/SkeletalMeshActor.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"

using namespace GlossaryKit;

ADemo0411_Outline::ADemo0411_Outline()
{
	DemoId = TEXT("0411");
	Controls = TEXT("Tab / 왼쪽 클릭 : 적 선택 (선택된 적만 Render CustomDepth ON)\n")
		TEXT("C : 외곽선 포스트 프로세스 ON/OFF    [ / ] : 두께 −/+\n")
		TEXT("관찰: 노란 테두리는 머티리얼이 아니라 화면 후처리 — Custom Depth 에 그려진 물체의 경계만 칠한다");
	Showcase = TEXT("선택한 적에 Render CustomDepth + 포스트 프로세스 머티리얼로 노란 외곽선, Tab 으로 선택 변경");
	bShowMouseCursor = true;

	OutlinePostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("OutlinePostProcess"));
	OutlinePostProcess->SetupAttachment(Root);
	OutlinePostProcess->bUnbound = true;
	OutlinePostProcess->Priority = 10.f;
}

void ADemo0411_Outline::BeginPlay()
{
	Super::BeginPlay();
	SetupFixedCamera(FVector(-750.f, -150.f, 330.f), FVector(0.f, -150.f, 90.f), 60.f);

	// 적 3명(마네킹) + 선택되지 않는 소품들
	for (int32 I = 0; I < 3; ++I)
	{
		ASkeletalMeshActor* Enemy = SpawnMannequin(FVector(I * 60.f, -80.f + I * 230.f, 0.f), FRotator(0.f, 70.f + I * 25.f, 0.f));
		Enemies.Add(Enemy->GetSkeletalMeshComponent());
		SpawnLabel(GetWorld(), FString::Printf(TEXT("적 %d"), I + 1), Enemy->GetActorLocation() + FVector(0.f, 0.f, 210.f), Colors::Enemy, 16.f);
	}
	SpawnBlock(GetWorld(), FVector(150.f, -300.f, 0.f), FVector(100.f, 100.f, 120.f), Colors::Wall);
	SpawnShape(GetWorld(), EShape::Sphere, FVector(-80.f, 380.f, 50.f), FVector(100.f), Colors::Ally);
	SpawnBlock(GetWorld(), FVector(250.f, 150.f, 0.f), FVector(60.f, 400.f, 160.f), Colors::Platform);

	// 블렌더블 등록: Post Process Volume 의 Rendering Features > Post Process Materials 와 같은 자리
	OutlineMaterial = UMaterialInstanceDynamic::Create(LoadMaterial(TEXT("M_OutlinePostProcess")), this);
	OutlinePostProcess->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, OutlineMaterial));
	Select(0);
}

void ADemo0411_Outline::Select(int32 Index)
{
	Selected = Index;
	for (int32 I = 0; I < Enemies.Num(); ++I)
	{
		// 핵심: 선택된 메시만 Custom Depth 버퍼에 그린다
		Enemies[I]->SetRenderCustomDepth(I == Selected);
	}
}

void ADemo0411_Outline::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::Tab)) { Select((Selected + 1) % FMath::Max(1, Enemies.Num())); }
	if (WasKeyPressed(EKeys::LeftMouseButton))
	{
		FHitResult Hit;
		if (GetMouseWorldHit(Hit))
		{
			const int32 Index = Enemies.IndexOfByKey(Hit.GetComponent());
			if (Index != INDEX_NONE) { Select(Index); }
		}
	}
	if (WasKeyPressed(EKeys::C)) { bOutline = !bOutline; }
	if (WasKeyPressed(EKeys::LeftBracket)) { OutlineThickness = FMath::Max(1.f, OutlineThickness - 1.f); }
	if (WasKeyPressed(EKeys::RightBracket)) { OutlineThickness = FMath::Min(10.f, OutlineThickness + 1.f); }

	if (OutlineMaterial)
	{
		OutlineMaterial->SetScalarParameterValue(TEXT("OutlineThickness"), OutlineThickness);
		OutlineMaterial->SetVectorParameterValue(TEXT("OutlineColor"), OutlineColor);
		OutlinePostProcess->Settings.WeightedBlendables.Array[0].Weight = bOutline ? 1.f : 0.f;
	}
	SetStatus(FString::Printf(TEXT("선택: 적 %d (Render CustomDepth ON) · 외곽선 PP %s · 두께 %.0fpx"),
		Selected + 1, bOutline ? TEXT("ON") : TEXT("OFF"), OutlineThickness));
}
