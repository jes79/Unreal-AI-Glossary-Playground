#include "Demo0704_BTNodes.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Decorators/BTDecorator_Blackboard.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "Camera/CameraActor.h"
#include "D0704_BTNodes.h"
#include "D07Common.h"
#include "DrawDebugHelpers.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0704_BTNodes::ADemo0704_BTNodes()
{
	DemoId = TEXT("0704");
	Controls = TEXT("WASD 이동 · 1 : 적에게 피해 25 · 2 : HP 20% 로 · V 카메라 전환\n")
		TEXT("관찰: Service 가 0.5초마다 거리를 갱신해 200 이내면 Attack Task,\n")
		TEXT("HP 30% 이하가 되면 Decorator 가 추격을 끊고 Flee. 3초 뒤 회복해 30%를 넘으면 다시 추격");
	Showcase = TEXT("C++ Decorator(HP ≤ 30% 도망)·Service(0.5초 거리 갱신)·Task(공격·도망)로 조립한 트리, 피해 키로 분기 확인");
	bNeedsNavMesh = true;
}

void ADemo0704_BTNodes::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// ---- Blackboard ----
	UBlackboardData* BB = NewObject<UBlackboardData>(this);
	{
		FBlackboardEntry Target;
		Target.EntryName = TEXT("TargetActor");
		UBlackboardKeyType_Object* ObjectType = NewObject<UBlackboardKeyType_Object>(BB);
		ObjectType->BaseClass = AActor::StaticClass();
		Target.KeyType = ObjectType;
		BB->Keys.Add(Target);
		FBlackboardEntry Distance;
		Distance.EntryName = TEXT("TargetDistance");
		Distance.KeyType = NewObject<UBlackboardKeyType_Float>(BB);
		BB->Keys.Add(Distance);
		FBlackboardEntry Ticks;
		Ticks.EntryName = TEXT("ServiceTicks");
		Ticks.KeyType = NewObject<UBlackboardKeyType_Int>(BB);
		BB->Keys.Add(Ticks);
		BB->UpdateKeyIDs();
	}

	// ---- 트리: 실행 중에 C++ 노드로 조립 (에디터에서는 같은 노드가 노드 목록에 "Attack (C++)" 등으로 나온다) ----
	Tree = NewObject<UBehaviorTree>(this);
	Tree->BlackboardAsset = BB;
	UBTComposite_Selector* RootSelector = NewObject<UBTComposite_Selector>(Tree);
	RootSelector->NodeName = TEXT("Selector");
	RootSelector->Services.Add(NewObject<UD0704_BTService_UpdateDistance>(Tree));

	FBTCompositeChild FleeChild;
	FleeChild.ChildTask = NewObject<UD0704_BTTask_Flee>(Tree);
	FleeChild.Decorators.Add(NewObject<UD0704_BTDecorator_HealthBelow>(Tree));
	RootSelector->Children.Add(FleeChild);

	UBTDecorator_Blackboard* InRange = NewObject<UBTDecorator_Blackboard>(Tree);
	InRange->NodeName = FString::Printf(TEXT("TargetDistance ≤ %.0f"), AttackRange);
	D07::SetBlackboardKey(InRange, TEXT("BlackboardKey"), TEXT("TargetDistance"));
	D07::SetProperty<uint8>(InRange, TEXT("OperationType"), static_cast<uint8>(EArithmeticKeyOperation::LessOrEqual));
	D07::SetProperty<float>(InRange, TEXT("FloatValue"), AttackRange);
	D07::SetProperty<uint8>(InRange, TEXT("FlowAbortMode"), static_cast<uint8>(EBTFlowAbortMode::Both));
	FBTCompositeChild AttackChild;
	AttackChild.ChildTask = NewObject<UD0704_BTTask_Attack>(Tree);
	AttackChild.Decorators.Add(InRange);
	RootSelector->Children.Add(AttackChild);

	UBTTask_MoveTo* Chase = NewObject<UBTTask_MoveTo>(Tree);
	Chase->NodeName = TEXT("MoveTo TargetActor (추격)");
	D07::SetBlackboardKey(Chase, TEXT("BlackboardKey"), TEXT("TargetActor"));
	Chase->AcceptableRadius = FValueOrBBKey_Float(AttackRange - 60.f);
	Chase->bReachTestIncludesAgentRadius = FValueOrBBKey_Bool(false);
	Chase->bReachTestIncludesGoalRadius = FValueOrBBKey_Bool(false);
	FBTCompositeChild ChaseChild;
	ChaseChild.ChildTask = Chase;
	RootSelector->Children.Add(ChaseChild);
	Tree->RootNode = RootSelector;

	SpawnBlock(World, FVector(300.f, 500.f, 0.f), FVector(200.f, 200.f, 250.f), Colors::Wall);
	SpawnBlock(World, FVector(300.f, -500.f, 0.f), FVector(200.f, 200.f, 250.f), Colors::Wall);

	Enemy = D07::SpawnEnemy(World, FVector(1000.f, 0.f, 0.f), 180.f, AAIController::StaticClass(), Colors::Enemy);
	if (AAIController* AIC = Cast<AAIController>(Enemy->GetController()))
	{
		AIC->RunBehaviorTree(Tree);
		AIC->GetBlackboardComponent()->SetValueAsObject(TEXT("TargetActor"), GetPlayerPawn());
	}

	SidePanel = D07::CreateSidePanel(this);
	OverviewCamera = D07::SetupOverviewCamera(this, FVector(-1400.f, -900.f, 1800.f), FRotator(-48.f, 22.f, 0.f));
}

void ADemo0704_BTNodes::Tick(float DeltaSeconds)
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
		return;
	}

	// 피해와 회복 (데모 입력)
	SinceLastHit += DeltaSeconds;
	if (WasKeyPressed(EKeys::One))
	{
		Enemy->Health = FMath::Max(5.f, Enemy->Health - DamagePerHit);
		SinceLastHit = 0.f;
	}
	if (WasKeyPressed(EKeys::Two))
	{
		Enemy->Health = 20.f;   // 한 번에 빈사 상태로
		SinceLastHit = 0.f;
	}
	if (SinceLastHit > RegenDelay)
	{
		Enemy->Health = FMath::Min(Enemy->MaxHealth, Enemy->Health + RegenPerSecond * DeltaSeconds);
	}

	const float Ratio = Enemy->GetHealthRatio();
	const UBTNode* Active = BTComp->GetActiveNode();
	const FString ActiveName = Active ? Active->GetNodeName() : TEXT("-");
	const bool bFleeing = Active && Active->IsA<UD0704_BTTask_Flee>();
	Enemy->SetMarkerColor(bFleeing ? Colors::Item : Colors::Enemy);
	Enemy->SetLabel(FString::Printf(TEXT("HP %.0f%% · %s"), Ratio * 100.f, *ActiveName), bFleeing ? Colors::Item : FLinearColor::White);

	// HP 바 (머리 위)와 공격 반경
	UWorld* World = GetWorld();
	const FVector Head = Enemy->GetActorLocation() + FVector(0.f, 0.f, 150.f);
	DrawDebugLine(World, Head - FVector(0.f, 60.f, 0.f), Head + FVector(0.f, 60.f, 0.f), FColor(40, 40, 40), false, -1.f, 0, 10.f);
	DrawDebugLine(World, Head - FVector(0.f, 60.f, 0.f), Head + FVector(0.f, -60.f + 120.f * Ratio, 0.f), Ratio <= 0.3f ? FColor::Yellow : FColor::Green, false, -1.f, 1, 10.f);
	DrawDebugCircle(World, Enemy->GetActorLocation() - FVector(0.f, 0.f, 90.f), AttackRange, 40, FColor::Orange, false, -1.f, 0, 3.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
	D07::DrawCurrentPath(AIC, bFleeing ? FColor::Yellow : FColor::Red, 4.f);

	const float Distance = BB->GetValueAsFloat(TEXT("TargetDistance"));
	const int32 Ticks = BB->GetValueAsInt(TEXT("ServiceTicks"));
	if (SidePanel)
	{
		SidePanel->SetPanelText(TEXT("C++ BT 노드로 조립한 트리"),
			D07::DescribeBehaviorTree(Tree->RootNode, Active)
			+ FString::Printf(TEXT("\nBlackboard\n  TargetDistance = %.0f  (Service 가 0.5초마다 씀)\n  ServiceTicks   = %d"), Distance, Ticks));
	}
	SetStatus(FString::Printf(TEXT("HP %.0f / %.0f (%.0f%%)%s · 실행 중 노드: %s\nTargetDistance %.0f (Service %d회 실행) · 공격 %d회"),
		Enemy->Health, Enemy->MaxHealth, Ratio * 100.f, SinceLastHit > RegenDelay && Ratio < 1.f ? TEXT(" 회복 중") : TEXT(""),
		*ActiveName, Distance, Ticks, Enemy->AttackCount));
}
