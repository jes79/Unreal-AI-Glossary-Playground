#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GlossaryDemoBase.h"
#include "Demo0701_AIController.generated.h"

class ACameraActor;
class AD07Enemy;

/**
 * 0701 에서 적이 쓰는 AI Controller: 빙의(OnPossess)하자마자 MoveToActor 로 플레이어를 따라간다.
 * 플레이어가 멀어지면 다시 MoveToActor 를 건다(도착하면 이동 요청이 끝나기 때문).
 */
UCLASS()
class GLOSSARYAI_API AD0701_FollowController : public AAIController
{
	GENERATED_BODY()

public:
	AD0701_FollowController();

	/** 이 거리 안에 들어오면 멈춘다 (MoveToActor 의 AcceptanceRadius) */
	UPROPERTY(EditAnywhere, Category = "AI Controller")
	float AcceptanceRadius = 150.f;

	UPROPERTY(EditAnywhere, Category = "AI Controller")
	bool bFollow = true;

	TWeakObjectPtr<AActor> FollowTarget;
	int32 MoveRequests = 0;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;
};

/**
 * 0701 AI Controller
 * 같은 적 캐릭터 두 명: 왼쪽은 Auto Possess AI 로 AD0701_FollowController 가 빙의해 MoveToActor 로 플레이어를 따라오고,
 * 오른쪽은 Auto Possess AI = Disabled 라서 컨트롤러(두뇌)가 없어 그대로 서 있다.
 */
UCLASS()
class GLOSSARYAI_API ADemo0701_AIController : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0701_AIController();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "AI Controller")
	float AcceptanceRadius = 150.f;

	UPROPERTY(Transient) TObjectPtr<AD07Enemy> Follower;
	UPROPERTY(Transient) TObjectPtr<AD07Enemy> Brainless;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> OverviewCamera;
};
