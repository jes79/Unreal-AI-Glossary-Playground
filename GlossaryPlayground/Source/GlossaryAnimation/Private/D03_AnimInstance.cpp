#include "D03_AnimInstance.h"

#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/BlendSpace.h"
#include "Animation/BoneReference.h"
#include "AnimationRuntime.h"
#include "BonePose.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "KismetAnimationLibrary.h"
#include "TwoBoneIK.h"

namespace
{
	const TCHAR* DefaultLocomotionBS = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/BS_Idle_Walk_Run.BS_Idle_Walk_Run");
	const TCHAR* DefaultFallLoop = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Fall_Loop.MM_Fall_Loop");
}

/**
 * 'C++ 로 쓴 Anim Graph'. AnimBP 가 컴파일되면 노드 그래프가 이 프록시 안에서 워커 스레드로 평가되는데,
 * 여기서는 같은 단계를 Evaluate() 한 함수에 순서대로 적었다.
 */
struct FD03AnimProxy : public FAnimInstanceProxy
{
	FD03AnimProxy() = default;
	explicit FD03AnimProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

	// ---- 게임 스레드에서 복사해 온 값 (PreUpdate) ----
	TObjectPtr<UBlendSpace> BlendSpace;
	TObjectPtr<UAnimSequence> Fall;
	TObjectPtr<UAnimSequence> SeqA;
	TObjectPtr<UAnimSequence> SeqB;
	float TimeA = 0.f, TimeB = 0.f, AlphaA = 1.f;
	bool bPoseOverride = false;
	FVector BSInput = FVector::ZeroVector;
	bool bFalling = false;
	FName Slot;
	bool bLayered = false;
	FName LayerBoneName;
	bool bAim = false;
	FQuat AimDelta = FQuat::Identity;
	bool bLook = false;
	FQuat LookDelta = FQuat::Identity;
	bool bFootIK = false;
	float FootL = 0.f, FootR = 0.f, Pelvis = 0.f;
	FVector NormalL = FVector::UpVector, NormalR = FVector::UpVector;
	FVector Fwd = FVector(0, 1, 0), Up = FVector::UpVector;
	float LegScale = 1.f;
	bool bScalePelvis = true;

	// ---- 프록시가 직접 관리하는 재생 상태 ----
	TArray<FBlendSampleData> SampleCache;
	int32 TriangulationIndex = 0;
	float Phase = 0.f;          // Blend Space 샘플들의 공통 정규화 시간(0~1) — 발 주기를 맞춘다(Sync Group 과 같은 효과)
	float FallTime = 0.f;
	float FallAlpha = 0.f;
	float SlotNodeWeight = 0.f, SlotSourceWeight = 1.f, SlotTotalWeight = 0.f;
	FGraphTraversalCounter SlotCounter;
	FName RegisteredSlot;

	virtual void Initialize(UAnimInstance* InAnimInstance) override
	{
		FAnimInstanceProxy::Initialize(InAnimInstance);
		RegisteredSlot = NAME_None;
	}

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override
	{
		FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
		const UD03_AnimInstance* I = CastChecked<UD03_AnimInstance>(InAnimInstance);
		if (BlendSpace != I->LocomotionBlendSpace)
		{
			SampleCache.Reset();
			TriangulationIndex = 0;
		}
		BlendSpace = I->LocomotionBlendSpace;
		Fall = I->FallLoop;
		bPoseOverride = I->bPoseOverride;
		SeqA = I->PoseSeqA; SeqB = I->PoseSeqB;
		TimeA = I->PoseTimeA; TimeB = I->PoseTimeB; AlphaA = I->PoseAlphaA;
		BSInput = I->GetBlendSpaceInput();
		bFalling = I->bIsFalling;
		Slot = I->SlotName;
		bLayered = I->bLayeredBlend;
		LayerBoneName = I->LayerBone;
		bAim = I->bAimOffset;
		AimDelta = I->AimDeltaCS;
		bLook = I->bLookAt;
		LookDelta = I->LookDeltaCS;
		bFootIK = I->bFootIK;
		FootL = I->FootOffsetL; FootR = I->FootOffsetR; Pelvis = I->PelvisOffset;
		NormalL = I->FootNormalL_CS; NormalR = I->FootNormalR_CS;
		Fwd = I->ForwardCS; Up = I->UpCS;
		LegScale = I->LegScale;
		bScalePelvis = I->bScalePelvisHeight;
	}

	/** 워커 스레드: 재생 시간 진행 + Slot 가중치 갱신 (AnimGraph 의 Update 단계) */
	virtual void Update(float DeltaSeconds) override
	{
		// Slot 노드가 하는 등록·가중치 갱신. 이걸 해야 몽타주의 Notify·블렌드가 '이 슬롯이 살아 있다'고 판단한다
		if (Slot != RegisteredSlot || !SlotCounter.IsSynchronized_Counter(GetSlotNodeInitializationCounter()))
		{
			SlotCounter.SynchronizeWith(GetSlotNodeInitializationCounter());
			RegisterSlotNodeWithAnimInstance(Slot);
			RegisteredSlot = Slot;
		}
		GetSlotWeight(Slot, SlotNodeWeight, SlotSourceWeight, SlotTotalWeight);
		UpdateSlotNodeWeight(Slot, SlotNodeWeight, 1.f);

		// Blend Space: 입력 위치로 샘플 가중치를 구하고, 가중 평균 길이로 공통 위상을 진행
		if (BlendSpace && !bPoseOverride)
		{
			BlendSpace->UpdateBlendSamples(BSInput, DeltaSeconds, SampleCache, TriangulationIndex);
			float Length = 0.f, TotalWeight = 0.f;
			for (const FBlendSampleData& S : SampleCache)
			{
				const UAnimSequence* Anim = BlendSpace->GetBlendSample(S.SampleDataIndex).Animation;
				if (Anim)
				{
					Length += S.GetClampedWeight() * Anim->GetPlayLength();
					TotalWeight += S.GetClampedWeight();
				}
			}
			if (Length > KINDA_SMALL_NUMBER && TotalWeight > KINDA_SMALL_NUMBER)
			{
				Phase = FMath::Fmod(Phase + DeltaSeconds / (Length / TotalWeight), 1.f);
			}
			for (FBlendSampleData& S : SampleCache)
			{
				if (const UAnimSequence* Anim = BlendSpace->GetBlendSample(S.SampleDataIndex).Animation)
				{
					S.PreviousTime = S.Time;
					S.Time = Phase * Anim->GetPlayLength();
				}
			}
		}

		// 공중 루프는 0.15초에 걸쳐 섞어 넣고 뺀다
		FallAlpha = FMath::FInterpConstantTo(FallAlpha, bFalling ? 1.f : 0.f, DeltaSeconds, 1.f / 0.15f);
		if (Fall)
		{
			FallTime = FMath::Fmod(FallTime + DeltaSeconds, FMath::Max(Fall->GetPlayLength(), 0.01f));
		}
	}

	static void EvalSequence(const UAnimSequence* Seq, float Time, FPoseContext& Out)
	{
		FAnimationPoseData PoseData(Out);
		Seq->GetAnimationPose(PoseData, FAnimExtractContext(static_cast<double>(Time), false));
	}

	void EvalLocomotion(FPoseContext& Out)
	{
		if (bPoseOverride && SeqA)
		{
			EvalSequence(SeqA, TimeA, Out);
			if (SeqB && AlphaA < 0.999f)
			{
				// 모션 매칭이 프레임을 바꾸면 직전 포즈(B)에서 새 포즈(A)로 짧게 크로스페이드
				FPoseContext B(Out);
				EvalSequence(SeqB, TimeB, B);
				FPoseContext Blended(Out);
				FAnimationPoseData OutData(Blended);
				FAnimationRuntime::BlendTwoPosesTogether(FAnimationPoseData(Out), FAnimationPoseData(B), AlphaA, OutData);
				Out = Blended;
			}
			return;
		}
		if (BlendSpace && SampleCache.Num() > 0)
		{
			FAnimationPoseData PoseData(Out);
			BlendSpace->GetAnimationPose(SampleCache, FAnimExtractContext(), PoseData);
			return;
		}
		Out.ResetToRefPose();
	}

	virtual bool Evaluate(FPoseContext& Output) override
	{
		// 1) 이동 포즈 (Blend Space)
		EvalLocomotion(Output);

		// 2) 공중이면 Fall Loop 를 섞는다 (State Machine 의 Jump/Fall 상태를 아주 단순하게)
		if (Fall && FallAlpha > 0.001f)
		{
			FPoseContext FallPose(Output);
			EvalSequence(Fall, FallTime, FallPose);
			FPoseContext Blended(Output);
			FAnimationPoseData OutData(Blended);
			FAnimationRuntime::BlendTwoPosesTogether(FAnimationPoseData(Output), FAnimationPoseData(FallPose), 1.f - FallAlpha, OutData);
			Output = Blended;
		}

		// 3) Slot: 몽타주가 재생 중이면 그 포즈로 덮는다 (AnimBP 의 Slot 'DefaultSlot' 노드)
		if (SlotNodeWeight > ZERO_ANIMWEIGHT_THRESH)
		{
			FPoseContext SlotPose(Output);
			FAnimationPoseData SlotData(SlotPose);
			SlotEvaluatePose(Slot, FAnimationPoseData(Output), SlotSourceWeight, SlotData, SlotNodeWeight, SlotTotalWeight);

			if (bLayered)
			{
				// 4) Layered Blend per Bone: LayerBone(spine_01)과 그 자식만 슬롯 포즈, 나머지(골반·다리)는 이동 포즈
				const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
				FBoneReference LayerRef(LayerBoneName);
				LayerRef.Initialize(Bones);
				const FCompactPoseBoneIndex LayerIndex = LayerRef.GetCompactPoseIndex(Bones);
				TArray<bool, TInlineAllocator<256>> bUpper;
				bUpper.SetNumZeroed(Output.Pose.GetNumBones());
				for (FCompactPoseBoneIndex B : Output.Pose.ForEachBoneIndex())
				{
					const FCompactPoseBoneIndex Parent = Bones.GetParentBoneIndex(B);
					bUpper[B.GetInt()] = (B == LayerIndex) || (Parent.GetInt() >= 0 && bUpper[Parent.GetInt()]);
					if (bUpper[B.GetInt()])
					{
						Output.Pose[B] = SlotPose.Pose[B];
					}
				}
			}
			else
			{
				Output = SlotPose;
			}
		}

		// 5) 다리 비율 (리타깃 비교): 허벅지·종아리 뼈 길이를 늘이고, 필요하면 골반 높이도 같은 비율로
		if (!FMath::IsNearlyEqual(LegScale, 1.f))
		{
			ApplyLegScale(Output);
		}

		// 6) 컴포넌트 공간 후처리: Aim Offset → Look At → Foot IK
		if (bAim || bLook || bFootIK)
		{
			FCSPose<FCompactPose> CS;
			CS.InitPose(Output.Pose);
			if (bAim)
			{
				static const FName AimChain[] = { TEXT("spine_01"), TEXT("spine_02"), TEXT("spine_03"), TEXT("spine_04"), TEXT("spine_05") };
				static const float AimWeights[] = { 0.15f, 0.15f, 0.2f, 0.25f, 0.25f };
				ApplyChainRotation(CS, AimChain, AimWeights, 5, AimDelta);
			}
			if (bLook)
			{
				static const FName LookChain[] = { TEXT("neck_01"), TEXT("head") };
				static const float LookWeights[] = { 0.4f, 0.6f };
				ApplyChainRotation(CS, LookChain, LookWeights, 2, LookDelta);
			}
			if (bFootIK)
			{
				ApplyFootIK(CS);
			}
			FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(CS), Output.Pose);
		}
		return true;
	}

	FCompactPoseBoneIndex FindBone(const FBoneContainer& Bones, FName Name) const
	{
		FBoneReference Ref(Name);
		Ref.Initialize(Bones);
		return Ref.GetCompactPoseIndex(Bones);
	}

	/** Delta 회전을 뼈 사슬에 가중치만큼 나눠 적용한다(부모부터). 컴포넌트 공간 회전 앞에 곱한다 */
	void ApplyChainRotation(FCSPose<FCompactPose>& CS, const FName* Chain, const float* Weights, int32 Num, const FQuat& Delta)
	{
		const FBoneContainer& Bones = CS.GetPose().GetBoneContainer();
		float Accum = 0.f;
		FQuat Applied = FQuat::Identity;
		for (int32 i = 0; i < Num; ++i)
		{
			const FCompactPoseBoneIndex Index = FindBone(Bones, Chain[i]);
			if (Index.GetInt() == INDEX_NONE)
			{
				continue;
			}
			Accum += Weights[i];
			// 지금까지 Accum 만큼 돌아가야 하므로, 이번 뼈는 (목표 누적 회전) * (이미 적용된 회전)^-1 만 더 돈다
			const FQuat Target = FQuat::Slerp(FQuat::Identity, Delta, FMath::Min(Accum, 1.f));
			const FQuat Step = Target * Applied.Inverse();
			Applied = Target;
			FTransform T = CS.GetComponentSpaceTransform(Index);
			T.SetRotation((Step * T.GetRotation()).GetNormalized());
			const FBoneTransform BT(Index, T);
			CS.SafeSetCSBoneTransforms(MakeArrayView(&BT, 1));
		}
	}

	void ApplyFootIK(FCSPose<FCompactPose>& CS)
	{
		const FBoneContainer& Bones = CS.GetPose().GetBoneContainer();
		const FCompactPoseBoneIndex PelvisI = FindBone(Bones, TEXT("pelvis"));
		struct FLeg { FName Thigh, Calf, Foot; float Offset; FVector Normal; };
		const FLeg Legs[2] = {
			{ TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l"), FootL, NormalL },
			{ TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r"), FootR, NormalR },
		};
		// 골반을 내리기 전에 애니메이션의 발 위치를 기억해 둔다
		FVector AnimFoot[2];
		for (int32 L = 0; L < 2; ++L)
		{
			const FCompactPoseBoneIndex F = FindBone(Bones, Legs[L].Foot);
			AnimFoot[L] = F.GetInt() != INDEX_NONE ? CS.GetComponentSpaceTransform(F).GetLocation() : FVector::ZeroVector;
		}
		// (1) 골반을 더 낮은 발 쪽으로 내린다 — 안 그러면 낮은 계단의 발이 닿지 않는다
		if (PelvisI.GetInt() != INDEX_NONE)
		{
			FTransform T = CS.GetComponentSpaceTransform(PelvisI);
			T.AddToTranslation(Up * Pelvis);
			const FBoneTransform BT(PelvisI, T);
			CS.SafeSetCSBoneTransforms(MakeArrayView(&BT, 1));
		}
		// (2) 다리마다 Two Bone IK: 허벅지-종아리-발이 목표(지면 높이) 에 닿도록 무릎을 굽힌다
		for (int32 L = 0; L < 2; ++L)
		{
			const FCompactPoseBoneIndex ThighI = FindBone(Bones, Legs[L].Thigh);
			const FCompactPoseBoneIndex CalfI = FindBone(Bones, Legs[L].Calf);
			const FCompactPoseBoneIndex FootI = FindBone(Bones, Legs[L].Foot);
			if (ThighI.GetInt() == INDEX_NONE || CalfI.GetInt() == INDEX_NONE || FootI.GetInt() == INDEX_NONE)
			{
				continue;
			}
			FTransform RootT = CS.GetComponentSpaceTransform(ThighI);
			FTransform JointT = CS.GetComponentSpaceTransform(CalfI);
			FTransform EndT = CS.GetComponentSpaceTransform(FootI);
			const FVector Effector = AnimFoot[L] + Up * Legs[L].Offset;
			const FVector JointTarget = JointT.GetLocation() + Fwd * 60.f;   // 무릎은 앞으로 굽힌다
			AnimationCore::SolveTwoBoneIK(RootT, JointT, EndT, JointTarget, Effector, false, 1.0, 1.0);
			// 발바닥을 지면 경사에 맞춰 기울인다
			EndT.SetRotation((FQuat::FindBetweenNormals(Up, Legs[L].Normal) * EndT.GetRotation()).GetNormalized());
			const FBoneTransform BT[3] = { FBoneTransform(ThighI, RootT), FBoneTransform(CalfI, JointT), FBoneTransform(FootI, EndT) };
			CS.SafeSetCSBoneTransforms(MakeArrayView(BT, 3));
		}
	}

	void ApplyLegScale(FPoseContext& Output)
	{
		const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
		static const FName Scaled[] = { TEXT("calf_l"), TEXT("calf_r"), TEXT("foot_l"), TEXT("foot_r") };
		// 다리 길이(허벅지→종아리 + 종아리→발)를 기준 포즈로 잰다
		float RefLeg = 0.f;
		for (int32 i = 0; i < 4; ++i)
		{
			const FCompactPoseBoneIndex B = FindBone(Bones, Scaled[i]);
			if (B.GetInt() != INDEX_NONE)
			{
				if (i == 0 || i == 2) { RefLeg += Bones.GetRefPoseTransform(B).GetTranslation().Size(); }
				Output.Pose[B].SetTranslation(Output.Pose[B].GetTranslation() * LegScale);
			}
		}
		if (bScalePelvis)
		{
			// Retarget Root: 골반 이동량을 '새 다리 길이 / 원래 다리 길이' 비율로 키운다 → 발이 바닥에 맞는다
			const FCompactPoseBoneIndex PelvisI = FindBone(Bones, TEXT("pelvis"));
			if (PelvisI.GetInt() != INDEX_NONE)
			{
				const float PelvisHeight = FMath::Max(1.f, (float)Bones.GetRefPoseTransform(PelvisI).GetTranslation().Size());
				const float Ratio = (PelvisHeight + RefLeg * (LegScale - 1.f)) / PelvisHeight;
				Output.Pose[PelvisI].SetTranslation(Output.Pose[PelvisI].GetTranslation() * Ratio);
			}
		}
	}
};

// ---------------------------------------------------------------- UD03_AnimInstance

FVector UD03_AnimInstance::GetBlendSpaceInput() const
{
	// 2D Blend Space(X=Direction, Y=Speed) 인지 1D(X=Speed) 인지 축 범위로 구분
	if (LocomotionBlendSpace && LocomotionBlendSpace->GetBlendParameter(1).Max > LocomotionBlendSpace->GetBlendParameter(1).Min + 1.f
		&& LocomotionBlendSpace->GetBlendParameter(0).Min < 0.f)
	{
		return FVector(Direction, Speed, 0.f);
	}
	return FVector(Speed, 0.f, 0.f);
}

void UD03_AnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	if (!LocomotionBlendSpace)
	{
		LocomotionBlendSpace = LoadObject<UBlendSpace>(nullptr, DefaultLocomotionBS);
	}
	if (!FallLoop)
	{
		FallLoop = LoadObject<UAnimSequence>(nullptr, DefaultFallLoop);
	}
}

void UD03_AnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// ---- AnimBP 'Event Blueprint Update Animation' 과 같은 계산 ----
	if (bAutoLocomotion)
	{
		if (const ACharacter* Character = Cast<ACharacter>(TryGetPawnOwner()))
		{
			const FVector Velocity = Character->GetVelocity();
			Speed = Velocity.Size2D();                                                          // Speed
			Direction = UKismetAnimationLibrary::CalculateDirection(Velocity, Character->GetActorRotation()); // Direction
			bIsFalling = Character->GetCharacterMovement() && Character->GetCharacterMovement()->IsFalling(); // IsFalling
		}
	}

	// 컴포넌트 공간 기준 축: 메시가 액터 기준 -90° 돌아 있으므로 액터 정면(+X)은 컴포넌트 +Y
	if (const USkeletalMeshComponent* Mesh = GetSkelMeshComponent())
	{
		const FQuat CompToWorld = Mesh->GetComponentQuat();
		const AActor* Owner = Mesh->GetOwner();
		const FVector ActorFwd = Owner ? Owner->GetActorForwardVector() : FVector::ForwardVector;
		ForwardCS = CompToWorld.UnrotateVector(ActorFwd).GetSafeNormal();
		UpCS = CompToWorld.UnrotateVector(FVector::UpVector).GetSafeNormal();
	}

	// Aim Offset: Yaw(위축) 와 Pitch(오른쪽 축) 를 컴포넌트 공간 회전 하나로
	if (bAimOffset)
	{
		const FVector PitchAxis = FVector::CrossProduct(ForwardCS, UpCS).GetSafeNormal();
		const FQuat Yaw(UpCS, FMath::DegreesToRadians(FMath::Clamp(AimYaw, -90.f, 90.f)));
		const FQuat Pitch(PitchAxis, FMath::DegreesToRadians(FMath::Clamp(AimPitch, -90.f, 90.f)));
		AimDeltaCS = Yaw * Pitch;
	}
	if (bLookAt)
	{
		UpdateLookAt(DeltaSeconds);
	}
	if (bFootIK)
	{
		UpdateFootIK(DeltaSeconds);
	}
	else
	{
		FootOffsetL = FootOffsetR = PelvisOffset = 0.f;
	}
}

void UD03_AnimInstance::UpdateLookAt(float DeltaSeconds)
{
	const USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	if (!Mesh)
	{
		return;
	}
	// 목표 방향 (컴포넌트 공간). 기준은 머리 위치, 정면은 캐릭터 정면
	const FTransform CompT = Mesh->GetComponentTransform();
	const FVector HeadCS = CompT.InverseTransformPosition(Mesh->GetSocketLocation(TEXT("neck_01")));
	const FVector TargetCS = CompT.InverseTransformPosition(LookAtTarget);
	const FVector Dir = (TargetCS - HeadCS).GetSafeNormal();
	LookAtDesiredAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(ForwardCS, Dir), -1.f, 1.f)));

	// 회전 한계: LookAtClamp 도를 넘으면 그 각도까지만 (Control Rig Aim 노드의 Limit 과 같은 역할)
	FQuat Desired = FQuat::FindBetweenNormals(ForwardCS, Dir);
	if (LookAtDesiredAngle > LookAtClamp && LookAtDesiredAngle > KINDA_SMALL_NUMBER)
	{
		Desired = FQuat::Slerp(FQuat::Identity, Desired, LookAtClamp / LookAtDesiredAngle);
	}
	// 부드럽게 따라가기 (초당 약 8 배 속도로 수렴)
	LookDeltaCS = FQuat::Slerp(LookDeltaCS, Desired, FMath::Clamp(DeltaSeconds * 8.f, 0.f, 1.f)).GetNormalized();
	LookAtAppliedAngle = FMath::RadiansToDegrees(LookDeltaCS.GetAngle());
}

float UD03_AnimInstance::TraceFoot(FName Bone, FVector& OutNormalWS) const
{
	const USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	OutNormalWS = FVector::UpVector;
	if (!Mesh || !GetWorld())
	{
		return 0.f;
	}
	// 발 XY 에서 캡슐 바닥 높이 기준 위아래로 Line Trace
	const FVector Foot = Mesh->GetSocketLocation(Bone);
	const float BaseZ = Mesh->GetComponentLocation().Z;
	const FVector Start(Foot.X, Foot.Y, BaseZ + FootTraceUp);
	const FVector End(Foot.X, Foot.Y, BaseZ - FootTraceDown);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(D03FootIK), false, Mesh->GetOwner());
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		OutNormalWS = Hit.ImpactNormal;
		return Hit.ImpactPoint.Z - BaseZ;   // +면 발밑이 캡슐 바닥보다 높고, -면 낮다
	}
	return 0.f;
}

void UD03_AnimInstance::UpdateFootIK(float DeltaSeconds)
{
	const USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	if (!Mesh)
	{
		return;
	}
	FVector NL, NR;
	const float TargetL = TraceFoot(TEXT("foot_l"), NL);
	const float TargetR = TraceFoot(TEXT("foot_r"), NR);
	// 골반은 '더 낮은 발' 만큼 내린다 (올리지는 않는다)
	const float TargetPelvis = FMath::Min3(TargetL, TargetR, 0.f);
	FootOffsetL = FMath::FInterpTo(FootOffsetL, TargetL, DeltaSeconds, FootInterpSpeed);
	FootOffsetR = FMath::FInterpTo(FootOffsetR, TargetR, DeltaSeconds, FootInterpSpeed);
	PelvisOffset = FMath::FInterpTo(PelvisOffset, TargetPelvis, DeltaSeconds, FootInterpSpeed);
	const FQuat CompToWorld = Mesh->GetComponentQuat();
	FootNormalL_CS = CompToWorld.UnrotateVector(NL);
	FootNormalR_CS = CompToWorld.UnrotateVector(NR);
}

FAnimInstanceProxy* UD03_AnimInstance::CreateAnimInstanceProxy()
{
	return new FD03AnimProxy(this);
}

void UD03_AnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete static_cast<FD03AnimProxy*>(InProxy);
}
