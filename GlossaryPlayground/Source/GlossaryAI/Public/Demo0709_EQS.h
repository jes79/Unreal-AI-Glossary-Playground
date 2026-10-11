#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "GlossaryDemoBase.h"
#include "Demo0709_EQS.generated.h"

class ACameraActor;
class AD07Enemy;
class UEnvQuery;

/** EQS 컨텍스트: "플레이어" — 테스트(Trace)가 어디를 향해 보이는지 검사할 기준 */
UCLASS(meta = (DisplayName = "Player (Glossary 0709)"))
class GLOSSARYAI_API UD0709_EnvQueryContext_Player : public UEnvQueryContext
{
	GENERATED_BODY()

public:
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};

/**
 * 0709 EQS (Environment Query)
 * 코드로 UEnvQuery 를 조립한다: Generator = 적 주변 격자(SimpleGrid 반경 900, 간격 150, NavMesh 투영)
 *   Test 1 Trace(→ 플레이어, 막혀야 통과 = 플레이어에게 안 보임) — 필터
 *   Test 2 Distance(→ 적 자신, 가까울수록 높은 점수) — 점수
 * FEnvQueryRequest 로 1초마다 실행해 가장 점수 높은 엄폐 지점으로 이동한다. 격자 점은 점수 색으로 그린다.
 */
UCLASS()
class GLOSSARYAI_API ADemo0709_EQS : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0709_EQS();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void RunQuery();
	void OnQueryFinished(TSharedPtr<FEnvQueryResult> Result);

	UPROPERTY(EditAnywhere, Category = "EQS")
	float GridHalfSize = 900.f;

	UPROPERTY(EditAnywhere, Category = "EQS")
	float SpaceBetween = 150.f;

	UPROPERTY(EditAnywhere, Category = "EQS")
	float QueryInterval = 1.f;

	UPROPERTY(Transient) TObjectPtr<UEnvQuery> CoverQuery;
	UPROPERTY(Transient) TObjectPtr<AD07Enemy> Enemy;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> OverviewCamera;

	float QueryTimer = 0.5f;
	int32 QueryCount = 0;
	FVector QueryCenter = FVector::ZeroVector;
	TArray<FVector> PassedLocations;
	TArray<float> PassedScores;
	FVector BestLocation = FVector::ZeroVector;
	bool bHasBest = false;
};
