#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0521_Bloom.generated.h"

class APostProcessVolume;
class UMaterialInstanceDynamic;
class UPointLightComponent;

/**
 * 0521 Bloom — Bloom Intensity 1.5 · Threshold -1 의 표준(가우시안) Bloom 과 Convolution(FFT) Bloom 비교.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0521_Bloom : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0521_Bloom();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Bloom") float BloomIntensity = 1.5f;
	UPROPERTY(EditAnywhere, Category = "Bloom") float BloomThreshold = -1.f;

private:
	void ApplyMode();
	UPROPERTY(Transient) TObjectPtr<APostProcessVolume> Volume;
	int32 Mode = 1;
};
