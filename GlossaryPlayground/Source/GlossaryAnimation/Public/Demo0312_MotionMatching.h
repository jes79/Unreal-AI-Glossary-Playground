#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0312_MotionMatching.generated.h"

class AD03_Mannequin;
class UD03_AnimInstance;
class UD03_GraphWidget;
class UAnimSequence;
class UBlendSpace;

/**
 * 0312 Motion Matching (대체 구현)
 * Idle + 8방향 걷기·조깅 17개 클립을 0.066초 간격으로 잘라 '포즈 데이터베이스'를 만들고,
 * 0.1초마다 '원하는 속도 + 지금 발 위치·발 속도' 와 가장 비슷한 프레임을 찾아 그 프레임으로 짧게 블렌드해 넘어간다.
 */
UCLASS()
class GLOSSARYANIMATION_API ADemo0312_MotionMatching : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0312_MotionMatching();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 검색 간격(초) — Pose Search 의 Search Throttle Time */
	UPROPERTY(EditAnywhere, Category = "Motion Matching")
	float SearchInterval = 0.1f;

	/** 다른 프레임으로 넘어갈 때 블렌드 시간 */
	UPROPERTY(EditAnywhere, Category = "Motion Matching")
	float BlendTime = 0.25f;

	/** 비용 가중치: 속도 / 발 위치 / 발 속도 */
	UPROPERTY(EditAnywhere, Category = "Motion Matching")
	float VelocityWeight = 2.f;

	UPROPERTY(EditAnywhere, Category = "Motion Matching")
	float PoseWeight = 0.3f;

	/** 지금 클립을 계속 재생하면 비용을 이만큼 깎아 준다 (잦은 전환 방지) */
	UPROPERTY(EditAnywhere, Category = "Motion Matching")
	float ContinuingBias = 0.15f;

private:
	struct FFrame
	{
		int32 Clip;
		float Time;
		FVector2D Velocity;          // 캐릭터 기준 (x=앞, y=오른쪽) cm/s
		FVector FootL, FootR;        // 컴포넌트 공간 발 위치
		FVector FootVelL, FootVelR;  // 발 속도
	};
	struct FClip { TObjectPtr<UAnimSequence> Seq; FString Name; FVector2D Velocity; };

	void BuildDatabase();
	FVector BoneCS(const UAnimSequence* Seq, int32 Bone, float Time) const;
	int32 FindFrame(int32 Clip, float Time) const;
	void Search(const FVector2D& DesiredVelocity);
	void PickWaypoint();

	TArray<FClip> Clips;
	TArray<FFrame> Database;

	UPROPERTY(Transient) TObjectPtr<AD03_Mannequin> Npc;
	UPROPERTY(Transient) TObjectPtr<UD03_AnimInstance> NpcAnim;
	UPROPERTY(Transient) TObjectPtr<UD03_GraphWidget> Graph;
	UPROPERTY(Transient) TObjectPtr<UBlendSpace> CompareBlendSpace;
	/** 데이터베이스 클립 (GC 방지용 참조) */
	UPROPERTY(Transient) TArray<TObjectPtr<UAnimSequence>> ClipAssets;

	int32 CurClip = 0;
	float CurTime = 0.f;
	int32 PrevClip = INDEX_NONE;
	float PrevTime = 0.f;
	float BlendAlpha = 1.f;
	float SearchTimer = 0.f;
	float LastCost = 0.f;
	int32 Switches = 0;
	FString TopCandidates;
	FVector Waypoint = FVector::ZeroVector;
	float WaypointSpeed = 200.f;
	float FacingYaw = 180.f;
	float Time = 0.f;
	bool bMotionMatching = true;
	FVector2D LastQuery = FVector2D::ZeroVector;
};
