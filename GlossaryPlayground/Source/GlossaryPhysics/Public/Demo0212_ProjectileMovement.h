#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0212_ProjectileMovement.generated.h"

class ACameraActor;
class AStaticMeshActor;
class UProjectileMovementComponent;

/**
 * 0212 Projectile Movement Component
 * 총알: InitialSpeed 3000, 중력 약하게 → 거의 직선.
 * 수류탄: InitialSpeed 1100, bShouldBounce=true(Bounciness 0.5) → 포물선으로 튀다가 3초 뒤 폭발.
 * 매 프레임 위치를 점으로 남겨 궤적을 비교한다.
 */
UCLASS()
class GLOSSARYPHYSICS_API ADemo0212_ProjectileMovement : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0212_ProjectileMovement();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Projectile Movement|Bullet")
	float BulletSpeed = 3000.f;

	UPROPERTY(EditAnywhere, Category = "Projectile Movement|Bullet")
	float BulletGravityScale = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Projectile Movement|Grenade")
	float GrenadeSpeed = 1100.f;

	UPROPERTY(EditAnywhere, Category = "Projectile Movement|Grenade")
	float GrenadeBounciness = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Projectile Movement|Grenade")
	float GrenadeFuse = 3.f;

	/** 자동 발사 (총알·수류탄 번갈아) */
	UPROPERTY(EditAnywhere, Category = "Projectile Movement")
	bool bAutoFire = true;

	UFUNCTION()
	void OnGrenadeBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

private:
	struct FTracked
	{
		TWeakObjectPtr<AStaticMeshActor> Actor;
		bool bGrenade = false;
		float Age = 0.f;
	};

	void FireBullet();
	void ThrowGrenade();
	UProjectileMovementComponent* AddMovement(AStaticMeshActor* Actor, const FVector& Velocity);

	UPROPERTY() TObjectPtr<ACameraActor> Camera;
	TArray<FTracked> Tracked;
	TArray<TPair<FVector, FColor>> TrailPoints;
	float AutoTimer = 0.f;
	int32 AutoIndex = 0;
	int32 Bounces = 0;
	int32 BulletHits = 0;
	FVector LastExplosion = FVector::ZeroVector;
	float ExplosionTime = 0.f;
};
