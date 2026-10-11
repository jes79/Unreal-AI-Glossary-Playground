#include "Demo0703_BehaviorTree.h"

#include "Animation/AnimationAsset.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BehaviorTree/Decorators/BTDecorator_Blackboard.h"
#include "BehaviorTree/Decorators/BTDecorator_IsAtLocation.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "BehaviorTree/Tasks/BTTask_PlayAnimation.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "Camera/CameraActor.h"
#include "D07Common.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryKit.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

using namespace GlossaryKit;

// ============================================================ BT 조립 (빌더·런타임 공용)

namespace D0703
{
	static FBTCompositeChild TaskChild(UBTTaskNode* Task, UBTDecorator* Decorator = nullptr)
	{
		FBTCompositeChild Child;
		Child.ChildTask = Task;
		if (Decorator)
		{
			Child.Decorators.Add(Decorator);
		}
		return Child;
	}

	static UBTTask_MoveTo* MakeMoveTo(UBehaviorTree* Tree, FName Key, float Radius, const FString& Name)
	{
		UBTTask_MoveTo* Move = NewObject<UBTTask_MoveTo>(Tree);
		Move->NodeName = Name;
		D07::SetBlackboardKey(Move, TEXT("BlackboardKey"), Key);
		Move->AcceptableRadius = FValueOrBBKey_Float(Radius);
		// 캡슐 반지름을 더하지 않고 중심 거리로만 도착 판정 (200 이내 공격과 맞추기 위해)
		Move->bReachTestIncludesAgentRadius = FValueOrBBKey_Bool(false);
		Move->bReachTestIncludesGoalRadius = FValueOrBBKey_Bool(false);
		Move->bObserveBlackboardValue = true;   // 키 값(순찰 지점)이 바뀌면 경로를 다시 잡는다
		return Move;
	}

	static UBTTask_Wait* MakeWait(UBehaviorTree* Tree, float Seconds, const FString& Name)
	{
		UBTTask_Wait* Wait = NewObject<UBTTask_Wait>(Tree);
		Wait->NodeName = Name;
		Wait->WaitTime = FValueOrBBKey_Float(Seconds);
		return Wait;
	}

	void BuildEnemyTree(UBehaviorTree* Tree, UBlackboardData* Blackboard, UAnimationAsset* AttackAnim, float AttackRange)
	{
		// ---- Blackboard: AI 의 기억 저장소. 키 이름과 타입만 정의한다 ----
		Blackboard->Keys.Reset();
		{
			FBlackboardEntry Target;
			Target.EntryName = TEXT("TargetActor");
			UBlackboardKeyType_Object* ObjectType = NewObject<UBlackboardKeyType_Object>(Blackboard);
			ObjectType->BaseClass = AActor::StaticClass();
			Target.KeyType = ObjectType;
			Blackboard->Keys.Add(Target);

			FBlackboardEntry Patrol;
			Patrol.EntryName = TEXT("PatrolLocation");
			Patrol.KeyType = NewObject<UBlackboardKeyType_Vector>(Blackboard);
			Blackboard->Keys.Add(Patrol);
		}
		Blackboard->UpdateKeyIDs();
		Tree->BlackboardAsset = Blackboard;

		// ---- Behavior Tree: 위 가지부터 조건을 검사해 처음 성공하는 가지를 실행 ----
		UBTComposite_Selector* Root = NewObject<UBTComposite_Selector>(Tree);
		Root->NodeName = TEXT("Selector");

		// 추격 → 공격 가지: TargetActor 가 있을 때만. Abort Both = 값이 바뀌면 즉시 이 가지/아래 가지를 끊는다
		UBTDecorator_Blackboard* HasTarget = NewObject<UBTDecorator_Blackboard>(Tree);
		HasTarget->NodeName = TEXT("TargetActor Is Set");
		D07::SetBlackboardKey(HasTarget, TEXT("BlackboardKey"), TEXT("TargetActor"));
		D07::SetProperty<uint8>(HasTarget, TEXT("OperationType"), static_cast<uint8>(EBasicKeyOperation::Set));
#if WITH_EDITORONLY_DATA
		D07::SetProperty<uint8>(HasTarget, TEXT("BasicOperation"), static_cast<uint8>(EBasicKeyOperation::Set));   // 디테일 패널 표시용
#endif
		D07::SetProperty<uint8>(HasTarget, TEXT("FlowAbortMode"), static_cast<uint8>(EBTFlowAbortMode::Both));

		UBTDecorator_IsAtLocation* InRange = NewObject<UBTDecorator_IsAtLocation>(Tree);
		InRange->NodeName = FString::Printf(TEXT("거리 ≤ %.0f"), AttackRange);
		D07::SetBlackboardKey(InRange, TEXT("BlackboardKey"), TEXT("TargetActor"));
		D07::SetProperty<float>(InRange, TEXT("AcceptableRadius"), AttackRange);

		UBTTask_PlayAnimation* Attack = NewObject<UBTTask_PlayAnimation>(Tree);
		Attack->NodeName = TEXT("PlayAnimation 공격");
		Attack->AnimationToPlay = FValueOrBBKey_Object(TObjectPtr<UAnimationAsset>(AttackAnim));

		UBTComposite_Sequence* Chase = NewObject<UBTComposite_Sequence>(Tree);
		Chase->NodeName = TEXT("Sequence 추격→공격");
		Chase->Children.Add(TaskChild(MakeMoveTo(Tree, TEXT("TargetActor"), AttackRange - 40.f, TEXT("MoveTo TargetActor (추격)"))));
		Chase->Children.Add(TaskChild(Attack, InRange));
		Chase->Children.Add(TaskChild(MakeWait(Tree, 0.3f, TEXT("Wait 0.3초"))));

		// 순찰 가지: 조건 없음(맨 마지막 = 기본 행동)
		UBTComposite_Sequence* Patrol = NewObject<UBTComposite_Sequence>(Tree);
		Patrol->NodeName = TEXT("Sequence 순찰");
		Patrol->Children.Add(TaskChild(MakeMoveTo(Tree, TEXT("PatrolLocation"), 50.f, TEXT("MoveTo PatrolLocation"))));
		Patrol->Children.Add(TaskChild(MakeWait(Tree, 1.f, TEXT("Wait 1초"))));

		FBTCompositeChild ChaseChild;
		ChaseChild.ChildComposite = Chase;
		ChaseChild.Decorators.Add(HasTarget);
		Root->Children.Add(ChaseChild);
		FBTCompositeChild PatrolChild;
		PatrolChild.ChildComposite = Patrol;
		Root->Children.Add(PatrolChild);

		Tree->RootNode = Root;
	}
}

// ============================================================ 컨트롤러

AD0703_EnemyController::AD0703_EnemyController()
{
	PrimaryActorTick.bCanEverTick = true;

	// 시야: 반경 1200, 시야각 90°(좌우 45°) — 보이면 TargetActor 를 채운다
	UAIPerceptionComponent* Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
	UAISenseConfig_Sight* Sight = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	Sight->SightRadius = 1200.f;
	Sight->LoseSightRadius = 1500.f;
	Sight->PeripheralVisionAngleDegrees = 45.f;
	Sight->DetectionByAffiliation.bDetectEnemies = true;
	Sight->DetectionByAffiliation.bDetectNeutrals = true;
	Sight->DetectionByAffiliation.bDetectFriendlies = true;
	Sight->SetMaxAge(3.f);
	Perception->ConfigureSense(*Sight);
	Perception->SetDominantSense(Sight->GetSenseImplementation());
	SetPerceptionComponent(*Perception);
}

void AD0703_EnemyController::StartTree(UBehaviorTree* Tree, const TArray<FVector>& InPatrolPoints)
{
	PatrolPoints = InPatrolPoints;
	PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AD0703_EnemyController::OnTargetPerceived);
	// RunBehaviorTree 가 Tree->BlackboardAsset 으로 Blackboard 컴포넌트를 만들고 트리를 시작한다
	RunBehaviorTree(Tree);
	if (Blackboard && PatrolPoints.Num() > 0)
	{
		Blackboard->SetValueAsVector(TEXT("PatrolLocation"), PatrolPoints[0]);
	}
}

void AD0703_EnemyController::OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Blackboard || Actor != UGameplayStatics::GetPlayerPawn(this, 0))
	{
		return;
	}
	if (Stimulus.WasSuccessfullySensed())
	{
		Blackboard->SetValueAsObject(TEXT("TargetActor"), Actor);
	}
	else
	{
		Blackboard->ClearValue(TEXT("TargetActor"));
	}
}

void AD0703_EnemyController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Blackboard || !GetPawn() || PatrolPoints.Num() == 0)
	{
		return;
	}
	// 순찰 지점에 도착하면 다음 지점으로 (BT 의 MoveTo 가 키 변경을 감지해 다음 지점으로 간다)
	const FVector Current = Blackboard->GetValueAsVector(TEXT("PatrolLocation"));
	if (FVector::Dist2D(GetPawn()->GetActorLocation(), Current) < 90.f)
	{
		PatrolIndex = (PatrolIndex + 1) % PatrolPoints.Num();
		Blackboard->SetValueAsVector(TEXT("PatrolLocation"), PatrolPoints[PatrolIndex]);
	}
}

// ============================================================ 데모

ADemo0703_BehaviorTree::ADemo0703_BehaviorTree()
{
	DemoId = TEXT("0703");
	Controls = TEXT("WASD 이동 · V 카메라 전환\n")
		TEXT("관찰: 적이 4지점을 순찰하다 시야(부채꼴)에 플레이어가 들어오면 TargetActor 가 채워져 추격,\n")
		TEXT("200 이내면 공격. 오른쪽 패널의 >> 가 지금 실행 중인 BT 노드");
	Showcase = TEXT("BT_Enemy(순찰→추격→200 이내 공격) + BB_Enemy(TargetActor·PatrolLocation), 실행 중 노드와 Blackboard 값 표시");
	bNeedsNavMesh = true;
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-300.f, -300.f, 100.f));
}

void ADemo0703_BehaviorTree::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 에셋 빌더가 만든 BT_Enemy 를 쓰고, 없으면(빌더를 안 돌렸으면) 같은 함수로 실행 중에 조립한다
	EnemyTree = LoadAsset<UBehaviorTree>(D0703::TreePath);
	bTreeFromAsset = EnemyTree != nullptr && EnemyTree->RootNode != nullptr;
	if (!bTreeFromAsset)
	{
		EnemyTree = NewObject<UBehaviorTree>(this);
		UBlackboardData* BB = NewObject<UBlackboardData>(this);
		D0703::BuildEnemyTree(EnemyTree, BB, LoadAsset<UAnimationAsset>(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01")), AttackRange);
	}

	const TArray<FVector> Patrol = { FVector(300.f, 600.f, 0.f), FVector(1200.f, 600.f, 0.f), FVector(1200.f, -600.f, 0.f), FVector(300.f, -600.f, 0.f) };
	for (const FVector& Point : Patrol)
	{
		AStaticMeshActor* Pad = SpawnShape(World, EShape::Cylinder, Point + FVector(0.f, 0.f, 3.f), FVector(90.f, 90.f, 6.f), Colors::Cyan);
		Pad->SetActorEnableCollision(false);
	}
	SpawnBlock(World, FVector(-100.f, -900.f, 0.f), FVector(200.f, 200.f, 250.f), Colors::Wall);

	Enemy = D07::SpawnEnemy(World, FVector(300.f, -600.f, 0.f), 90.f, AD0703_EnemyController::StaticClass(), Colors::Enemy);
	if (AD0703_EnemyController* AIC = Cast<AD0703_EnemyController>(Enemy->GetController()))
	{
		AIC->StartTree(EnemyTree, Patrol);
	}

	SidePanel = D07::CreateSidePanel(this);
	OverviewCamera = D07::SetupOverviewCamera(this, FVector(-1300.f, -900.f, 1900.f), FRotator(-50.f, 22.f, 0.f));
}

void ADemo0703_BehaviorTree::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::V))
	{
		D07::ToggleOverviewCamera(this, OverviewCamera);
	}
	AAIController* AIC = Enemy ? Cast<AAIController>(Enemy->GetController()) : nullptr;
	UBehaviorTreeComponent* BTComp = AIC ? Cast<UBehaviorTreeComponent>(AIC->GetBrainComponent()) : nullptr;
	UBlackboardComponent* BB = AIC ? AIC->GetBlackboardComponent() : nullptr;
	if (!BTComp || !BB)
	{
		SetStatus(TEXT("Behavior Tree 가 실행되지 않음"));
		return;
	}

	// 시야 부채꼴(1200, ±45°)과 공격 반경(200)
	UWorld* World = GetWorld();
	const AActor* Target = Cast<AActor>(BB->GetValueAsObject(TEXT("TargetActor")));
	const FColor ConeColor = Target ? FColor::Red : FColor(255, 220, 60);
	D07::DrawSightFan(World, Enemy->GetActorLocation() - FVector(0.f, 0.f, 85.f), Enemy->GetActorForwardVector(), 1200.f, 45.f, ConeColor);
	DrawDebugCircle(World, Enemy->GetActorLocation() - FVector(0.f, 0.f, 90.f), AttackRange, 40, FColor::Orange, false, -1.f, 0, 3.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
	D07::DrawCurrentPath(AIC, Target ? FColor::Red : FColor::Cyan, 4.f);

	const UBTNode* Active = BTComp->GetActiveNode();
	const FString ActiveName = Active ? Active->GetNodeName() : TEXT("-");
	if (Active != LastActiveNode && Active && Active->IsA<UBTTask_PlayAnimation>())
	{
		++Enemy->AttackCount;   // 공격 노드에 새로 들어올 때마다 1회
	}
	LastActiveNode = Active;
	Enemy->SetMarkerColor(Target ? Colors::Enemy : Colors::Cyan);
	Enemy->SetLabel(ActiveName, Target ? FLinearColor(1.f, 0.4f, 0.3f) : FLinearColor(0.6f, 0.9f, 1.f));

	const FVector PatrolLocation = BB->GetValueAsVector(TEXT("PatrolLocation"));
	const FString BBText = FString::Printf(TEXT("Blackboard\n  TargetActor    = %s\n  PatrolLocation = (%.0f, %.0f)"),
		Target ? *Target->GetName() : TEXT("None"), PatrolLocation.X, PatrolLocation.Y);
	if (SidePanel)
	{
		SidePanel->SetPanelText(FString::Printf(TEXT("BT_Enemy %s"), bTreeFromAsset ? TEXT("(에셋)") : TEXT("(실행 중 조립)")),
			D07::DescribeBehaviorTree(EnemyTree->RootNode, Active) + TEXT("\n") + BBText);
	}

	const APawn* Player = GetPlayerPawn();
	SetStatus(FString::Printf(TEXT("실행 중 노드: %s\nTargetActor %s · 플레이어까지 %.0fcm · 공격 %d회"),
		*ActiveName, Target ? TEXT("있음 → 추격/공격 가지") : TEXT("없음 → 순찰 가지"),
		Player ? FVector::Dist2D(Player->GetActorLocation(), Enemy->GetActorLocation()) : 0.f,
		Enemy->AttackCount));
}
