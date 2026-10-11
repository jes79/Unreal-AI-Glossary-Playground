#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0204_ShapeTrace.generated.h"

class AStaticMeshActor;
class AGlossaryLabel;

/**
 * 0204 Sweep / Shape Trace
 * 근접 공격 판정: 캐릭터 앞쪽으로 SphereTraceMulti(반지름 50) 를 쓸어서 맞은 적을 "모두" 처리한다.
 * 1 키로 반지름 0(= 얇은 Line Trace)과 비교하면 옆에 선 적을 놓치는 것이 보인다.
 */
UCLASS()
class GLOSSARYPHYSICS_API ADemo0204_ShapeTrace : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0204_ShapeTrace();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 구 반지름 (cm). 0 이면 선(Line) 트레이스와 같다 */
	UPROPERTY(EditAnywhere, Category = "Shape Trace", meta = (ClampMin = 0))
	float SphereRadius = 50.f;

	/** 공격 판정 길이: 캐릭터 앞 StartOffset ~ AttackRange */
	UPROPERTY(EditAnywhere, Category = "Shape Trace")
	float StartOffset = 40.f;

	UPROPERTY(EditAnywhere, Category = "Shape Trace")
	float AttackRange = 220.f;

	UPROPERTY(EditAnywhere, Category = "Shape Trace")
	float Damage = 25.f;

private:
	void Attack();
	void ResetEnemies();

	UPROPERTY() TArray<TObjectPtr<AStaticMeshActor>> Enemies;
	UPROPERTY() TArray<TObjectPtr<AGlossaryLabel>> EnemyLabels;
	TArray<float> Health;
	TArray<float> FlashTime;
	TArray<FVector> HomeLocations;
	bool bLineMode = false;
	int32 LastHitCount = -1;
	float AttackCooldown = 0.f;
};
