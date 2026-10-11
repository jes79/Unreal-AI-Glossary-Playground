#include "Demo0712_DetourCrowd.h"

#include "AIController.h"
#include "Camera/CameraActor.h"
#include "D07Common.h"
#include "DetourCrowdAIController.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GlossaryKit.h"
#include "Navigation/PathFollowingComponent.h"

using namespace GlossaryKit;

ADemo0712_DetourCrowd::ADemo0712_DetourCrowd()
{
	DemoId = TEXT("0712");
	Controls = TEXT("C : ADetourCrowdAIController ↔ 일반 AIController 전환 (다시 스폰)\n")
		TEXT("관찰: 빨강 15명·보라 15명이 좁은 통로에서 엇갈린다. Detour Crowd 는 서로 비켜 줄지어 지나가고,\n")
		TEXT("일반 컨트롤러는 경로만 따라가 가운데서 부딪혀 밀리거나 멈춘다");
	Showcase = TEXT("좁은 통로에서 30명이 엇갈릴 때 DetourCrowdAIController(비켜 감) vs 일반 AIController(충돌·정체) 비교");
	bNeedsNavMesh = true;
	PlayerPawnClass = nullptr;
}

void ADemo0712_DetourCrowd::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 좁은 통로 (길이 1400, 폭 400) + 입구 깔때기
	const float HalfWidth = CorridorWidth * 0.5f;
	SpawnBlock(World, FVector(0.f, HalfWidth + 40.f, 0.f), FVector(1400.f, 80.f, 200.f), Colors::Wall);
	SpawnBlock(World, FVector(0.f, -HalfWidth - 40.f, 0.f), FVector(1400.f, 80.f, 200.f), Colors::Wall);
	for (const float Side : { -1.f, 1.f })
	{
		SpawnBlock(World, FVector(Side * 860.f, HalfWidth + 260.f, 0.f), FVector(80.f, 500.f, 200.f), Colors::Wall, FRotator(0.f, Side * 30.f, 0.f));
		SpawnBlock(World, FVector(Side * 860.f, -HalfWidth - 260.f, 0.f), FVector(80.f, 500.f, 200.f), Colors::Wall, FRotator(0.f, -Side * 30.f, 0.f));
	}

	for (int32 Index = 0; Index < AgentsPerSide; ++Index)
	{
		const int32 Column = Index / 5, Row = Index % 5;
		const FVector A(-1300.f - Column * 160.f, -360.f + Row * 180.f, 0.f);
		HomeA.Add(A);
		HomeB.Add(FVector(-A.X, A.Y, 0.f));
	}
	SpawnAgents();

	OverviewCamera = D07::SetupOverviewCamera(this, FVector(0.f, -1700.f, 2500.f), FRotator(-58.f, 90.f, 0.f), 80.f);
}

void ADemo0712_DetourCrowd::SpawnAgents()
{
	for (AD07Enemy* Agent : Agents)
	{
		if (Agent)
		{
			if (AController* C = Agent->GetController())
			{
				C->Destroy();
			}
			Agent->Destroy();
		}
	}
	Agents.Reset();
	GoingToB.Reset();
	RunTime = 0.f;
	Crossings = 0;
	StartDelay = 1.f;

	// 핵심: 컨트롤러 클래스만 바꾼다. DetourCrowdAIController 는 PathFollowingComponent 대신 CrowdFollowingComponent 를 쓴다
	const TSubclassOf<AAIController> ControllerClass = bUseDetourCrowd ? TSubclassOf<AAIController>(ADetourCrowdAIController::StaticClass()) : TSubclassOf<AAIController>(AAIController::StaticClass());
	for (int32 Side = 0; Side < 2; ++Side)
	{
		for (int32 Index = 0; Index < AgentsPerSide; ++Index)
		{
			const FVector Start = Side == 0 ? HomeA[Index] : HomeB[Index];
			AD07Enemy* Agent = D07::SpawnEnemy(GetWorld(), Start, Side == 0 ? 0.f : 180.f, ControllerClass, Side == 0 ? Colors::Enemy : Colors::Purple);
			Agent->GetCharacterMovement()->MaxWalkSpeed = 300.f;
			Agents.Add(Agent);
			GoingToB.Add(Side == 0);
		}
	}
}

void ADemo0712_DetourCrowd::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::V))
	{
		D07::ToggleOverviewCamera(this, OverviewCamera);
	}
	if (WasKeyPressed(EKeys::C))
	{
		bUseDetourCrowd = !bUseDetourCrowd;
		SpawnAgents();
		return;
	}
	StartDelay -= DeltaSeconds;
	if (StartDelay > 0.f)
	{
		SetStatus(TEXT("NavMesh 준비 중..."));
		return;
	}
	RunTime += DeltaSeconds;

	float SpeedSum = 0.f;
	int32 Stuck = 0;
	for (int32 Index = 0; Index < Agents.Num(); ++Index)
	{
		AD07Enemy* Agent = Agents[Index];
		AAIController* AIC = Agent ? Cast<AAIController>(Agent->GetController()) : nullptr;
		if (!AIC)
		{
			continue;
		}
		// 반대편 같은 줄 자리로 갔다가 돌아온다
		const int32 Slot = Index % AgentsPerSide;
		const bool bLeftTeam = Index < AgentsPerSide;
		const FVector Goal = GoingToB[Index] ? HomeB[Slot] : HomeA[Slot];
		if (AIC->GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			if (FVector::Dist2D(Agent->GetActorLocation(), Goal) < 120.f)
			{
				GoingToB[Index] = !GoingToB[Index];
				++Crossings;
			}
			AIC->MoveToLocation(GoingToB[Index] ? HomeB[Slot] : HomeA[Slot], 60.f);
		}
		const float Speed = Agent->GetVelocity().Size2D();
		SpeedSum += Speed;
		if (AIC->GetMoveStatus() == EPathFollowingStatus::Moving && Speed < 40.f)
		{
			++Stuck;
			DrawDebugCircle(GetWorld(), Agent->GetActorLocation() - FVector(0.f, 0.f, 90.f), 55.f, 16, FColor::Yellow, false, -1.f, 0, 4.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
		}
		(void)bLeftTeam;
	}
	const float AvgSpeed = Agents.Num() > 0 ? SpeedSum / Agents.Num() : 0.f;
	SetStatus(FString::Printf(TEXT("컨트롤러: %s · 에이전트 %d명\n평균 속도 %.0f cm/s · 막혀서 거의 못 움직이는 수(노란 원) %d · 도착 %d회 · %.0f초"),
		bUseDetourCrowd ? TEXT("ADetourCrowdAIController (Crowd 회피)") : TEXT("AAIController (회피 없음)"),
		Agents.Num(), AvgSpeed, Stuck, Crossings, RunTime));
}
