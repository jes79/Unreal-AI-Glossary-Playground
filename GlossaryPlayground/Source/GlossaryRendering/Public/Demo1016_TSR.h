#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "RenderingDemoKit.h"
#include "Demo1016_TSR.generated.h"

class ACameraActor;

/**
 * 1016 TSR / Screen Percentage
 * 가는 철사·먼 울타리·회전 날개처럼 계단 현상이 잘 보이는 장면에서
 * r.AntiAliasingMethod(TSR/TAA/FXAA/없음)와 r.ScreenPercentage 67 을 바꿔 화질과 GPU 시간을 비교한다.
 */
UCLASS()
class GLOSSARYRENDERING_API ADemo1016_TSR : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1016_TSR();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** r.AntiAliasingMethod: 0 없음 · 1 FXAA · 2 TAA · 4 TSR */
	UPROPERTY(EditAnywhere, Category = "TSR") int32 AntiAliasingMethod = 4;
	/** r.ScreenPercentage (요청 예문 67%) */
	UPROPERTY(EditAnywhere, Category = "TSR") float ScreenPercentage = 67.f;

	UPROPERTY(Transient) TObjectPtr<USceneComponent> Fan;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;

private:
	void Apply();
	RenderKit::FCVarScope CVars;
	RenderKit::FFrameStats Stats;
};
