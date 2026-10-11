#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0203_LineTrace.generated.h"

class AGlossaryLabel;

/**
 * 0203 Line Trace (레이캐스트)
 * 매 프레임 카메라 정면으로 LineTraceSingleByChannel(Visibility) 1000cm 를 쏴서
 * 맞은 액터 이름·거리를 표시하고 DrawDebugLine 으로 선을 그린다(맞으면 초록, 빗나가면 빨강).
 */
UCLASS()
class GLOSSARYPHYSICS_API ADemo0203_LineTrace : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0203_LineTrace();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 트레이스 길이 (cm) */
	UPROPERTY(EditAnywhere, Category = "Line Trace", meta = (ClampMin = 100))
	float TraceDistance = 1000.f;

	UPROPERTY(EditAnywhere, Category = "Line Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

private:
	UPROPERTY() TObjectPtr<AGlossaryLabel> HitLabel;
	UPROPERTY() TObjectPtr<AActor> Mover;
	float Time = 0.f;
};
