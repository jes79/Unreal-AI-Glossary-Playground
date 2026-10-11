#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "MovementKit.h"
#include "Demo0112_CameraShake.generated.h"

class UCameraShakeBase;

/**
 * 0112 Camera Shake
 * 포탑의 공에 맞으면 PlayerCameraManager->StartCameraShake(Perlin Noise 0.2초).
 * 1 = 피격 흔들림 직접 재생, 2 = 폭발(Wave Oscillator 1초) 비교, C = 흔들림 ON/OFF.
 */
UCLASS()
class GLOSSARYMOVEMENT_API ADemo0112_CameraShake : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0112_CameraShake();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 피격 시 재생할 흔들림 (Perlin Noise 패턴, 0.2초) */
	UPROPERTY(EditAnywhere, Category = "Camera Shake") TSubclassOf<UCameraShakeBase> HitShake;
	/** 비교용 폭발 흔들림 (Wave Oscillator 패턴, 1초) */
	UPROPERTY(EditAnywhere, Category = "Camera Shake") TSubclassOf<UCameraShakeBase> ExplosionShake;
	UPROPERTY(EditAnywhere, Category = "Camera Shake") float HitShakeScale = 1.f;
	UPROPERTY(EditAnywhere, Category = "Camera Shake") float FireInterval = 1.5f;

private:
	void PlayShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale, const FString& What);

	MovementKit::FSimpleProjectiles Projectiles;
	FVector TurretPos = FVector(900.f, 0.f, 0.f);
	float FireTimer = 0.f;
	bool bShakeEnabled = true;
	int32 HitCount = 0;
	FString LastShake;
	float LastShakeTime = -10.f;
	float LastShakeDuration = 0.2f;
};
