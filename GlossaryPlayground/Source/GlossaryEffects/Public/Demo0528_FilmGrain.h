#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0528_FilmGrain.generated.h"

class APostProcessVolume;
class UMaterialInstanceDynamic;
class UPointLightComponent;

/**
 * 0528 Film Grain — 공포 분위기용 Film Grain Intensity 0.3, 켜기/끄기 비교.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0528_FilmGrain : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0528_FilmGrain();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Film Grain") float FilmGrainIntensity = 0.3f;

private:
	UPROPERTY(Transient) TObjectPtr<APostProcessVolume> Volume;
	UPROPERTY(Transient) TObjectPtr<UPointLightComponent> Bulb;
	bool bGrain = true;
	float Time = 0.f;
};
