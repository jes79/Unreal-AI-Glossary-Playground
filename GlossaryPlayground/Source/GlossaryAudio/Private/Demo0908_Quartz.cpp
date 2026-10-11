#include "Demo0908_Quartz.h"

#include "Camera/CameraActor.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GlossaryAudioKit.h"
#include "GlossaryKit.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Quartz/AudioMixerClockHandle.h"
#include "Quartz/QuartzSubsystem.h"
#include "Sound/SoundWave.h"

using namespace GlossaryKit;

namespace
{
	const FName ClockName(TEXT("GlossaryClock"));
	constexpr int32 MarkerCount = 16;
	constexpr float PadX = 600.f;
	// 마디 위치(0~4 박) → Y 좌표
	float BarToY(float BarPosition) { return -600.f + BarPosition * 300.f; }
	const FLinearColor PressColor(0.25f, 0.25f, 0.28f);
	const FLinearColor PadColor(0.12f, 0.14f, 0.18f);
}

ADemo0908_Quartz::ADemo0908_Quartz()
{
	DemoId = TEXT("0908");
	Controls = TEXT("SpaceBar : 타악기 요청 · 2 : 자동 요청(박과 어긋난 시각) ON/OFF\n")
		TEXT("C : PlayQuantized(다음 박) ↔ 즉시 Play 비교\n")
		TEXT("관찰: 회색 점 = 누른 시각, 주황 점 = 실제 소리 난 시각. Quantize 면 주황 점이 항상 박 경계(흰 선)에 붙는다");
	Showcase = TEXT("Quartz Clock BPM 120, 아무 때나 눌러도 PlayQuantized 로 다음 박에 타악기 — 박 패드·누름/재생 마커·대기 ms");
	PlayerPawnClass = nullptr;
}

void ADemo0908_Quartz::BeginPlay()
{
	Super::BeginPlay();
	GlossaryAudioKit::PrepareAudioDemo();
	UWorld* World = GetWorld();
	Camera = GlossaryAudioKit::SpawnFixedCamera(World, FVector(-500.f, 0.f, 420.f), FRotator(-20.f, 0.f, 0.f), 72.f);
	Percussion = LoadAsset<USoundWave>(GlossaryAudioKit::AssetPath(TEXT("S_Percussion")));

	// 4박 패드 + 박 경계선
	for (int32 i = 0; i < 4; ++i)
	{
		UStaticMeshComponent* Pad = AddShape(this, EShape::Cube, FVector(PadX, BarToY(i + 0.5f), 40.f), FVector(220.f, 270.f, 80.f), PadColor);
		Pads.Add(Pad);
		PadMats.Add(SetColor(Pad, PadColor, 0.5f, 0.f, 0.f));
		SpawnLabel(World, FString::Printf(TEXT("%d박"), i + 1), FVector(PadX - 140.f, BarToY(i + 0.5f), 10.f), FLinearColor::White, 18.f);
	}
	for (int32 i = 0; i <= 4; ++i)
	{
		UStaticMeshComponent* Line = AddShape(this, EShape::Cube, FVector(PadX, BarToY(i), 200.f), FVector(8.f, 8.f, 320.f), FLinearColor::White);
		SetColor(Line, FLinearColor::White, 0.5f, 0.f, 2.f);
	}
	Playhead = AddShape(this, EShape::Cube, FVector(PadX - 20.f, BarToY(0.f), 200.f), FVector(12.f, 14.f, 360.f), Colors::Cyan);
	SetColor(Playhead, Colors::Cyan, 0.5f, 0.f, 6.f);
	for (int32 i = 0; i < MarkerCount; ++i)
	{
		UStaticMeshComponent* Marker = AddShape(this, EShape::Sphere, FVector(PadX - 40.f, 0.f, -200.f), FVector(50.f, 50.f, 50.f), PressColor);
		Marker->SetVisibility(false);
		Marker->SetCastShadow(false);
		Markers.Add(Marker);
	}
	SpawnLabel(World, TEXT("● 회색 = 누른 시각   ● 주황 = 실제 재생 시각"), FVector(PadX, 0.f, 420.f), Colors::Item, 18.f);

	// 타악기 플레이어 4개 (번갈아 사용 — 같은 박에 여러 요청이 겹쳐도 서로 끊지 않게)
	for (int32 i = 0; i < 4; ++i)
	{
		UAudioComponent* Player = NewObject<UAudioComponent>(this);
		Player->bAutoActivate = false;
		Player->SetupAttachment(Root);
		Player->RegisterComponent();
		Player->SetSound(Percussion);
		Players.Add(Player);
	}

	// ---- Quartz 클락: BPM 120, 4/4
	if (UQuartzSubsystem* Quartz = UQuartzSubsystem::Get(World))
	{
		FQuartzClockSettings Settings;
		Clock = Quartz->CreateNewClock(this, ClockName, Settings, /*bOverrideSettingsIfClockExists*/ true);
	}
	if (Clock)
	{
		FQuartzQuantizationBoundary Immediately;
		Immediately.Quantization = EQuartzCommandQuantization::None;
		UQuartzClockHandle* ClockRef = Clock;
		Clock->SetBeatsPerMinute(this, Immediately, FOnQuartzCommandEventBP(), ClockRef, BeatsPerMinute);

		// 박마다 OnBeat (패드 번쩍)
		FOnQuartzMetronomeEventBP OnBeatDelegate;
		OnBeatDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ADemo0908_Quartz, OnBeat));
		Clock->SubscribeToQuantizationEvent(this, EQuartzCommandQuantization::Beat, OnBeatDelegate, ClockRef);
		Clock->StartClock(this, ClockRef);
	}

	Panel = UGlossaryAudioPanel::Create(this, TEXT("Quartz Clock"));
	if (Panel)
	{
		Panel->AddRow(TEXT("박 (1~4)"), Colors::Cyan);
		Panel->AddRow(TEXT("다음 박까지 대기"), Colors::Accent);
		Panel->AddRow(TEXT("타악기"), Colors::Item);
		Panel->SetFooter(FString::Printf(TEXT("BPM %.0f → 박 간격 %.0f ms\nAudio->PlayQuantized(Clock, Beat 경계)"), BeatsPerMinute, 60000.f / BeatsPerMinute));
		Panel->AddToViewport(1);
	}
}

void ADemo0908_Quartz::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Clock)
	{
		UQuartzClockHandle* ClockRef = Clock;
		Clock->StopClock(this, true, ClockRef);
		if (UQuartzSubsystem* Quartz = UQuartzSubsystem::Get(GetWorld()))
		{
			Quartz->DeleteClockByName(this, ClockName);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0908_Quartz::OnBeat(FName InClockName, EQuartzCommandQuantization QuantizationType, int32 NumBars, int32 Beat, float BeatFraction)
{
	LastBeat = FMath::Clamp(Beat, 1, 4);
	LastBeatTime = FPlatformTime::Seconds();
	++BeatCount;
	PadFlash[LastBeat - 1] = 1.f;
}

float ADemo0908_Quartz::GetBarPosition() const
{
	if (LastBeatTime < 0.0)
	{
		return 0.f;
	}
	const float BeatDuration = 60.f / BeatsPerMinute;
	const float Into = FMath::Clamp(static_cast<float>(FPlatformTime::Seconds() - LastBeatTime) / BeatDuration, 0.f, 0.999f);
	return (LastBeat - 1) + Into;
}

void ADemo0908_Quartz::AddMarker(float BarPosition, bool bPlayed)
{
	UStaticMeshComponent* Marker = Markers[MarkerCursor];
	MarkerCursor = (MarkerCursor + 1) % Markers.Num();
	Marker->SetVisibility(true);
	Marker->SetRelativeLocation(FVector(PadX - 140.f, BarToY(BarPosition), bPlayed ? 170.f : 300.f));
	SetColor(Marker, bPlayed ? Colors::Accent : PressColor, 0.5f, 0.f, bPlayed ? 0.8f : 0.f);
}

void ADemo0908_Quartz::Trigger()
{
	if (Players.Num() == 0 || !Percussion)
	{
		return;
	}
	UAudioComponent* Player = Players[NextPlayer];
	NextPlayer = (NextPlayer + 1) % Players.Num();
	AddMarker(GetBarPosition(), false);

	if (bQuantize && Clock)
	{
		// 핵심: 지금 바로가 아니라 "다음 Beat 경계"에 재생하도록 오디오 렌더 스레드에 예약한다
		FQuartzQuantizationBoundary NextBeat;
		NextBeat.Quantization = EQuartzCommandQuantization::Beat;
		NextBeat.Multiplier = 1.f;
		NextBeat.CountingReferencePoint = EQuarztQuantizationReference::BarRelative;
		FOnQuartzCommandEventBP OnCommand;
		OnCommand.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ADemo0908_Quartz, OnQuantizedCommand));
		UQuartzClockHandle* ClockRef = Clock;
		PendingPressTimes.Add(FPlatformTime::Seconds());
		PendingPressPositions.Add(GetBarPosition());
		Player->PlayQuantized(this, ClockRef, NextBeat, OnCommand);
	}
	else
	{
		Player->Play();
		LastWaitMs = 0.f;
		HitFlash = 1.f;
		AddMarker(GetBarPosition(), true);
	}
}

void ADemo0908_Quartz::OnQuantizedCommand(EQuartzCommandDelegateSubType EventType, FName Name)
{
	if (EventType == EQuartzCommandDelegateSubType::CommandOnStarted && PendingPressTimes.Num() > 0)
	{
		const double Pressed = PendingPressTimes[0];
		const float PressPos = PendingPressPositions.Num() > 0 ? PendingPressPositions[0] : GetBarPosition();
		PendingPressTimes.RemoveAt(0);
		if (PendingPressPositions.Num() > 0)
		{
			PendingPressPositions.RemoveAt(0);
		}
		LastWaitMs = static_cast<float>((FPlatformTime::Seconds() - Pressed) * 1000.0);
		HitFlash = 1.f;
		// 소리는 오디오 렌더 스레드에서 "누른 뒤 다음 Beat 경계"에 정확히 시작한다.
		// 시작 알림은 게임 스레드에 조금 늦게 도착하므로, 마커는 누른 위치의 다음 박 경계에 놓는다
		float NextBeat = FMath::FloorToFloat(PressPos) + 1.f;
		if (NextBeat >= 4.f)
		{
			NextBeat = 0.f;
		}
		AddMarker(NextBeat, true);
	}
	else if (EventType == EQuartzCommandDelegateSubType::CommandOnFailedToQueue || EventType == EQuartzCommandDelegateSubType::CommandOnCanceled)
	{
		if (PendingPressTimes.Num() > 0)
		{
			PendingPressTimes.RemoveAt(0);
		}
		if (PendingPressPositions.Num() > 0)
		{
			PendingPressPositions.RemoveAt(0);
		}
	}
}

void ADemo0908_Quartz::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	GlossaryAudioKit::KeepViewTarget(this, Camera);

	if (WasKeyPressed(EKeys::C))
	{
		bQuantize = !bQuantize;
	}
	if (WasKeyPressed(EKeys::Two))
	{
		bAutoPress = !bAutoPress;
	}
	if (WasKeyPressed(EKeys::SpaceBar))
	{
		Trigger();
	}
	if (bAutoPress)
	{
		// 박과 상관없는 들쭉날쭉한 시각에 요청 (사람이 대충 누르는 것처럼)
		AutoTimer += DeltaSeconds;
		if (AutoTimer >= NextAutoDelay)
		{
			AutoTimer = 0.f;
			NextAutoDelay = Random.FRandRange(0.55f, 1.3f);
			Trigger();
		}
	}

	// 패드 번쩍임 + 재생 헤드
	for (int32 i = 0; i < 4; ++i)
	{
		PadFlash[i] = FMath::Max(0.f, PadFlash[i] - DeltaSeconds * 3.f);
		const bool bCurrent = (i == LastBeat - 1);
		const FLinearColor Color = FMath::Lerp(bCurrent ? Colors::Cyan * 0.4f : PadColor, Colors::Cyan, PadFlash[i]);
		PadMats[i]->SetVectorParameterValue(TEXT("Color"), Color);
		PadMats[i]->SetScalarParameterValue(TEXT("Emissive"), PadFlash[i] * 4.f + (bCurrent ? 0.4f : 0.f));
		Pads[i]->SetRelativeScale3D(FVector(2.2f, 2.7f, 0.8f + PadFlash[i] * 0.6f));
	}
	HitFlash = FMath::Max(0.f, HitFlash - DeltaSeconds * 3.f);
	const float BarPos = GetBarPosition();
	if (Playhead)
	{
		Playhead->SetRelativeLocation(FVector(PadX - 20.f, BarToY(BarPos), 200.f));
	}

	const float BeatMs = 60000.f / BeatsPerMinute;
	if (Panel)
	{
		Panel->SetRow(0, LastBeat / 4.f, LastBeatTime < 0.0 ? TEXT("클락 대기") : FString::Printf(TEXT("%d / 4"), LastBeat));
		Panel->SetRow(1, LastWaitMs / BeatMs, FString::Printf(TEXT("%.0f ms"), LastWaitMs));
		Panel->SetRow(2, HitFlash, HitFlash > 0.f ? TEXT("쿵!") : TEXT(""));
	}
	SetStatus(FString::Printf(TEXT("Quartz Clock %s · BPM %.0f · 박 %d (총 %d) · 모드: %s\n마지막 요청 → 재생 대기 %.0f ms%s"),
		Clock ? TEXT("실행 중") : TEXT("없음(오디오 장치?)"), BeatsPerMinute, LastBeat, BeatCount,
		bQuantize ? TEXT("PlayQuantized(다음 Beat)") : TEXT("즉시 Play"), LastWaitMs,
		bQuantize ? TEXT(" — 어긋나게 눌러도 박에 맞춰 난다") : TEXT(" — 누른 순간 바로 나서 박과 어긋난다")));
}
