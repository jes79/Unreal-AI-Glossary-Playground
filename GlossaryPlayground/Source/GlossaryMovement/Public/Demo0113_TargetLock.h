#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0113_TargetLock.generated.h"

/**
 * 0113 록온 (Target Lock)
 * Tab 으로 가장 가까운 적을 고정하고, 매 프레임 Control Rotation 을 RInterpTo 로 적 방향에 보간한다.
 * 고정 중에는 캐릭터가 적을 바라본 채 옆걸음(스트레이프)한다.
 */
UCLASS()
class GLOSSARYMOVEMENT_API ADemo0113_TargetLock : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0113_TargetLock();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 이 거리 안의 적만 록온 */
	UPROPERTY(EditAnywhere, Category = "Target Lock") float LockRange = 2500.f;
	/** RInterpTo 보간 속도 (클수록 빨리 돌아감) */
	UPROPERTY(EditAnywhere, Category = "Target Lock") float LockInterpSpeed = 8.f;
	/** 적을 내려다보는 각도 */
	UPROPERTY(EditAnywhere, Category = "Target Lock") float LockPitchOffset = -12.f;

private:
	AActor* FindNearestEnemy() const;
	void SetLocked(AActor* Target);
	FString EnemyName(const AActor* Actor) const;

	struct FEnemy
	{
		TWeakObjectPtr<AActor> Actor;
		FVector Home = FVector::ZeroVector;
		float Swing = 0.f;   // 좌우 왕복 폭 (0 이면 가만히)
		float Phase = 0.f;
		FString Name;
		TWeakObjectPtr<AActor> Label;
	};
	TArray<FEnemy> Enemies;
	TWeakObjectPtr<AActor> LockedTarget;
	TWeakObjectPtr<AActor> Marker;
	float Time = 0.f;
};
