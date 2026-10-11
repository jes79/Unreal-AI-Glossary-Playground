#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BTTaskNode.h"
#include "D0704_BTNodes.generated.h"

/**
 * Decorator(조건): 폰의 체력 비율이 HealthThreshold(30%) 이하인가?
 * 블랙보드 값이 아니라 폰 상태를 직접 보므로 매 틱 검사해 결과가 바뀌면 RequestExecution 으로 트리에 알린다
 * (FlowAbortMode = Both → 체력이 떨어지면 낮은 우선순위 가지(추격·공격)를 끊고, 회복하면 도망을 끊는다).
 */
UCLASS(meta = (DisplayName = "Health Below (C++)"))
class GLOSSARYAI_API UD0704_BTDecorator_HealthBelow : public UBTDecorator
{
	GENERATED_BODY()

public:
	UD0704_BTDecorator_HealthBelow();

	UPROPERTY(EditAnywhere, Category = "Condition", meta = (ClampMin = 0, ClampMax = 1))
	float HealthThreshold = 0.3f;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual FString GetStaticDescription() const override;
};

/**
 * Service(주기적 검사): Interval 0.5초마다 TargetActor 까지 거리를 계산해 Blackboard 의 TargetDistance 에 쓴다.
 * 붙은 가지(여기서는 루트 Selector)가 실행되는 동안만 돈다. 실행 횟수는 ServiceTicks 키에 누적(관찰용).
 */
UCLASS(meta = (DisplayName = "Update Target Distance (C++)"))
class GLOSSARYAI_API UD0704_BTService_UpdateDistance : public UBTService
{
	GENERATED_BODY()

public:
	UD0704_BTService_UpdateDistance();

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector DistanceKey;

protected:
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;
};

/** Task(행동): 타겟을 보고 공격 애니메이션을 재생, AttackDuration 동안 진행 중(InProgress)으로 있다가 성공 */
UCLASS(meta = (DisplayName = "Attack (C++)"))
class GLOSSARYAI_API UD0704_BTTask_Attack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UD0704_BTTask_Attack();

	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = 0.1))
	float AttackDuration = 1.0f;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;
};

/** Task(행동): 타겟 반대 방향 FleeDistance 지점으로 NavMesh 이동, 도착하면 성공. 중단(Abort)되면 이동을 멈춘다 */
UCLASS(meta = (DisplayName = "Flee (C++)"))
class GLOSSARYAI_API UD0704_BTTask_Flee : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UD0704_BTTask_Flee();

	UPROPERTY(EditAnywhere, Category = "Flee", meta = (ClampMin = 100))
	float FleeDistance = 900.f;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
