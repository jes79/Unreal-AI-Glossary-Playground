#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0524_DepthOfField.generated.h"

class APostProcessVolume;
class UMaterialInstanceDynamic;
class UPointLightComponent;

/**
 * 0524 Depth of Field — Cinematic DOF 의 Focal Distance 를 대화 상대까지의 거리로 맞추고 배경을 흐린다.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0524_DepthOfField : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0524_DepthOfField();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Depth of Field") float Aperture = 1.4f;

private:
	UPROPERTY(Transient) TObjectPtr<APostProcessVolume> Volume;
	UPROPERTY(Transient) TObjectPtr<AActor> Partner;
	FVector CameraLocation = FVector::ZeroVector;
	bool bDOF = true;
	bool bFocusFar = false;
};
