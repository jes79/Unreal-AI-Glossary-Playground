#include "Demo0706_Patrol.h"

#include "AIController.h"
#include "Camera/CameraActor.h"
#include "Components/SplineComponent.h"
#include "D07Common.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GlossaryKit.h"
#include "Navigation/PathFollowingComponent.h"

using namespace GlossaryKit;

ADemo0706_Patrol::ADemo0706_Patrol()
{
	DemoId = TEXT("0706");
	Controls = TEXT("C : 이동 방식 전환 (Spline 곡선 따라가기 ↔ NavMesh MoveTo 직선)  · V 카메라 전환\n")
		TEXT("관찰: 하늘색 Spline 을 따라 순찰 지점 4곳(원판)을 돌고, 지점마다 2초 멈춰 두리번거린다");
	Showcase = TEXT("닫힌 Spline 의 4개 점을 순찰 지점으로, 곡선을 따라 걷고 지점마다 2초 대기 (MoveTo 직선 방식과 비교)");
	bNeedsNavMesh = true;

	// 순찰 경로 = 스플라인 컴포넌트 (에디터에서 점을 끌어 경로를 바꿀 수 있다)
	PatrolSpline = CreateDefaultSubobject<USplineComponent>(TEXT("PatrolSpline"));
	PatrolSpline->SetupAttachment(Root);
	PatrolSpline->ClearSplinePoints(false);
	PatrolSpline->AddSplinePoint(FVector(-100.f, -700.f, 5.f), ESplineCoordinateSpace::Local, false);
	PatrolSpline->AddSplinePoint(FVector(1000.f, -700.f, 5.f), ESplineCoordinateSpace::Local, false);
	PatrolSpline->AddSplinePoint(FVector(1000.f, 700.f, 5.f), ESplineCoordinateSpace::Local, false);
	PatrolSpline->AddSplinePoint(FVector(-100.f, 700.f, 5.f), ESplineCoordinateSpace::Local, false);
	PatrolSpline->SetClosedLoop(true, false);
	PatrolSpline->UpdateSpline();
}

void ADemo0706_Patrol::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	const int32 NumPoints = PatrolSpline->GetNumberOfSplinePoints();
	for (int32 Index = 0; Index < NumPoints; ++Index)
	{
		const FVector Point = PatrolSpline->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::World);
		AStaticMeshActor* Pad = SpawnShape(World, EShape::Cylinder, FVector(Point.X, Point.Y, 3.f), FVector(110.f, 110.f, 6.f), Colors::Cyan);
		Pad->SetActorEnableCollision(false);
		SpawnLabel(World, FString::Printf(TEXT("순찰 지점 %d"), Index), FVector(Point.X, Point.Y, 60.f), Colors::Cyan, 16.f);
	}
	SpawnBlock(World, FVector(450.f, 0.f, 0.f), FVector(400.f, 400.f, 150.f), Colors::Platform);

	const FVector Start = PatrolSpline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
	Enemy = D07::SpawnEnemy(World, FVector(Start.X, Start.Y, 0.f), 0.f, AAIController::StaticClass(), Colors::Enemy);
	Enemy->GetCharacterMovement()->MaxWalkSpeed = 260.f;   // 순찰은 걷는 속도

	OverviewCamera = D07::SetupOverviewCamera(this, FVector(-1300.f, -380.f, 1900.f), FRotator(-52.f, 8.f, 0.f));
}

void ADemo0706_Patrol::ArriveAtPoint()
{
	WaitRemaining = WaitTime;
	bMoveIssued = false;
	NextPoint = (NextPoint + 1) % PatrolSpline->GetNumberOfSplinePoints();
	if (NextPoint == 1)
	{
		++Laps;
	}
}

void ADemo0706_Patrol::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* World = GetWorld();
	if (WasKeyPressed(EKeys::V))
	{
		D07::ToggleOverviewCamera(this, OverviewCamera);
	}
	AAIController* AIC = Enemy ? Cast<AAIController>(Enemy->GetController()) : nullptr;
	if (!AIC)
	{
		return;
	}
	if (WasKeyPressed(EKeys::C))
	{
		bFollowSplineCurve = !bFollowSplineCurve;
		AIC->StopMovement();
		bMoveIssued = false;
	}

	// 스플라인 그리기
	const float Length = PatrolSpline->GetSplineLength();
	FVector Prev = PatrolSpline->GetLocationAtDistanceAlongSpline(0.f, ESplineCoordinateSpace::World);
	for (int32 Step = 1; Step <= 96; ++Step)
	{
		const FVector Next = PatrolSpline->GetLocationAtDistanceAlongSpline(Length * Step / 96.f, ESplineCoordinateSpace::World);
		DrawDebugLine(World, Prev, Next, bFollowSplineCurve ? FColor(60, 200, 255) : FColor(60, 110, 140), false, -1.f, 0, 5.f);
		Prev = Next;
	}

	const FVector PawnLocation = Enemy->GetActorLocation();
	const FVector Target = PatrolSpline->GetLocationAtSplinePoint(NextPoint, ESplineCoordinateSpace::World);
	FString Action;

	if (WaitRemaining > 0.f)
	{
		// 지점에서 대기: 좌우로 두리번
		WaitRemaining -= DeltaSeconds;
		Enemy->AddActorWorldRotation(FRotator(0.f, FMath::Sin(WaitRemaining * 3.f) * 120.f * DeltaSeconds, 0.f));
		Action = FString::Printf(TEXT("대기 %.1f초"), FMath::Max(0.f, WaitRemaining));
		Enemy->SetMarkerColor(Colors::Item);
	}
	else if (bFollowSplineCurve)
	{
		// 곡선 따라가기: 스플라인에서 내 위치에 가장 가까운 거리 + LookAhead 지점을 향해 걷는다(목표 지점은 넘지 않음)
		const float Key = PatrolSpline->FindInputKeyClosestToWorldLocation(PawnLocation);
		float MyDistance = PatrolSpline->GetDistanceAlongSplineAtSplineInputKey(Key);
		float TargetDistance = NextPoint == 0 ? Length : PatrolSpline->GetDistanceAlongSplineAtSplinePoint(NextPoint);
		if (MyDistance > TargetDistance + 1.f)
		{
			MyDistance = 0.f;   // 고리의 끝(0번 점) 근처에서 0 으로 감긴 경우
		}
		const float CarrotDistance = FMath::Min(MyDistance + LookAhead, TargetDistance);
		const FVector Carrot = PatrolSpline->GetLocationAtDistanceAlongSpline(FMath::Fmod(CarrotDistance, Length), ESplineCoordinateSpace::World);
		DrawDebugSphere(World, Carrot + FVector(0.f, 0.f, 20.f), 18.f, 8, FColor::White, false, -1.f, 0, 2.f);
		Enemy->AddMovementInput((Carrot - PawnLocation).GetSafeNormal2D(), 1.f);
		Action = FString::Printf(TEXT("Spline 따라 지점 %d 로"), NextPoint);
		Enemy->SetMarkerColor(Colors::Cyan);
		if (FVector::Dist2D(PawnLocation, Target) < 45.f)
		{
			ArriveAtPoint();
		}
	}
	else
	{
		// 비교: NavMesh MoveTo — 지점 사이를 최단(직선) 경로로 간다
		if (!bMoveIssued)
		{
			AIC->MoveToLocation(Target, 30.f);
			bMoveIssued = true;
		}
		D07::DrawCurrentPath(AIC, FColor(255, 160, 40), 4.f);
		Action = FString::Printf(TEXT("MoveTo 지점 %d"), NextPoint);
		Enemy->SetMarkerColor(Colors::Accent);
		if (AIC->GetMoveStatus() == EPathFollowingStatus::Idle && FVector::Dist2D(PawnLocation, Target) < 90.f)
		{
			ArriveAtPoint();
		}
	}
	Enemy->SetLabel(Action, WaitRemaining > 0.f ? Colors::Item : FLinearColor::White);

	SetStatus(FString::Printf(TEXT("방식: %s · 지점 %d개 · 대기 %.0f초\n현재: %s · 다음 지점 %d · 바퀴 %d"),
		bFollowSplineCurve ? TEXT("Spline 곡선 따라가기") : TEXT("NavMesh MoveTo (직선)"),
		PatrolSpline->GetNumberOfSplinePoints(), WaitTime, *Action, NextPoint, Laps));
}
