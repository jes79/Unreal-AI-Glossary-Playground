#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0508_CurlNoise.generated.h"

class UNiagaraComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class APostProcessVolume;

/**
 * 0508 Curl Noise Force — 반딧불이 Curl Noise Force 로 소용돌이치듯 떠다닌다. 왼쪽은 같은 입자에 힘 없음(직선 이동).
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0508_CurlNoise : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0508_CurlNoise();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> WithCurl;
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> WithoutCurl;
	bool bShowBoth = true;
};
