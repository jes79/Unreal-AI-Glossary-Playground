#include "Demo0708_NavLink.h"

#include "AIController.h"
#include "Camera/CameraActor.h"
#include "D07Common.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GlossaryKit.h"
#include "NavLinkCustomComponent.h"
#include "Navigation/NavLinkProxy.h"
#include "Navigation/PathFollowingComponent.h"

using namespace GlossaryKit;

namespace
{
	const FVector LinkStart(430.f, 0.f, 300.f);   // 절벽 위 끝
	const FVector LinkEnd(820.f, 0.f, 0.f);       // 절벽 아래
}

/** 절벽 끝의 Smart Link: 단순 링크(PointLinks)는 비우고 Smart Link 하나만 위 → 아래 한 방향으로 */
static ANavLinkProxy* SpawnJumpLink(UWorld* World)
{
	ANavLinkProxy* Proxy = World->SpawnActorDeferred<ANavLinkProxy>(ANavLinkProxy::StaticClass(), FTransform::Identity);
	Proxy->PointLinks.Reset();
	Proxy->bSmartLinkIsRelevant = true;   // Smart Link 를 NavMesh 에 넣는다
	Proxy->GetSmartLinkComp()->SetLinkData(LinkStart, LinkEnd, ENavLinkDirection::LeftToRight);
	Proxy->FinishSpawning(FTransform::Identity);
	return Proxy;
}

#if WITH_EDITOR
void ADemo0708_NavLink::BuildLevel(UWorld* World)
{
	// 레벨에 저장되는 Nav Link Proxy (에디터에서 손으로 놓는 것과 같다)
	SpawnJumpLink(World)->SetActorLabel(TEXT("NavLinkProxy_Jump"));
}
#endif

ADemo0708_NavLink::ADemo0708_NavLink()
{
	DemoId = TEXT("0708");
	Controls = TEXT("C : Smart Link ON/OFF · V 카메라 전환\n")
		TEXT("관찰: 고지대의 AI 가 아래 목표로 갈 때 초록 Nav Link 를 지나는 경로를 골라 절벽에서 점프한다.\n")
		TEXT("링크를 끄면 왼쪽 먼 경사로로 돌아 내려간다 (올라올 때는 항상 경사로)");
	Showcase = TEXT("절벽 끝 Nav Link Proxy(Smart Link) → OnSmartLinkReached 에서 LaunchCharacter 점프, 끄면 경사로 우회");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-700.f, 1000.f, 100.f));
	bNeedsNavMesh = true;
}

void ADemo0708_NavLink::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 고지대(높이 300)와 왼쪽 끝의 경사로(15°) — 실행 중 스폰이라 Dynamic NavMesh 가 위층·경사로를 굽는다
	SpawnBlock(World, FVector(0.f, 0.f, 0.f), FVector(1000.f, 1000.f, CliffHeight), Colors::Platform);
	const float RampAngle = 15.f;
	const float RampLength = CliffHeight / FMath::Sin(FMath::DegreesToRadians(RampAngle));
	const float RampRun = CliffHeight / FMath::Tan(FMath::DegreesToRadians(RampAngle));
	SpawnShape(World, EShape::Cube, FVector(-500.f - RampRun * 0.5f, -350.f, CliffHeight * 0.5f - 8.f),
		FVector(RampLength + 30.f, 300.f, 20.f), Colors::Ally, FRotator(RampAngle, 0.f, 0.f));
	SpawnLabel(World, TEXT("경사로 (먼 길)"), FVector(-500.f - RampRun * 0.5f, -350.f, CliffHeight + 80.f), Colors::Ally, 18.f);

	TopGoal = FVector(-250.f, 250.f, CliffHeight);
	BottomGoal = FVector(1400.f, 0.f, 0.f);
	AStaticMeshActor* Pad = SpawnShape(World, EShape::Cylinder, BottomGoal + FVector(0.f, 0.f, 3.f), FVector(110.f, 110.f, 6.f), Colors::Item);
	Pad->SetActorEnableCollision(false);
	Pad->GetStaticMeshComponent()->SetCanEverAffectNavigation(false);
	SpawnLabel(World, TEXT("목표"), BottomGoal + FVector(0.f, 0.f, 80.f), Colors::Item, 18.f);

	// ---- Nav Link Proxy: 단순 링크(PointLinks)는 비우고 Smart Link 하나만 쓴다 ----
	// 빌더가 레벨에 놓아 둔 Nav Link Proxy (BuildLevel). 없으면 실행 중에 같은 설정으로 만든다
	for (TActorIterator<ANavLinkProxy> It(World); It; ++It)
	{
		LinkProxy = *It;
	}
	if (!LinkProxy)
	{
		LinkProxy = SpawnJumpLink(World);
	}
	// 실행 중 스폰한 절벽·경사로와 함께 링크 데이터를 내비게이션 옥트리에 다시 알린다
	FNavigationSystem::UpdateActorAndComponentData(*LinkProxy);
	// 링크에 도착하면 호출 (블루프린트의 Event Receive Smart Link Reached 와 같음)
	// (OnSmartLinkReached 는 C++ 에서 protected 라 블루프린트가 바인딩하는 것과 같은 리플렉션 경로로 연결)
	FScriptDelegate Delegate;
	Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ADemo0708_NavLink, OnSmartLinkReached));
	if (FMulticastDelegateProperty* Property = FindFProperty<FMulticastDelegateProperty>(ANavLinkProxy::StaticClass(), TEXT("OnSmartLinkReached")))
	{
		Property->AddDelegate(MoveTemp(Delegate), LinkProxy);
	}
	SpawnLabel(World, TEXT("Nav Link Proxy (Smart Link)"), (LinkStart + LinkEnd) * 0.5f + FVector(0.f, 0.f, 260.f), FLinearColor(0.4f, 1.f, 0.5f), 18.f);

	Agent = D07::SpawnEnemy(World, TopGoal, 0.f, AAIController::StaticClass(), Colors::Enemy);

	OverviewCamera = D07::SetupOverviewCamera(this, FVector(-100.f, -2900.f, 1900.f), FRotator(-33.f, 90.f, 0.f), 75.f);
}

void ADemo0708_NavLink::OnSmartLinkReached(AActor* MovingActor, const FVector& DestinationPoint)
{
	ACharacter* Character = Cast<ACharacter>(MovingActor);
	if (!Character)
	{
		LinkProxy->ResumePathFollowing(MovingActor);
		return;
	}
	// 포물선 점프: 위로 VerticalSpeed 로 뛰어 Destination 에 떨어지도록 수평 속도를 계산
	const float Gravity = -Character->GetCharacterMovement()->GetGravityZ();
	const FVector From = Character->GetActorLocation();
	const FVector To = DestinationPoint + FVector(0.f, 0.f, Character->GetDefaultHalfHeight());
	const float VerticalSpeed = 420.f;
	const float Dz = To.Z - From.Z;
	const float Time = (VerticalSpeed + FMath::Sqrt(FMath::Max(0.f, VerticalSpeed * VerticalSpeed + 2.f * Gravity * Dz))) / Gravity;
	FVector Velocity = (To - From) / FMath::Max(Time, 0.1f);
	Velocity.Z = VerticalSpeed;
	Character->SetActorRotation(FRotator(0.f, (To - From).Rotation().Yaw, 0.f));
	Character->LaunchCharacter(Velocity, true, true);
	bJumping = true;
	JumpElapsed = 0.f;
	bUsedLinkThisTrip = true;
	++JumpCount;
}

void ADemo0708_NavLink::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* World = GetWorld();
	if (WasKeyPressed(EKeys::V))
	{
		D07::ToggleOverviewCamera(this, OverviewCamera);
	}
	AAIController* AIC = Agent ? Cast<AAIController>(Agent->GetController()) : nullptr;
	if (!AIC || !LinkProxy)
	{
		return;
	}
	if (WasKeyPressed(EKeys::C))
	{
		// 끄면 링크 영역이 DisabledArea(기본 NavArea_Null)가 되어 길찾기에서 빠진다
		LinkProxy->SetSmartLinkEnabled(!LinkProxy->IsSmartLinkEnabled());
		if (!bJumping)
		{
			AIC->StopMovement();
			WaitTimer = 0.3f;
		}
	}

	// 점프 중: 착지하면 경로 따라가기를 넘겨준다
	if (bJumping)
	{
		JumpElapsed += DeltaSeconds;
		if (JumpElapsed > 0.25f && Agent->GetCharacterMovement()->IsMovingOnGround())
		{
			bJumping = false;
			LinkProxy->ResumePathFollowing(Agent);
		}
	}
	else if (AIC->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		// 위 → 아래(링크 또는 경사로) → 위(경사로) 왕복
		const FVector Goal = bGoingDown ? BottomGoal : TopGoal;
		if (FVector::Dist2D(Agent->GetActorLocation(), Goal) < 150.f)
		{
			if (bGoingDown && !bUsedLinkThisTrip)
			{
				++RampTrips;
			}
			bGoingDown = !bGoingDown;
			bUsedLinkThisTrip = false;
			WaitTimer = 1.f;
		}
		WaitTimer -= DeltaSeconds;
		if (WaitTimer <= 0.f)
		{
			AIC->MoveToLocation(bGoingDown ? BottomGoal : TopGoal, 60.f);
		}
	}

	// 링크 포물선 표시
	const bool bEnabled = LinkProxy->IsSmartLinkEnabled();
	const FColor LinkColor = bEnabled ? FColor(60, 255, 90) : FColor(110, 110, 110);
	FVector Prev = LinkStart;
	for (int32 Step = 1; Step <= 16; ++Step)
	{
		const float T = Step / 16.f;
		const FVector Next = FMath::Lerp(LinkStart, LinkEnd, T) + FVector(0.f, 0.f, FMath::Sin(T * PI) * 200.f);
		DrawDebugLine(World, Prev, Next, LinkColor, false, -1.f, 0, 6.f);
		Prev = Next;
	}
	DrawDebugSphere(World, LinkStart, 30.f, 10, LinkColor, false, -1.f, 0, 3.f);
	DrawDebugSphere(World, LinkEnd, 30.f, 10, LinkColor, false, -1.f, 0, 3.f);
	D07::DrawNavMesh(World, 5.f, 0.5f);
	D07::DrawCurrentPath(AIC, FColor(255, 220, 40), 6.f);

	const TCHAR* Action = bJumping ? TEXT("점프! (Smart Link)") : (bGoingDown ? TEXT("아래 목표로") : TEXT("경사로로 복귀"));
	Agent->SetLabel(Action, bJumping ? Colors::Ally : FLinearColor::White);
	Agent->SetMarkerColor(bJumping ? Colors::Ally : Colors::Enemy);
	SetStatus(FString::Printf(TEXT("Smart Link %s · 현재: %s\n링크 점프 %d회 · 링크 없이 경사로로 내려간 횟수 %d · 경로 꼭짓점 %d"),
		bEnabled ? TEXT("ON") : TEXT("OFF (경사로 우회)"), Action, JumpCount, RampTrips,
		AIC->GetPathFollowingComponent()->GetPath().IsValid() ? AIC->GetPathFollowingComponent()->GetPath()->GetPathPoints().Num() : 0));
}
