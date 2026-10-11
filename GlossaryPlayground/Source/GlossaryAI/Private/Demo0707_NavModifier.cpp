#include "Demo0707_NavModifier.h"

#include "AIController.h"
#include "Camera/CameraActor.h"
#include "D07Common.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GlossaryKit.h"
#include "NavAreas/NavArea_Default.h"
#include "NavAreas/NavArea_Null.h"
#include "NavModifierVolume.h"
#include "Navigation/PathFollowingComponent.h"

#if WITH_EDITOR
#include "BSPOps.h"
#include "Builders/CubeBuilder.h"
#include "Components/BrushComponent.h"
#include "Engine/Polys.h"
#endif

using namespace GlossaryKit;

UD0707_NavArea_Swamp::UD0707_NavArea_Swamp()
{
	DefaultCost = 10.f;                    // 이 영역을 지나는 거리 × 10 이 경로 비용
	DrawColor = FColor(140, 90, 30);       // NavMesh 표시에서 갈색
}

ADemo0707_NavModifier::ADemo0707_NavModifier()
{
	DemoId = TEXT("0707");
	Controls = TEXT("C : 늪 Nav Area 비용 10 ↔ 기본(1) 전환 · V 카메라 전환\n")
		TEXT("관찰: AI 가 좌우를 왕복할 때 갈색 늪(비용 10)을 아래로 돌아가고, 위쪽 용암(NavArea_Null)은\n")
		TEXT("NavMesh 가 잘려 지나갈 수 없다. C 를 누르면 늪을 곧장 가로지른다");
	Showcase = TEXT("Nav Modifier Volume: 늪 = 비용 10 Nav Area(우회), 용암 = NavArea_Null(차단), 비용 토글로 경로 비교");
	bNeedsNavMesh = true;
}

#if WITH_EDITOR
/** 박스 브러시 볼륨 (레벨 빌더의 NavMeshBounds 와 같은 방법) */
static ANavModifierVolume* SpawnModifierVolume(UWorld* World, const FVector& Center, const FVector& Size, TSubclassOf<UNavArea> Area, const FString& Label)
{
	ANavModifierVolume* Volume = World->SpawnActor<ANavModifierVolume>(Center + FVector(0.f, 0.f, Size.Z * 0.5f - 50.f), FRotator::ZeroRotator);
	Volume->PolyFlags = 0;
	Volume->Brush = NewObject<UModel>(Volume, NAME_None, RF_Transactional);
	Volume->Brush->Initialize(nullptr, true);
	Volume->Brush->Polys = NewObject<UPolys>(Volume->Brush, NAME_None, RF_Transactional);
	Volume->GetBrushComponent()->Brush = Volume->Brush;
	UCubeBuilder* Builder = NewObject<UCubeBuilder>(Volume);
	Builder->X = Size.X;
	Builder->Y = Size.Y;
	Builder->Z = Size.Z;
	Volume->BrushBuilder = Builder;
	Builder->Build(World, Volume);
	FBSPOps::csgPrepMovingBrush(Volume);
	Volume->GetBrushComponent()->RequestUpdateBrushCollision();
	Volume->SetAreaClass(Area);
	Volume->Tags.Add(*Label);
	Volume->SetActorLabel(Label);
	Volume->PostEditChange();
	return Volume;
}

void ADemo0707_NavModifier::BuildLevel(UWorld* World)
{
	// 레벨에 저장되는 Nav Modifier Volume 두 개 → 빌더가 NavMesh 를 구울 때 바로 반영된다
	SpawnModifierVolume(World, SwampCenter, SwampSize, UD0707_NavArea_Swamp::StaticClass(), TEXT("NavModifier_Swamp"));
	SpawnModifierVolume(World, LavaCenter, LavaSize, UNavArea_Null::StaticClass(), TEXT("NavModifier_Lava"));
}
#endif

void ADemo0707_NavModifier::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	for (TActorIterator<ANavModifierVolume> It(World); It; ++It)
	{
		if (It->Tags.Contains(TEXT("NavModifier_Swamp")))
		{
			SwampVolume = *It;
		}
	}

	// 눈에 보이는 늪·용암 바닥 (NavMesh 에는 영향 없음 — 비용은 볼륨이 정한다)
	auto MakeFloor = [World](const FVector& Center, const FVector& Size, const FLinearColor& Color, float Emissive)
	{
		AStaticMeshActor* Floor = SpawnShape(World, EShape::Cube, FVector(Center.X, Center.Y, 1.f), FVector(Size.X, Size.Y, 2.f), Color);
		Floor->SetActorEnableCollision(false);
		Floor->GetStaticMeshComponent()->SetCanEverAffectNavigation(false);
		SetColor(Floor->GetStaticMeshComponent(), Color, 0.3f, 0.f, Emissive);
	};
	MakeFloor(SwampCenter, SwampSize, FLinearColor(0.18f, 0.12f, 0.04f), 0.f);
	MakeFloor(LavaCenter, LavaSize, FLinearColor(1.f, 0.25f, 0.02f), 3.f);
	SpawnLabel(World, TEXT("늪 (Nav Area 비용 10)"), FVector(SwampCenter.X, SwampCenter.Y - 300.f, 120.f), FLinearColor(0.9f, 0.7f, 0.4f), 20.f);
	SpawnLabel(World, TEXT("용암 (NavArea_Null)"), FVector(LavaCenter.X, LavaCenter.Y, 120.f), FLinearColor(1.f, 0.5f, 0.2f), 20.f);

	Agent = D07::SpawnEnemy(World, PointA, 0.f, AAIController::StaticClass(), Colors::Enemy);
	for (const FVector& P : { PointA, PointB })
	{
		AStaticMeshActor* Pad = SpawnShape(World, EShape::Cylinder, P + FVector(0.f, 0.f, 3.f), FVector(100.f, 100.f, 6.f), Colors::Cyan);
		Pad->SetActorEnableCollision(false);
		Pad->GetStaticMeshComponent()->SetCanEverAffectNavigation(false);
	}

	OverviewCamera = D07::SetupOverviewCamera(this, FVector(0.f, -2700.f, 2900.f), FRotator(-55.f, 90.f, 0.f), 75.f);
}

void ADemo0707_NavModifier::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* World = GetWorld();
	if (WasKeyPressed(EKeys::V))
	{
		D07::ToggleOverviewCamera(this, OverviewCamera);
	}
	AAIController* AIC = Agent ? Cast<AAIController>(Agent->GetController()) : nullptr;
	if (!AIC)
	{
		return;
	}

	if (WasKeyPressed(EKeys::C) && SwampVolume)
	{
		bSwampCostly = !bSwampCostly;
		// 실행 중 AreaClass 를 바꾸면 Dynamic NavMesh 가 그 영역을 다시 굽는다
		SwampVolume->SetAreaClass(bSwampCostly ? TSubclassOf<UNavArea>(UD0707_NavArea_Swamp::StaticClass()) : TSubclassOf<UNavArea>(UNavArea_Default::StaticClass()));
		WaitTimer = 0.4f;   // 다시 구운 뒤 경로를 새로 잡는다
		AIC->StopMovement();
	}

	// 왕복
	const FVector Goal = bGoingRight ? PointB : PointA;
	if (AIC->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		if (FVector::Dist2D(Agent->GetActorLocation(), Goal) < 120.f)
		{
			bGoingRight = !bGoingRight;
			WaitTimer = 0.8f;
		}
		WaitTimer -= DeltaSeconds;
		if (WaitTimer <= 0.f)
		{
			AIC->MoveToLocation(bGoingRight ? PointB : PointA, 50.f);
		}
	}

	D07::DrawNavMesh(World, 6.f, 0.5f);
	D07::DrawCurrentPath(AIC, FColor(255, 230, 40), 8.f);

	float PathLength = 0.f, PathCost = 0.f;
	if (const UPathFollowingComponent* Follow = AIC->GetPathFollowingComponent())
	{
		if (Follow->GetPath().IsValid() && Follow->GetPath()->IsValid())
		{
			PathLength = Follow->GetPath()->GetLength();
			PathCost = Follow->GetPath()->GetCost();
		}
	}
	const float StraightCost = (PointB.X - PointA.X - SwampSize.X) + SwampSize.X * (bSwampCostly ? 10.f : 1.f);
	Agent->SetLabel(bSwampCostly ? TEXT("늪은 비싸다 → 우회") : TEXT("늪 비용 1 → 직진"), Colors::Item);
	SetStatus(FString::Printf(TEXT("늪 AreaClass: %s · 용암: NavArea_Null\n현재 경로 길이 %.0fcm · 경로 비용 %.0f (늪을 직진했다면 %.0f)"),
		bSwampCostly ? TEXT("NavArea_Swamp (DefaultCost 10)") : TEXT("NavArea_Default (비용 1)"), PathLength, PathCost, StraightCost));
}
