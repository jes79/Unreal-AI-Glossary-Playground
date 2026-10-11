#include "Demo0702_NavMeshMoveTo.h"

#include "AIController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Camera/CameraActor.h"
#include "D07Common.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

using namespace GlossaryKit;

ADemo0702_NavMeshMoveTo::ADemo0702_NavMeshMoveTo()
{
	DemoId = TEXT("0702");
	Controls = TEXT("마우스 왼쪽 클릭 : 그 위치로 AI 이동 (SimpleMoveToLocation)\n")
		TEXT("P : NavMesh 표시 ON/OFF\n")
		TEXT("관찰: 초록 NavMesh 는 벽 주변이 깎여 있고, AI 는 노란 경로를 따라 벽을 돌아간다");
	Showcase = TEXT("Nav Mesh Bounds Volume 으로 구운 NavMesh 위에서 클릭 위치로 SimpleMoveToLocation, P 로 NavMesh 표시");
	bNeedsNavMesh = true;
	NavBoundsExtent = FVector(2500.f, 2500.f, 600.f);
	PlayerPawnClass = nullptr;   // 고정 카메라 + 마우스 클릭
	bShowMouseCursor = true;
}

void ADemo0702_NavMeshMoveTo::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 미로 같은 벽 (실행 중 스폰 → Dynamic NavMesh 가 이 부분을 깎아 다시 굽는다)
	SpawnBlock(World, FVector(0.f, -250.f, 0.f), FVector(60.f, 1300.f, 220.f), Colors::Wall);
	SpawnBlock(World, FVector(500.f, 400.f, 0.f), FVector(1000.f, 60.f, 220.f), Colors::Wall);
	SpawnBlock(World, FVector(1100.f, -350.f, 0.f), FVector(60.f, 900.f, 220.f), Colors::Wall);
	SpawnBlock(World, FVector(-700.f, 700.f, 0.f), FVector(400.f, 400.f, 220.f), Colors::Wall);
	SpawnBlock(World, FVector(-600.f, -800.f, 0.f), FVector(300.f, 300.f, 220.f), Colors::Wall);

	Agent = D07::SpawnEnemy(World, FVector(-1300.f, -200.f, 0.f), 0.f, AAIController::StaticClass(), Colors::Enemy);
	Agent->SetLabel(TEXT("AI (NavMesh 길찾기)"), Colors::Item);

	GoalMarker = SpawnShape(World, EShape::Cylinder, FVector(0.f, 0.f, -500.f), FVector(80.f, 80.f, 6.f), Colors::Item);
	GoalMarker->SetActorEnableCollision(false);
	GoalMarker->GetStaticMeshComponent()->SetCanEverAffectNavigation(false);

	OverviewCamera = D07::SetupOverviewCamera(this, FVector(-1100.f, 0.f, 2300.f), FRotator(-62.f, 0.f, 0.f));
}

void ADemo0702_NavMeshMoveTo::MoveAgentTo(const FVector& Location)
{
	AAIController* AIC = Agent ? Cast<AAIController>(Agent->GetController()) : nullptr;
	if (!AIC)
	{
		return;
	}
	// 핵심 한 줄: 컨트롤러의 PathFollowingComponent 가 NavMesh 경로를 찾아 그대로 걷는다
	UAIBlueprintHelperLibrary::SimpleMoveToLocation(AIC, Location);
	LastGoal = Location;
	GoalMarker->SetActorLocation(Location + FVector(0.f, 0.f, 4.f));
}

void ADemo0702_NavMeshMoveTo::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* World = GetWorld();

	// 시연: 1.5초 뒤(실행 중 스폰한 벽이 NavMesh 에 반영된 뒤) 벽 너머로 한 번 이동
	DemoTimer += DeltaSeconds;
	if (!bDemoMoveIssued && DemoTimer > 1.5f)
	{
		bDemoMoveIssued = true;
		MoveAgentTo(DemoGoal);
	}

	if (WasKeyPressed(EKeys::LeftMouseButton))
	{
		FHitResult Hit;
		if (GetMouseWorldHit(Hit))
		{
			++ClickCount;
			MoveAgentTo(Hit.Location);
		}
	}
	if (WasKeyPressed(EKeys::P))
	{
		bShowNavMesh = !bShowNavMesh;
	}
	if (bShowNavMesh)
	{
		// 에디터의 "show Navigation"(P 키) 과 같은 내용을 DrawDebug 로 그린다 (게임 실행 중에도 보이도록)
		D07::DrawNavMesh(World);
	}

	AAIController* AIC = Agent ? Cast<AAIController>(Agent->GetController()) : nullptr;
	if (!AIC)
	{
		return;
	}
	D07::DrawCurrentPath(AIC, FColor(255, 210, 30), 8.f);

	// 목표가 NavMesh 위로 투영되는지 (벽 안을 클릭하면 실패)
	FNavLocation Projected;
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	const bool bOnNav = NavSys && NavSys->ProjectPointToNavigation(LastGoal, Projected, FVector(50.f, 50.f, 200.f));

	float PathLength = 0.f;
	int32 PathPoints = 0;
	if (UPathFollowingComponent* Follow = AIC->GetPathFollowingComponent())
	{
		if (Follow->GetPath().IsValid())
		{
			PathLength = Follow->GetPath()->GetLength();
			PathPoints = Follow->GetPath()->GetPathPoints().Num();
		}
	}
	SetStatus(FString::Printf(TEXT("NavMesh 표시 %s · 클릭 %d회\n이동 상태 %s · 경로 꼭짓점 %d개 · 경로 길이 %.0fcm · 남은 직선거리 %.0fcm\n목표가 NavMesh 위? %s"),
		bShowNavMesh ? TEXT("ON") : TEXT("OFF"), ClickCount,
		AIC->GetMoveStatus() == EPathFollowingStatus::Moving ? TEXT("Moving") : TEXT("Idle"),
		PathPoints, PathLength, FVector::Dist2D(Agent->GetActorLocation(), LastGoal),
		bOnNav ? TEXT("예") : TEXT("아니오 (벽 안·NavMesh 밖은 갈 수 없음)")));
}
