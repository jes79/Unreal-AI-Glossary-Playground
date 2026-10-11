#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0526_ColorGrading.generated.h"

class APostProcessVolume;
class UMaterialInstanceDynamic;
class UPointLightComponent;

/**
 * 0526 Color Grading / LUT — 채도를 낮추고 색온도를 높인 석양 톤 그레이딩, 같은 톤을 LUT 텍스처로 적용하는 방식 비교.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0526_ColorGrading : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0526_ColorGrading();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Color Grading") float Saturation = 0.7f;
	UPROPERTY(EditAnywhere, Category = "Color Grading") float WhiteTemp = 8500.f;

private:
	void ApplyMode();
	UPROPERTY(Transient) TObjectPtr<APostProcessVolume> Volume;
	int32 Mode = 2;
};
