#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0706_Patrol.generated.h"

class ACameraActor;
class AD07Enemy;
class USplineComponent;

/**
 * 0706 순찰 (Patrol)
 * 닫힌 Spline(점 4개) 을 따라 적이 걷고, 스플라인 점(순찰 지점)마다 2초 멈춘다.
 * 이동은 스플라인 위 앞쪽 지점(Carrot)을 향해 AddMovementInput — 곡선을 그대로 따라간다.
 * C 키로 "NavMesh MoveTo 로 지점 사이 직선 이동" 방식과 비교한다.
 */
UCLASS()
class GLOSSARYAI_API ADemo0706_Patrol : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0706_Patrol();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 지점마다 멈추는 시간 */
	UPROPERTY(EditAnywhere, Category = "Patrol", meta = (ClampMin = 0))
	float WaitTime = 2.f;

	/** 스플라인 위에서 얼마나 앞을 보고 걷는지 (cm) */
	UPROPERTY(EditAnywhere, Category = "Patrol", meta = (ClampMin = 10))
	float LookAhead = 150.f;

	UPROPERTY(EditAnywhere, Category = "Patrol")
	bool bFollowSplineCurve = true;

	UPROPERTY(VisibleAnywhere, Category = "Patrol")
	TObjectPtr<USplineComponent> PatrolSpline;

	UPROPERTY(Transient) TObjectPtr<AD07Enemy> Enemy;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> OverviewCamera;

	int32 NextPoint = 1;        // 다음에 멈출 스플라인 점
	float Distance = 0.f;       // 스플라인 위 진행 거리 (곡선 모드)
	float WaitRemaining = 0.f;
	int32 Laps = 0;
	bool bMoveIssued = false;

	void ArriveAtPoint();
};
