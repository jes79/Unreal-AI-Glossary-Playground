#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "GlossaryDemoBase.h"
#include "D11_SidePanel.h"
#include "Demo1106_ObjectPooling.generated.h"

class UStaticMeshComponent;
class UD1106_BulletPool;

/** 총알: 앞으로 날아가다 수명이 끝나면 풀로 돌아가거나(Pool 이 있으면) Destroy 된다 */
UCLASS()
class GLOSSARYPATTERNS_API AD1106_Bullet : public AActor
{
	GENERATED_BODY()

public:
	AD1106_Bullet();

	/** 발사(새로 스폰했든 풀에서 꺼냈든 같은 함수로 초기화) */
	void Launch(const FVector& Location, const FVector& InVelocity, float InLifetime);

	/** 풀에서 꺼낼 때/넣을 때 켜고 끈다: 숨김 + 충돌 끔 + Tick 끔 */
	void SetActive(bool bActive);

	/** 이 총알을 만든 풀 (없으면 Spawn/Destroy 방식) */
	UPROPERTY(Transient)
	TObjectPtr<UD1106_BulletPool> OwnerPool;

	UPROPERTY(VisibleAnywhere, Category = Components)
	TObjectPtr<UStaticMeshComponent> Mesh;

protected:
	virtual void Tick(float DeltaSeconds) override;

	FVector Velocity = FVector::ZeroVector;
	float Lifetime = 0.f;
};

/**
 * 오브젝트 풀: 시작할 때 PoolSize 개를 미리 만들어 두고 Acquire/Release 로 빌려주고 돌려받는다.
 * 실행 중에는 SpawnActor·Destroy 를 한 번도 부르지 않는다 → 스폰 비용·GC 부담이 없다.
 */
UCLASS(ClassGroup = (Glossary), meta = (BlueprintSpawnableComponent))
class GLOSSARYPATTERNS_API UD1106_BulletPool : public UActorComponent
{
	GENERATED_BODY()

public:
	UD1106_BulletPool();

	UPROPERTY(EditAnywhere, Category = Pool, meta = (ClampMin = 1))
	int32 PoolSize = 50;

	/** 쉬고 있는 총알 하나를 꺼낸다. 다 쓰고 있으면 null (풀을 늘리지 않는 고정 크기 풀) */
	AD1106_Bullet* Acquire();
	void Release(AD1106_Bullet* Bullet);

	int32 NumActive() const { return All.Num() - Free.Num(); }
	int32 NumTotal() const { return All.Num(); }

	int32 ReuseCount = 0;
	int32 EmptyCount = 0;
	int32 SpawnCalls = 0;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(Transient) TArray<TObjectPtr<AD1106_Bullet>> All;
	UPROPERTY(Transient) TArray<TObjectPtr<AD1106_Bullet>> Free;
};

/**
 * 1106 오브젝트 풀링
 * 왼쪽 포탑은 매 발마다 SpawnActor / 수명 끝에 Destroy, 오른쪽 포탑은 풀 50개를 재사용한다.
 * 같은 연사 속도로 쏘면서 SpawnActor·Destroy 누적 호출 수와 측정한 스폰 비용을 나란히 보여 준다.
 */
UCLASS()
class GLOSSARYPATTERNS_API ADemo1106_ObjectPooling : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1106_ObjectPooling();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 초당 발사 수 (두 포탑 같음) */
	UPROPERTY(EditAnywhere, Category = "Object Pooling", meta = (ClampMin = 1, ClampMax = 200))
	float FireRate = 40.f;

	UPROPERTY(EditAnywhere, Category = "Object Pooling", meta = (ClampMin = 0.1))
	float BulletLifetime = 1.f;

	UPROPERTY(EditAnywhere, Category = "Object Pooling", meta = (ClampMin = 100))
	float BulletSpeed = 1500.f;

private:
	void FireSpawnDestroy(const FVector& Muzzle, const FVector& Velocity);
	void FirePooled(const FVector& Muzzle, const FVector& Velocity);

	UPROPERTY(Transient) TObjectPtr<UD1106_BulletPool> Pool;
	UPROPERTY(Transient) TObjectPtr<UD11_SidePanel> Panel;

	float Accumulator = 0.f;
	float SweepTime = 0.f;
	bool bLeftOn = true;
	bool bRightOn = true;

	int64 SpawnCalls = 0;
	double SpawnSeconds = 0.0;   // 왼쪽 SpawnActor 에 쓴 누적 시간
	int64 PooledShots = 0;
	double AcquireSeconds = 0.0; // 오른쪽 Acquire+Launch 에 쓴 누적 시간
};
