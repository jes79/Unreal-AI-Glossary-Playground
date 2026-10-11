#pragma once

#include "CoreMinimal.h"
#include "Components/TimelineComponent.h"
#include "GlossaryDemoBase.h"
#include "Demo0314_Timeline.generated.h"

class UCurveFloat;
class UD03_GraphWidget;

/**
 * 0314 Timeline (커브 보간)
 * FTimeline 에 1초짜리 Ease In/Out 커브(C_DoorEaseInOut)를 걸어 문을 열고, 다시 누르면 Reverse 로 닫는다.
 * 옆 문은 같은 1초지만 Linear 커브 — 시작·끝의 느낌 차이를 비교.
 */
UCLASS()
class GLOSSARYANIMATION_API ADemo0314_Timeline : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0314_Timeline();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 문이 열리는 각도 */
	UPROPERTY(EditAnywhere, Category = "Timeline")
	float OpenAngle = 100.f;

	/** 타임라인 길이(초) — 커브 길이 1초 */
	UPROPERTY(EditAnywhere, Category = "Timeline")
	float Duration = 1.f;

	UPROPERTY(EditAnywhere, Category = "Timeline")
	bool bAutoToggle = true;

private:
	UFUNCTION() void OnEaseProgress(float Value);
	UFUNCTION() void OnLinearProgress(float Value);
	void Toggle();
	AActor* MakeDoor(const FVector& Hinge, const FLinearColor& Color, const FString& Label);

	FTimeline EaseTimeline;
	FTimeline LinearTimeline;

	UPROPERTY(Transient) TObjectPtr<UCurveFloat> EaseCurve;
	UPROPERTY(Transient) TObjectPtr<UCurveFloat> LinearCurve;
	UPROPERTY(Transient) TObjectPtr<AActor> EaseDoor;
	UPROPERTY(Transient) TObjectPtr<AActor> LinearDoor;
	UPROPERTY(Transient) TObjectPtr<UD03_GraphWidget> Graph;
	float EaseValue = 0.f;
	float LinearValue = 0.f;
	bool bOpen = false;
	float AutoTimer = 0.f;
};
