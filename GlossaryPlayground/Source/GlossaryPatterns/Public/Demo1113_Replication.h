#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GlossaryCharacter.h"
#include "GlossaryDemoBase.h"
#include "D11_SidePanel.h"
#include "Engine/NetSerialization.h"
#include "Demo1113_Replication.generated.h"

class AGlossaryLabel;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * 표적: 체력을 서버가 바꾸고, ReplicatedUsing=OnRep_Health 로 클라이언트에 동기화한다.
 * OnRep 은 "클라이언트가 새 값을 받았을 때" 자동으로 불린다. 서버(리슨 서버의 호스트 포함)에서는 자동으로 안 불리므로
 * 값을 바꾼 직후 직접 OnRep_Health() 를 불러 같은 연출을 실행한다 — 언리얼 멀티플레이 C++ 의 기본 관례.
 */
UCLASS()
class GLOSSARYPATTERNS_API AD1113_NetDummy : public AActor
{
	GENERATED_BODY()

public:
	AD1113_NetDummy();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 서버 전용: 피해 적용 */
	void ApplyDamageOnServer(float Amount);

	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = Health)
	float Health = 100.f;

	int32 OnRepAuto = 0;   // 복제를 받아 자동으로 불린 횟수 (클라이언트에서만 증가)
	int32 OnRepManual = 0; // 서버가 직접 부른 횟수

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

	/** 클라이언트: 새 Health 를 받으면 자동 호출. 인자로 이전 값을 받을 수 있다 */
	UFUNCTION()
	void OnRep_Health(float OldHealth);

	UPROPERTY(VisibleAnywhere, Category = Components)
	TObjectPtr<UStaticMeshComponent> Body;

	bool bManualRepCall = false;
	float RespawnTimer = 0.f;
	float Flash = 0.f;
};

/** 서버가 스폰하는 복제 총알. 움직임은 서버에서 계산하고 ReplicateMovement 로 클라이언트에 보낸다. */
UCLASS()
class GLOSSARYPATTERNS_API AD1113_NetProjectile : public AActor
{
	GENERATED_BODY()

public:
	AD1113_NetProjectile();

	UPROPERTY(VisibleAnywhere, Category = Components)
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = Components)
	TObjectPtr<UProjectileMovementComponent> Movement;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};

/**
 * 플레이어: F 를 누르면 RequestFire → ServerFire(Server RPC).
 * 클라이언트에서 부르면 네트워크로 서버에 요청이 가고, 서버(또는 싱글/리슨 호스트)에서 부르면 바로 실행된다.
 * 총알 스폰·피해 판정은 서버에서만 — 클라이언트는 결과(복제된 총알·체력)를 받기만 한다(치트 방지).
 */
UCLASS()
class GLOSSARYPATTERNS_API AD1113_NetCharacter : public AGlossaryCharacter
{
	GENERATED_BODY()

public:
	AD1113_NetCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 입력 쪽에서 부르는 함수 (어디서 실행되든 같은 코드) */
	void RequestFire();

	/** Server RPC: 클라이언트 → 서버. Reliable = 반드시 도착, WithValidation = 서버가 요청을 검증 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire(FVector_NetQuantize Origin, FVector_NetQuantizeNormal Direction);

	/** Multicast RPC: 서버 → 모든 클라이언트(+서버). 총구 섬광 같은 일회성 연출 */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastMuzzleFlash();

	/** 단순 복제 값(OnRep 없음) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = Combat)
	int32 ShotsFired = 0;

	int32 ServerFireExecuted = 0; // 이 프로세스에서 ServerFire_Implementation 이 실행된 횟수
	int32 RequestCount = 0;
	int32 MuzzleFlashes = 0;
};

/**
 * 1113 Replication (멀티플레이)
 * 단일 -game 실행은 Standalone(= 이 프로세스가 서버 권한)이라 RPC 도 로컬에서 실행되고 OnRep 은 수동 호출만 보인다.
 * L 키로 같은 레벨을 ?listen 으로 다시 열면 진짜 Listen Server 가 되어 클라이언트 접속을 받을 수 있다.
 * 2인 테스트는 에디터 Play 옵션에서 Number of Players 2 + Net Mode 'Play As Listen Server'.
 */
UCLASS()
class GLOSSARYPATTERNS_API ADemo1113_Replication : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1113_Replication();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	static FString NetModeName(ENetMode Mode);
	static FString RoleName(ENetRole Role);

	UPROPERTY(Transient) TObjectPtr<AD1113_NetDummy> Dummy;
	UPROPERTY(Transient) TObjectPtr<AGlossaryLabel> DummyLabel;
	UPROPERTY(Transient) TObjectPtr<UD11_SidePanel> Panel;
};
