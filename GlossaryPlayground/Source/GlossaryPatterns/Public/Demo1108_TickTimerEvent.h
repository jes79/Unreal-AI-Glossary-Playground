#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GlossaryDemoBase.h"
#include "D11_SidePanel.h"
#include "Demo1108_TickTimerEvent.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/** 세 방식의 센서가 공유하는 겉모습(불이 켜지는 기둥)과 화면용 카운터 */
UCLASS(Abstract)
class GLOSSARYPATTERNS_API AD1108_SensorBase : public AActor
{
	GENERATED_BODY()

public:
	AD1108_SensorBase();

	/** 이 거리 안에 Target 이 들어오면 켜진다 (cm) */
	UPROPERTY(EditAnywhere, Category = Sensor)
	float DetectRadius = 300.f;

	UPROPERTY(Transient)
	TObjectPtr<AActor> Target;

	/** 방식별 검사 횟수 (화면용). 0=Tick, 1=Timer, 2=Overlap */
	static int64 Checks[3];

protected:
	void SetLit(bool bLit);

	UPROPERTY(VisibleAnywhere, Category = Components)
	TObjectPtr<UStaticMeshComponent> Lamp;

	bool bIsLit = false;
};

/** 방식 1: 매 프레임 Tick 에서 거리 검사 (가장 단순하지만 프레임마다 비용) */
UCLASS()
class GLOSSARYPATTERNS_API AD1108_TickSensor : public AD1108_SensorBase
{
	GENERATED_BODY()
public:
	AD1108_TickSensor();
protected:
	virtual void Tick(float DeltaSeconds) override;
};

/** 방식 2: 0.2초 Timer 로 거리 검사, bCanEverTick = false */
UCLASS()
class GLOSSARYPATTERNS_API AD1108_TimerSensor : public AD1108_SensorBase
{
	GENERATED_BODY()
public:
	AD1108_TimerSensor();

	UPROPERTY(EditAnywhere, Category = Sensor, meta = (ClampMin = 0.01))
	float CheckInterval = 0.2f;

	/** 간격을 바꾸고 타이머를 다시 건다 */
	void SetInterval(float NewInterval);

protected:
	virtual void BeginPlay() override;
	void CheckDistance();
	FTimerHandle CheckTimer;
};

/** 방식 3: 구체 콜리전의 Overlap 이벤트 — 들어오고 나갈 때만 호출, bCanEverTick = false */
UCLASS()
class GLOSSARYPATTERNS_API AD1108_OverlapSensor : public AD1108_SensorBase
{
	GENERATED_BODY()
public:
	AD1108_OverlapSensor();
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY(VisibleAnywhere, Category = Components)
	TObjectPtr<USphereComponent> Trigger;

	int32 OverlapCount = 0;
};

/** 레인을 왕복하는 탐지 대상 공. Overlap 을 일으키려면 이쪽도 GenerateOverlapEvents 가 켜져 있어야 한다. */
UCLASS()
class GLOSSARYPATTERNS_API AD1108_Probe : public AActor
{
	GENERATED_BODY()
public:
	AD1108_Probe();
	float MinX = -200.f;
	float MaxX = 2000.f;
	float Speed = 500.f;
protected:
	virtual void Tick(float DeltaSeconds) override;
	UPROPERTY(VisibleAnywhere, Category = Components)
	TObjectPtr<UStaticMeshComponent> Ball;
	float Dir = 1.f;
};

/**
 * 1108 Tick vs Timer vs 이벤트
 * 같은 일("공이 3m 안에 오면 불 켜기")을 세 방식으로 8개씩 놓고, 초당 검사 횟수를 비교한다.
 * 오른쪽 장식 기둥 30개는 bCanEverTick = false 라 Tick 비용이 0.
 */
UCLASS()
class GLOSSARYPATTERNS_API ADemo1108_TickTimerEvent : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1108_TickTimerEvent();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Tick Timer Event", meta = (ClampMin = 1))
	int32 SensorsPerLane = 8;

private:
	UPROPERTY(Transient) TObjectPtr<UD11_SidePanel> Panel;
	UPROPERTY(Transient) TArray<TObjectPtr<AD1108_TimerSensor>> TimerSensors;

	int64 LastChecks[3] = { 0, 0, 0 };
	double PerSecond[3] = { 0, 0, 0 };
	float WindowTime = 0.f;
	int32 FramesInWindow = 0;
	float Fps = 0.f;
	int32 IdleActors = 0;
	float TimerInterval = 0.2f;
};
