#pragma once

#include "CoreMinimal.h"
#include "GlossaryWidgets.h"
#include "UObject/Object.h"
#include "GlossaryAudioKit.generated.h"

class AActor;
class ACameraActor;
class AGlossaryDemoBase;
class UAudioComponent;
class UHorizontalBox;
class UMetaSoundSourceBuilder;
class UProgressBar;
class USoundWave;
class UTextBlock;
class UWorld;

/**
 * 09 오디오 분야 공용 헬퍼.
 * 소리는 스크린샷에 안 나오므로 모든 데모가 "지금 무슨 일이 일어나는지"를 눈으로 보이게 만든다:
 *  - UGlossaryAudioProbe : 오디오 컴포넌트의 Envelope(실제 출력 크기)·Playback Percent(재생 위치)를 받아 둔다
 *  - UGlossaryAudioPanel : 화면 오른쪽 위 막대 계기판(볼륨·피치·보이스 수 등)
 *  - SpawnRing          : 거리(감쇠 반경)·메아리를 보여 주는 바닥 링
 */
namespace GlossaryAudioKit
{
	/** /Game/Glossary/09_Audio/{Name} */
	GLOSSARYAUDIO_API FString AssetPath(const TCHAR* Name);

	/** 고정 카메라 (플레이어 폰이 없는 데모용). 오디오 리스너도 이 카메라 위치가 된다 */
	GLOSSARYAUDIO_API ACameraActor* SpawnFixedCamera(UWorld* World, const FVector& Location, const FRotator& Rotation, float FOV = 70.f);
	GLOSSARYAUDIO_API void KeepViewTarget(const AGlossaryDemoBase* Demo, ACameraActor* Camera);

	/** 바닥에 놓인 링(작은 상자 Segments 개). Radius cm. 반환 액터의 스케일로 링 크기를 바꿀 수 있다 */
	GLOSSARYAUDIO_API AActor* SpawnRing(UWorld* World, const FVector& Center, float Radius, const FLinearColor& Color,
		int32 Segments = 48, float Thickness = 12.f, float Emissive = 1.f);

	/** 오디오 리스너 위치(보통 카메라). 없으면 false */
	GLOSSARYAUDIO_API bool GetListener(const AGlossaryDemoBase* Demo, FVector& OutLocation, FRotator& OutRotation);

	/**
	 * 0902: MetaSound Source 빌더로 "RPM(0~1) → Map Range(40~220Hz) → Saw 오실레이터 → 출력" 그래프를 만든다.
	 * 에디터 에셋 빌더(MS_EngineRPM 에셋 저장)와 실행 중 대체 경로(에셋이 없을 때 임시 MetaSound) 가 같은 함수를 쓴다.
	 * 실패하면 nullptr, OutLog 에 단계별 결과.
	 */
	GLOSSARYAUDIO_API UMetaSoundSourceBuilder* CreateEngineRPMBuilder(FName BuilderName, FString& OutLog);

	/** dB 변환(진폭 0~1 → -60~0dB 를 0~1 막대로) */
	GLOSSARYAUDIO_API float AmplitudeToBar(float Amplitude, float FloorDb = -60.f);

	/**
	 * 오디오 데모 공통 준비: 창이 비활성일 때도 소리를 끄지 않는다(기본 UnfocusedVolumeMultiplier=0).
	 * 소리가 꺼지면 Envelope 계기판도 0 이 되어 자동 스크린샷·옆 창 작업 중에 데모가 멈춘 것처럼 보이기 때문.
	 */
	GLOSSARYAUDIO_API void PrepareAudioDemo();
}

/**
 * 오디오 컴포넌트에 붙여 실제 재생 상태를 읽는 프로브.
 * OnAudioSingleEnvelopeValue = 출력 Envelope(소리 크기), OnAudioPlaybackPercent = 재생 위치(어떤 웨이브가 몇 % 재생됐는지).
 * 주의: 두 델리게이트는 Play() 전에 바인딩돼 있어야 오디오 스레드가 값을 보내 준다.
 */
UCLASS()
class GLOSSARYAUDIO_API UGlossaryAudioProbe : public UObject
{
	GENERATED_BODY()

public:
	void Bind(UAudioComponent* Component);

	/** 최근 Envelope (0.2초 넘게 소식이 없으면 0) */
	float GetEnvelope() const;
	/** 현재(또는 마지막) 재생 중인 웨이브 */
	const USoundWave* GetWave() const { return CurrentWave.Get(); }
	float GetPercent() const { return LastPercent; }

	/** 재생 위치가 흐르는 속도로 실제 피치를 추정한다: 피치 = (재생 비율/초) × 웨이브 길이 */
	bool EstimatePitch(float& OutPitch) const;
	/** 새 재생을 시작하기 전에 피치 측정 샘플을 비운다 */
	void ResetPitchSamples();

	/** 0907 처럼 컴포넌트가 끝났는지 보는 용도 */
	TWeakObjectPtr<UAudioComponent> Component;

private:
	UFUNCTION() void HandleEnvelope(const USoundWave* PlayingSoundWave, const float EnvelopeValue);
	UFUNCTION() void HandlePercent(const USoundWave* PlayingSoundWave, const float PlaybackPercent);

	float Envelope = 0.f;
	double EnvelopeTime = -1.0;
	float LastPercent = 0.f;
	TWeakObjectPtr<const USoundWave> CurrentWave;
	TArray<FVector2D> PitchSamples; // (시각, 재생 비율)
};

/**
 * 화면 오른쪽 위 계기판: 제목 + 막대 줄 여러 개 + 아래 설명.
 * AddRow 는 AddToViewport 전에 호출한다(트리는 처음 그려질 때 만들어짐).
 */
UCLASS()
class GLOSSARYAUDIO_API UGlossaryAudioPanel : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	void SetTitle(const FString& InTitle) { Title = InTitle; }
	int32 AddRow(const FString& Label, const FLinearColor& Color);
	void SetRow(int32 Index, float Fraction, const FString& Value);
	void SetRowColor(int32 Index, const FLinearColor& Color);
	void SetFooter(const FString& Text);

	/** 화면에 올리는 편의 함수 */
	static UGlossaryAudioPanel* Create(AGlossaryDemoBase* Demo, const FString& Title, float BarWidth = 220.f);

protected:
	virtual void BuildTree() override;

	struct FRowDef { FString Label; FLinearColor Color; };
	TArray<FRowDef> RowDefs;
	FString Title;
	FString PendingFooter;
	float BarWidth = 220.f;

	UPROPERTY(Transient) TArray<TObjectPtr<UProgressBar>> Bars;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> Values;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Footer;
};

/** 화면 아래 가운데 큰 글씨 배너 ("일시정지", "전투 시작!" 처럼 상태 전환을 크게 보여 줄 때) */
UCLASS()
class GLOSSARYAUDIO_API UGlossaryAudioBanner : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	static UGlossaryAudioBanner* Create(AGlossaryDemoBase* Demo);
	void Show(const FString& Text, const FLinearColor& Color);
	void Hide();

protected:
	virtual void BuildTree() override;

	UPROPERTY(Transient) TObjectPtr<class UBorder> Frame;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Label;
};
