#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0517_Weather.generated.h"

class UNiagaraComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class APostProcessVolume;

/**
 * 0517 Weather (비) — 카메라를 따라다니는 GPU 비 이미터와 바닥에 튀는 물방울.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0517_Weather : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0517_Weather();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Weather") float RainRate = 6000.f;

private:
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> Rain;
	bool bRainOn = true;
};
