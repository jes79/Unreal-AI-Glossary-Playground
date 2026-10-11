#include "Demo0113_TargetLock.h"

#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"
#include "Kismet/KismetMathLibrary.h"

using namespace GlossaryKit;

ADemo0113_TargetLock::ADemo0113_TargetLock()
{
	DemoId = TEXT("0113");
	Controls = TEXT("WASD 이동 · 마우스 시점 · Tab : 가장 가까운 적 록온 / 해제\n")
		TEXT("관찰: 록온 중에는 카메라(Control Rotation)가 RInterpTo 로 부드럽게 적을 따라가고, 캐릭터는 적을 본 채 옆걸음한다");
	Showcase = TEXT("Tab 으로 가장 가까운 적 록온, Control Rotation 을 RInterpTo 로 적에게 보간·스트레이프 이동, 노란 마커 표시");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-500.f, 0.f, 100.f));
}

void ADemo0113_TargetLock::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 적 허수아비: 가만히 있는 것 + 좌우로 움직이는 것
	struct FSpawn { FVector Pos; float Swing; };
	const FSpawn Spawns[] = {
		{ FVector(400.f, -500.f, 0.f), 0.f },
		{ FVector(700.f, 300.f, 0.f), 500.f },
		{ FVector(1200.f, -200.f, 0.f), 700.f },
		{ FVector(300.f, 900.f, 0.f), 0.f },
		{ FVector(-200.f, -1100.f, 0.f), 300.f },
	};
	for (int32 i = 0; i < UE_ARRAY_COUNT(Spawns); ++i)
	{
		AStaticMeshActor* Body = SpawnShape(World, EShape::Cylinder, Spawns[i].Pos + FVector(0.f, 0.f, 90.f), FVector(80.f, 80.f, 180.f), Colors::Enemy);
		// 머리 (몸에 붙여 같이 움직인다)
		// 몸통 액터는 (0.8, 0.8, 1.8) 로 스케일돼 있으므로 자식 값은 그 비율로 나눈 값 → 지름 60cm 구, 몸 위 30cm
		AddShape(Body, EShape::Sphere, FVector(0.f, 0.f, 67.f), FVector(75.f, 75.f, 33.f), Colors::Enemy * 0.7f);
		AActor* NameLabel = SpawnLabel(World, FString::Printf(TEXT("적 %d"), i + 1), Body->GetActorLocation() + FVector(0.f, 0.f, 150.f), FLinearColor::White, 14.f);
		FEnemy E;
		E.Actor = Body;
		E.Home = Body->GetActorLocation();
		E.Swing = Spawns[i].Swing;
		E.Phase = i * 1.3f;
		E.Name = FString::Printf(TEXT("적 %d"), i + 1);
		E.Label = NameLabel;
		Enemies.Add(E);
	}

	// 록온 마커: 노란 마름모 (회전하는 큐브)
	AStaticMeshActor* M = SpawnShape(World, EShape::Cube, FVector::ZeroVector, FVector(40.f), Colors::Item, FRotator(45.f, 0.f, 45.f));
	M->SetActorEnableCollision(false);
	SetColor(M->GetStaticMeshComponent(), Colors::Item, 0.3f, 0.f, 3.f);
	M->SetActorHiddenInGame(true);
	Marker = M;
}

AActor* ADemo0113_TargetLock::FindNearestEnemy() const
{
	const APawn* Pawn = GetPlayerPawn();
	if (!Pawn)
	{
		return nullptr;
	}
	AActor* Best = nullptr;
	float BestDist = LockRange;
	for (const FEnemy& E : Enemies)
	{
		if (AActor* A = E.Actor.Get())
		{
			const float D = FVector::Dist(A->GetActorLocation(), Pawn->GetActorLocation());
			if (D < BestDist)
			{
				BestDist = D;
				Best = A;
			}
		}
	}
	return Best;
}

void ADemo0113_TargetLock::SetLocked(AActor* Target)
{
	LockedTarget = Target;
	if (ACharacter* Character = GetPlayerCharacter())
	{
		// 록온 중: 이동 방향이 아니라 Control Rotation(=적 방향)을 바라보게 → 적을 본 채 옆걸음
		UCharacterMovementComponent* Move = Character->GetCharacterMovement();
		Move->bOrientRotationToMovement = (Target == nullptr);
		Move->bUseControllerDesiredRotation = (Target != nullptr);
		Move->RotationRate = FRotator(0.f, 720.f, 0.f);
	}
	if (AActor* M = Marker.Get())
	{
		M->SetActorHiddenInGame(Target == nullptr);
	}
}

void ADemo0113_TargetLock::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;

	// 움직이는 적
	for (FEnemy& E : Enemies)
	{
		if (AActor* A = E.Actor.Get(); A && E.Swing > 0.f)
		{
			A->SetActorLocation(E.Home + FVector(0.f, FMath::Sin(Time * 0.8f + E.Phase) * E.Swing, 0.f));
			if (AActor* L = E.Label.Get()) L->SetActorLocation(A->GetActorLocation() + FVector(0.f, 0.f, 150.f));
		}
	}

	if (WasKeyPressed(EKeys::Tab))
	{
		SetLocked(LockedTarget.IsValid() ? nullptr : FindNearestEnemy());
	}

	APlayerController* PC = GetPC();
	APawn* Pawn = GetPlayerPawn();
	AActor* Target = LockedTarget.Get();
	if (Target && Pawn && FVector::Dist(Target->GetActorLocation(), Pawn->GetActorLocation()) > LockRange * 1.2f)
	{
		SetLocked(nullptr);   // 너무 멀어지면 자동 해제
		Target = nullptr;
	}

	float AngleError = 0.f;
	if (Target && Pawn && PC)
	{
		// ★ 핵심: 적을 바라보는 회전을 구하고 Control Rotation 을 RInterpTo 로 조금씩 맞춘다
		FRotator Desired = UKismetMathLibrary::FindLookAtRotation(Pawn->GetActorLocation(), Target->GetActorLocation());
		Desired.Pitch += LockPitchOffset;
		const FRotator Current = PC->GetControlRotation();
		PC->SetControlRotation(FMath::RInterpTo(Current, Desired, DeltaSeconds, LockInterpSpeed));
		AngleError = FMath::Abs(FRotator::NormalizeAxis(Desired.Yaw - Current.Yaw));

		if (AActor* M = Marker.Get())
		{
			M->SetActorLocation(Target->GetActorLocation() + FVector(0.f, 0.f, 190.f + FMath::Sin(Time * 4.f) * 10.f));
			M->SetActorRotation(FRotator(45.f, Time * 180.f, 45.f));
		}
	}

	SetStatus(Target && Pawn
		? FString::Printf(TEXT("록온: %s · 거리 %.0fcm · Interp Speed %.0f\n카메라와 적 방향의 차이 %.1f° (RInterpTo 로 줄어든다)"),
			*EnemyName(Target), FVector::Dist(Target->GetActorLocation(), Pawn->GetActorLocation()), LockInterpSpeed, AngleError)
		: FString::Printf(TEXT("록온 없음 — Tab 으로 %.0fcm 안의 가장 가까운 적을 고정\n가장 가까운 후보: %s"),
			LockRange, *EnemyName(FindNearestEnemy())));
}

FString ADemo0113_TargetLock::EnemyName(const AActor* Actor) const
{
	for (const FEnemy& E : Enemies)
	{
		if (Actor && E.Actor.Get() == Actor)
		{
			return E.Name;
		}
	}
	return TEXT("없음");
}
