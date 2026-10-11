#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0511_SubUV.generated.h"

class UNiagaraComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class APostProcessVolume;

/**
 * 0511 SubUV (플립북) — 코드로 만든 8x8 폭발 플립북 텍스처를 SubUV Animation 모듈로 재생한다. 오른쪽 판은 원본 시트.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0511_SubUV : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0511_SubUV();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "SubUV") float Interval = 1.6f;

private:
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> Flipbook;
	float Timer = 0.f;
};
