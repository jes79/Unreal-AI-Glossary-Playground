#include "D0710_StateTreeNodes.h"

#include "Components/StateTreeAIComponent.h"
#include "D07Common.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "StateTreeExecutionContext.h"

// ============================================================ 컨트롤러

AD0710_StateTreeController::AD0710_StateTreeController()
{
	PrimaryActorTick.bCanEverTick = true;
	// StateTree 를 실행하는 컴포넌트 (AI 용: 컨텍스트로 AIController 와 Pawn 을 넘긴다)
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTree"));
	StateTreeComponent->SetStartLogicAutomatically(false);   // 데모가 트리를 지정한 뒤 StartLogic
}

APawn* AD0710_StateTreeController::GetPlayer() const
{
	return UGameplayStatics::GetPlayerPawn(this, 0);
}

float AD0710_StateTreeController::DistanceToPlayer() const
{
	const APawn* Player = GetPlayer();
	return Player && GetPawn() ? FVector::Dist2D(Player->GetActorLocation(), GetPawn()->GetActorLocation()) : 1e9f;
}

float AD0710_StateTreeController::SecondsSinceSeen() const
{
	return GetWorld()->GetTimeSeconds() - LastSeenTime;
}

void AD0710_StateTreeController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// 감지: 거리·시야각·가림(라인 트레이스). 조건 노드가 이 결과를 읽는다
	bSeesPlayer = false;
	const APawn* MyPawn = GetPawn();
	const APawn* Player = GetPlayer();
	if (MyPawn && Player)
	{
		const FVector ToPlayer = Player->GetActorLocation() - MyPawn->GetActorLocation();
		const float CosAngle = FVector::DotProduct(ToPlayer.GetSafeNormal2D(), MyPawn->GetActorForwardVector().GetSafeNormal2D());
		if (ToPlayer.Size2D() <= SightRadius && CosAngle >= FMath::Cos(FMath::DegreesToRadians(SightHalfAngle)))
		{
			FHitResult Hit;
			FCollisionQueryParams Params(TEXT("D0710Sight"), false, MyPawn);
			Params.AddIgnoredActor(Player);
			bSeesPlayer = !GetWorld()->LineTraceSingleByChannel(Hit, MyPawn->GetActorLocation() + FVector(0, 0, 60), Player->GetActorLocation() + FVector(0, 0, 40), ECC_Visibility, Params);
		}
	}
	if (bSeesPlayer)
	{
		LastSeenTime = GetWorld()->GetTimeSeconds();
	}
}

// ============================================================ 태스크

static AD0710_StateTreeController* GetController(FStateTreeExecutionContext& Context)
{
	return Cast<AD0710_StateTreeController>(Context.GetOwner());
}

EStateTreeRunStatus FD0710_Task_Idle::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	Context.GetInstanceData(*this).Elapsed = 0.f;
	if (AD0710_StateTreeController* AIC = GetController(Context))
	{
		AIC->StopMovement();
		AIC->CurrentTask = TEXT("Idle (2초 대기)");
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FD0710_Task_Idle::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FD0710_TimerData& Data = Context.GetInstanceData(*this);
	Data.Elapsed += DeltaTime;
	if (AD0710_StateTreeController* AIC = GetController(Context))
	{
		if (APawn* Pawn = AIC->GetPawn())
		{
			Pawn->AddActorWorldRotation(FRotator(0.f, 70.f * DeltaTime, 0.f));   // 두리번
		}
	}
	return Data.Elapsed >= Duration ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FD0710_Task_Patrol::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AD0710_StateTreeController* AIC = GetController(Context);
	if (!AIC || AIC->PatrolPoints.Num() == 0)
	{
		return EStateTreeRunStatus::Failed;
	}
	AIC->CurrentTask = FString::Printf(TEXT("Patrol → 지점 %d"), AIC->PatrolIndex);
	AIC->MoveToLocation(AIC->PatrolPoints[AIC->PatrolIndex], 40.f);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FD0710_Task_Patrol::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	AD0710_StateTreeController* AIC = GetController(Context);
	if (!AIC)
	{
		return EStateTreeRunStatus::Failed;
	}
	if (AIC->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		AIC->PatrolIndex = (AIC->PatrolIndex + 1) % AIC->PatrolPoints.Num();
		return EStateTreeRunStatus::Succeeded;
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FD0710_Task_Chase::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (AD0710_StateTreeController* AIC = GetController(Context))
	{
		AIC->CurrentTask = TEXT("Chase (MoveToActor)");
		AIC->MoveToActor(AIC->GetPlayer(), 120.f);
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FD0710_Task_Chase::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	AD0710_StateTreeController* AIC = GetController(Context);
	if (AIC && AIC->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		AIC->MoveToActor(AIC->GetPlayer(), 120.f);
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FD0710_Task_Attack::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	Context.GetInstanceData(*this).Elapsed = Interval;   // 들어오자마자 한 번 공격
	if (AD0710_StateTreeController* AIC = GetController(Context))
	{
		AIC->StopMovement();
		AIC->CurrentTask = TEXT("Attack");
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FD0710_Task_Attack::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FD0710_TimerData& Data = Context.GetInstanceData(*this);
	AD0710_StateTreeController* AIC = GetController(Context);
	AD07Enemy* Enemy = AIC ? Cast<AD07Enemy>(AIC->GetPawn()) : nullptr;
	if (!Enemy)
	{
		return EStateTreeRunStatus::Failed;
	}
	if (const APawn* Player = AIC->GetPlayer())
	{
		const FVector To = Player->GetActorLocation() - Enemy->GetActorLocation();
		Enemy->SetActorRotation(FRotator(0.f, To.Rotation().Yaw, 0.f));
	}
	Data.Elapsed += DeltaTime;
	if (Data.Elapsed >= Interval)
	{
		Data.Elapsed = 0.f;
		Enemy->PlayAttack();
	}
	return EStateTreeRunStatus::Running;
}

// ============================================================ 조건

bool FD0710_Cond_CanSeePlayer::TestCondition(FStateTreeExecutionContext& Context) const
{
	const AD0710_StateTreeController* AIC = GetController(Context);
	const bool bSeen = AIC && (Memory <= 0.f ? AIC->CanSeePlayer() : AIC->SecondsSinceSeen() <= Memory);
	return bSeen != bInvert;
}

bool FD0710_Cond_PlayerInRange::TestCondition(FStateTreeExecutionContext& Context) const
{
	const AD0710_StateTreeController* AIC = GetController(Context);
	const bool bIn = AIC && AIC->DistanceToPlayer() <= Range;
	return bIn != bInvert;
}
