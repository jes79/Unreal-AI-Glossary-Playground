#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0307_LayeredBlend.generated.h"

class AD03_Mannequin;
class UAnimSequence;

/**
 * 0307 Layered Blend per Bone (상하체 분리)
 * C++ AnimInstance 의 Anim Graph 단계에서 'UpperBody' 슬롯 포즈를 spine_01 아래에만 덮어쓴다.
 * 두 NPC 가 걸으며 같은 공격을 UpperBody 슬롯(PlaySlotAnimationAsDynamicMontage)으로 재생 — 왼쪽은 상체만, 오른쪽은 전신.
 */
UCLASS()
class GLOSSARYANIMATION_API ADemo0307_LayeredBlend : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0307_LayeredBlend();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 이 뼈와 그 자식이 상체 레이어 */
	UPROPERTY(EditAnywhere, Category = "Layered Blend")
	FName LayerBone = TEXT("spine_01");

	/** 슬롯 이름 (AnimBP 의 Slot 노드 이름과 같아야 한다) */
	UPROPERTY(EditAnywhere, Category = "Layered Blend")
	FName UpperBodySlot = TEXT("UpperBody");

	UPROPERTY(EditAnywhere, Category = "Layered Blend")
	float AttackInterval = 1.6f;

private:
	void Attack(class ACharacter* Who);

	UPROPERTY(Transient) TObjectPtr<AD03_Mannequin> Layered;
	UPROPERTY(Transient) TObjectPtr<AD03_Mannequin> FullBody;
	UPROPERTY(Transient) TObjectPtr<UAnimSequence> AttackAnim;
	float Timer = 0.f;
	bool bPlayerLayered = true;
};
