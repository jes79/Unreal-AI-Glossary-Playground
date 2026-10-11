#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0505_RibbonTrail.generated.h"

class UNiagaraComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class APostProcessVolume;

/**
 * 0505 Ribbon Renderer — 휘두르는 칼의 칼끝(Tip 소켓 역할 컴포넌트)에 Ribbon 이미터를 붙여 검기 트레일을 그린다.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0505_RibbonTrail : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0505_RibbonTrail();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 한 번 휘두르는 시간(초) */
	UPROPERTY(EditAnywhere, Category = "Ribbon") float SwingDuration = 0.45f;
	/** 휘두르는 각도 범위 */
	UPROPERTY(EditAnywhere, Category = "Ribbon") float SwingArc = 150.f;

private:
	UPROPERTY(Transient) TObjectPtr<USceneComponent> Pivot;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> SwordTip;
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> Trail;
	bool bTrailOn = true;
	float SwingTime = 0.f;
	float Pause = 0.f;
	int32 Direction = 1;
};
