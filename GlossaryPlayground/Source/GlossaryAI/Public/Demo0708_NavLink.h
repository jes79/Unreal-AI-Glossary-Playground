#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0708_NavLink.generated.h"

class ACameraActor;
class AD07Enemy;
class ANavLinkProxy;

/**
 * 0708 Nav Link Proxy
 * 높이 300 의 절벽(고지대) 위의 AI 가 아래 목표로 간다. 고지대와 바닥은 멀리 있는 경사로로만 이어져 있다.
 * 절벽 끝에 Nav Link Proxy(Smart Link) 를 놓으면 길찾기가 링크를 지나는 경로를 고르고,
 * 링크에 도착하면 OnSmartLinkReached 에서 LaunchCharacter 로 점프(ABP 가 점프·낙하 애니메이션) → 착지 후 ResumePathFollowing.
 * C 키로 Smart Link 를 끄면 경사로로 돌아간다.
 */
UCLASS()
class GLOSSARYAI_API ADemo0708_NavLink : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0708_NavLink();

#if WITH_EDITOR
	virtual void BuildLevel(UWorld* World) override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void OnSmartLinkReached(AActor* MovingActor, const FVector& DestinationPoint);

	/** 절벽 높이 */
	UPROPERTY(EditAnywhere, Category = "Nav Link")
	float CliffHeight = 300.f;

	/** 점프 체공 시간 (속도 계산용) */
	UPROPERTY(EditAnywhere, Category = "Nav Link")
	float JumpTime = 0.7f;

	UPROPERTY(Transient) TObjectPtr<ANavLinkProxy> LinkProxy;
	UPROPERTY(Transient) TObjectPtr<AD07Enemy> Agent;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> OverviewCamera;

	FVector TopGoal;      // 고지대 위 출발점
	FVector BottomGoal;   // 바닥 목표
	bool bGoingDown = true;
	bool bJumping = false;
	float JumpElapsed = 0.f;
	float WaitTimer = 2.5f;   // 실행 중 스폰한 고지대가 NavMesh 에 반영된 뒤 출발
	int32 JumpCount = 0;
	int32 RampTrips = 0;
	bool bUsedLinkThisTrip = false;
};
