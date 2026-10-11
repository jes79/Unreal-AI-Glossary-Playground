#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0115_Possess.generated.h"

class AD0115_Drone;
class AGlossaryLabel;
class UMovementSidePanel;

/**
 * 0115 Possess (빙의)
 * 캐릭터가 드론 근처에서 E → PlayerController->Possess(Drone). 드론에서 다시 E → 캐릭터로 Possess.
 * 오른쪽 패널에 현재 폰·컨트롤러·활성 매핑 컨텍스트를 보여 준다.
 */
UCLASS()
class GLOSSARYMOVEMENT_API ADemo0115_Possess : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0115_Possess();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "Possess") float PossessDistance = 500.f;

private:
	UPROPERTY(Transient) TObjectPtr<AD0115_Drone> Drone;
	UPROPERTY(Transient) TObjectPtr<APawn> Character;
	UPROPERTY(Transient) TObjectPtr<UMovementSidePanel> Panel;
	TWeakObjectPtr<AGlossaryLabel> Prompt;
	int32 SwitchCount = 0;
	FString LastEvent;
};
