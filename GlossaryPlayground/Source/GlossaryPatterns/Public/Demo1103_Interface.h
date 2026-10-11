#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/Interface.h"
#include "GlossaryDemoBase.h"
#include "D11_SidePanel.h"
#include "Demo1103_Interface.generated.h"

class UStaticMeshComponent;

/**
 * Damageable 인터페이스. UINTERFACE 는 두 클래스로 이루어진다:
 *   UD1103_Damageable — 리플렉션용 껍데기(건드리지 않는다)
 *   ID1103_Damageable — 실제 함수 선언. 구현하는 클래스가 상속한다.
 * Blueprintable 이라 블루프린트 클래스도 Class Settings > Interfaces 에서 구현할 수 있다.
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UD1103_Damageable : public UInterface
{
	GENERATED_BODY()
};

class GLOSSARYPATTERNS_API ID1103_Damageable
{
	GENERATED_BODY()

public:
	/**
	 * 맞았을 때. BlueprintNativeEvent 라서 C++ 은 TakeHit_Implementation 을 구현하고,
	 * 호출은 반드시 ID1103_Damageable::Execute_TakeHit(대상, ...) 으로 한다(BP 구현도 같이 처리됨).
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Damage)
	void TakeHit(float Damage, AActor* InstigatorActor);
};

/** 서로 상속 관계가 없는 세 클래스가 같은 인터페이스를 구현한다. 공통 부모는 AActor 뿐. */
UCLASS(Abstract)
class GLOSSARYPATTERNS_API AD1103_PropBase : public AActor
{
	GENERATED_BODY()

public:
	AD1103_PropBase();
	int32 HitCount = 0;
	FString LastReaction;

protected:
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = Components)
	TObjectPtr<UStaticMeshComponent> Body;

	float Pulse = 0.f;
	FVector BaseScale = FVector::OneVector;
};

/** 폭발통: 3번 맞으면 터졌다가 다시 생긴다 */
UCLASS()
class GLOSSARYPATTERNS_API AD1103_Barrel : public AD1103_PropBase, public ID1103_Damageable
{
	GENERATED_BODY()
public:
	AD1103_Barrel();
	virtual void TakeHit_Implementation(float Damage, AActor* InstigatorActor) override;
protected:
	virtual void BeginPlay() override;
};

/** 훈련용 허수아비: 맞으면 흔들리고 누적 피해를 센다 */
UCLASS()
class GLOSSARYPATTERNS_API AD1103_Dummy : public AD1103_PropBase, public ID1103_Damageable
{
	GENERATED_BODY()
public:
	AD1103_Dummy();
	virtual void TakeHit_Implementation(float Damage, AActor* InstigatorActor) override;
	float TotalDamage = 0.f;
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	float Wobble = 0.f;
};

/** 유리창: 한 번 맞으면 깨진다(숨김 + 파편) */
UCLASS()
class GLOSSARYPATTERNS_API AD1103_Glass : public AD1103_PropBase, public ID1103_Damageable
{
	GENERATED_BODY()
public:
	AD1103_Glass();
	virtual void TakeHit_Implementation(float Damage, AActor* InstigatorActor) override;
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	float RepairTimer = 0.f;
};

/**
 * 1103 Blueprint Interface (UINTERFACE)
 * F 를 누르면 플레이어 주변 반경의 모든 액터에 "Damageable 이면 TakeHit" 을 보낸다.
 * 캐스트(Cast<AD1103_Barrel> ...) 없이 Implements<> + Execute_TakeHit 만 쓴다. 인터페이스가 없는 벽은 건너뛴다.
 */
UCLASS()
class GLOSSARYPATTERNS_API ADemo1103_Interface : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1103_Interface();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** F 키 충격파 반경 (cm) */
	UPROPERTY(EditAnywhere, Category = "Interface Demo", meta = (ClampMin = 100))
	float HitRadius = 1300.f;

	UPROPERTY(EditAnywhere, Category = "Interface Demo", meta = (ClampMin = 0))
	float HitDamage = 10.f;

private:
	void HitAround(const FVector& Center, float Radius);

	UPROPERTY(Transient) TObjectPtr<UD11_SidePanel> Panel;
	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> Targets;
	FD11_LogLines Log{8};
	int32 WaveCount = 0;
	float RingTimer = 0.f;
	FVector RingCenter = FVector::ZeroVector;
};
