#include "Demo0909_AudioFade.h"

#include "Camera/CameraActor.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryAudioKit.h"
#include "GlossaryKit.h"
#include "Sound/SoundWave.h"

using namespace GlossaryKit;

namespace
{
	const FVector CalmPos(400.f, -380.f, 0.f);
	const FVector BattlePos(400.f, 380.f, 0.f);
}

ADemo0909_AudioFade::ADemo0909_AudioFade()
{
	DemoId = TEXT("0909");
	Controls = TEXT("C : 전투 시작 / 종료 · 2 : 크로스페이드 ↔ 즉시 교체 비교\n")
		TEXT("관찰: 전투가 시작되면 잔잔한 BGM 이 2초 동안 FadeOut, 전투 BGM 이 2초 동안 FadeIn — 스피커 크기와 막대가 엇갈려 바뀐다");
	Showcase = TEXT("전투 시작 시 잔잔한 BGM FadeOut 2초 + 전투 BGM FadeIn 2초 크로스페이드, 볼륨 막대·Envelope, 즉시 교체 비교");
	PlayerPawnClass = nullptr;
}

void ADemo0909_AudioFade::BeginPlay()
{
	Super::BeginPlay();
	GlossaryAudioKit::PrepareAudioDemo();
	UWorld* World = GetWorld();
	Camera = GlossaryAudioKit::SpawnFixedCamera(World, FVector(-1000.f, 0.f, 360.f), FRotator(-9.f, 0.f, 0.f), 70.f);

	auto MakeSpeaker = [&](const FVector& Base, const FLinearColor& Color, const TCHAR* Label, TObjectPtr<UStaticMeshComponent>& OutCone, TObjectPtr<UStaticMeshComponent>& OutBar)
	{
		SpawnBlock(World, Base, FVector(180.f, 220.f, 60.f), Colors::Dark);
		OutCone = AddShape(this, EShape::Cylinder, Base + FVector(0.f, 0.f, 210.f), FVector(220.f, 220.f, 40.f), Color, nullptr, FRotator(90.f, 0.f, 0.f));
		SetColor(OutCone, Color, 0.4f, 0.f, 2.f);
		OutBar = AddShape(this, EShape::Cube, Base + FVector(-60.f, 170.f, 60.f), FVector(20.f, 34.f, 10.f), Color);
		SetColor(OutBar, Color, 0.4f, 0.f, 3.f);
		SpawnLabel(World, Label, Base + FVector(-140.f, 0.f, 20.f), Color, 17.f);
	};
	MakeSpeaker(CalmPos, Colors::Cyan, TEXT("잔잔한 BGM (S_BGM_Calm_Loop)"), CalmSpeaker, CalmBar);
	MakeSpeaker(BattlePos, Colors::Enemy, TEXT("전투 BGM (S_BGM_Battle_Loop)"), BattleSpeaker, BattleBar);

	auto MakeAudio = [&](const TCHAR* Asset, TObjectPtr<UGlossaryAudioProbe>& OutProbe)
	{
		UAudioComponent* Audio = NewObject<UAudioComponent>(this);
		Audio->bAutoActivate = false;
		Audio->SetupAttachment(Root);
		Audio->RegisterComponent();
		Audio->SetSound(LoadAsset<USoundWave>(GlossaryAudioKit::AssetPath(Asset)));
		OutProbe = NewObject<UGlossaryAudioProbe>(this);
		OutProbe->Bind(Audio);  // FadeIn 도 내부적으로 Play 를 부르므로 미리 바인딩
		return Audio;
	};
	CalmBGM = MakeAudio(TEXT("S_BGM_Calm_Loop"), CalmProbe);
	BattleBGM = MakeAudio(TEXT("S_BGM_Battle_Loop"), BattleProbe);

	// 시작: 잔잔한 BGM 만 1초 FadeIn
	CalmBGM->FadeIn(1.f, 1.f);
	CalmTrack.Start(1.f, 1.f);

	Banner = UGlossaryAudioBanner::Create(this);
	Panel = UGlossaryAudioPanel::Create(this, TEXT("BGM 크로스페이드"));
	if (Panel)
	{
		Panel->AddRow(TEXT("잔잔한 BGM 볼륨"), Colors::Cyan);
		Panel->AddRow(TEXT("  Envelope"), Colors::Cyan * 0.6f);
		Panel->AddRow(TEXT("전투 BGM 볼륨"), Colors::Enemy);
		Panel->AddRow(TEXT("  Envelope"), Colors::Enemy * 0.6f);
		Panel->SetFooter(TEXT("볼륨 = FadeIn/FadeOut 진행(계산), Envelope = 실제 출력(측정)"));
		Panel->AddToViewport(1);
	}
}

void ADemo0909_AudioFade::SetBattle(bool bInBattle)
{
	bBattle = bInBattle;
	UAudioComponent* Out = bBattle ? CalmBGM : BattleBGM;
	UAudioComponent* In = bBattle ? BattleBGM : CalmBGM;
	FFadeTrack& OutTrack = bBattle ? CalmTrack : BattleTrack;
	FFadeTrack& InTrack = bBattle ? BattleTrack : CalmTrack;

	if (bCrossfade)
	{
		// 핵심 두 줄: 나가는 곡은 2초 동안 0 으로(끝나면 자동 정지), 들어오는 곡은 2초 동안 1 로
		Out->FadeOut(FadeOutDuration, 0.f);
		In->FadeIn(FadeInDuration, 1.f);
		OutTrack.Start(0.f, FadeOutDuration);
		InTrack.Start(1.f, FadeInDuration);
	}
	else
	{
		Out->Stop();
		In->Play();
		OutTrack.Start(0.f, 0.f);
		InTrack.Start(1.f, 0.f);
	}

	BannerTime = 2.5f;
	if (Banner)
	{
		Banner->Show(bBattle ? TEXT("전투 시작!") : TEXT("전투 종료"), bBattle ? Colors::Enemy : Colors::Cyan);
	}

	// 전투 상황 표시용 적
	if (bBattle && !Enemy)
	{
		Enemy = SpawnShape(GetWorld(), EShape::Cone, FVector(700.f, 0.f, 100.f), FVector(120.f, 120.f, 200.f), Colors::Enemy);
	}
	else if (!bBattle && Enemy)
	{
		Enemy->Destroy();
		Enemy = nullptr;
	}
}

void ADemo0909_AudioFade::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	GlossaryAudioKit::KeepViewTarget(this, Camera);
	Time += DeltaSeconds;

	if (WasKeyPressed(EKeys::C))
	{
		SetBattle(!bBattle);
	}
	if (WasKeyPressed(EKeys::Two))
	{
		bCrossfade = !bCrossfade;
	}

	CalmTrack.Elapsed += DeltaSeconds;
	BattleTrack.Elapsed += DeltaSeconds;
	const float CalmVolume = CalmTrack.Value();
	const float BattleVolume = BattleTrack.Value();
	const float CalmEnv = CalmProbe ? CalmProbe->GetEnvelope() : 0.f;
	const float BattleEnv = BattleProbe ? BattleProbe->GetEnvelope() : 0.f;

	auto UpdateSpeaker = [](UStaticMeshComponent* Cone, UStaticMeshComponent* Bar, const FVector& Base, float Volume, float Envelope)
	{
		const float Pulse = 1.f + FMath::Clamp(Envelope * 1.5f, 0.f, 0.25f);
		const float Size = FMath::Lerp(0.3f, 1.f, Volume) * Pulse;
		Cone->SetRelativeScale3D(FVector(2.2f * Size, 2.2f * Size, 0.4f));
		const float H = FMath::Max(2.f, Volume * 280.f);
		Bar->SetRelativeScale3D(FVector(0.2f, 0.34f, H / 100.f));
		Bar->SetRelativeLocation(Base + FVector(-60.f, 170.f, 60.f + H * 0.5f));
	};
	UpdateSpeaker(CalmSpeaker, CalmBar, CalmPos, CalmVolume, CalmEnv);
	UpdateSpeaker(BattleSpeaker, BattleBar, BattlePos, BattleVolume, BattleEnv);

	if (Enemy)
	{
		Enemy->SetActorRotation(FRotator(0.f, Time * 120.f, 0.f));
	}
	if (BannerTime > 0.f)
	{
		BannerTime -= DeltaSeconds;
		if (BannerTime <= 0.f && Banner)
		{
			Banner->Hide();
		}
	}

	if (Panel)
	{
		Panel->SetRow(0, CalmVolume, FString::Printf(TEXT("%.0f %%"), CalmVolume * 100.f));
		Panel->SetRow(1, GlossaryAudioKit::AmplitudeToBar(CalmEnv), FString::Printf(TEXT("%.3f"), CalmEnv));
		Panel->SetRow(2, BattleVolume, FString::Printf(TEXT("%.0f %%"), BattleVolume * 100.f));
		Panel->SetRow(3, GlossaryAudioKit::AmplitudeToBar(BattleEnv), FString::Printf(TEXT("%.3f"), BattleEnv));
	}
	const bool bFading = (CalmTrack.Elapsed < CalmTrack.Duration) || (BattleTrack.Elapsed < BattleTrack.Duration);
	SetStatus(FString::Printf(TEXT("%s · 방식: %s%s\n잔잔한 %.0f%% (%s) · 전투 %.0f%% (%s)"),
		bBattle ? TEXT("전투 중") : TEXT("평화"),
		bCrossfade ? TEXT("크로스페이드 (FadeOut 2초 + FadeIn 2초)") : TEXT("즉시 교체 (Stop + Play)"),
		bFading ? TEXT(" · 페이드 진행 중") : TEXT(""),
		CalmVolume * 100.f, CalmBGM && CalmBGM->IsPlaying() ? TEXT("재생") : TEXT("정지"),
		BattleVolume * 100.f, BattleBGM && BattleBGM->IsPlaying() ? TEXT("재생") : TEXT("정지")));
}
