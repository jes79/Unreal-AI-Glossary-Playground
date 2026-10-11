#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0304_Montage.generated.h"

class AD03_Mannequin;
class UAnimMontage;
class UD03_GraphWidget;
class ACharacter;

/**
 * 0304 Animation Montage
 * 빌더가 만든 AM_Attack_Combo(섹션 Attack1·Attack2·Attack3, DefaultSlot)를 공격 키로 재생하고,
 * 섹션 재생 중 다시 누르면 Montage_SetNextSection 으로 다음 섹션을 예약해 3단 콤보를 잇는다.
 */
UCLASS()
class GLOSSARYANIMATION_API ADemo0304_Montage : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0304_Montage();

	/** 콤보 입력 한 번: 재생 중이 아니면 Attack1 부터, 재생 중이면 다음 섹션 예약. 예약한 섹션 이름을 돌려준다 */
	static FName ComboInput(ACharacter* Character, UAnimMontage* Montage);
	/** 몽타주 섹션 타임라인을 그래프에 그린다 */
	static void DrawMontageTimeline(UD03_GraphWidget* Graph, UAnimMontage* Montage, ACharacter* Character, bool bNotifies);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** NPC 가 3단 콤보를 반복하는 간격(초) */
	UPROPERTY(EditAnywhere, Category = "Montage")
	float NpcComboInterval = 3.5f;

private:
	FString Describe(ACharacter* Character) const;

	UPROPERTY(Transient) TObjectPtr<UAnimMontage> Combo;
	UPROPERTY(Transient) TObjectPtr<AD03_Mannequin> Npc;
	UPROPERTY(Transient) TObjectPtr<UD03_GraphWidget> Graph;
	float NpcTimer = 0.f;
	int32 NpcPresses = 0;
	FName PlayerQueued;
	FName PlayerLastSection;
	FString PlayerHistory;   // 플레이어가 재생한 섹션 기록 (콤보가 이어졌는지 확인용)
};
