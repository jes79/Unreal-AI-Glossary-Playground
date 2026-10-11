#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0310_ControlRigLookAt.generated.h"

class AD03_Mannequin;
class UD03_AnimInstance;
class AStaticMeshActor;

/**
 * 0310 Control Rig (Look At)
 * 머리가 가장 가까운 타겟을 바라보는 Aim 을 절차적으로 계산한다. 회전 한계 60°.
 * 타겟 세 개가 주위를 돌고, 가장 가까운 것을 고른다. 60° 를 넘으면 한계에서 멈춘다.
 */
UCLASS()
class GLOSSARYANIMATION_API ADemo0310_ControlRigLookAt : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0310_ControlRigLookAt();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Aim 노드의 회전 한계 (도) */
	UPROPERTY(EditAnywhere, Category = "Look At", meta = (ClampMin = 0, ClampMax = 180))
	float RotationLimit = 60.f;

	/** 이 거리 안의 타겟만 본다 */
	UPROPERTY(EditAnywhere, Category = "Look At")
	float LookRadius = 900.f;

private:
	UPROPERTY(Transient) TObjectPtr<AD03_Mannequin> Npc;
	UPROPERTY(Transient) TObjectPtr<UD03_AnimInstance> NpcAnim;
	UPROPERTY(Transient) TArray<TObjectPtr<AStaticMeshActor>> Targets;
	bool bEnabled = true;
	bool bLimit = true;
	float Time = 0.f;
};
