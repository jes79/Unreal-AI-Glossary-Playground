#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0305_AnimNotify.generated.h"

class AD03_Mannequin;
class UAnimMontage;
class UD03_GraphWidget;
class USoundBase;
class USkeletalMeshComponent;
class UAnimSequenceBase;
class AStaticMeshActor;

/**
 * 0305 Anim Notify
 * 공격 몽타주의 타격 프레임에 들어 있는 C++ AnimNotify(UD0305_AnimNotify_Hit)가 불리는 순간
 * 데미지 판정(Sphere Overlap)·효과음·피격 리액션을 실행한다. 비교용으로 '재생 시작 즉시 판정' 모드.
 */
UCLASS()
class GLOSSARYANIMATION_API ADemo0305_AnimNotify : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0305_AnimNotify();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

	/** 타격 판정 구의 반지름 / 공격자 앞 거리 */
	UPROPERTY(EditAnywhere, Category = "Anim Notify")
	float HitRadius = 70.f;

	UPROPERTY(EditAnywhere, Category = "Anim Notify")
	float HitForward = 90.f;

	UPROPERTY(EditAnywhere, Category = "Anim Notify")
	float Damage = 10.f;

	/** false 면 Notify 를 무시하고 공격 시작 즉시 판정 (타이밍이 어긋나는 나쁜 예) */
	UPROPERTY(EditAnywhere, Category = "Anim Notify")
	bool bUseNotify = true;

private:
	void OnAnimHit(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation);
	void DoHitCheck(ACharacter* Attacker, const FString& Source);

	UPROPERTY(Transient) TObjectPtr<UAnimMontage> Combo;
	UPROPERTY(Transient) TObjectPtr<AD03_Mannequin> Attacker;
	UPROPERTY(Transient) TObjectPtr<AD03_Mannequin> Target;
	UPROPERTY(Transient) TObjectPtr<UD03_GraphWidget> Graph;
	UPROPERTY(Transient) TObjectPtr<USoundBase> HitSound;
	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> HitReact;
	UPROPERTY(Transient) TObjectPtr<AStaticMeshActor> Flash;

	FDelegateHandle HitHandle;
	float AttackTimer = 0.f;
	int32 Presses = 0;
	int32 HitCount = 0;
	float TargetHP = 100.f;
	float FlashTimer = 0.f;
	FString LastHit;
};
