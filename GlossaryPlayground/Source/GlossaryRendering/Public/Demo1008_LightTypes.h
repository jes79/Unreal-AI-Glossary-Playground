#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "RenderingDemoKit.h"
#include "Demo1008_LightTypes.generated.h"

class ACameraActor;
class UPointLightComponent;
class USpotLightComponent;
class URectLightComponent;
class UMaterialInterface;

/**
 * 1008 라이트 종류 (Point · Spot · Rect)
 * 복도 천장에 Rect Light 형광등, 끝의 무대에 Spot Light(Inner 20°, Outer 35°), 중간에 Point Light 전구.
 * 빌더가 만든 Light Function 머티리얼(MD_LightFunction)로 형광등 하나와 스포트라이트를 깜빡이게 한다.
 */
UCLASS()
class GLOSSARYRENDERING_API ADemo1008_LightTypes : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1008_LightTypes();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Light Types") float SpotInnerConeAngle = 20.f;
	UPROPERTY(EditAnywhere, Category = "Light Types") float SpotOuterConeAngle = 35.f;
	UPROPERTY(EditAnywhere, Category = "Light Types") bool bLightFunction = true;

	UPROPERTY(Transient) TArray<TObjectPtr<UPointLightComponent>> PointLights;
	UPROPERTY(Transient) TArray<TObjectPtr<USpotLightComponent>> SpotLights;
	UPROPERTY(Transient) TArray<TObjectPtr<URectLightComponent>> RectLights;
	UPROPERTY(Transient) TObjectPtr<URectLightComponent> FlickerRect;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> LightFunctionMaterial;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;

private:
	void ApplyLightFunction();
	RenderKit::FFrameStats Stats;
};
