#include "Demo0104_Acceleration.h"

#include "AIController.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GlossaryCharacter.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"

using namespace GlossaryKit;

namespace
{
	constexpr float LaneY = 650.f;      // 레인 길이의 절반
	const FVector LaneDefault(250.f, 0.f, 0.f);
	const FVector LaneSlippery(650.f, 0.f, 0.f);
}

ADemo0104_Acceleration::ADemo0104_Acceleration()
{
	DemoId = TEXT("0104");
	Controls = TEXT("WASD 이동 · Space 점프 · C : 내 캐릭터 미끄러운 값 ON/OFF\n")
		TEXT("두 러너가 같은 시간만 달리고 멈춘다: 가까운 줄 = 기본 값, 먼 줄(주황) = 미끄러운 값\n")
		TEXT("관찰: 미끄러운 쪽은 출발이 느리고, 입력을 뗀 뒤에도 한참 미끄러진다(주황 기둥 = 멈춘 자리)");
	Showcase = TEXT("Max Acceleration 1000 · Braking Deceleration 500 · Ground Friction 2 러너와 기본 값 러너의 출발·제동 거리 비교");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-250.f, -750.f, 100.f));
}

ACharacter* ADemo0104_Acceleration::SpawnRunner(const FVector& Location, bool bSlippery)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AGlossaryCharacter* Runner = GetWorld()->SpawnActor<AGlossaryCharacter>(Location + FVector(0.f, 0.f, 100.f), FRotator(0.f, 90.f, 0.f), Params);
	if (!Runner)
	{
		return nullptr;
	}
	// AddMovementInput 이 처리되려면 컨트롤러가 있어야 한다 → 빈 AI 컨트롤러
	Runner->AIControllerClass = AAIController::StaticClass();
	Runner->SpawnDefaultController();
	ApplySettings(Runner, bSlippery);

	FRunner Info;
	Info.Character = Runner;
	Info.bSlippery = bSlippery;
	Info.Label = SpawnLabel(GetWorld(), TEXT(""), Location + FVector(0.f, 0.f, 260.f), bSlippery ? Colors::Accent : Colors::Cyan, 17.f);
	AStaticMeshActor* Marker = SpawnShape(GetWorld(), EShape::Cylinder, Location + FVector(0.f, 0.f, 60.f), FVector(30.f, 30.f, 120.f), bSlippery ? Colors::Accent : Colors::Cyan);
	Marker->SetActorEnableCollision(false);
	Marker->SetActorHiddenInGame(true);
	Info.StopMarker = Marker;
	Runners.Add(Info);
	return Runner;
}

void ADemo0104_Acceleration::ApplySettings(ACharacter* Character, bool bSlippery) const
{
	if (!Character)
	{
		return;
	}
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	// 핵심 세 값: 얼마나 빨리 최고 속도에 닿는지(가속), 입력이 없을 때 얼마나 빨리 서는지(제동·마찰)
	Move->MaxAcceleration = bSlippery ? MaxAcceleration : DefaultMaxAcceleration;
	Move->BrakingDecelerationWalking = bSlippery ? BrakingDecelerationWalking : DefaultBrakingDecelerationWalking;
	Move->GroundFriction = bSlippery ? GroundFriction : DefaultGroundFriction;
	// 제동에도 Ground Friction 을 같이 쓰도록 (기본 true 지만 의도를 명시)
	Move->bUseSeparateBrakingFriction = false;
}

void ADemo0104_Acceleration::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 레인 바닥 + 1m 눈금
	for (const FVector& Lane : { LaneDefault, LaneSlippery })
	{
		const bool bSlip = Lane.X > 400.f;
		SpawnBlock(World, Lane, FVector(160.f, LaneY * 2.f + 200.f, 2.f), bSlip ? Colors::Accent * 0.35f : Colors::Cyan * 0.35f);
		for (float Y = -LaneY; Y <= LaneY + 1.f; Y += 100.f)
		{
			SpawnBlock(World, Lane + FVector(-90.f, Y, 0.f), FVector(20.f, 6.f, 4.f), FLinearColor::White);
		}
		SpawnLabel(World, bSlip ? TEXT("미끄러운 값 (가속 1000 · 제동 500 · 마찰 2)") : TEXT("기본 값 (가속 2048 · 제동 2000 · 마찰 8)"),
			Lane + FVector(0.f, -LaneY - 150.f, bSlip ? 330.f : 120.f), bSlip ? Colors::Accent : Colors::Cyan, 16.f);
	}

	SpawnRunner(LaneDefault + FVector(0.f, -LaneY + 100.f, 0.f), false);
	SpawnRunner(LaneSlippery + FVector(0.f, -LaneY + 100.f, 0.f), true);

	ApplySettings(GetPlayerCharacter(), bPlayerSlippery);
}

void ADemo0104_Acceleration::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (WasKeyPressed(EKeys::C))
	{
		bPlayerSlippery = !bPlayerSlippery;
		ApplySettings(GetPlayerCharacter(), bPlayerSlippery);
	}

	// 러너 왕복 스케줄: RunTime 동안 입력, RestTime 동안 입력 없음
	Phase += DeltaSeconds;
	if (bRunning && Phase >= RunTime)
	{
		bRunning = false;
		Phase = 0.f;
		for (FRunner& R : Runners)
		{
			if (ACharacter* C = R.Character.Get()) R.ReleaseX = C->GetActorLocation().Y;
			R.LastSlide = 0.f;
			if (AActor* M = R.StopMarker.Get()) M->SetActorHiddenInGame(true);
		}
	}
	else if (!bRunning && Phase >= RestTime)
	{
		bRunning = true;
		Phase = 0.f;
		Direction = -Direction;
		for (FRunner& R : Runners)
		{
			R.TimeToTop = 0.f;
			R.bReachedTop = false;
		}
	}

	for (FRunner& R : Runners)
	{
		ACharacter* C = R.Character.Get();
		if (!C)
		{
			continue;
		}
		UCharacterMovementComponent* Move = C->GetCharacterMovement();
		const float Speed = Move->Velocity.Size2D();
		if (bRunning)
		{
			// 처음엔 +Y, 쉬고 나면 -Y … (레인 위를 왕복)
			C->AddMovementInput(FVector(0.f, Direction, 0.f), 1.f);
			if (!R.bReachedTop)
			{
				R.TimeToTop += DeltaSeconds;
				R.bReachedTop = Speed >= Move->MaxWalkSpeed * 0.9f;
			}
		}
		else
		{
			R.LastSlide = FMath::Abs(C->GetActorLocation().Y - R.ReleaseX);
			if (Speed < 5.f)
			{
				if (AActor* M = R.StopMarker.Get())
				{
					M->SetActorHiddenInGame(false);
					M->SetActorLocation(FVector(C->GetActorLocation().X - 120.f, C->GetActorLocation().Y, 60.f));
				}
			}
		}
		if (AGlossaryLabel* L = R.Label.Get())
		{
			L->SetActorLocation(C->GetActorLocation() + FVector(0.f, 0.f, R.bSlippery ? 330.f : 130.f));
			L->SetText(FString::Printf(TEXT("%s\n속도 %3.0f · 최고속 도달 %.2fs\n뗀 뒤 미끄러짐 %.0fcm"),
				R.bSlippery ? TEXT("미끄러움") : TEXT("기본"), Speed, R.TimeToTop, R.LastSlide));
		}
	}

	if (ACharacter* Player = GetPlayerCharacter())
	{
		const UCharacterMovementComponent* Move = Player->GetCharacterMovement();
		SetStatus(FString::Printf(TEXT("내 캐릭터: %s — Max Acceleration %.0f · Braking Deceleration Walking %.0f · Ground Friction %.1f\n속도 %.0f / %.0f cm/s · 가속 입력 %s"),
			bPlayerSlippery ? TEXT("미끄러운 값") : TEXT("기본 값"),
			Move->MaxAcceleration, Move->BrakingDecelerationWalking, Move->GroundFriction,
			Move->Velocity.Size2D(), Move->MaxWalkSpeed,
			Move->GetCurrentAcceleration().IsNearlyZero() ? TEXT("없음(제동 중)") : TEXT("있음")));
	}
}
