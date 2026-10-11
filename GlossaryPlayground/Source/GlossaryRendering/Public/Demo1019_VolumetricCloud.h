#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "RenderingDemoKit.h"
#include "Demo1019_VolumetricCloud.generated.h"

class ACameraActor;
class UMaterialInstanceDynamic;
class UVolumetricCloudComponent;

/**
 * 1019 Volumetric Cloud
 * 공통 환경의 Volumetric Cloud 컴포넌트를 Layer Bottom Altitude 5km 로 올리고,
 * 엔진 기본 구름 머티리얼(m_SimpleVolumetricCloud)의 Dynamic Material Instance 로 구름 양(Cloud_GlobalCoverage)을 키로 조절한다.
 */
UCLASS()
class GLOSSARYRENDERING_API ADemo1019_VolumetricCloud : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1019_VolumetricCloud();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Volumetric Cloud > Layer > Layer Bottom Altitude (km) */
	UPROPERTY(EditAnywhere, Category = "Volumetric Cloud", meta = (ClampMin = 0)) float LayerBottomAltitude = 5.f;
	/** Layer Height (km) */
	UPROPERTY(EditAnywhere, Category = "Volumetric Cloud", meta = (ClampMin = 0.1)) float LayerHeight = 6.f;
	/** 머티리얼 파라미터: 구름 양 (0~1) */
	UPROPERTY(EditAnywhere, Category = "Volumetric Cloud", meta = (ClampMin = 0, ClampMax = 1)) float CloudCoverage = 0.5f;
	/** 해 고도 (도) */
	UPROPERTY(EditAnywhere, Category = "Volumetric Cloud") float SunElevation = 25.f;

	/** 구름 양을 넣을 머티리얼 파라미터 이름 (엔진 m_SimpleVolumetricCloud) */
	UPROPERTY(EditAnywhere, Category = "Volumetric Cloud") FName CoverageParameter = TEXT("Cloud_GlobalCoverage");

	UPROPERTY(Transient) TObjectPtr<UVolumetricCloudComponent> Cloud;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> CloudMID;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;

private:
	void Apply();
	bool bApplied = false;
	FString ParamInfo;
	RenderKit::FFrameStats Stats;
};
