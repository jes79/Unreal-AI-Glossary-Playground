#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "D03_AnimInstance.generated.h"

class UBlendSpace;
class UAnimSequence;

/**
 * 03 애니메이션 분야가 함께 쓰는 C++ AnimInstance.
 *
 * Animation Blueprint 는 두 부분으로 되어 있다.
 *   - Event Graph : 매 프레임 변수(Speed, IsFalling …)를 계산 → 여기서는 NativeUpdateAnimation
 *   - Anim Graph  : 변수로 포즈를 만든다 → 여기서는 FD03AnimProxy::Evaluate (C++ 로 쓴 Anim Graph)
 * 에디터 그래프는 코드로 만들 수 없으므로, AnimBP 가 하는 일을 같은 엔진 API 로 C++ 에서 그대로 한다:
 *   Blend Space 샘플링 → (공중) Fall Loop 블렌드 → Slot(몽타주) → Layered Blend per Bone
 *   → Aim Offset / Look At / Foot IK(Two Bone IK) → 다리 비율(리타깃 비교용)
 * 데모마다 필요한 기능만 켠다.
 */
UCLASS(Transient, Blueprintable)
class GLOSSARYANIMATION_API UD03_AnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	// ------------------------------------------------ Event Graph 결과 (매 프레임 계산)

	/** 수평 속도 cm/s — AnimBP 의 'Speed' 변수 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Event Graph")
	float Speed = 0.f;

	/** 이동 방향(-180~180) — 캐릭터 정면 기준 속도 방향 (CalculateDirection) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Event Graph")
	float Direction = 0.f;

	/** 공중인지 — CharacterMovement->IsFalling() */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Event Graph")
	bool bIsFalling = false;

	/** false 면 Speed·Direction·bIsFalling 을 데모 코드가 직접 넣는다(폰 없이 제자리 재생할 때) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Graph")
	bool bAutoLocomotion = true;

	// ------------------------------------------------ Anim Graph 재료

	/** 이동 Blend Space. 1D 면 X=Speed, 2D 면 X=Direction · Y=Speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Graph")
	TObjectPtr<UBlendSpace> LocomotionBlendSpace;

	/** 공중일 때 섞는 루프 (MM_Fall_Loop) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Graph")
	TObjectPtr<UAnimSequence> FallLoop;

	/** Blend Space 대신 시퀀스 하나를 직접 재생 (모션 매칭 대체 구현이 프레임을 고른다) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Graph|Pose Override")
	bool bPoseOverride = false;

	/** 지금 포즈(A)와 직전 포즈(B), A 의 블렌드 비율 — 게임 스레드가 시간을 관리한다 */
	UPROPERTY(Transient) TObjectPtr<UAnimSequence> PoseSeqA;
	UPROPERTY(Transient) TObjectPtr<UAnimSequence> PoseSeqB;
	float PoseTimeA = 0.f;
	float PoseTimeB = 0.f;
	float PoseAlphaA = 1.f;

	/** 몽타주를 받는 Slot 이름. AnimBP 의 Slot 노드와 같다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Graph|Slot")
	FName SlotName = TEXT("DefaultSlot");

	/** Layered Blend per Bone: LayerBone 아래(상체)만 슬롯 포즈를 쓰고 하체는 이동 포즈 유지 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Graph|Layered Blend")
	bool bLayeredBlend = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Graph|Layered Blend")
	FName LayerBone = TEXT("spine_01");

	/** Aim Offset: Pitch·Yaw 를 척추 5개에 나눠 돌린다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Graph|Aim Offset")
	bool bAimOffset = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Graph|Aim Offset", meta = (ClampMin = -90, ClampMax = 90))
	float AimPitch = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Graph|Aim Offset", meta = (ClampMin = -90, ClampMax = 90))
	float AimYaw = 0.f;

	/** Look At: 머리(목 40% + 머리 60%)가 LookAtTarget(월드)을 본다. LookAtClamp 도 이상은 돌지 않는다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Graph|Look At")
	bool bLookAt = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Graph|Look At")
	FVector LookAtTarget = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Graph|Look At", meta = (ClampMin = 0, ClampMax = 180))
	float LookAtClamp = 60.f;

	/** 목표까지 각도와 실제로 돈 각도 (상태 표시용) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim Graph|Look At")
	float LookAtDesiredAngle = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim Graph|Look At")
	float LookAtAppliedAngle = 0.f;

	/** Foot IK: 발밑 Line Trace → 골반 내리기 + Two Bone IK 로 발을 지면에 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Graph|Foot IK")
	bool bFootIK = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Graph|Foot IK")
	float FootTraceUp = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Graph|Foot IK")
	float FootTraceDown = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Graph|Foot IK")
	float FootInterpSpeed = 15.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim Graph|Foot IK")
	float FootOffsetL = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim Graph|Foot IK")
	float FootOffsetR = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim Graph|Foot IK")
	float PelvisOffset = 0.f;

	/** 리타깃 비교용: 허벅지·종아리 길이 배율 (다른 체형 캐릭터 흉내) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Graph|Proportions", meta = (ClampMin = 0.5, ClampMax = 2))
	float LegScale = 1.f;

	/** true 면 골반 높이를 다리 길이 비율만큼 올린다 (IK Retargeter 의 Retarget Root 가 하는 일) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Graph|Proportions")
	bool bScalePelvisHeight = true;

	// ------------------------------------------------ 프록시가 쓰는 계산 결과 (컴포넌트 공간)
	FQuat AimDeltaCS = FQuat::Identity;
	FQuat LookDeltaCS = FQuat::Identity;
	FVector FootNormalL_CS = FVector::UpVector;
	FVector FootNormalR_CS = FVector::UpVector;
	FVector ForwardCS = FVector(0, 1, 0);
	FVector UpCS = FVector::UpVector;

	/** 사용할 Blend Space 입력 (1D/2D 자동) */
	FVector GetBlendSpaceInput() const;

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

private:
	void UpdateFootIK(float DeltaSeconds);
	void UpdateLookAt(float DeltaSeconds);
	float TraceFoot(FName Bone, FVector& OutNormalWS) const;
};
