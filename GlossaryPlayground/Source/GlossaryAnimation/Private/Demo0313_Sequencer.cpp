#include "Demo0313_Sequencer.h"

#include "CineCameraActor.h"
#include "Components/BoxComponent.h"
#include "D03_Mannequin.h"
#include "Engine/TriggerBox.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieScene.h"

using namespace GlossaryKit;

ADemo0313_Sequencer::ADemo0313_Sequencer()
{
	DemoId = TEXT("0313");
	Controls = TEXT("WASD 로 앞의 초록 발판(Trigger Box)에 들어가면 컷신 재생 · E : 바로 재생\n")
		TEXT("관찰: 5초 동안 카메라 컷 3개(와이드→클로즈업→하이앵글), 처음·끝 페이드, 주인공 공격 애니메이션이\n")
		TEXT("시간축대로 재생되고 끝나면 플레이어 카메라로 돌아온다");
	Showcase = TEXT("코드로 만든 Level Sequence(카메라 컷 3 · 카메라 이동 · 페이드 · 애니메이션 트랙)를 Trigger Box 진입 시 재생, 카메라·주인공은 런타임 바인딩");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-700.f, 0.f, 100.f));
}

void ADemo0313_Sequencer::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 무대: 주인공 Manny 와 기둥들
	Hero = D03::SpawnMannequin(World, FVector(600.f, 0.f, 0.f), 180.f);
	if (Hero) { Hero->SetHeadLabel(TEXT("주인공"), Colors::Item, 16.f); }
	for (int32 i = 0; i < 6; ++i)
	{
		const float A = 2.f * PI * i / 6.f;
		SpawnBlock(World, FVector(600.f, 0.f, 0.f) + FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * 380.f, FVector(60.f, 60.f, 300.f), i % 2 ? Colors::Purple : Colors::Wall);
	}

	// 트리거 박스 (보이도록 바닥에 초록 발판)
	const FVector TriggerLoc(-200.f, 0.f, TriggerExtent.Z);
	Trigger = World->SpawnActor<ATriggerBox>(TriggerLoc, FRotator::ZeroRotator);
	if (Trigger)
	{
		if (UBoxComponent* Box = Cast<UBoxComponent>(Trigger->GetCollisionComponent()))
		{
			Box->SetBoxExtent(TriggerExtent);
		}
		Trigger->OnActorBeginOverlap.AddDynamic(this, &ADemo0313_Sequencer::OnTriggerOverlap);
	}
	SpawnShape(World, EShape::Cube, FVector(TriggerLoc.X, 0.f, 1.f), FVector(TriggerExtent.X * 2.f, TriggerExtent.Y * 2.f, 2.f), Colors::Ally);
	SpawnLabel(World, TEXT("Trigger Box → 컷신"), FVector(TriggerLoc.X, 0.f, 120.f), Colors::Ally, 18.f);

	// 시퀀스 플레이어 + 런타임 바인딩
	Sequence = LoadAsset<ULevelSequence>(D03::Folder + TEXT("LS_Cutscene"));
	if (!Sequence)
	{
		SetStatus(TEXT("LS_Cutscene 없음 — tools/glossary-build.sh -filter=03 먼저"));
		return;
	}
	FMovieSceneSequencePlaybackSettings Settings;
	Settings.bDisableMovementInput = true;   // 컷신 동안 조작 막기
	Settings.bDisableLookAtInput = true;
	ALevelSequenceActor* OutActor = nullptr;
	Player = ULevelSequencePlayer::CreateLevelSequencePlayer(World, Sequence, Settings, OutActor);
	SequenceActor = OutActor;

	UMovieScene* MS = Sequence->GetMovieScene();
	for (int32 i = 0; i < MS->GetPossessableCount(); ++i)
	{
		const FMovieScenePossessable& P = MS->GetPossessable(i);
		AActor* Bound = nullptr;
		if (P.GetName() == TEXT("Hero"))
		{
			Bound = Hero;
		}
		else
		{
			// 이름이 Cam_* 인 Possessable 마다 시네 카메라 하나
			ACineCameraActor* Cam = World->SpawnActor<ACineCameraActor>(FVector(0, 0, 300.f), FRotator::ZeroRotator);
			Cameras.Add(Cam);
			Bound = Cam;
		}
		if (Bound && SequenceActor)
		{
			// 에셋의 Possessable(Guid) 을 이 레벨의 실제 액터로 연결 (Binding Override)
			SequenceActor->SetBinding(FMovieSceneObjectBindingID(UE::MovieScene::FRelativeObjectBindingID(P.GetGuid())), { Bound });
		}
	}
}

void ADemo0313_Sequencer::OnTriggerOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	if (OtherActor && OtherActor == GetPlayerPawn())
	{
		PlayCutscene();
	}
}

void ADemo0313_Sequencer::PlayCutscene()
{
	if (Player && !Player->IsPlaying())
	{
		Player->SetPlaybackPosition(FMovieSceneSequencePlaybackParams(FFrameTime(0), EUpdatePositionMethod::Jump));
		Player->Play();
		++PlayCount;
	}
}

void ADemo0313_Sequencer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::E))
	{
		PlayCutscene();
	}
	if (!Player)
	{
		return;
	}
	const bool bPlaying = Player->IsPlaying();
	const float T = bPlaying ? Player->GetCurrentTime().AsSeconds() : 0.f;
	const TCHAR* Shot = !bPlaying ? TEXT("-") : T < 1.7f ? TEXT("Cam_Wide (와이드)") : T < 3.4f ? TEXT("Cam_Close (클로즈업)") : TEXT("Cam_Orbit (하이앵글)");
	const AActor* View = GetPC() ? GetPC()->GetViewTarget() : nullptr;
	SetStatus(FString::Printf(TEXT("LS_Cutscene %s · %.2f / 5.00 초 · 재생 %d회\n카메라 컷: %s · 뷰 타깃 %s"),
		bPlaying ? TEXT("재생 중") : TEXT("대기"), T, PlayCount, Shot, *GetNameSafe(View)));
}
