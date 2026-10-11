#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "StateTreeConditionBase.h"
#include "StateTreeTaskBase.h"
#include "D0710_StateTreeNodes.generated.h"

class UStateTreeAIComponent;

/**
 * 0710 의 StateTree 를 실행하는 AI 컨트롤러. UStateTreeAIComponent 가 ST_Enemy 를 돌리고,
 * 태스크·조건은 Context.GetOwner()(= 이 컨트롤러)의 함수로 감지/이동을 한다.
 */
UCLASS()
class GLOSSARYAI_API AD0710_StateTreeController : public AAIController
{
	GENERATED_BODY()

public:
	AD0710_StateTreeController();

	UStateTreeAIComponent* GetStateTreeComponent() const { return StateTreeComponent; }

	/** 시야 1000, 좌우 45°, 벽에 가리면 못 봄 — 이번 프레임에 보이나 */
	bool CanSeePlayer() const { return bSeesPlayer; }
	float SecondsSinceSeen() const;
	float DistanceToPlayer() const;
	APawn* GetPlayer() const;

	TArray<FVector> PatrolPoints;
	int32 PatrolIndex = 0;
	FString CurrentTask;
	float SightRadius = 1000.f;
	float SightHalfAngle = 45.f;

protected:
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "StateTree")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

	bool bSeesPlayer = false;
	float LastSeenTime = -100.f;
};

/** 타이머용 인스턴스 데이터 (StateTree 노드 구조체는 공유되므로 AI 별 값은 인스턴스 데이터에) */
USTRUCT()
struct GLOSSARYAI_API FD0710_TimerData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "Timer")
	float Elapsed = 0.f;
};

/** Idle: Duration 초 동안 제자리 → 끝나면 Succeeded (상태 완료 전환으로 Patrol 로) */
USTRUCT(meta = (DisplayName = "Glossary Idle", Category = "Glossary 0710"))
struct GLOSSARYAI_API FD0710_Task_Idle : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FD0710_TimerData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float Duration = 2.f;
};

/** Patrol: 다음 순찰 지점으로 MoveTo → 도착하면 Succeeded */
USTRUCT(meta = (DisplayName = "Glossary Patrol", Category = "Glossary 0710"))
struct GLOSSARYAI_API FD0710_Task_Patrol : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	// StateTree 태스크는 인스턴스 데이터 타입이 있어야 컴파일된다 (여기서는 비어 있어도 타이머 구조체를 재사용)
	using FInstanceDataType = FD0710_TimerData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

/** Chase: 플레이어를 MoveToActor 로 계속 추격 (Running) */
USTRUCT(meta = (DisplayName = "Glossary Chase", Category = "Glossary 0710"))
struct GLOSSARYAI_API FD0710_Task_Chase : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FD0710_TimerData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

/** Attack: 멈춰서 Interval 초마다 공격 (Running) */
USTRUCT(meta = (DisplayName = "Glossary Attack", Category = "Glossary 0710"))
struct GLOSSARYAI_API FD0710_Task_Attack : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FD0710_TimerData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float Interval = 1.f;
};

/** 조건: 플레이어가 보이나 (Memory 초 안에 본 적이 있으면 참). bInvert 로 반대 */
USTRUCT(meta = (DisplayName = "Glossary Can See Player", Category = "Glossary 0710"))
struct GLOSSARYAI_API FD0710_Cond_CanSeePlayer : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FD0710_TimerData;   // 노드마다 인스턴스 데이터 타입이 필요 (값은 쓰지 않음)
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float Memory = 0.f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bInvert = false;
};

/** 조건: 플레이어가 Range 이내인가. bInvert 로 반대 */
USTRUCT(meta = (DisplayName = "Glossary Player In Range", Category = "Glossary 0710"))
struct GLOSSARYAI_API FD0710_Cond_PlayerInRange : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FD0710_TimerData;   // 노드마다 인스턴스 데이터 타입이 필요 (값은 쓰지 않음)
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float Range = 200.f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bInvert = false;
};

namespace D0710
{
	inline const TCHAR* StateTreePath = TEXT("/Game/Glossary/07_AI/ST_Enemy");
}
