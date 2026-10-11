#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0519_HitFlash.generated.h"

class UNiagaraComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class APostProcessVolume;

/**
 * 0519 Hit Flash — 피격 시 Dynamic Material Instance 의 FlashAmount 를 1→0 으로 0.15초 동안 내려 하얗게 번쩍인다.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0519_HitFlash : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0519_HitFlash();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Hit Flash") float FlashDuration = 0.15f;
	UPROPERTY(EditAnywhere, Category = "Hit Flash") float AutoHitInterval = 1.2f;

private:
	void Hit();
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> FlashMIDs;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> Dummy;
	bool bFlashEnabled = true;
	float FlashTime = 99.f;
	float Timer = 0.45f;  // 정수 초 캡처 직전에 피격되도록 위상 조정
	int32 HitCount = 0;
};
