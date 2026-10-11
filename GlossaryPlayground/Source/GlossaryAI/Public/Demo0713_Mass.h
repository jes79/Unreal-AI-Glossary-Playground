#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Mass/EntityHandle.h"
#include "MassEntityQuery.h"
#include "MassEntityTypes.h"
#include "MassProcessor.h"
#include "Demo0713_Mass.generated.h"

class ACameraActor;
class UInstancedStaticMeshComponent;

// ---------------------------------------------------------------- Fragment: 엔티티 하나가 가진 데이터 조각 (액터·컴포넌트가 아니라 순수 구조체)

/** 길 위의 위치: 몇 번 차선, 차선을 따라 얼마나 왔나, 속도, 옆으로 비킨 거리 */
USTRUCT()
struct GLOSSARYAI_API FD0713_LaneFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY() int32 Lane = 0;
	UPROPERTY() float Distance = 0.f;
	UPROPERTY() float Speed = 150.f;
	UPROPERTY() float Lateral = 0.f;
	UPROPERTY() float Phase = 0.f;
};

/** 월드 변환 (표현 단계가 읽어 인스턴스 메시로 그린다) */
USTRUCT()
struct GLOSSARYAI_API FD0713_TransformFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY() FVector Location = FVector::ZeroVector;
	UPROPERTY() float Yaw = 0.f;
};

// ---------------------------------------------------------------- Processor: 같은 Fragment 조합(아키타입)을 가진 엔티티를 청크 단위로 한꺼번에 처리

/** 차선 따라 걷기: Distance += Speed·dt → 사각 순환로 위 위치·방향 계산 */
UCLASS()
class GLOSSARYAI_API UD0713_LaneFollowProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UD0713_LaneFollowProcessor();

	/** 차선 i 의 사각형 반폭/반높이 (데모가 채운다) */
	TArray<FVector2D> LaneHalfExtents;
	TArray<float> LaneDirections;
	int32 LastChunkCount = 0;

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};

/** 표현: 모든 엔티티의 Transform 을 배열로 모은다 (Mass Representation 이 ISM/액터로 바꿔 그리는 단계에 해당) */
UCLASS()
class GLOSSARYAI_API UD0713_RepresentationProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UD0713_RepresentationProcessor();

	TArray<FTransform> TransformsByLane[2];

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};

/**
 * 0713 Mass (대규모 군중)
 * Mass Entity 코어 API 로 보행자 1000명을 만든다: Fragment 2종으로 아키타입 생성 → BatchCreateEntities(1000)
 * → 매 프레임 Processor 2개(차선 따라 걷기, 표현)를 Executor 로 실행 → 결과를 Instanced Static Mesh 로 그린다.
 * 액터 1000개가 아니라 데이터 배열 1000줄이라 가볍다.
 */
UCLASS()
class GLOSSARYAI_API ADemo0713_Mass : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0713_Mass();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Mass")
	int32 NumPedestrians = 1000;

	UPROPERTY(Transient) TObjectPtr<UD0713_LaneFollowProcessor> LaneProcessor;
	UPROPERTY(Transient) TObjectPtr<UD0713_RepresentationProcessor> RepresentationProcessor;
	UPROPERTY(Transient) TObjectPtr<UInstancedStaticMeshComponent> Bodies[2];
	UPROPERTY(Transient) TObjectPtr<UInstancedStaticMeshComponent> Heads[2];
	UPROPERTY(Transient) TObjectPtr<ACameraActor> OverviewCamera;

	TArray<FMassEntityHandle> Entities;
	bool bPaused = false;
	double LastProcessMs = 0.0;
};
