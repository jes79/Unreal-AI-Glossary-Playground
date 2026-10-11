#include "Demo0905_Submix.h"

#include "AudioMixerBlueprintLibrary.h"
#include "Camera/CameraActor.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryAudioKit.h"
#include "GlossaryKit.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundSubmix.h"
#include "Sound/SoundWave.h"
#include "SubmixEffects/SubmixEffectFilter.h"

using namespace GlossaryKit;

ADemo0905_Submix::ADemo0905_Submix()
{
	DemoId = TEXT("0905");
	Controls = TEXT("P : 일시정지 ON/OFF (실제로 게임을 멈춘다)\n")
		TEXT("관찰: 일시정지하면 SFX Submix 의 Low Pass Cutoff 가 20000 → 500Hz 로 내려가 효과음이 먹먹해진다\n")
		TEXT("스펙트럼 막대에서 1kHz 위 대역이 사라진다. 음악(BGM)은 SFX Submix 를 거치지 않아 그대로");
	Showcase = TEXT("SFX Submix 에 Low Pass Submix Effect, 일시정지 시 Cutoff 500Hz 로 먹먹하게 + 서브믹스 스펙트럼 막대");
	PlayerPawnClass = nullptr;
	// 일시정지 중에도 Cutoff 를 움직이고 스펙트럼을 갱신해야 하므로
	PrimaryActorTick.bTickEvenWhenPaused = true;
	BandFrequencies = { 125.f, 250.f, 500.f, 1000.f, 2000.f, 4000.f, 8000.f };
}

void ADemo0905_Submix::BeginPlay()
{
	Super::BeginPlay();
	GlossaryAudioKit::PrepareAudioDemo();
	UWorld* World = GetWorld();
	Camera = GlossaryAudioKit::SpawnFixedCamera(World, FVector(-1000.f, 0.f, 380.f), FRotator(-13.f, 0.f, 0.f), 72.f);

	SFXSubmix = LoadAsset<USoundSubmix>(GlossaryAudioKit::AssetPath(TEXT("SSub_SFX")));
	LowPass = LoadAsset<USubmixEffectFilterPreset>(GlossaryAudioKit::AssetPath(TEXT("SSFX_LowPass")));
	Cutoff = NormalCutoff;
	if (LowPass)
	{
		LowPass->SetFilterCutoffFrequency(Cutoff);
	}

	// 소리 3개: 차임·모닥불 = SFX(→ SSub_SFX), 잔잔한 BGM = Music(→ Master)
	const TCHAR* Assets[3] = { TEXT("S_Chime_Loop"), TEXT("S_Campfire_Loop"), TEXT("S_BGM_Calm_Loop") };
	const float Volumes[3] = { 1.f, 0.8f, 0.5f };
	for (int32 i = 0; i < 3; ++i)
	{
		UAudioComponent* Source = NewObject<UAudioComponent>(this);
		Source->bAutoActivate = false;
		Source->SetupAttachment(Root);
		Source->RegisterComponent();
		Source->SetSound(LoadAsset<USoundWave>(GlossaryAudioKit::AssetPath(Assets[i])));
		Source->SetVolumeMultiplier(Volumes[i]);
		// 일시정지해도 멈추지 않는 소리로 표시 (먹먹해진 소리를 들려주기 위해. 보통 메뉴 배경음에 쓰는 설정)
		Source->SetUISound(true);
		Source->Play();
		Sources.Add(Source);
	}

	// 소리 경로 그림: 소스 → (SSub_SFX: Low Pass) → Master
	SpawnBlock(World, FVector(250.f, -560.f, 0.f), FVector(120.f, 120.f, 120.f), Colors::Accent);
	SpawnLabel(World, TEXT("효과음 (차임·모닥불)\n→ SSub_SFX [Low Pass]"), FVector(120.f, -560.f, -20.f), Colors::Accent, 16.f);
	SpawnBlock(World, FVector(250.f, 560.f, 0.f), FVector(120.f, 120.f, 120.f), Colors::Purple);
	SpawnLabel(World, TEXT("음악 (BGM)\n→ Master (필터 없음)"), FVector(120.f, 560.f, -20.f), Colors::Purple, 16.f);

	// SFX 서브믹스 스펙트럼 막대
	for (int32 i = 0; i < BandFrequencies.Num(); ++i)
	{
		const float Y = -390.f + i * 130.f;
		UStaticMeshComponent* Bar = AddShape(this, EShape::Cube, FVector(600.f, Y, 5.f), FVector(60.f, 90.f, 10.f), Colors::Cyan);
		SetColor(Bar, Colors::Cyan, 0.4f, 0.f, 2.f);
		SpectrumBars.Add(Bar);
		const float F = BandFrequencies[i];
		SpawnLabel(World, F >= 1000.f ? FString::Printf(TEXT("%.0fk"), F / 1000.f) : FString::Printf(TEXT("%.0f"), F), FVector(560.f, Y, -10.f), FLinearColor::White, 15.f);
	}
	SpawnLabel(World, TEXT("SSub_SFX 출력 스펙트럼 (Hz)"), FVector(600.f, 0.f, 470.f), Colors::Item, 19.f);
	SmoothedBands.Init(0.f, BandFrequencies.Num());

	// 서브믹스 출력 분석 시작 (Audio Mixer 의 FFT 분석기)
	if (SFXSubmix)
	{
		UAudioMixerBlueprintLibrary::StartAnalyzingOutput(this, SFXSubmix);
	}

	Banner = UGlossaryAudioBanner::Create(this);
	Panel = UGlossaryAudioPanel::Create(this, TEXT("SSub_SFX · Low Pass"));
	if (Panel)
	{
		Panel->AddRow(TEXT("Cutoff"), Colors::Accent);
		Panel->AddRow(TEXT("저음 125~500"), Colors::Cyan);
		Panel->AddRow(TEXT("고음 2k~8k"), Colors::Cyan);
		Panel->SetFooter(TEXT("LowPass->SetFilterCutoffFrequency(Hz)\n스펙트럼 = StartAnalyzingOutput(SSub_SFX)"));
		Panel->AddToViewport(1);
	}
}

void ADemo0905_Submix::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SFXSubmix)
	{
		UAudioMixerBlueprintLibrary::StopAnalyzingOutput(this, SFXSubmix);
	}
	if (LowPass)
	{
		LowPass->SetFilterCutoffFrequency(NormalCutoff); // 프리셋은 공유 에셋이므로 원래대로
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0905_Submix::SetPaused(bool bPause)
{
	bPaused = bPause;
	// 일시정지 중에도 플레이어 컨트롤러가 완전히 Tick 하게 한다 (메뉴 UI·F1/F2·자동 스크린샷이 계속 동작).
	// 블루프린트 디테일에서 켜는 'Should Perform Full Tick When Paused' 와 같은 값 (C++ 에선 protected 라 리플렉션으로 설정)
	if (APlayerController* PC = GetPC())
	{
		if (FBoolProperty* FullTick = FindFProperty<FBoolProperty>(APlayerController::StaticClass(), TEXT("bShouldPerformFullTickWhenPaused")))
		{
			FullTick->SetPropertyValue_InContainer(PC, true);
		}
	}
	UGameplayStatics::SetGamePaused(this, bPause);
	if (Banner)
	{
		if (bPause)
		{
			Banner->Show(TEXT("일시정지 — 효과음 Low Pass 500Hz"), FLinearColor(1.f, 0.85f, 0.3f));
		}
		else
		{
			Banner->Hide();
		}
	}
}

void ADemo0905_Submix::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	GlossaryAudioKit::KeepViewTarget(this, Camera);

	if (WasKeyPressed(EKeys::P))
	{
		SetPaused(!bPaused);
	}

	// Cutoff 를 로그 스케일로 부드럽게 이동 (주파수는 귀에 로그로 들린다)
	const float Target = bPaused ? PausedCutoff : NormalCutoff;
	const float Alpha = FMath::Clamp(DeltaSeconds / FMath::Max(TransitionTime * 0.3f, 0.01f), 0.f, 1.f);
	Cutoff = FMath::Exp(FMath::Lerp(FMath::Loge(Cutoff), FMath::Loge(Target), Alpha));
	if (LowPass)
	{
		// 핵심 한 줄: 서브믹스 이펙트 프리셋 값을 바꾸면 이 서브믹스를 지나는 모든 소리에 적용된다
		LowPass->SetFilterCutoffFrequency(Cutoff);
	}

	// 스펙트럼
	TArray<float> Magnitudes;
	if (SFXSubmix)
	{
		UAudioMixerBlueprintLibrary::GetMagnitudeForFrequencies(this, BandFrequencies, Magnitudes, SFXSubmix);
	}
	float Low = 0.f, High = 0.f;
	for (int32 i = 0; i < BandFrequencies.Num(); ++i)
	{
		const float Mag = Magnitudes.IsValidIndex(i) ? Magnitudes[i] : 0.f;
		MaxMagnitudeSeen = FMath::Max(MaxMagnitudeSeen, Mag);
		// 지금까지 본 최대값 대비 dB (0 ~ -60dB 를 막대 0~1 로)
		const float Bar = MaxMagnitudeSeen > 0.f ? GlossaryAudioKit::AmplitudeToBar(Mag / MaxMagnitudeSeen, -60.f) : 0.f;
		SmoothedBands[i] = FMath::Lerp(SmoothedBands[i], Bar, FMath::Clamp(DeltaSeconds * 8.f, 0.f, 1.f));
		const float H = FMath::Max(4.f, SmoothedBands[i] * 400.f);
		SpectrumBars[i]->SetRelativeScale3D(FVector(0.6f, 0.9f, H / 100.f));
		SpectrumBars[i]->SetRelativeLocation(FVector(600.f, -390.f + i * 130.f, H * 0.5f));
		if (i <= 2) { Low += SmoothedBands[i] / 3.f; }
		else if (i >= 4) { High += SmoothedBands[i] / 3.f; }
	}

	if (Panel)
	{
		Panel->SetRow(0, FMath::Loge(Cutoff / 20.f) / FMath::Loge(20000.f / 20.f), FString::Printf(TEXT("%.0f Hz"), Cutoff));
		Panel->SetRow(1, Low, FString::Printf(TEXT("%.0f %%"), Low * 100.f));
		Panel->SetRow(2, High, FString::Printf(TEXT("%.0f %%"), High * 100.f));
	}
	SetStatus(FString::Printf(TEXT("%s · SSub_SFX Low Pass Cutoff %.0f Hz%s"),
		bPaused ? TEXT("일시정지 (SetGamePaused)") : TEXT("플레이 중"), Cutoff,
		Magnitudes.Num() == 0 ? TEXT("\n(스펙트럼 분석 결과 없음 — 오디오 장치 확인)") : (bPaused ? TEXT("\n효과음 먹먹함 · 음악은 그대로") : TEXT("\n모든 소리 또렷함"))));
}
