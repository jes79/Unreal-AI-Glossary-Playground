#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0514_Explosion.generated.h"

class UNiagaraComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class APostProcessVolume;

/**
 * 0514 Explosion — 섬광·파편·연기·충격파 링 이미터가 시간차로 터지고, 카메라 쉐이크 + 섬광 라이트.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0514_Explosion : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0514_Explosion();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Explosion") float AutoInterval = 4.f;

private:
	void Explode();
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> Explosion;
	UPROPERTY(Transient) TObjectPtr<UPointLightComponent> FlashLight;
	float Timer = 0.f;
	float SinceExplode = 99.f;
	bool bShake = true;
};
