#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0712_DetourCrowd.generated.h"

class ACameraActor;
class AD07Enemy;

/**
 * 0712 Crowd 회피 (Detour Crowd)
 * 폭 400 의 좁은 통로 양쪽에서 15명씩, 30명이 서로 반대편으로 건너간다.
 * ADetourCrowdAIController(UCrowdFollowingComponent) 는 이웃을 보고 속도를 조정해 비켜 가고,
 * C 키로 바꾸는 일반 AAIController 는 경로만 따라가서 통로 가운데서 서로 밀치고 막힌다.
 */
UCLASS()
class GLOSSARYAI_API ADemo0712_DetourCrowd : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0712_DetourCrowd();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void SpawnAgents();

	UPROPERTY(EditAnywhere, Category = "Detour Crowd")
	int32 AgentsPerSide = 15;

	UPROPERTY(EditAnywhere, Category = "Detour Crowd")
	float CorridorWidth = 400.f;

	UPROPERTY(EditAnywhere, Category = "Detour Crowd")
	bool bUseDetourCrowd = true;

	UPROPERTY(Transient) TArray<TObjectPtr<AD07Enemy>> Agents;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> OverviewCamera;

	TArray<FVector> HomeA;     // 왼쪽 출발 지점들
	TArray<FVector> HomeB;     // 오른쪽 출발 지점들
	TArray<bool> GoingToB;
	float RunTime = 0.f;
	int32 Crossings = 0;
	float StartDelay = 1.f;
};
