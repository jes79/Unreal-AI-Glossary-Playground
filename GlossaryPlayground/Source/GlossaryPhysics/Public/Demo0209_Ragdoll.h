#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0209_Ragdoll.generated.h"

class ACameraActor;
class ACharacter;
class UAnimSequence;

/**
 * 0209 래그돌 (Physics Asset)
 * Manny(SKM_Manny_Simple, Physics Asset = PA_Mannequin)가 죽으면
 * 캡슐 콜리전을 끄고 Mesh 를 Ragdoll 프로필 + SetSimulatePhysics(true) 로 바꾼다.
 * 비교용으로 '죽음 애니메이션만 재생'하는 방식도 Q 로 볼 수 있다.
 */
UCLASS()
class GLOSSARYPHYSICS_API ADemo0209_Ragdoll : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0209_Ragdoll();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 죽을 때 몸통에 주는 속도 변화 (cm/s, 카메라 반대쪽으로) */
	UPROPERTY(EditAnywhere, Category = "Ragdoll")
	float DeathPush = 350.f;

	UPROPERTY(EditAnywhere, Category = "Ragdoll")
	TObjectPtr<UAnimSequence> DeathAnimation;

private:
	void SpawnDummies();
	void KillWithRagdoll(ACharacter* Character);
	void KillWithAnimation(ACharacter* Character);
	ACharacter* NextAlive() const;

	UPROPERTY() TObjectPtr<ACameraActor> Camera;
	UPROPERTY() TArray<TObjectPtr<ACharacter>> Dummies;
	TArray<FString> States;
	FString LastAction;
};
