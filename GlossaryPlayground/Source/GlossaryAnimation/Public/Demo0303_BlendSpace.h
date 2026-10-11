#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0303_BlendSpace.generated.h"

class AD03_Mannequin;
class UD03_AnimInstance;
class UD03_GraphWidget;
class UBlendSpace;

/**
 * 0303 Blend Space
 * 빌더가 만든 2D Blend Space(BS_Locomotion8Dir: X=Direction -180~180, Y=Speed 0~600, 8방향 걷기·조깅)를
 * NPC 가 원을 그리며 몸 방향을 고정한 채(옆·뒤 걸음) 재생한다. 오른쪽 아래에 샘플 격자와 현재 입력 위치.
 */
UCLASS()
class GLOSSARYANIMATION_API ADemo0303_BlendSpace : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0303_BlendSpace();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** NPC 가 도는 원의 반지름 */
	UPROPERTY(EditAnywhere, Category = "Blend Space")
	float CircleRadius = 380.f;

	/** 1 = 걷기(200), 2 = 조깅(500), 3 = 자동(200↔500 왕복) */
	UPROPERTY(EditAnywhere, Category = "Blend Space")
	int32 SpeedMode = 3;

	/** false 면 가장 가까운 샘플 하나로 스냅(섞지 않음) — 비교용 */
	UPROPERTY(EditAnywhere, Category = "Blend Space")
	bool bBlend = true;

private:
	UPROPERTY(Transient) TObjectPtr<AD03_Mannequin> Npc;
	UPROPERTY(Transient) TObjectPtr<UD03_AnimInstance> NpcAnim;
	UPROPERTY(Transient) TObjectPtr<UD03_GraphWidget> Graph;
	UPROPERTY(Transient) TObjectPtr<UBlendSpace> BlendSpace;
	float Angle = 0.f;
	float Time = 0.f;
};
