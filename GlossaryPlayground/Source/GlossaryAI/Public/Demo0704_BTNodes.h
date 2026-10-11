#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0704_BTNodes.generated.h"

class ACameraActor;
class AD07Enemy;
class UBehaviorTree;
class UD07SidePanel;

/**
 * 0704 BT Task · Decorator · Service
 * C++ 로 만든 노드 4개(D0704_BTNodes.h)로 트리를 실행 중에 조립한다.
 *  Selector  (서비스: 0.5초마다 TargetDistance 갱신)
 *   ├ [HP ≤ 30%] Flee
 *   ├ [TargetDistance ≤ 200] Attack
 *   └ MoveTo TargetActor
 * 1 키로 적에게 피해를 주면 30% 이하에서 도망가고, 3초 뒤부터 회복해 30%를 넘으면 다시 추격한다.
 */
UCLASS()
class GLOSSARYAI_API ADemo0704_BTNodes : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0704_BTNodes();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "BT Nodes")
	float DamagePerHit = 25.f;

	/** 마지막 피해 후 이 시간이 지나면 초당 RegenPerSecond 회복 */
	UPROPERTY(EditAnywhere, Category = "BT Nodes")
	float RegenDelay = 3.f;

	UPROPERTY(EditAnywhere, Category = "BT Nodes")
	float RegenPerSecond = 4.f;

	UPROPERTY(EditAnywhere, Category = "BT Nodes")
	float AttackRange = 200.f;

	UPROPERTY(Transient) TObjectPtr<UBehaviorTree> Tree;
	UPROPERTY(Transient) TObjectPtr<AD07Enemy> Enemy;
	UPROPERTY(Transient) TObjectPtr<UD07SidePanel> SidePanel;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> OverviewCamera;

	float SinceLastHit = 100.f;
};
