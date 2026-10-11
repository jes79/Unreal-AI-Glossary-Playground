#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0201_CollisionPreset.generated.h"

class ACameraActor;
class AGlossaryLabel;
class UPrimitiveComponent;
class UStaticMeshComponent;

/**
 * 0201 Collision Preset / Object Channel
 * 프로젝트에 만든 Projectile 오브젝트 채널(ECC_GameTraceChannel1)로 총알을 쏜다.
 * 총알 프리셋: WorldStatic·WorldDynamic = Block, Pawn = Overlap, Projectile = Ignore.
 * 세 레인(Pawn / WorldDynamic / 다른 Projectile)에서 같은 총알이 통과·막힘·무시되는 차이를 보여 준다.
 */
UCLASS()
class GLOSSARYPHYSICS_API ADemo0201_CollisionPreset : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0201_CollisionPreset();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 총알 오브젝트 채널 (DefaultEngine.ini 의 Projectile = ECC_GameTraceChannel1) */
	UPROPERTY(EditAnywhere, Category = "Collision Preset")
	TEnumAsByte<ECollisionChannel> ProjectileChannel = ECC_GameTraceChannel1;

	/** true = Projectile 프리셋(Pawn Overlap), false = BlockAll 프리셋 (비교용) */
	UPROPERTY(EditAnywhere, Category = "Collision Preset")
	bool bUseProjectilePreset = true;

	UPROPERTY(EditAnywhere, Category = "Collision Preset", meta = (ClampMin = 0.2))
	float FireInterval = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Collision Preset")
	float BulletSpeed = 1500.f;

	UFUNCTION()
	void OnBulletHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnBulletOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	void FireVolley();
	void ApplyBulletPreset(UPrimitiveComponent* Bullet) const;
	void RefreshLabels();
	void Flash(AActor* Target, const FString& Event);

	UPROPERTY() TObjectPtr<ACameraActor> Camera;
	UPROPERTY() TArray<TObjectPtr<AActor>> Targets;       // 0 Pawn, 1 WorldDynamic, 2 Projectile
	UPROPERTY() TArray<TObjectPtr<AGlossaryLabel>> TargetLabels;
	UPROPERTY() TArray<TObjectPtr<AActor>> Bullets;

	TMap<TWeakObjectPtr<AActor>, float> FlashTimers;
	TArray<FString> EventLog;
	float FireTimer = 0.f;
	int32 HitCount = 0;
	int32 OverlapCount = 0;
};
