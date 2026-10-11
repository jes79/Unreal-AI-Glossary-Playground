#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0510_Beam.generated.h"

class UNiagaraComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class APostProcessVolume;

/**
 * 0510 Beam — 두 점 사이를 잇는 번개 빔. 시작·끝점을 User.BeamStart / User.BeamEnd 로 C++ 에서 매 프레임 넘긴다.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0510_Beam : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0510_Beam();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Beam") float OrbitSpeed = 40.f;

private:
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> Beam;
	UPROPERTY(Transient) TObjectPtr<AActor> StartOrb;
	UPROPERTY(Transient) TObjectPtr<AActor> EndOrb;
	bool bMoving = true;
	float Angle = 0.f;
};
