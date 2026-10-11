#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0102_EnhancedInput.generated.h"

class UMovementSidePanel;

/**
 * 0102 Enhanced Input
 * IA_Move · IA_Jump · IA_Attack(+ IA_HeavyAttack) 와 IMC_Default 를 실행 중에 만들고(AD0102_InputCharacter),
 * 키보드·게임패드 매핑표와 트리거 이벤트 로그를 오른쪽 패널에 보여 준다.
 */
UCLASS()
class GLOSSARYMOVEMENT_API ADemo0102_EnhancedInput : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0102_EnhancedInput();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(Transient) TObjectPtr<UMovementSidePanel> Panel;
};
