#include "Demo0705_AIPerception.h"

#include "Camera/CameraActor.h"
#include "D07Common.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GlossaryKit.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"

using namespace GlossaryKit;

// ============================================================ 컨트롤러

AD0705_PerceptionController::AD0705_PerceptionController()
{
	PrimaryActorTick.bCanEverTick = true;

	UAIPerceptionComponent* Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));

	// 눈: 반경 1500, 시야각 90° (PeripheralVisionAngleDegrees 는 정면 기준 반각이라 45)
	UAISenseConfig_Sight* Sight = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight"));
	Sight->SightRadius = SightRadius;
	Sight->LoseSightRadius = SightRadius + 300.f;
	Sight->PeripheralVisionAngleDegrees = SightHalfAngle;
	Sight->DetectionByAffiliation.bDetectEnemies = true;
	Sight->DetectionByAffiliation.bDetectNeutrals = true;
	Sight->DetectionByAffiliation.bDetectFriendlies = true;
	Sight->SetMaxAge(5.f);
	Perception->ConfigureSense(*Sight);

	// 귀: MakeNoise / ReportNoiseEvent 로 알린 소리를 HearingRange × Loudness 안에서 듣는다
	UAISenseConfig_Hearing* Hearing = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("Hearing"));
	Hearing->HearingRange = HearingRange;
	Hearing->DetectionByAffiliation.bDetectEnemies = true;
	Hearing->DetectionByAffiliation.bDetectNeutrals = true;
	Hearing->DetectionByAffiliation.bDetectFriendlies = true;
	Hearing->SetMaxAge(3.f);
	Perception->ConfigureSense(*Hearing);

	Perception->SetDominantSense(Sight->GetSenseImplementation());
	SetPerceptionComponent(*Perception);
}

void AD0705_PerceptionController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	HomeLocation = InPawn->GetActorLocation();
	PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AD0705_PerceptionController::OnTargetPerceptionUpdated);
}

void AD0705_PerceptionController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (Actor != UGameplayStatics::GetPlayerPawn(this, 0))
	{
		return;
	}
	// 어떤 감각의 자극인지는 Stimulus.Type (Sense ID) 로 구분한다
	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
	{
		bSeesPlayer = Stimulus.WasSuccessfullySensed();
		LastSeenLocation = Stimulus.StimulusLocation;
		if (bSeesPlayer)
		{
			State = ED0705_State::Chase;
		}
		else if (State == ED0705_State::Chase)
		{
			// 놓쳤다 → 마지막으로 본 위치로 가 본다
			State = ED0705_State::Search;
			MoveToLocation(LastSeenLocation, 60.f);
			StateTimer = 0.f;
		}
	}
	else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>() && Stimulus.WasSuccessfullySensed())
	{
		LastHeardLocation = Stimulus.StimulusLocation;
		LastHeardTime = GetWorld()->GetTimeSeconds();
		++HeardCount;
		if (State != ED0705_State::Chase)
		{
			State = ED0705_State::Investigate;
			MoveToLocation(LastHeardLocation, 80.f);
			StateTimer = 0.f;
		}
	}
}

void AD0705_PerceptionController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APawn* MyPawn = GetPawn();
	if (!MyPawn)
	{
		return;
	}
	StateTimer += DeltaSeconds;
	switch (State)
	{
	case ED0705_State::Chase:
		if (GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			MoveToActor(UGameplayStatics::GetPlayerPawn(this, 0), 120.f);
		}
		break;
	case ED0705_State::Investigate:
	case ED0705_State::Search:
		// 도착하면 2초 두리번거린 뒤 제자리로
		if (GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			MyPawn->AddActorWorldRotation(FRotator(0.f, 90.f * DeltaSeconds, 0.f));
			if (StateTimer > 4.f)
			{
				State = ED0705_State::Idle;
				MoveToLocation(HomeLocation, 40.f);
			}
		}
		break;
	case ED0705_State::Idle:
		if (GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			// 원래 경계 방향(+Y)으로 천천히 돌아선다
			const FRotator Current = MyPawn->GetActorRotation();
			MyPawn->SetActorRotation(FMath::RInterpConstantTo(Current, FRotator(0.f, HomeYaw, 0.f), DeltaSeconds, 120.f));
		}
		break;
	}
}

// ============================================================ 데모

ADemo0705_AIPerception::ADemo0705_AIPerception()
{
	DemoId = TEXT("0705");
	Controls = TEXT("WASD 이동(걸으면 발소리 MakeNoise) · N : 큰 소리 · V 카메라 전환\n")
		TEXT("관찰: 파란 원(소리가 닿는 범위) 안에 적이 있으면 들려서 조사하러 오고,\n")
		TEXT("노란 시야 부채꼴(1500, 90°)에 들어가면 빨갛게 바뀌며 추격. 벽 뒤에 숨으면 놓친다");
	Showcase = TEXT("AIPerception Sight(1500·90°)+Hearing, 발소리 MakeNoise 로 조사 → 보이면 추격, 시야 부채꼴·소리 범위 표시");
	bNeedsNavMesh = true;
}

void ADemo0705_AIPerception::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 숨을 수 있는 벽
	SpawnBlock(World, FVector(100.f, 500.f, 0.f), FVector(60.f, 700.f, 260.f), Colors::Wall);
	SpawnBlock(World, FVector(-200.f, -700.f, 0.f), FVector(500.f, 60.f, 260.f), Colors::Wall);

	Enemy = D07::SpawnEnemy(World, FVector(700.f, -300.f, 0.f), 90.f, AD0705_PerceptionController::StaticClass(), Colors::Enemy);

	// 플레이어를 감각 자극의 출처로 등록 (Pawn 은 기본으로도 시야 대상이지만 명시적으로 보여 준다)
	if (APawn* Player = GetPlayerPawn())
	{
		UAIPerceptionStimuliSourceComponent* Source = NewObject<UAIPerceptionStimuliSourceComponent>(Player);
		Source->RegisterComponent();
		Source->RegisterForSense(UAISense_Sight::StaticClass());
		Source->RegisterForSense(UAISense_Hearing::StaticClass());
		Source->RegisterWithPerceptionSystem();
	}

	OverviewCamera = D07::SetupOverviewCamera(this, FVector(-1400.f, -100.f, 2100.f), FRotator(-58.f, 0.f, 0.f));
}

void ADemo0705_AIPerception::EmitNoise(float Loudness)
{
	APawn* Player = GetPlayerPawn();
	if (!Player)
	{
		return;
	}
	// 핵심: 소리를 낸다 → 듣는 AI 의 Hearing 감각에 자극이 전달된다 (UAISense_Hearing::ReportNoiseEvent 와 같은 경로)
	Player->MakeNoise(Loudness, Player, Player->GetActorLocation());
	++NoiseCount;
	Rings.Add({ Player->GetActorLocation() - FVector(0.f, 0.f, 90.f), HearingRange * Loudness, 0.f });
}

void ADemo0705_AIPerception::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* World = GetWorld();
	if (WasKeyPressed(EKeys::V))
	{
		D07::ToggleOverviewCamera(this, OverviewCamera);
	}
	if (WasKeyPressed(EKeys::N))
	{
		EmitNoise(1.f);
	}

	// 발소리: 땅 위에서 걷는 동안 일정 간격으로
	if (const ACharacter* Player = GetPlayerCharacter())
	{
		if (Player->GetCharacterMovement()->IsMovingOnGround() && Player->GetVelocity().Size2D() > 100.f)
		{
			FootstepTimer += DeltaSeconds;
			if (FootstepTimer >= FootstepInterval)
			{
				FootstepTimer = 0.f;
				EmitNoise(FootstepLoudness);
			}
		}
	}

	// 소리 링: 0.6초 동안 퍼지며 사라진다 (반지름 = 들리는 거리)
	for (int32 Index = Rings.Num() - 1; Index >= 0; --Index)
	{
		FNoiseRing& Ring = Rings[Index];
		Ring.Age += DeltaSeconds;
		const float T = FMath::Clamp(Ring.Age / 0.6f, 0.f, 1.f);
		DrawDebugCircle(World, Ring.Location, Ring.Radius * T, 64, FColor(80, 170, 255), false, -1.f, 0, 4.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
		if (Ring.Age > 0.9f)
		{
			Rings.RemoveAt(Index);
		}
	}

	AD0705_PerceptionController* AIC = Enemy ? Cast<AD0705_PerceptionController>(Enemy->GetController()) : nullptr;
	if (!AIC)
	{
		return;
	}

	// 시야 부채꼴: 반경 1500, 좌우 45°. 보이면 빨강
	const FVector Eye = Enemy->GetActorLocation() + FVector(0.f, 0.f, -80.f);
	const FVector Forward = Enemy->GetActorForwardVector();
	const FColor SightColor = AIC->bSeesPlayer ? FColor::Red : FColor(255, 220, 60);
	const float Half = SightAngle * 0.5f;
	FVector Prev = Eye + Forward.RotateAngleAxis(-Half, FVector::UpVector) * SightRadius;
	DrawDebugLine(World, Eye, Prev, SightColor, false, -1.f, 0, 4.f);
	for (int32 Step = 1; Step <= 18; ++Step)
	{
		const float Angle = -Half + SightAngle * Step / 18.f;
		const FVector Next = Eye + Forward.RotateAngleAxis(Angle, FVector::UpVector) * SightRadius;
		DrawDebugLine(World, Prev, Next, SightColor, false, -1.f, 0, 4.f);
		Prev = Next;
	}
	DrawDebugLine(World, Eye, Prev, SightColor, false, -1.f, 0, 4.f);
	if (AIC->bSeesPlayer && GetPlayerPawn())
	{
		DrawDebugLine(World, Enemy->GetActorLocation() + FVector(0.f, 0.f, 60.f), GetPlayerPawn()->GetActorLocation(), FColor::Red, false, -1.f, 0, 2.f);
	}
	const float SinceHeard = World->GetTimeSeconds() - AIC->LastHeardTime;
	if (SinceHeard < 4.f)
	{
		DrawDebugSphere(World, AIC->LastHeardLocation, 40.f, 12, FColor(80, 170, 255), false, -1.f, 0, 2.f);
	}
	D07::DrawCurrentPath(AIC, FColor(255, 160, 40), 4.f);

	const TCHAR* StateName = TEXT("Idle (경계)");
	FLinearColor StateColor = Colors::Ally;
	switch (AIC->State)
	{
	case ED0705_State::Investigate: StateName = TEXT("Investigate (소리 조사)"); StateColor = Colors::Cyan; break;
	case ED0705_State::Chase:       StateName = TEXT("Chase (보임 → 추격)"); StateColor = Colors::Enemy; break;
	case ED0705_State::Search:      StateName = TEXT("Search (놓친 곳 확인)"); StateColor = Colors::Item; break;
	default: break;
	}
	Enemy->SetMarkerColor(StateColor);
	Enemy->SetLabel(StateName, StateColor);

	SetStatus(FString::Printf(TEXT("상태 %s · Sight %s\nHearing: 들은 횟수 %d · 마지막 소리 %s · 낸 소리 %d회 (발소리 Loudness %.1f → %.0fcm)"),
		StateName, AIC->bSeesPlayer ? TEXT("보임") : TEXT("안 보임"), AIC->HeardCount,
		SinceHeard < 100.f ? *FString::Printf(TEXT("%.1f초 전"), SinceHeard) : TEXT("없음"),
		NoiseCount, FootstepLoudness, HearingRange * FootstepLoudness));
}
