#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0909_AudioFade.generated.h"

class ACameraActor;
class UAudioComponent;
class UGlossaryAudioBanner;
class UGlossaryAudioPanel;
class UGlossaryAudioProbe;
class UStaticMeshComponent;

/**
 * 0909 Audio Component Fade
 * 전투가 시작되면 잔잔한 BGM 을 FadeOut(2초), 전투 BGM 을 FadeIn(2초) 으로 크로스페이드한다.
 * 화면: 두 BGM 스피커의 크기·막대 = 현재 페이드 볼륨, 패널 = 계산 볼륨과 측정 Envelope. 2 로 즉시 교체와 비교.
 */
UCLASS()
class GLOSSARYAUDIO_API ADemo0909_AudioFade : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0909_AudioFade();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Audio Fade", meta = (ClampMin = 0))
	float FadeOutDuration = 2.f;

	UPROPERTY(EditAnywhere, Category = "Audio Fade", meta = (ClampMin = 0))
	float FadeInDuration = 2.f;

	/** false 면 Stop/Play 로 즉시 교체 (비교용) */
	UPROPERTY(EditAnywhere, Category = "Audio Fade")
	bool bCrossfade = true;

private:
	void SetBattle(bool bInBattle);

	/** 화면 표시용으로 페이드 진행을 따라 계산한다 (엔진 내부 페이드 값은 읽는 API 가 없어서) */
	struct FFadeTrack
	{
		float From = 0.f, To = 0.f, Duration = 0.f, Elapsed = 0.f;
		float Value() const { return Duration <= 0.f ? To : FMath::Lerp(From, To, FMath::Clamp(Elapsed / Duration, 0.f, 1.f)); }
		void Start(float InTo, float InDuration) { From = Value(); To = InTo; Duration = InDuration; Elapsed = 0.f; }
	};

	UPROPERTY(Transient) TObjectPtr<UAudioComponent> CalmBGM;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> BattleBGM;
	UPROPERTY(Transient) TObjectPtr<UGlossaryAudioProbe> CalmProbe;
	UPROPERTY(Transient) TObjectPtr<UGlossaryAudioProbe> BattleProbe;
	UPROPERTY(Transient) TObjectPtr<UGlossaryAudioPanel> Panel;
	UPROPERTY(Transient) TObjectPtr<UGlossaryAudioBanner> Banner;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> CalmSpeaker;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> BattleSpeaker;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> CalmBar;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> BattleBar;
	UPROPERTY(Transient) TObjectPtr<AActor> Enemy;

	FFadeTrack CalmTrack;
	FFadeTrack BattleTrack;
	bool bBattle = false;
	float BannerTime = 0.f;
	float Time = 0.f;
};
