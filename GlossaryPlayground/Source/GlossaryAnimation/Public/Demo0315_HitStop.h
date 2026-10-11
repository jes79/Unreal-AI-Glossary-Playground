#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0315_HitStop.generated.h"

class AD03_Mannequin;
class UAnimMontage;
class UAnimSequenceBase;
class USkeletalMeshComponent;
class USoundBase;
class AStaticMeshActor;

/**
 * 0315 히트 스톱 (Time Dilation)
 * 타격 Notify 순간 공격자와 피격자의 Custom Time Dilation 을 0.05 로 0.08초(실시간) 낮췄다가 되돌린다.
 * 월드 전체가 아니라 두 액터만 멈추므로 다른 물체(회전하는 기준 큐브)는 계속 움직인다.
 */
UCLASS()
class GLOSSARYANIMATION_API ADemo0315_HitStop : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0315_HitStop();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

	/** 히트 스톱 동안의 Custom Time Dilation */
	UPROPERTY(EditAnywhere, Category = "Hit Stop", meta = (ClampMin = 0, ClampMax = 1))
	float HitStopDilation = 0.05f;

	/** 히트 스톱 길이 (실시간 초) */
	UPROPERTY(EditAnywhere, Category = "Hit Stop", meta = (ClampMin = 0))
	float HitStopDuration = 0.08f;

	UPROPERTY(EditAnywhere, Category = "Hit Stop")
	bool bHitStopEnabled = true;

private:
	void OnAnimHit(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation);
	void SetDilation(float Value);

	UPROPERTY(Transient) TObjectPtr<UAnimMontage> Combo;
	UPROPERTY(Transient) TObjectPtr<AD03_Mannequin> Attacker;
	UPROPERTY(Transient) TObjectPtr<AD03_Mannequin> Target;
	UPROPERTY(Transient) TObjectPtr<USoundBase> HitSound;
	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> HitReact;
	UPROPERTY(Transient) TObjectPtr<AStaticMeshActor> Spinner;
	UPROPERTY(Transient) TObjectPtr<AStaticMeshActor> Flash;

	FDelegateHandle HitHandle;
	double StopEndRealTime = 0.0;
	double StopStartRealTime = 0.0;
	bool bStopping = false;
	float LastStopLength = 0.f;
	int32 StopCount = 0;
	float AttackTimer = 0.f;
	int32 Presses = 0;
	float DurationScale = 1.f;
};
