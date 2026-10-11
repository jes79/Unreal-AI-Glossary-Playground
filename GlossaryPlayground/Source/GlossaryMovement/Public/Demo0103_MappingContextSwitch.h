#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0103_MappingContextSwitch.generated.h"

class UMovementSidePanel;
class AGlossaryLabel;

/**
 * 0103 Input Mapping Context 전환
 * 차 옆에서 E → IMC_Vehicle(우선순위 1) 추가, 다시 E → 제거. 오른쪽 패널에 활성 컨텍스트 목록과 키 의미를 보여 준다.
 */
UCLASS()
class GLOSSARYMOVEMENT_API ADemo0103_MappingContextSwitch : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0103_MappingContextSwitch();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 차를 만든다: 빈 액터 + 몸체·지붕·바퀴 도형 */
	AActor* SpawnCar(const FVector& Location, float Yaw);

	UPROPERTY(Transient) TObjectPtr<UMovementSidePanel> Panel;
	UPROPERTY(Transient) TObjectPtr<AActor> Car;
	UPROPERTY(Transient) TObjectPtr<AGlossaryLabel> Prompt;
};
