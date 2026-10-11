#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "EffectsDemoKit.generated.h"

class AActor;
class ACameraActor;
class AGlossaryDemoBase;
class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;
class UPointLightComponent;

/** 05 분야 데모 공통 헬퍼: 생성된 NS_ 에셋 불러오기, Niagara 컴포넌트 붙이기, 고정 카메라 */
namespace EffectsKit
{
	/** "/Game/Glossary/05_Effects/" + Name */
	GLOSSARYEFFECTS_API FString AssetPath(const FString& Name);

	/** 빌더가 만든 Niagara System (없으면 로그 오류 후 nullptr) */
	GLOSSARYEFFECTS_API UNiagaraSystem* LoadSystem(const FString& Name);

	/** Owner 에 Niagara 컴포넌트를 붙여 재생한다 (Parent 가 null 이면 루트) */
	GLOSSARYEFFECTS_API UNiagaraComponent* AddNiagara(AActor* Owner, const FString& SystemName, const FVector& RelativeLocation,
		USceneComponent* Parent = nullptr, bool bAutoActivate = true, const FRotator& RelativeRotation = FRotator::ZeroRotator);

	/** 고정 카메라를 놓고 플레이어 시점을 그 카메라로 바꾼다 */
	GLOSSARYEFFECTS_API ACameraActor* SetupFixedCamera(AGlossaryDemoBase* Demo, const FVector& Location, const FVector& LookAt, float FOV = 70.f);

	/** 깜빡임용 포인트 라이트 */
	GLOSSARYEFFECTS_API UPointLightComponent* AddPointLight(AActor* Owner, const FVector& RelativeLocation, const FLinearColor& Color,
		float Intensity, float Radius, USceneComponent* Parent = nullptr);
}

/**
 * 간단한 감쇠 흔들림 패턴 (엔진 기본 Wave/Perlin 패턴은 EngineCameras 플러그인 소속이라 직접 구현).
 * 시작 세기에서 Duration 동안 선형으로 줄어드는 사인 흔들림.
 */
UCLASS()
class GLOSSARYEFFECTS_API UGlossaryDecayShakePattern : public UCameraShakePattern
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = Shake) float Duration = 0.6f;
	UPROPERTY(EditAnywhere, Category = Shake) float LocationAmplitude = 12.f;
	UPROPERTY(EditAnywhere, Category = Shake) float RotationAmplitude = 1.5f;
	UPROPERTY(EditAnywhere, Category = Shake) float Frequency = 22.f;

protected:
	virtual void GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const override;
	virtual void StartShakePatternImpl(const FCameraShakePatternStartParams& Params) override;
	virtual void UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult) override;
	virtual bool IsFinishedImpl() const override;

	float Elapsed = 0.f;
};

/** 폭발용 카메라 쉐이크: PlayerCameraManager->StartCameraShake(UGlossaryExplosionShake::StaticClass()) */
UCLASS()
class GLOSSARYEFFECTS_API UGlossaryExplosionShake : public UCameraShakeBase
{
	GENERATED_BODY()
public:
	UGlossaryExplosionShake(const FObjectInitializer& ObjectInitializer);
};

class APostProcessVolume;
class AVolume;

namespace EffectsKit
{
	/** 실행 중 Unbound(Infinite Extent) Post Process Volume 을 하나 놓는다 — 화면 전체에 적용 */
	GLOSSARYEFFECTS_API APostProcessVolume* SpawnUnboundPostProcess(UWorld* World, float Priority = 10.f);

#if WITH_EDITOR
	/** BuildLevel 용: 상자 브러시를 가진 볼륨(Post Process Volume 등)을 레벨에 놓는다 */
	GLOSSARYEFFECTS_API AVolume* SpawnBoxVolume(UWorld* World, UClass* VolumeClass, const FVector& Center, const FVector& Size);
#endif
}
