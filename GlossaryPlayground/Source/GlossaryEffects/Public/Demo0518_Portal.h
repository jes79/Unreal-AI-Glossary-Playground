#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0518_Portal.generated.h"

class UNiagaraComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class APostProcessVolume;

/**
 * 0518 Portal — Vortex Velocity 로 입자가 원을 그리며 중심으로 빨려 들어가는 소용돌이 포탈.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0518_Portal : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0518_Portal();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> Portal;
	UPROPERTY(Transient) TObjectPtr<UPointLightComponent> Glow;
	float Time = 0.f;
};
