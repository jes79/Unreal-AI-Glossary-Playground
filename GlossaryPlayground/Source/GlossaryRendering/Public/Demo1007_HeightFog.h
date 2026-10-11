#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "RenderingDemoKit.h"
#include "Demo1007_HeightFog.generated.h"

class ACameraActor;

/**
 * 1007 Exponential Height Fog / Volumetric Fog
 * 창문이 하나 있는 어두운 방. 공통 환경의 Exponential Height Fog 에 Volumetric Fog 를 켜면
 * 창살 사이로 들어오는 햇빛이 공기 중에 빛줄기(God Ray)로 보인다.
 */
UCLASS()
class GLOSSARYRENDERING_API ADemo1007_HeightFog : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1007_HeightFog();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Height Fog")
	bool bVolumetricFog = true;

	UPROPERTY(EditAnywhere, Category = "Height Fog")
	float FogDensity = 0.04f;

	/** 0 = 사방으로 고르게, 0.9 = 빛 방향으로 강하게 (빛을 마주 볼 때 빛줄기가 밝다) */
	UPROPERTY(EditAnywhere, Category = "Height Fog", meta = (ClampMin = 0, ClampMax = 0.9))
	float ScatteringDistribution = 0.6f;

	/** Directional Light > Volumetric Scattering Intensity */
	UPROPERTY(EditAnywhere, Category = "Height Fog")
	float SunScatteringIntensity = 4.f;

	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;

private:
	void ApplyFog();
	bool bFogEnabled = true;
	bool bApplied = false;
	RenderKit::FFrameStats Stats;
};
