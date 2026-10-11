#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0311_IKRetargeter.generated.h"

class AD03_Mannequin;
class UD03_AnimInstance;

/**
 * 0311 IK Retargeter
 * 같은 Manny 애니메이션을 체형이 다른 캐릭터에 쓰는 문제를 나란히 보여 준다:
 * 소스 Manny / 같은 스켈레톤 Quinn / 다리가 긴 캐릭터에 그대로 복사(발이 바닥을 뚫음) / 골반 높이를 비율로 보정(리타깃).
 */
UCLASS()
class GLOSSARYANIMATION_API ADemo0311_IKRetargeter : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0311_IKRetargeter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** '내 캐릭터' 의 다리 길이 배율 */
	UPROPERTY(EditAnywhere, Category = "IK Retargeter", meta = (ClampMin = 0.5, ClampMax = 2))
	float TargetLegScale = 1.35f;

	/** 재생 속도 입력 (Blend Space Speed) */
	UPROPERTY(EditAnywhere, Category = "IK Retargeter")
	float PlaySpeed = 300.f;

private:
	UPROPERTY(Transient) TArray<TObjectPtr<AD03_Mannequin>> Actors;
	UPROPERTY(Transient) TArray<TObjectPtr<UD03_AnimInstance>> Anims;
};
