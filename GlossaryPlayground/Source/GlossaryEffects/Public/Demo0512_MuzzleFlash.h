#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0512_MuzzleFlash.generated.h"

class UNiagaraComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class APostProcessVolume;

/**
 * 0512 Muzzle Flash — 총구(Muzzle) 위치에 0.05초 화염 Niagara + Point Light 깜빡임. 연사, 슬로 모션으로 확인.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0512_MuzzleFlash : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0512_MuzzleFlash();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 화염·빛이 보이는 시간 */
	UPROPERTY(EditAnywhere, Category = "Muzzle Flash") float FlashDuration = 0.05f;
	/** 연사 간격 */
	UPROPERTY(EditAnywhere, Category = "Muzzle Flash") float FireInterval = 0.12f;
	UPROPERTY(EditAnywhere, Category = "Muzzle Flash") float LightIntensity = 60.f;

private:
	void Fire();
	UPROPERTY(Transient) TObjectPtr<USceneComponent> Muzzle;
	UPROPERTY(Transient) TObjectPtr<UPointLightComponent> FlashLight;
	bool bAutoFire = true;
	bool bSlowMo = false;
	bool bHold = false;   // H: 0.05초 화염을 끊김 없이 이어 붙여 모양을 계속 보여 준다(관찰용)
	float FireTimer = 0.f;
	float LightTimer = 0.f;
	int32 Shots = 0;
};
