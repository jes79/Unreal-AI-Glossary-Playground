#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GlossaryDemoBase.h"
#include "Demo0516_FootstepDust.generated.h"

class UAnimSequence;
class UNiagaraSystem;
class UPhysicalMaterial;
class USkeletalMeshComponent;

/**
 * 발이 닿는 순간 호출되는 Anim Notify: 발 아래로 트레이스해 지면의 Physical Material 을 읽고,
 * 그 재질에 맞는 색으로 먼지 Niagara 를 스폰한다.
 */
UCLASS(meta = (DisplayName = "Footstep Dust"))
class GLOSSARYEFFECTS_API UAnimNotify_FootstepDust : public UAnimNotify
{
	GENERATED_BODY()
public:
	/** 어느 발인가 (foot_l / foot_r) */
	UPROPERTY(EditAnywhere, Category = Footstep) FName FootBone = TEXT("foot_l");
	/** 스폰할 먼지 시스템 (User.Color 를 노출) */
	UPROPERTY(EditAnywhere, Category = Footstep) TObjectPtr<UNiagaraSystem> DustSystem;
	/** 지면 Physical Material → 먼지 색 */
	UPROPERTY(EditAnywhere, Category = Footstep) TMap<TObjectPtr<UPhysicalMaterial>, FLinearColor> SurfaceColors;
	UPROPERTY(EditAnywhere, Category = Footstep) FLinearColor DefaultColor = FLinearColor(0.5f, 0.45f, 0.4f);

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override { return TEXT("FootstepDust ") + FootBone.ToString(); }
};

/**
 * 0516 발자국 먼지 — Jog 애니메이션의 발 착지 시각에 Anim Notify 를 넣고, Notify 에서 먼지 Niagara 를 스폰.
 * 흙(갈색)·눈(흰색) 바닥은 서로 다른 Physical Material 을 가져 먼지 색이 바뀐다.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0516_FootstepDust : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0516_FootstepDust();

	/** Notify 가 불러 준다 (상태 줄 표시용) */
	void ReportFootstep(FName Foot, const FString& Surface);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Footstep") float RunSpeed = 380.f;
	UPROPERTY(EditAnywhere, Category = "Footstep") float PathRadius = 450.f;

private:
	void InjectFootstepNotifies();
	static TArray<float> FindFootContactTimes(UAnimSequence* Sequence, FName FootBone);

	UPROPERTY(Transient) TObjectPtr<USceneComponent> RunnerRoot;
	UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> Runner;
	UPROPERTY(Transient) TObjectPtr<UAnimSequence> JogSequence;
	UPROPERTY(Transient) TObjectPtr<UPhysicalMaterial> DirtMaterial;
	UPROPERTY(Transient) TObjectPtr<UPhysicalMaterial> SnowMaterial;
	UPROPERTY(Transient) TArray<TObjectPtr<UAnimNotify_FootstepDust>> AddedNotifies;
	float Angle = 0.f;
	int32 Steps = 0;
	FString LastStep;
	FString NotifyTimes;
};
