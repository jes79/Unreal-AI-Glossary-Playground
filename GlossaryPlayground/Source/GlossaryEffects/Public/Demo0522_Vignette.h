#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0522_Vignette.generated.h"

class APostProcessVolume;
class UMaterialInstanceDynamic;
class UPointLightComponent;

/**
 * 0522 Vignette — 체력이 30% 이하로 떨어지면 Vignette Intensity 를 0.8 로 올려 화면 가장자리를 어둡게.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0522_Vignette : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0522_Vignette();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Vignette") float LowHealthThreshold = 0.3f;
	UPROPERTY(EditAnywhere, Category = "Vignette") float LowHealthVignette = 0.8f;
	UPROPERTY(EditAnywhere, Category = "Vignette") float NormalVignette = 0.2f;

private:
	UPROPERTY(Transient) TObjectPtr<APostProcessVolume> Volume;
	float Health = 1.f;
	float CurrentVignette = 0.2f;
	float Time = 0.f;
};
