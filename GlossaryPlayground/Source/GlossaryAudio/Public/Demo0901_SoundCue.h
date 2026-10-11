#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0901_SoundCue.generated.h"

class ACameraActor;
class UAudioComponent;
class UGlossaryAudioPanel;
class UGlossaryAudioProbe;
class UMaterialInstanceDynamic;
class USoundCue;
class USoundWave;
class UStaticMeshComponent;

/**
 * 0901 Sound Cue (Random · Modulator)
 * 빌더가 만든 SCue_Footsteps = 발소리 4개 → Random → Modulator(피치 0.9~1.1) → 출력.
 * 발소리를 반복 재생하면서 "어떤 웨이브가 골렸는지"와 "실제 피치"를 막대 그래프로 쌓아 보여 준다.
 * C 로 같은 웨이브 1개를 고정 피치로 반복하는 것과 비교.
 */
UCLASS()
class GLOSSARYAUDIO_API ADemo0901_SoundCue : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0901_SoundCue();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** true = Sound Cue(Random + Modulator), false = 웨이브 하나 고정 */
	UPROPERTY(EditAnywhere, Category = "Sound Cue")
	bool bUseSoundCue = true;

	/** 자동 발소리 간격(초) */
	UPROPERTY(EditAnywhere, Category = "Sound Cue", meta = (ClampMin = 0.3))
	float StepInterval = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Sound Cue")
	bool bAutoStep = true;

private:
	void PlayStep();
	UFUNCTION() void HandleStepFinished();
	void RefreshHistory();

	struct FStepRecord { int32 WaveIndex = -1; float Pitch = 1.f; bool bMeasured = false; };

	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Audio;
	UPROPERTY(Transient) TObjectPtr<UGlossaryAudioProbe> Probe;
	UPROPERTY(Transient) TObjectPtr<USoundCue> FootstepCue;
	UPROPERTY(Transient) TArray<TObjectPtr<USoundWave>> Footsteps;
	UPROPERTY(Transient) TObjectPtr<UGlossaryAudioPanel> Panel;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> HistoryBars;
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> HistoryMats;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Foot;

	TArray<FStepRecord> History;
	float StepTimer = 0.f;
	float FootBounce = 0.f;
	int32 StepCount = 0;
	bool bStepPlaying = false;
};
