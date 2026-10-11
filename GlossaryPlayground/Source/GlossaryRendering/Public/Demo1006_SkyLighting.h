#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "RenderingDemoKit.h"
#include "Demo1006_SkyLighting.generated.h"

class ACameraActor;

/**
 * 1006 하늘 조명 세트 (Directional Light · Sky Atmosphere · Sky Light · Volumetric Cloud)
 * 공통 환경 액터의 해(Atmosphere Sun Light) 각도를 키로 돌리면 하늘색·노을·구름 색·그림자·하늘빛이 함께 바뀐다.
 * 각 구성 요소를 하나씩 끄면 어떤 역할을 하는지 보인다.
 */
UCLASS()
class GLOSSARYRENDERING_API ADemo1006_SkyLighting : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1006_SkyLighting();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 해 고도 (도). 0 이면 지평선, 음수면 밤 */
	UPROPERTY(EditAnywhere, Category = "Sky Lighting", meta = (ClampMin = -30, ClampMax = 90))
	float SunElevation = 35.f;

	UPROPERTY(EditAnywhere, Category = "Sky Lighting")
	float SunAzimuth = -35.f;

	/** 켜면 하루가 60초에 지나간다 */
	UPROPERTY(EditAnywhere, Category = "Sky Lighting")
	bool bAutoDayCycle = false;

	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;

private:
	void ToggleComponent(UClass* Class, const TCHAR* Name);
	FString ComponentState() const;
	RenderKit::FFrameStats Stats;
};
