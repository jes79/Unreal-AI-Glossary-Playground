#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0702_NavMeshMoveTo.generated.h"

class ACameraActor;
class AD07Enemy;
class AStaticMeshActor;

/**
 * 0702 NavMesh + MoveTo
 * 빌더가 Nav Mesh Bounds Volume 을 깔고 NavMesh 를 굽는다(bNeedsNavMesh). 실행 중 스폰한 벽은 Dynamic NavMesh 가 다시 굽는다.
 * 바닥을 클릭하면 AI 가 SimpleMoveToLocation 으로 벽을 돌아 이동하고, P 키로 NavMesh 를 보였다 숨긴다.
 */
UCLASS()
class GLOSSARYAI_API ADemo0702_NavMeshMoveTo : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0702_NavMeshMoveTo();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void MoveAgentTo(const FVector& Location);

	/** 시작하자마자 시연용으로 이 위치로 한 번 이동시킨다 (클릭하면 그 위치로 바뀜) */
	UPROPERTY(EditAnywhere, Category = "NavMesh")
	FVector DemoGoal = FVector(900.f, -150.f, 0.f);

	UPROPERTY(EditAnywhere, Category = "NavMesh")
	bool bShowNavMesh = true;

	UPROPERTY(Transient) TObjectPtr<AD07Enemy> Agent;
	UPROPERTY(Transient) TObjectPtr<AStaticMeshActor> GoalMarker;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> OverviewCamera;

	float DemoTimer = 0.f;
	bool bDemoMoveIssued = false;
	int32 ClickCount = 0;
	FVector LastGoal = FVector::ZeroVector;
};
