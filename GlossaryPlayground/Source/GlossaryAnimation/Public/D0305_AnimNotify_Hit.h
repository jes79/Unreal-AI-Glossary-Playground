#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "D0305_AnimNotify_Hit.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FD0305OnAnimHit, USkeletalMeshComponent* /*Mesh*/, UAnimSequenceBase* /*Animation*/);

/**
 * 0305 Anim Notify: 공격 몽타주의 '타격 프레임'에 놓이는 C++ Notify.
 * 빌더가 AM_Attack_Combo 의 각 섹션 타격 시점에 이 Notify 를 넣는다(Montage->Notifies).
 * 애니메이션이 그 프레임을 지나가면 엔진이 Notify() 를 부르고, 데모는 OnHit 에서 데미지 판정·효과음을 실행한다.
 */
UCLASS(meta = (DisplayName = "Glossary Hit"))
class GLOSSARYANIMATION_API UD0305_AnimNotify_Hit : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override { return TEXT("Hit"); }

	/** 모든 데모가 구독하는 전역 이벤트 (MeshComp 의 주인으로 자기 캐릭터인지 구분) */
	static FD0305OnAnimHit OnHit;
};
