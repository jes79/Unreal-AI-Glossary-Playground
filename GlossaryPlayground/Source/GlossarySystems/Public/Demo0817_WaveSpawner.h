#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GlossaryDemoBase.h"
#include "Demo0817_WaveSpawner.generated.h"

class UStaticMeshComponent;
class UD08_SidePanel;

/** 웨이브 적: 플레이어에게 다가오는 빨간 상자 */
UCLASS()
class GLOSSARYSYSTEMS_API AD0817_Enemy : public AActor
{
	GENERATED_BODY()

public:
	AD0817_Enemy();
	UPROPERTY(EditAnywhere, Category = "Wave") float MoveSpeed = 160.f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	UPROPERTY(VisibleAnywhere, Category = "Wave") TObjectPtr<UStaticMeshComponent> Body;
	float SpawnAnim = 0.f;
};

/**
 * 0817 스폰 웨이브
 * 웨이브마다 적 수 = 기본 4 × 1.5^(웨이브-1). 모두 처치하면 5초 카운트다운 뒤 다음 웨이브를 SpawnActor.
 */
UCLASS()
class GLOSSARYSYSTEMS_API ADemo0817_WaveSpawner : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0817_WaveSpawner();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Wave") int32 BaseCount = 4;
	/** 웨이브마다 곱하는 배수 */
	UPROPERTY(EditAnywhere, Category = "Wave") float CountMultiplier = 1.5f;
	/** 전멸 후 다음 웨이브까지 (초) */
	UPROPERTY(EditAnywhere, Category = "Wave") float TimeBetweenWaves = 5.f;
	/** 한 마리씩 나오는 간격 (초) */
	UPROPERTY(EditAnywhere, Category = "Wave") float SpawnInterval = 0.25f;
	/** 공격(Space 아닌 1) 반경 */
	UPROPERTY(EditAnywhere, Category = "Wave") float AttackRadius = 450.f;
	UPROPERTY(EditAnywhere, Category = "Wave") TSubclassOf<AD0817_Enemy> EnemyClass;

private:
	int32 CountForWave(int32 InWave) const;
	void StartWave();
	void SpawnOne();
	void KillEnemy(AActor* Enemy);

	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> Alive;
	UPROPERTY(Transient) TObjectPtr<UD08_SidePanel> Panel;
	TArray<FVector> SpawnPoints;
	int32 Wave = 0;
	int32 ToSpawn = 0;
	int32 Killed = 0;
	float SpawnTimer = 0.f;
	float Countdown = 0.f;
	bool bWaitingNext = false;
};
