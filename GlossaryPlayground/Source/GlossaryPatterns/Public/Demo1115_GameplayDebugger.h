#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameFramework/Character.h"
#include "GlossaryDemoBase.h"
#include "Perception/AIPerceptionTypes.h"
#include "D11_SidePanel.h"
#include "Demo1115_GameplayDebugger.generated.h"

class UBehaviorTree;
class UBlackboardData;
class UAISenseConfig_Sight;
class UStaticMeshComponent;

/** BT 태스크: 적의 순찰 지점 목록에서 다음 지점을 골라 블랙보드 PatrolLocation 에 넣는다 */
UCLASS()
class GLOSSARYPATTERNS_API UD1115_BTTask_NextPatrolPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UD1115_BTTask_NextPatrolPoint();

	UPROPERTY(EditAnywhere, Category = Blackboard)
	FName PatrolLocationKey = TEXT("PatrolLocation");

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
};

/** 순찰하는 적 (Gameplay Debugger 로 들여다볼 대상) */
UCLASS()
class GLOSSARYPATTERNS_API AD1115_Enemy : public ACharacter
{
	GENERATED_BODY()

public:
	AD1115_Enemy();

	UPROPERTY(EditAnywhere, Category = Patrol)
	TArray<FVector> PatrolPoints;

	int32 NextPatrolIndex = 0;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = Components)
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = Components)
	TObjectPtr<UStaticMeshComponent> Visor;
};

/**
 * 적 AI 컨트롤러: AI Perception(시야) + 실행 중에 조립한 Behavior Tree.
 * (Behavior Tree 에셋 그래프는 에디터에서만 그릴 수 있어서, 같은 노드 구조를 C++ 로 만들어 RunBehaviorTree 한다.
 *  Gameplay Debugger 의 BehaviorTree 카테고리는 에셋이든 런타임 트리든 똑같이 보여 준다.)
 *
 *   Selector
 *    ├─ [Blackboard: TargetActor Is Set, Abort Both] Sequence "Chase"
 *    │     └─ MoveTo(TargetActor)
 *    └─ Sequence "Patrol"
 *          ├─ NextPatrolPoint → PatrolLocation
 *          ├─ MoveTo(PatrolLocation)
 *          └─ Wait 1.5s
 */
UCLASS()
class GLOSSARYPATTERNS_API AD1115_EnemyController : public AAIController
{
	GENERATED_BODY()

public:
	AD1115_EnemyController();

	UBehaviorTree* GetRuntimeTree() const { return RuntimeTree; }

protected:
	virtual void OnPossess(APawn* InPawn) override;

	UFUNCTION()
	void HandleTargetPerception(AActor* Actor, FAIStimulus Stimulus);

	UBehaviorTree* BuildBehaviorTree();

	UPROPERTY(VisibleAnywhere, Category = AI)
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(Transient) TObjectPtr<UBehaviorTree> RuntimeTree;
	UPROPERTY(Transient) TObjectPtr<UBlackboardData> RuntimeBlackboard;
};

/**
 * 1115 Gameplay Debugger
 * 플레이 중 ' (작은따옴표) 키 → 화면 가운데에 가까운 AI 를 골라 BehaviorTree·Perception·NavMesh 경로 등을 오버레이로 보여 준다.
 * 이 모듈은 'Glossary' 카테고리도 하나 추가 등록한다(FGameplayDebuggerCategory 확장 예제).
 */
UCLASS()
class GLOSSARYPATTERNS_API ADemo1115_GameplayDebugger : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1115_GameplayDebugger();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(Transient) TObjectPtr<AD1115_Enemy> Enemy;
	UPROPERTY(Transient) TObjectPtr<UD11_SidePanel> Panel;
};

#if WITH_GAMEPLAY_DEBUGGER
/** 모듈 시작/종료 때 'Glossary' Gameplay Debugger 카테고리 등록 */
namespace D1115_DebuggerCategory
{
	void Register();
	void Unregister();
}
#endif
