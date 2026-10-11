#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Sound/QuartzQuantizationUtilities.h"
#include "Demo0908_Quartz.generated.h"

class ACameraActor;
class UAudioComponent;
class UGlossaryAudioPanel;
class UMaterialInstanceDynamic;
class UQuartzClockHandle;
class USoundWave;
class UStaticMeshComponent;

/**
 * 0908 Quartz (박자 동기화)
 * UQuartzSubsystem 으로 BPM 120 클락을 만들고, 타악기를 PlayQuantized(다음 Beat) 로 재생한다.
 * 아무 때나 눌러도(회색 점) 실제 소리는 다음 박(주황 점)에 정확히 맞춰 난다.
 * 화면: 4박 패드(박마다 번쩍), 재생 헤드, 누른 시점/재생 시점 마커, 대기 시간(ms).
 */
UCLASS()
class GLOSSARYAUDIO_API ADemo0908_Quartz : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0908_Quartz();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "Quartz", meta = (ClampMin = 30, ClampMax = 300))
	float BeatsPerMinute = 120.f;

	/** true = PlayQuantized(다음 Beat), false = 즉시 Play (비교용) */
	UPROPERTY(EditAnywhere, Category = "Quartz")
	bool bQuantize = true;

private:
	void Trigger();
	float GetBarPosition() const;  // 0~4 (마디 안 박 위치)
	void AddMarker(float BarPosition, bool bPlayed);

	UFUNCTION() void OnBeat(FName ClockName, EQuartzCommandQuantization QuantizationType, int32 NumBars, int32 Beat, float BeatFraction);
	UFUNCTION() void OnQuantizedCommand(EQuartzCommandDelegateSubType EventType, FName Name);

	UPROPERTY(Transient) TObjectPtr<UQuartzClockHandle> Clock;
	UPROPERTY(Transient) TArray<TObjectPtr<UAudioComponent>> Players;
	UPROPERTY(Transient) TObjectPtr<USoundWave> Percussion;
	UPROPERTY(Transient) TObjectPtr<UGlossaryAudioPanel> Panel;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Pads;
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> PadMats;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Playhead;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Markers;

	TArray<double> PendingPressTimes;  // PlayQuantized 를 요청한 시각 (시작 이벤트가 오면 앞에서부터 꺼낸다)
	TArray<float> PendingPressPositions; // 요청 순간의 마디 위치(0~4)
	double LastBeatTime = -1.0;
	int32 LastBeat = 0;                // 1~4
	int32 BeatCount = 0;
	int32 NextPlayer = 0;
	float PadFlash[4] = { 0.f, 0.f, 0.f, 0.f };
	float HitFlash = 0.f;
	float LastWaitMs = 0.f;
	float AutoTimer = 0.f;
	float NextAutoDelay = 0.7f;
	bool bAutoPress = true;
	int32 MarkerCursor = 0;
	FRandomStream Random{ 7 };
};
