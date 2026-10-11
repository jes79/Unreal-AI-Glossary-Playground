#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0313_Sequencer.generated.h"

class AD03_Mannequin;
class ALevelSequenceActor;
class ULevelSequence;
class ULevelSequencePlayer;
class ATriggerBox;
class ACineCameraActor;

/**
 * 0313 Sequencer (컷신)
 * 빌더가 만든 LS_Cutscene(5초: 카메라 컷 3개 + Transform 트랙 + 페이드 + 주인공 애니메이션 트랙)을
 * Trigger Box 에 들어가면 ULevelSequencePlayer 로 재생한다. 카메라·주인공은 실행 중 SetBinding 으로 연결.
 */
UCLASS()
class GLOSSARYANIMATION_API ADemo0313_Sequencer : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0313_Sequencer();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 트리거 박스 크기(반) */
	UPROPERTY(EditAnywhere, Category = "Sequencer")
	FVector TriggerExtent = FVector(150.f, 150.f, 120.f);

private:
	UFUNCTION()
	void OnTriggerOverlap(AActor* OverlappedActor, AActor* OtherActor);

	void PlayCutscene();

	UPROPERTY(Transient) TObjectPtr<ULevelSequence> Sequence;
	UPROPERTY(Transient) TObjectPtr<ULevelSequencePlayer> Player;
	UPROPERTY(Transient) TObjectPtr<ALevelSequenceActor> SequenceActor;
	UPROPERTY(Transient) TObjectPtr<ATriggerBox> Trigger;
	UPROPERTY(Transient) TObjectPtr<AD03_Mannequin> Hero;
	UPROPERTY(Transient) TArray<TObjectPtr<ACineCameraActor>> Cameras;
	int32 PlayCount = 0;
};
