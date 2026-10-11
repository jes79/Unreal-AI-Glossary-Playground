#include "Demo0710_StateTree.h"

#include "Camera/CameraActor.h"
#include "Components/StateTreeAIComponent.h"
#include "D0710_StateTreeNodes.h"
#include "D07Common.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryKit.h"
#include "StateTree.h"

using namespace GlossaryKit;

ADemo0710_StateTree::ADemo0710_StateTree()
{
	DemoId = TEXT("0710");
	Controls = TEXT("WASD 이동 · V 카메라 전환\n")
		TEXT("관찰: Idle(2초) → Patrol 을 반복하다 시야(1000, 90°)에 플레이어가 들어오면 Chase,\n")
		TEXT("200 이내면 Attack. 벽 뒤로 숨어 2초가 지나면 Idle 로 돌아간다. 오른쪽 패널 = 활성 상태");
	Showcase = TEXT("코드로 작성·컴파일한 StateTree 에셋(Idle→Patrol→Chase→Attack, 보임·거리 조건 전환)을 StateTreeAIComponent 로 실행");
	bNeedsNavMesh = true;
}

void ADemo0710_StateTree::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	const TArray<FVector> Patrol = { FVector(300.f, 600.f, 0.f), FVector(1200.f, 600.f, 0.f), FVector(1200.f, -600.f, 0.f), FVector(300.f, -600.f, 0.f) };
	for (const FVector& Point : Patrol)
	{
		AStaticMeshActor* Pad = SpawnShape(World, EShape::Cylinder, Point + FVector(0.f, 0.f, 3.f), FVector(90.f, 90.f, 6.f), Colors::Cyan);
		Pad->SetActorEnableCollision(false);
	}
	SpawnBlock(World, FVector(-150.f, 700.f, 0.f), FVector(80.f, 500.f, 260.f), Colors::Wall);

	Enemy = D07::SpawnEnemy(World, FVector(300.f, -600.f, 0.f), 90.f, AD0710_StateTreeController::StaticClass(), Colors::Enemy);
	AD0710_StateTreeController* AIC = Cast<AD0710_StateTreeController>(Enemy->GetController());

	// 빌더가 만든 StateTree 에셋 (에디터의 StateTree 편집기로 열어 상태·전환을 그대로 볼 수 있다)
	EnemyStateTree = LoadAsset<UStateTree>(D0710::StateTreePath);
	if (AIC && EnemyStateTree && EnemyStateTree->IsReadyToRun())
	{
		AIC->PatrolPoints = Patrol;
		AIC->GetStateTreeComponent()->SetStateTree(EnemyStateTree);
		AIC->GetStateTreeComponent()->StartLogic();
	}

	SidePanel = D07::CreateSidePanel(this);
	OverviewCamera = D07::SetupOverviewCamera(this, FVector(-1500.f, 0.f, 1700.f), FRotator(-45.f, 0.f, 0.f));
}

void ADemo0710_StateTree::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::V))
	{
		D07::ToggleOverviewCamera(this, OverviewCamera);
	}
	AD0710_StateTreeController* AIC = Enemy ? Cast<AD0710_StateTreeController>(Enemy->GetController()) : nullptr;
	if (!AIC)
	{
		return;
	}
	if (!EnemyStateTree || !EnemyStateTree->IsReadyToRun())
	{
		SetStatus(TEXT("ST_Enemy 에셋이 없거나 컴파일되지 않음 — tools/glossary-build.sh -filter=07 로 빌더를 실행하세요"));
		return;
	}

	const TArray<FName> Active = AIC->GetStateTreeComponent()->GetActiveStateNames();
	const FString Leaf = Active.Num() > 0 ? Active.Last().ToString() : TEXT("-");
	if (Leaf != LastState)
	{
		LastState = Leaf;
		StateTime = 0.f;
		++TransitionCount;
	}
	StateTime += DeltaSeconds;

	// 시야 부채꼴과 공격 반경
	UWorld* World = GetWorld();
	const bool bSees = AIC->CanSeePlayer();
	DrawDebugCone(World, Enemy->GetActorLocation() - FVector(0.f, 0.f, 60.f), Enemy->GetActorForwardVector(), AIC->SightRadius,
		FMath::DegreesToRadians(AIC->SightHalfAngle), FMath::DegreesToRadians(5.f), 12, bSees ? FColor::Red : FColor(255, 220, 60), false, -1.f, 0, 2.f);
	DrawDebugCircle(World, Enemy->GetActorLocation() - FVector(0.f, 0.f, 90.f), AttackRange, 40, FColor::Orange, false, -1.f, 0, 3.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
	D07::DrawCurrentPath(AIC, FColor::Cyan, 4.f);

	FLinearColor Color = Colors::Ally;
	if (Leaf == TEXT("Patrol")) Color = Colors::Cyan;
	else if (Leaf == TEXT("Chase")) Color = Colors::Accent;
	else if (Leaf == TEXT("Attack")) Color = Colors::Enemy;
	Enemy->SetMarkerColor(Color);
	Enemy->SetLabel(Leaf, Color);

	// 상태 목록 (활성 상태 >> 표시) + 전환 조건
	auto Line = [&Leaf](const TCHAR* State, const TCHAR* Transitions)
	{
		return FString::Printf(TEXT("%s%s\n      %s\n"), Leaf == State ? TEXT(">> ") : TEXT("    "), State, Transitions);
	};
	FString Body = TEXT("Root\n");
	Body += Line(TEXT("Idle"), TEXT("완료(2초) → Patrol · [보임] → Chase"));
	Body += Line(TEXT("Patrol"), TEXT("완료(도착) → Idle · [보임] → Chase"));
	Body += Line(TEXT("Chase"), TEXT("[200 이내] → Attack · [2초간 못 봄] → Idle"));
	Body += Line(TEXT("Attack"), TEXT("[250 밖] → Chase"));
	Body += FString::Printf(TEXT("\n활성 경로: %s"), *FString::JoinBy(Active, TEXT(" / "), [](const FName& N) { return N.ToString(); }));
	if (SidePanel)
	{
		SidePanel->SetPanelText(TEXT("ST_Enemy (StateTree 에셋)"), Body);
	}

	SetStatus(FString::Printf(TEXT("활성 상태 %s (%.1f초째) · 태스크: %s\n플레이어 %s · 거리 %.0fcm · 상태 전환 %d회 · 공격 %d회"),
		*Leaf, StateTime, *AIC->CurrentTask, bSees ? TEXT("보임") : TEXT("안 보임"), AIC->DistanceToPlayer(), TransitionCount - 1, Enemy->AttackCount));
}
