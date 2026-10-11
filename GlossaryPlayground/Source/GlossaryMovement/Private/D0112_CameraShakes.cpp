#include "D0112_CameraShakes.h"

#include "Shakes/PerlinNoiseCameraShakePattern.h"
#include "Shakes/WaveOscillatorCameraShakePattern.h"

UD0112_PerlinHitShake::UD0112_PerlinHitShake(const FObjectInitializer& ObjectInitializer)
	// 부모가 만드는 "RootShakePattern" 서브오브젝트의 클래스를 Perlin Noise 로 지정
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPerlinNoiseCameraShakePattern>(TEXT("RootShakePattern")))
{
	bSingleInstance = false;
	if (UPerlinNoiseCameraShakePattern* Pattern = Cast<UPerlinNoiseCameraShakePattern>(GetRootShakePattern()))
	{
		// 요청 예문: 0.2초. 짧은 피격이므로 블렌드도 아주 짧게
		Pattern->Duration = 0.2f;
		Pattern->BlendInTime = 0.02f;
		Pattern->BlendOutTime = 0.08f;
		// 위치 흔들림 (cm) — 불규칙한 Perlin 노이즈라 "툭" 맞는 느낌
		Pattern->X.Amplitude = 6.f;  Pattern->X.Frequency = 25.f;
		Pattern->Y.Amplitude = 10.f; Pattern->Y.Frequency = 30.f;
		Pattern->Z.Amplitude = 10.f; Pattern->Z.Frequency = 30.f;
		// 회전 흔들림 (도)
		Pattern->Pitch.Amplitude = 2.5f; Pattern->Pitch.Frequency = 25.f;
		Pattern->Yaw.Amplitude = 2.5f;   Pattern->Yaw.Frequency = 25.f;
		Pattern->Roll.Amplitude = 1.5f;  Pattern->Roll.Frequency = 20.f;
	}
}

UD0112_WaveExplosionShake::UD0112_WaveExplosionShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UWaveOscillatorCameraShakePattern>(TEXT("RootShakePattern")))
{
	if (UWaveOscillatorCameraShakePattern* Pattern = Cast<UWaveOscillatorCameraShakePattern>(GetRootShakePattern()))
	{
		Pattern->Duration = 1.0f;
		Pattern->BlendInTime = 0.05f;
		Pattern->BlendOutTime = 0.7f;   // 길게 사라짐
		Pattern->Z.Amplitude = 18.f;  Pattern->Z.Frequency = 9.f;
		Pattern->Pitch.Amplitude = 3.f; Pattern->Pitch.Frequency = 8.f;
		Pattern->Roll.Amplitude = 2.f;  Pattern->Roll.Frequency = 6.f;
	}
}
