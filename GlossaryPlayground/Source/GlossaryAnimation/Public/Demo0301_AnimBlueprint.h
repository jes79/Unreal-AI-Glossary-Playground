#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0301_AnimBlueprint.generated.h"

class AD03_Mannequin;
class UD03_AnimInstance;

/**
 * 0301 Animation Blueprint
 * AnimBP 의 Event Graph(Speed·IsFalling 계산)를 C++ AnimInstance 의 NativeUpdateAnimation 으로 구현해
 * 플레이어와 NPC 에 적용하고, 값이 실시간으로 바뀌는 것을 보여 준다. 템플릿 ABP_Unarmed 와 바꿔 볼 수 있다.
 */
UCLASS()
class GLOSSARYANIMATION_API ADemo0301_AnimBlueprint : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0301_AnimBlueprint();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** NPC 가 점프하는 간격(초) — IsFalling 이 바뀌는 것을 보여 주려고 */
	UPROPERTY(EditAnywhere, Category = "Animation Blueprint")
	float NpcJumpInterval = 2.5f;

private:
	void ApplyAnimClass();

	UPROPERTY(Transient) TObjectPtr<AD03_Mannequin> Npc;
	bool bUseCpp = true;
	float JumpTimer = 0.f;
};
