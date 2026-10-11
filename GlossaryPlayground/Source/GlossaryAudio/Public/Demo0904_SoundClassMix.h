#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "Demo0904_SoundClassMix.generated.h"

class ACameraActor;
class UAudioComponent;
class UGlossaryAudioPanel;
class UGlossaryAudioProbe;
class USlider;
class USoundClass;
class USoundMix;
class UStaticMeshComponent;
class UTextBlock;
class ADemo0904_SoundClassMix;

/** 0904 옵션 창: Music / SFX / Voice 볼륨 슬라이더 (실제 게임의 설정 화면 역할) */
UCLASS()
class GLOSSARYAUDIO_API UD0904VolumeOptions : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<ADemo0904_SoundClassMix> Demo;
	void SetValue(int32 Index, float Value, bool bSelected);

protected:
	virtual void BuildTree() override;

	UFUNCTION() void OnMusicChanged(float Value);
	UFUNCTION() void OnSFXChanged(float Value);
	UFUNCTION() void OnVoiceChanged(float Value);

	UPROPERTY(Transient) TArray<TObjectPtr<USlider>> Sliders;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> Labels;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> ValueTexts;

	/** 코드에서 슬라이더 값을 맞출 때 OnValueChanged 가 다시 데모를 부르지 않게 막는다 */
	bool bSyncingFromCode = false;
};

/**
 * 0904 Sound Class / Sound Mix
 * SC_Music · SC_SFX · SC_Voice 클래스에 소리를 나눠 두고, 옵션 슬라이더가
 * UGameplayStatics::SetSoundMixClassOverride(SMix_Options, 클래스, 볼륨) 로 그룹 볼륨을 바꾼다.
 * 스피커 3개의 크기·막대가 각 그룹 볼륨과 실제 Envelope 를 보여 준다.
 */
UCLASS()
class GLOSSARYAUDIO_API ADemo0904_SoundClassMix : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0904_SoundClassMix();

	/** 슬라이더/키에서 호출: Index 0=Music, 1=SFX, 2=Voice */
	void SetClassVolume(int32 Index, float Volume);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 볼륨이 바뀔 때 페이드 시간(초) */
	UPROPERTY(EditAnywhere, Category = "Sound Mix")
	float FadeInTime = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Sound Mix")
	TArray<float> ClassVolumes = { 1.f, 1.f, 1.f };

private:
	UPROPERTY(Transient) TObjectPtr<USoundMix> OptionsMix;
	UPROPERTY(Transient) TArray<TObjectPtr<USoundClass>> Classes;
	UPROPERTY(Transient) TArray<TObjectPtr<UAudioComponent>> Sources;
	UPROPERTY(Transient) TArray<TObjectPtr<UGlossaryAudioProbe>> Probes;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Speakers;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> VolumeBars;
	UPROPERTY(Transient) TObjectPtr<UD0904VolumeOptions> Options;
	UPROPERTY(Transient) TObjectPtr<UGlossaryAudioPanel> Panel;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;

	int32 Selected = 0;
	float Time = 0.f;
};
