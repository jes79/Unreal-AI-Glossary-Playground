#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0309_FootIK.generated.h"

class AD03_Mannequin;
class UD03_AnimInstance;

/**
 * 0309 Foot IK (Two Bone IK)
 * 한 발은 계단 위, 한 발은 아래에 걸친 Manny 세 명:
 *  ① IK 없음(발이 뜸) ② C++ AnimInstance — 발밑 Line Trace + 골반 내리기 + Two Bone IK ③ 템플릿 ABP_Unarmed(CR_Mannequin_FootIK Control Rig)
 */
UCLASS()
class GLOSSARYANIMATION_API ADemo0309_FootIK : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0309_FootIK();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 계단 한 칸 높이 */
	UPROPERTY(EditAnywhere, Category = "Foot IK")
	float StepHeight = 25.f;

	/** 발밑 Line Trace 를 위·아래로 얼마나 쏘는지 */
	UPROPERTY(EditAnywhere, Category = "Foot IK")
	float TraceUp = 60.f;

	UPROPERTY(EditAnywhere, Category = "Foot IK")
	float TraceDown = 80.f;

private:
	UPROPERTY(Transient) TObjectPtr<AD03_Mannequin> NoIK;
	UPROPERTY(Transient) TObjectPtr<AD03_Mannequin> CppIK;
	UPROPERTY(Transient) TObjectPtr<AD03_Mannequin> RigIK;
	UPROPERTY(Transient) TObjectPtr<AD03_Mannequin> Walker;
	UPROPERTY(Transient) TObjectPtr<UD03_AnimInstance> CppAnim;
	UPROPERTY(Transient) TObjectPtr<UD03_AnimInstance> WalkerAnim;
	bool bIK = true;
};
