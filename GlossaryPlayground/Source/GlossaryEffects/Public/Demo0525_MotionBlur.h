#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0525_MotionBlur.generated.h"

class APostProcessVolume;
class UMaterialInstanceDynamic;
class UPointLightComponent;

/**
 * 0525 Motion Blur — 빠르게 달리는 차를 따라가는 카메라, Motion Blur Amount 0.5 로 배경이 흐르는 속도감. 옵션처럼 끌 수 있다.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0525_MotionBlur : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0525_MotionBlur();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Motion Blur") float MotionBlurAmount = 0.5f;
	UPROPERTY(EditAnywhere, Category = "Motion Blur") float CarSpeed = 3000.f;

private:
	UPROPERTY(Transient) TObjectPtr<APostProcessVolume> Volume;
	UPROPERTY(Transient) TObjectPtr<AActor> Car;
	UPROPERTY(Transient) TObjectPtr<AActor> ChaseCamera;
	bool bBlurOn = true;
	float Angle = 0.f;
};
