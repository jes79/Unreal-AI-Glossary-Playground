#include "D0305_AnimNotify_Hit.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"

FD0305OnAnimHit UD0305_AnimNotify_Hit::OnHit;

void UD0305_AnimNotify_Hit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	// 에디터 미리보기(페르소나)에서도 불리므로 게임 월드일 때만 알린다
	if (MeshComp && MeshComp->GetWorld() && MeshComp->GetWorld()->IsGameWorld())
	{
		OnHit.Broadcast(MeshComp, Animation);
	}
}
