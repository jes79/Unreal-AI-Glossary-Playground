#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "RenderingDemoKit.h"
#include "Engine/EngineTypes.h"
#include "Demo1001_LumenGI.generated.h"

class UPostProcessComponent;

/**
 * 1001 Lumen GI
 * 코넬 박스(왼쪽 빨간 벽 · 오른쪽 초록 벽 · 흰 바닥)에 스포트라이트를 빨간 벽에 비춘다.
 * Lumen GI 를 켜면 빨간 벽에서 튕긴 빛이 흰 바닥·상자를 붉게 물들이고, 끄면 직사광 말고는 검다.
 */
UCLASS()
class GLOSSARYRENDERING_API ADemo1001_LumenGI : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1001_LumenGI();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Post Process > Global Illumination > Method (Lumen / None / Screen Space) */
	UPROPERTY(EditAnywhere, Category = "Lumen GI")
	TEnumAsByte<EDynamicGlobalIlluminationMethod::Type> DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::Lumen;

	/** 빨간 벽을 비추는 스포트라이트 세기 (cd) */
	UPROPERTY(EditAnywhere, Category = "Lumen GI")
	float SpotIntensity = 400.f;

	UPROPERTY(Transient) TObjectPtr<UPostProcessComponent> PostProcess;

private:
	void ApplyMethod();
	RenderKit::FFrameStats Stats;
};
