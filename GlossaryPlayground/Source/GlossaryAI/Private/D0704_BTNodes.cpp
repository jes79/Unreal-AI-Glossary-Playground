#include "D0704_BTNodes.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "D07Common.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

namespace
{
	/** 노드 메모리: BT 노드 객체는 모든 AI 가 공유하므로 AI 별 상태는 NodeMemory 에 둔다 */
	struct FHealthMemory { bool bLastResult = false; };
	struct FAttackMemory { float Remaining = 0.f; };

	AD07Enemy* GetEnemy(UBehaviorTreeComponent& OwnerComp)
	{
		const AAIController* AIC = OwnerComp.GetAIOwner();
		return AIC ? Cast<AD07Enemy>(AIC->GetPawn()) : nullptr;
	}
}

// ============================================================ Decorator: 체력 30% 이하

UD0704_BTDecorator_HealthBelow::UD0704_BTDecorator_HealthBelow()
{
	NodeName = TEXT("HP ≤ 30% ? (C++ Decorator)");
	bNotifyTick = true;           // 체력은 블랙보드 키가 아니어서 직접 지켜본다
	bAllowAbortNone = false;
	bAllowAbortLowerPri = true;
	bAllowAbortChildNodes = true;
	FlowAbortMode = EBTFlowAbortMode::Both;
}

bool UD0704_BTDecorator_HealthBelow::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AD07Enemy* Enemy = GetEnemy(OwnerComp);
	return Enemy && Enemy->GetHealthRatio() <= HealthThreshold;
}

void UD0704_BTDecorator_HealthBelow::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FHealthMemory* Memory = CastInstanceNodeMemory<FHealthMemory>(NodeMemory);
	const bool bNow = CalculateRawConditionValue(OwnerComp, NodeMemory);
	if (bNow != Memory->bLastResult)
	{
		Memory->bLastResult = bNow;
		// 조건이 바뀌었다 → 트리에 재평가 요청 (Abort 모드에 따라 지금 가지를 끊고 다시 고른다)
		OwnerComp.RequestExecution(this);
	}
}

uint16 UD0704_BTDecorator_HealthBelow::GetInstanceMemorySize() const
{
	return sizeof(FHealthMemory);
}

void UD0704_BTDecorator_HealthBelow::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	InitializeNodeMemory<FHealthMemory>(NodeMemory, InitType);
}

FString UD0704_BTDecorator_HealthBelow::GetStaticDescription() const
{
	return FString::Printf(TEXT("HP 비율 ≤ %.0f%%"), HealthThreshold * 100.f);
}

// ============================================================ Service: 0.5초마다 거리 갱신

UD0704_BTService_UpdateDistance::UD0704_BTService_UpdateDistance()
{
	NodeName = TEXT("거리 갱신 0.5초 (C++ Service)");
	Interval = 0.5f;               // 0.5초마다 TickNode
	RandomDeviation = 0.f;
	bNotifyTick = true;
	bCallTickOnSearchStart = true; // 가지가 선택되는 순간에도 한 번 갱신

	TargetKey.SelectedKeyName = TEXT("TargetActor");
	DistanceKey.SelectedKeyName = TEXT("TargetDistance");
	// 에디터의 키 드롭다운이 맞는 타입만 보여 주도록 필터
	TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UD0704_BTService_UpdateDistance, TargetKey), AActor::StaticClass());
	DistanceKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UD0704_BTService_UpdateDistance, DistanceKey));
}

void UD0704_BTService_UpdateDistance::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	if (const UBlackboardData* BB = GetBlackboardAsset())
	{
		// 키 이름 → 키 ID 로 변환 (실행 중에는 ID 로 빠르게 읽고 쓴다)
		TargetKey.ResolveSelectedKey(*BB);
		DistanceKey.ResolveSelectedKey(*BB);
	}
}

void UD0704_BTService_UpdateDistance::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	const APawn* Pawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
	if (!BB || !Pawn)
	{
		return;
	}
	const AActor* Target = Cast<AActor>(BB->GetValue<UBlackboardKeyType_Object>(TargetKey.GetSelectedKeyID()));
	const float Distance = Target ? FVector::Dist2D(Pawn->GetActorLocation(), Target->GetActorLocation()) : 99999.f;
	BB->SetValue<UBlackboardKeyType_Float>(DistanceKey.GetSelectedKeyID(), Distance);
	BB->SetValueAsInt(TEXT("ServiceTicks"), BB->GetValueAsInt(TEXT("ServiceTicks")) + 1);
}

FString UD0704_BTService_UpdateDistance::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s → %s, %.1f초마다"), *TargetKey.SelectedKeyName.ToString(), *DistanceKey.SelectedKeyName.ToString(), Interval);
}

// ============================================================ Task: 공격

UD0704_BTTask_Attack::UD0704_BTTask_Attack()
{
	NodeName = TEXT("Attack (C++ Task)");
	bNotifyTick = true;
}

EBTNodeResult::Type UD0704_BTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AD07Enemy* Enemy = GetEnemy(OwnerComp);
	if (!Enemy)
	{
		return EBTNodeResult::Failed;
	}
	if (const AActor* Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(TEXT("TargetActor"))))
	{
		const FVector ToTarget = Target->GetActorLocation() - Enemy->GetActorLocation();
		Enemy->SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
	}
	Enemy->PlayAttack();
	CastInstanceNodeMemory<FAttackMemory>(NodeMemory)->Remaining = AttackDuration;
	return EBTNodeResult::InProgress;   // 끝날 때까지 TickTask 에서 기다린다
}

void UD0704_BTTask_Attack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FAttackMemory* Memory = CastInstanceNodeMemory<FAttackMemory>(NodeMemory);
	Memory->Remaining -= DeltaSeconds;
	if (Memory->Remaining <= 0.f)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

uint16 UD0704_BTTask_Attack::GetInstanceMemorySize() const
{
	return sizeof(FAttackMemory);
}

// ============================================================ Task: 도망

UD0704_BTTask_Flee::UD0704_BTTask_Flee()
{
	NodeName = TEXT("Flee (C++ Task)");
	bNotifyTick = true;
}

EBTNodeResult::Type UD0704_BTTask_Flee::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	const AActor* Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(TEXT("TargetActor")));
	if (!Pawn || !Target)
	{
		return EBTNodeResult::Failed;
	}
	// 타겟 반대 방향, 막혀 있으면 조금씩 옆으로 돌려 본다
	const FVector Away = (Pawn->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Pawn->GetWorld());
	for (const float Angle : { 0.f, 40.f, -40.f, 80.f, -80.f, 120.f, -120.f })
	{
		FVector Goal = Pawn->GetActorLocation() + Away.RotateAngleAxis(Angle, FVector::UpVector) * FleeDistance;
		Goal.X = FMath::Clamp(Goal.X, -2500.f, 2500.f);
		Goal.Y = FMath::Clamp(Goal.Y, -2500.f, 2500.f);
		FNavLocation OnNav;
		if (NavSys && NavSys->ProjectPointToNavigation(Goal, OnNav) && FVector::Dist2D(OnNav.Location, Pawn->GetActorLocation()) > FleeDistance * 0.5f)
		{
			AIC->MoveToLocation(OnNav.Location, 50.f);
			return EBTNodeResult::InProgress;
		}
	}
	return EBTNodeResult::Failed;
}

void UD0704_BTTask_Flee::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	const AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC || AIC->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UD0704_BTTask_Flee::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AIC = OwnerComp.GetAIOwner())
	{
		AIC->StopMovement();
	}
	return EBTNodeResult::Aborted;
}
