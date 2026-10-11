#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "RenderingDemoKit.h"
#include "Demo1004_VirtualShadowMaps.generated.h"

class ACameraActor;

/**
 * 1004 Virtual Shadow Maps
 * 가는 철망·울타리·격자 지붕처럼 그림자 디테일이 많은 장면을 만들고
 * r.Shadow.Virtual.Enable 1(VSM) ↔ 0(기존 Cascaded Shadow Maps)으로 그림자 해상도를 비교한다.
 */
UCLASS()
class GLOSSARYRENDERING_API ADemo1004_VirtualShadowMaps : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1004_VirtualShadowMaps();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** Project Settings > Shadow Map Method 와 같은 콘솔 변수 r.Shadow.Virtual.Enable */
	UPROPERTY(EditAnywhere, Category = "Virtual Shadow Maps")
	bool bVirtualShadowMaps = true;

	/** 기존 Shadow Map 의 해상도 (r.Shadow.MaxCSMResolution) — 올려도 VSM 만큼 선명해지지 않는다 */
	UPROPERTY(EditAnywhere, Category = "Virtual Shadow Maps")
	int32 ShadowMapResolution = 2048;

	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;

private:
	void Apply();
	RenderKit::FCVarScope CVars;
	RenderKit::FFrameStats Stats;
};
