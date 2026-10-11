#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0905_Submix.generated.h"

class ACameraActor;
class UAudioComponent;
class UGlossaryAudioBanner;
class UGlossaryAudioPanel;
class USoundSubmix;
class USubmixEffectFilterPreset;
class UStaticMeshComponent;

/**
 * 0905 Submix / Submix Effect
 * 효과음(차임·모닥불)은 SSub_SFX 서브믹스로 보내고, 그 이펙트 체인에 Low Pass 필터(SSFX_LowPass)를 둔다.
 * P 로 일시정지하면 필터 Cutoff 를 20000Hz → 500Hz 로 내려 효과음만 먹먹해진다(음악은 SFX 서브믹스를 안 거쳐 그대로).
 * 화면: SFX 서브믹스 출력 스펙트럼(주파수 대역별 막대) — 높은 대역이 사라지는 것이 보인다.
 */
UCLASS()
class GLOSSARYAUDIO_API ADemo0905_Submix : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0905_Submix();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 일시정지 중 Low Pass Cutoff (Hz) */
	UPROPERTY(EditAnywhere, Category = "Submix")
	float PausedCutoff = 500.f;

	/** 평소 Cutoff (사실상 필터 없음) */
	UPROPERTY(EditAnywhere, Category = "Submix")
	float NormalCutoff = 20000.f;

	/** Cutoff 가 목표로 바뀌는 시간(초) */
	UPROPERTY(EditAnywhere, Category = "Submix")
	float TransitionTime = 0.4f;

private:
	void SetPaused(bool bPause);

	UPROPERTY(Transient) TObjectPtr<USoundSubmix> SFXSubmix;
	UPROPERTY(Transient) TObjectPtr<USubmixEffectFilterPreset> LowPass;
	UPROPERTY(Transient) TArray<TObjectPtr<UAudioComponent>> Sources;
	UPROPERTY(Transient) TObjectPtr<UGlossaryAudioPanel> Panel;
	UPROPERTY(Transient) TObjectPtr<UGlossaryAudioBanner> Banner;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> SpectrumBars;

	TArray<float> BandFrequencies;
	TArray<float> SmoothedBands;
	bool bPaused = false;
	float Cutoff = 20000.f;
	float MaxMagnitudeSeen = 0.f;
};
