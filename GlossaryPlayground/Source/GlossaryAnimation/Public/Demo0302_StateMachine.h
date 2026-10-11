#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0302_StateMachine.generated.h"

class AD03_Mannequin;
class UD03_GraphWidget;

/**
 * 0302 State Machine
 * 템플릿 ABP_Unarmed 안의 실제 State Machine 을 실행 중에 조회(GetCurrentStateName)해서
 * Idle·Locomotion·Jump·Fall·Land 다이어그램에 현재 상태를 강조하고 전이 조건을 적어 보여 준다.
 */
UCLASS()
class GLOSSARYANIMATION_API ADemo0302_StateMachine : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0302_StateMachine();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** NPC 행동 한 주기(초): 서 있기 → 이동 중 점프 → 서 있기 → 제자리 점프 */
	UPROPERTY(EditAnywhere, Category = "State Machine")
	float CycleLength = 7.f;

private:
	FString ReadStates(class ACharacter* Character, FString& OutLeafState) const;
	void DrawDiagram(const FString& Leaf);

	UPROPERTY(Transient) TObjectPtr<AD03_Mannequin> Npc;
	UPROPERTY(Transient) TObjectPtr<UD03_GraphWidget> Diagram;
	float CycleTime = 0.f;
	bool bJumpedMoving = false, bJumpedIdle = false;
};
