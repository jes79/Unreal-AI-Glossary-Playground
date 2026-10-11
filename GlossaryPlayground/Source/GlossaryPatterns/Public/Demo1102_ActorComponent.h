#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "GlossaryDemoBase.h"
#include "D11_SidePanel.h"
#include "Demo1102_ActorComponent.generated.h"

class AGlossaryLabel;
class UStaticMeshComponent;
class UD1102_HealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FD1102_OnHealthChanged, UD1102_HealthComponent*, Health, float, NewHealth, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FD1102_OnDepleted, UD1102_HealthComponent*, Health);

/**
 * 체력 로직만 떼어 낸 부품(Actor Component). 주인이 플레이어든 적이든 상자든 상관없이 똑같이 동작한다.
 * 주인은 OnHealthChanged / OnDepleted 에 자기만의 반응(쓰러짐·부서짐·리스폰)을 연결한다.
 * BlueprintSpawnableComponent → 블루프린트 에디터의 'Add Component' 목록에도 나타난다.
 */
UCLASS(ClassGroup = (Glossary), meta = (BlueprintSpawnableComponent))
class GLOSSARYPATTERNS_API UD1102_HealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UD1102_HealthComponent();

	/** 최대 체력 — 주인마다 다르게(플레이어 100, 적 60, 상자 30) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Health, meta = (ClampMin = 1))
	float MaxHealth = 100.f;

	/** 받는 피해 배율 (상자는 단단해서 0.5 등) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Health, meta = (ClampMin = 0))
	float DamageMultiplier = 1.f;

	UFUNCTION(BlueprintCallable, Category = Health)
	void ApplyDamage(float Amount);

	UFUNCTION(BlueprintCallable, Category = Health)
	void Heal(float Amount);

	UFUNCTION(BlueprintPure, Category = Health)
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = Health)
	bool IsDead() const { return Health <= 0.f; }

	UPROPERTY(BlueprintAssignable, Category = Health)
	FD1102_OnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = Health)
	FD1102_OnDepleted OnDepleted;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = Health)
	float Health = 0.f;
};

/** 적: 컴포넌트를 생성자에서 CreateDefaultSubobject 로 붙인다(가장 흔한 방법) */
UCLASS()
class GLOSSARYPATTERNS_API AD1102_Enemy : public AActor
{
	GENERATED_BODY()

public:
	AD1102_Enemy();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components)
	TObjectPtr<UD1102_HealthComponent> HealthComponent;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 체력이 0 이 되면: 적은 옆으로 쓰러지고 3초 뒤 일어난다 */
	UFUNCTION()
	void HandleDepleted(UD1102_HealthComponent* Health);

	UPROPERTY(VisibleAnywhere, Category = Components)
	TObjectPtr<UStaticMeshComponent> Body;

	float ReviveTimer = 0.f;
};

/**
 * 1102 Actor Component
 * 같은 UD1102_HealthComponent 를 플레이어(실행 중 NewObject 로 추가)·적(생성자에서 추가)·상자(실행 중 추가)에 붙이고,
 * 데미지를 주는 코드는 주인의 종류를 모른 채 FindComponentByClass 로 컴포넌트만 찾아 ApplyDamage 한다.
 */
UCLASS()
class GLOSSARYPATTERNS_API ADemo1102_ActorComponent : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1102_ActorComponent();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Actor Component", meta = (ClampMin = 1))
	float HitDamage = 25.f;

private:
	/** 주인이 누구든 같은 코드: 컴포넌트만 찾아서 피해를 준다 */
	void HitActor(AActor* Target);

	UFUNCTION() void HandlePlayerDepleted(UD1102_HealthComponent* Health);
	UFUNCTION() void HandleCrateDepleted(UD1102_HealthComponent* Health);
	UFUNCTION() void HandleAnyHealthChanged(UD1102_HealthComponent* Health, float NewHealth, float Delta);

	void UpdateLabels();

	UPROPERTY(Transient) TObjectPtr<AActor> Crate;
	UPROPERTY(Transient) TObjectPtr<AD1102_Enemy> Enemy;
	UPROPERTY(Transient) TArray<TObjectPtr<AGlossaryLabel>> Labels;
	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> Owners;
	UPROPERTY(Transient) TObjectPtr<UD11_SidePanel> Panel;

	FD11_LogLines Log{7};
	float CrateRespawnTimer = 0.f;
};
