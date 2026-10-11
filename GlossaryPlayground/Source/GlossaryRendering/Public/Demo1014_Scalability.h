#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "RenderingDemoKit.h"
#include "Demo1014_Scalability.generated.h"

class UD1014_ScalabilityWidget;

/**
 * 1014 Scalability (그래픽 옵션)
 * UGameUserSettings 로 전체 품질(낮음~시네마틱)·해상도·프레임 제한을 바꾸고 ApplySettings 한다.
 * 그림자·GI·반사·안개가 많은 숲 장면에서 품질 단계별 차이와 GPU 시간을 비교한다.
 * 레벨을 나갈 때 원래 설정으로 되돌린다(다른 예제 레벨에 영향이 남지 않게).
 */
UCLASS()
class GLOSSARYRENDERING_API ADemo1014_Scalability : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1014_Scalability();

	/** 메뉴에서 고른 값 (적용 전) */
	int32 PendingQuality = 3;
	FIntPoint PendingResolution = FIntPoint(1600, 900);
	float PendingFrameLimit = 0.f;

	/** 고른 값을 UGameUserSettings 에 넣고 ApplySettings */
	void ApplyPending();
	void RefreshWidget();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(Transient) TObjectPtr<UD1014_ScalabilityWidget> Menu;

private:
	int32 OriginalQuality = 3;
	FIntPoint OriginalResolution = FIntPoint(1600, 900);
	float OriginalFrameLimit = 0.f;
	bool bChangedSettings = false;
	RenderKit::FFrameStats Stats;
};
