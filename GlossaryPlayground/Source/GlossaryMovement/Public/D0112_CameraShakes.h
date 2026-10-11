#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "D0112_CameraShakes.generated.h"

/**
 * 0112 피격용 Camera Shake: 루트 패턴이 Perlin Noise, 0.2초.
 * 에디터에서는 Blueprint Class > CameraShakeBase 를 만들고 Root Shake Pattern 을 "Perlin Noise Camera Shake Pattern" 으로
 * 고르는 것과 같은 구성이다. (UE 5.x 의 UCameraShakeBase + 패턴 구조, 옛 UMatineeCameraShake 대체)
 */
UCLASS()
class GLOSSARYMOVEMENT_API UD0112_PerlinHitShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	UD0112_PerlinHitShake(const FObjectInitializer& ObjectInitializer);
};

/** 비교용 폭발 흔들림: Wave Oscillator 패턴(사인파), 1초, 점점 약해짐 */
UCLASS()
class GLOSSARYMOVEMENT_API UD0112_WaveExplosionShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	UD0112_WaveExplosionShake(const FObjectInitializer& ObjectInitializer);
};
