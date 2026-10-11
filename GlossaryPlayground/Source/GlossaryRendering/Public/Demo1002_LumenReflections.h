#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "RenderingDemoKit.h"
#include "Engine/EngineTypes.h"
#include "Demo1002_LumenReflections.generated.h"

class UPostProcessComponent;
class ACameraActor;

/**
 * 1002 Lumen Reflections
 * Roughness 0.05 대리석 바닥 위에 기둥·구를 놓고, 화면 밖(카메라 뒤·위)에 큰 색 벽을 둔다.
 * Lumen 반사는 화면 밖 물체도 비추고, Screen Space Reflections 는 화면에 보이는 것만 비춘다(가장자리에서 끊김).
 */
UCLASS()
class GLOSSARYRENDERING_API ADemo1002_LumenReflections : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1002_LumenReflections();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Post Process > Reflections > Method */
	UPROPERTY(EditAnywhere, Category = "Lumen Reflections")
	TEnumAsByte<EReflectionMethod::Type> ReflectionMethod = EReflectionMethod::Lumen;

	/** 대리석 바닥 Roughness (요청 예문 0.05) */
	UPROPERTY(EditAnywhere, Category = "Lumen Reflections", meta = (ClampMin = 0, ClampMax = 1))
	float FloorRoughness = 0.05f;

	UPROPERTY(Transient) TObjectPtr<UPostProcessComponent> PostProcess;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;

private:
	void ApplyMethod();
	RenderKit::FFrameStats Stats;
};
