#include "Demo0901_SoundCue.h"

#include "Camera/CameraActor.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GlossaryAudioKit.h"
#include "GlossaryKit.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundWave.h"

using namespace GlossaryKit;

namespace
{
	constexpr int32 HistoryCount = 12;
	// 웨이브 번호별 색: 그래프에서 어떤 발소리가 골렸는지 한눈에 보이게
	const FLinearColor WaveColors[4] = { Colors::Cyan, Colors::Item, Colors::Ally, Colors::Purple };
	// 피치 → 막대 높이(cm): 0.85 → 0, 1.0 → 225, 1.15 → 450
	float PitchToHeight(float Pitch) { return FMath::Clamp((Pitch - 0.85f) * 1500.f, 10.f, 470.f); }
}

ADemo0901_SoundCue::ADemo0901_SoundCue()
{
	DemoId = TEXT("0901");
	Controls = TEXT("자동으로 0.5초마다 발소리 재생 · F : 한 번 재생 · 2 : 자동 재생 ON/OFF\n")
		TEXT("C : Sound Cue(Random+Modulator) ↔ 웨이브 1개 고정 비교\n")
		TEXT("관찰: 막대 색 = 골린 발소리(4종), 높이 = 실제 재생 피치. Cue 는 색과 높이가 매번 달라 반복감이 없다");
	Showcase = TEXT("발소리 4개를 Random 으로 고르고 Modulator 피치 0.9~1.1, 골린 웨이브·측정 피치 막대 그래프");
	PlayerPawnClass = nullptr;
}

void ADemo0901_SoundCue::BeginPlay()
{
	Super::BeginPlay();
	GlossaryAudioKit::PrepareAudioDemo();
	UWorld* World = GetWorld();

	FootstepCue = LoadAsset<USoundCue>(GlossaryAudioKit::AssetPath(TEXT("SCue_Footsteps")));
	for (int32 i = 1; i <= 4; ++i)
	{
		Footsteps.Add(LoadAsset<USoundWave>(GlossaryAudioKit::AssetPath(*FString::Printf(TEXT("S_Footstep_%02d"), i))));
	}

	Camera = GlossaryAudioKit::SpawnFixedCamera(World, FVector(-950.f, 0.f, 470.f), FRotator(-14.f, 0.f, 0.f), 75.f);

	// 피치 막대 그래프 (오래된 것 → 최근 것, 왼쪽 → 오른쪽)
	for (int32 i = 0; i < HistoryCount; ++i)
	{
		const FVector Pos(500.f, -770.f + i * 140.f, 5.f);
		UStaticMeshComponent* Bar = AddShape(this, EShape::Cube, Pos, FVector(60.f, 90.f, 10.f), Colors::Neutral);
		HistoryBars.Add(Bar);
		HistoryMats.Add(SetColor(Bar, Colors::Neutral, 0.5f, 0.f, 0.f));
	}
	// 기준선: 피치 1.0 높이, 0.9 / 1.1 경계
	for (const float Pitch : { 0.9f, 1.0f, 1.1f })
	{
		const float H = PitchToHeight(Pitch);
		UStaticMeshComponent* Line = AddShape(this, EShape::Cube, FVector(540.f, 0.f, H), FVector(4.f, 1760.f, Pitch == 1.0f ? 5.f : 2.5f),
			Pitch == 1.0f ? FLinearColor::White : Colors::Accent);
		SetColor(Line, Pitch == 1.0f ? FLinearColor::White : Colors::Accent, 0.5f, 0.f, 2.f);
		SpawnLabel(World, FString::Printf(TEXT("피치 %.1f"), Pitch), FVector(540.f, 890.f, H), FLinearColor::White, 16.f);
	}
	SpawnLabel(World, TEXT("최근 12번 발소리 (색 = 골린 웨이브, 높이 = 측정 피치)"), FVector(500.f, 0.f, 640.f), Colors::Item, 20.f);

	// 발 (재생할 때마다 쿵 하고 내려온다)
	Foot = AddShape(this, EShape::Cube, FVector(150.f, 0.f, 40.f), FVector(110.f, 60.f, 40.f), Colors::Player);
	for (int32 i = 0; i < 4; ++i)
	{
		SpawnLabel(World, FString::Printf(TEXT("S_Footstep_%02d"), i + 1), FVector(500.f, -330.f + i * 220.f, 540.f), WaveColors[i], 16.f);
	}

	// 오디오 컴포넌트: 프로브는 Play 전에 바인딩해야 Envelope·재생 위치를 받는다
	Audio = NewObject<UAudioComponent>(this, TEXT("FootstepAudio"));
	Audio->bAutoActivate = false;
	Audio->SetupAttachment(Root);
	Audio->RegisterComponent();
	Probe = NewObject<UGlossaryAudioProbe>(this);
	Probe->Bind(Audio);
	Audio->OnAudioFinished.AddDynamic(this, &ADemo0901_SoundCue::HandleStepFinished);

	Panel = UGlossaryAudioPanel::Create(this, TEXT("Sound Cue 재생 결과"));
	if (Panel)
	{
		Panel->AddRow(TEXT("골린 웨이브"), Colors::Cyan);
		Panel->AddRow(TEXT("측정 피치"), Colors::Accent);
		Panel->AddRow(TEXT("Envelope 최대"), Colors::Ally);
		Panel->SetFooter(TEXT("피치는 OnAudioPlaybackPercent 의 재생 속도로 측정 (≈)"));
		Panel->AddToViewport(1);
	}

	if (!FootstepCue)
	{
		SetStatus(TEXT("SCue_Footsteps 가 없음 — tools/glossary-build.sh -filter=09 로 에셋을 먼저 만드세요"));
	}
}

void ADemo0901_SoundCue::PlayStep()
{
	USoundBase* Sound = bUseSoundCue ? static_cast<USoundBase*>(FootstepCue) : static_cast<USoundBase*>(Footsteps.Num() ? Footsteps[0] : nullptr);
	if (!Sound)
	{
		return;
	}
	if (bStepPlaying)
	{
		HandleStepFinished(); // 이전 발소리 결과를 먼저 기록
	}
	Probe->ResetPitchSamples();
	Audio->SetSound(Sound);
	Audio->Play();   // Cue 라면 재생할 때마다 Random 이 웨이브를 고르고 Modulator 가 피치를 정한다
	bStepPlaying = true;
	FootBounce = 1.f;
	++StepCount;
}

void ADemo0901_SoundCue::HandleStepFinished()
{
	if (!bStepPlaying)
	{
		return;
	}
	bStepPlaying = false;
	FStepRecord Record;
	const USoundWave* Played = Probe->GetWave();
	for (int32 i = 0; i < Footsteps.Num(); ++i)
	{
		if (Footsteps[i] == Played)
		{
			Record.WaveIndex = i;
		}
	}
	Record.bMeasured = Probe->EstimatePitch(Record.Pitch);
	History.Add(Record);
	if (History.Num() > HistoryCount)
	{
		History.RemoveAt(0);
	}
	RefreshHistory();
}

void ADemo0901_SoundCue::RefreshHistory()
{
	for (int32 i = 0; i < HistoryCount; ++i)
	{
		const int32 RecordIndex = History.Num() - HistoryCount + i;
		UStaticMeshComponent* Bar = HistoryBars[i];
		if (!History.IsValidIndex(RecordIndex))
		{
			Bar->SetRelativeScale3D(FVector(0.6f, 0.9f, 0.1f));
			Bar->SetRelativeLocation(FVector(500.f, -770.f + i * 140.f, 5.f));
			continue;
		}
		const FStepRecord& R = History[RecordIndex];
		const float H = R.bMeasured ? PitchToHeight(R.Pitch) : 20.f;
		Bar->SetRelativeScale3D(FVector(0.6f, 0.9f, H / 100.f));
		Bar->SetRelativeLocation(FVector(500.f, -770.f + i * 140.f, H * 0.5f));
		const FLinearColor Color = R.WaveIndex >= 0 ? WaveColors[R.WaveIndex] : Colors::Neutral;
		if (UMaterialInstanceDynamic* Mat = HistoryMats[i])
		{
			Mat->SetVectorParameterValue(TEXT("Color"), Color);
			Mat->SetScalarParameterValue(TEXT("Emissive"), i == HistoryCount - 1 ? 1.5f : 0.3f);
		}
	}
}

void ADemo0901_SoundCue::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	GlossaryAudioKit::KeepViewTarget(this, Camera);

	if (WasKeyPressed(EKeys::C))
	{
		bUseSoundCue = !bUseSoundCue;
		History.Reset();
		RefreshHistory();
	}
	if (WasKeyPressed(EKeys::Two))
	{
		bAutoStep = !bAutoStep;
	}
	if (WasKeyPressed(EKeys::F))
	{
		PlayStep();
	}
	if (bAutoStep)
	{
		StepTimer += DeltaSeconds;
		if (StepTimer >= StepInterval)
		{
			StepTimer = 0.f;
			PlayStep();
		}
	}

	// 발 애니메이션: 재생 순간 바닥을 쿵
	FootBounce = FMath::Max(0.f, FootBounce - DeltaSeconds * 4.f);
	const int32 FootLane = (StepCount % 2 == 0) ? -1 : 1;
	if (Foot)
	{
		Foot->SetRelativeLocation(FVector(150.f, FootLane * 90.f, 20.f + (1.f - FootBounce) * 60.f));
	}

	// 실시간 값
	float LivePitch = 1.f;
	const bool bLivePitch = Probe && Probe->EstimatePitch(LivePitch);
	int32 LiveWave = -1;
	for (int32 i = 0; Probe && i < Footsteps.Num(); ++i)
	{
		if (Footsteps[i] == Probe->GetWave())
		{
			LiveWave = i;
		}
	}
	const float Envelope = Probe ? Probe->GetPeakEnvelope() : 0.f; // 짧은 발소리라 이번 발소리의 최대값
	if (Panel)
	{
		Panel->SetRow(0, LiveWave >= 0 ? (LiveWave + 1) / 4.f : 0.f, LiveWave >= 0 ? FString::Printf(TEXT("S_Footstep_%02d"), LiveWave + 1) : TEXT("-"));
		if (LiveWave >= 0)
		{
			Panel->SetRowColor(0, WaveColors[LiveWave]);
		}
		Panel->SetRow(1, bLivePitch ? (LivePitch - 0.8f) / 0.4f : 0.f, bLivePitch ? FString::Printf(TEXT("≈ %.2f"), LivePitch) : TEXT("측정 중"));
		Panel->SetRow(2, GlossaryAudioKit::AmplitudeToBar(Envelope), FString::Printf(TEXT("%.3f"), Envelope));
	}

	FString Recent;
	for (int32 i = FMath::Max(0, History.Num() - 6); i < History.Num(); ++i)
	{
		Recent += History[i].WaveIndex >= 0 ? FString::Printf(TEXT("#%d(%.2f) "), History[i].WaveIndex + 1, History[i].Pitch) : TEXT("? ");
	}
	SetStatus(FString::Printf(TEXT("%s · 자동 %s · 재생 %d회\n최근: %s"),
		bUseSoundCue ? TEXT("Sound Cue: Random(4) → Modulator(피치 0.9~1.1)") : TEXT("웨이브 1개 고정 (S_Footstep_01, 피치 1.0)"),
		bAutoStep ? TEXT("ON") : TEXT("OFF"), StepCount, *Recent));
}
