#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GlossaryDemoBase.h"
#include "Perception/AIPerceptionTypes.h"
#include "Demo0703_BehaviorTree.generated.h"

class ACameraActor;
class AD07Enemy;
class UAnimationAsset;
class UBehaviorTree;
class UBlackboardData;
class UD07SidePanel;

namespace D0703
{
	/** 빌더가 만드는 에셋 경로 */
	inline const TCHAR* TreePath = TEXT("/Game/Glossary/07_AI/BT_Enemy");
	inline const TCHAR* BlackboardPath = TEXT("/Game/Glossary/07_AI/BB_Enemy");

	/**
	 * BT_Enemy / BB_Enemy 의 내용을 채운다 (에디터 빌더와 런타임 대체 경로가 같은 함수를 쓴다).
	 *  Selector
	 *   ├ [TargetActor Is Set · Abort Both] Sequence 추격→공격 : MoveTo(TargetActor) → [거리 ≤ 200] PlayAnimation(공격) → Wait
	 *   └ Sequence 순찰 : MoveTo(PatrolLocation) → Wait 1초
	 */
	GLOSSARYAI_API void BuildEnemyTree(UBehaviorTree* Tree, UBlackboardData* Blackboard, UAnimationAsset* AttackAnim, float AttackRange);
}

/**
 * BT_Enemy 를 실행하는 컨트롤러. "감각"(AIPerception 시야)으로 Blackboard 의 TargetActor 를 채우고,
 * 순찰 지점(PatrolLocation)을 도착할 때마다 다음 지점으로 바꾼다. 판단은 전부 Behavior Tree 가 한다.
 */
UCLASS()
class GLOSSARYAI_API AD0703_EnemyController : public AAIController
{
	GENERATED_BODY()

public:
	AD0703_EnemyController();

	void StartTree(UBehaviorTree* Tree, const TArray<FVector>& InPatrolPoints);

	TArray<FVector> PatrolPoints;
	int32 PatrolIndex = 0;

protected:
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus);
};

/**
 * 0703 Behavior Tree + Blackboard
 * 에셋 빌더가 만든 BT_Enemy(UBehaviorTree) + BB_Enemy(UBlackboardData) 를 RunBehaviorTree 로 실행한다.
 * 순찰하다 플레이어가 시야(1200, 90°)에 들어오면 추격, 200 이내면 공격. 오른쪽 패널에 트리와 현재 노드, Blackboard 값.
 */
UCLASS()
class GLOSSARYAI_API ADemo0703_BehaviorTree : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0703_BehaviorTree();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 이 거리 이내면 공격 (BT 의 [거리 ≤ 200] 데코레이터와 같은 값) */
	UPROPERTY(EditAnywhere, Category = "Behavior Tree")
	float AttackRange = 200.f;

	UPROPERTY(Transient) TObjectPtr<UBehaviorTree> EnemyTree;
	UPROPERTY(Transient) TObjectPtr<AD07Enemy> Enemy;
	UPROPERTY(Transient) TObjectPtr<UD07SidePanel> SidePanel;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> OverviewCamera;

	bool bTreeFromAsset = false;
	const class UBTNode* LastActiveNode = nullptr;
};
