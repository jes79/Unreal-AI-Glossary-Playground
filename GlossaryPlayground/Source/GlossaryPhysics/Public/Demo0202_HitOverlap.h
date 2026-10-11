#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0202_HitOverlap.generated.h"

class AStaticMeshActor;
class UPrimitiveComponent;

/**
 * 0202 Hit vs Overlap 이벤트
 * 벽은 Block → OnComponentHit, 코인은 Overlap → OnComponentBeginOverlap (Generate Overlap Events 켬).
 * 플레이어(Manny)와 자동으로 굴러가는 공 둘 다 같은 이벤트를 받는다.
 */
UCLASS()
class GLOSSARYPHYSICS_API ADemo0202_HitOverlap : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0202_HitOverlap();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 코인의 Generate Overlap Events. 끄면 겹쳐도 BeginOverlap 이 오지 않는다 */
	UPROPERTY(EditAnywhere, Category = "Hit Overlap")
	bool bGenerateOverlapEvents = true;

	UPROPERTY(EditAnywhere, Category = "Hit Overlap")
	int32 CoinScore = 10;

	UPROPERTY(EditAnywhere, Category = "Hit Overlap")
	float BallSpeed = 450.f;

	UFUNCTION()
	void OnWallHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnCoinOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	void SpawnCoins();
	void ResetBall();
	void ApplyOverlapSetting();

	UPROPERTY() TArray<TObjectPtr<AStaticMeshActor>> Coins;
	UPROPERTY() TArray<TObjectPtr<AStaticMeshActor>> Walls;
	UPROPERTY() TObjectPtr<AStaticMeshActor> Ball;

	int32 Score = 0;
	int32 WallHits = 0;
	float WallFlash = 0.f;
	float BallPause = 0.f;
	FString LastEvent;
};
