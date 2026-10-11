#include "Demo0701_AIController.h"

#include "Camera/CameraActor.h"
#include "D07Common.h"
#include "DrawDebugHelpers.h"
#include "GlossaryKit.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

using namespace GlossaryKit;

// ============================================================ 컨트롤러

AD0701_FollowController::AD0701_FollowController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AD0701_FollowController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	// 빙의 = 이 컨트롤러가 폰의 두뇌가 됨. 이제부터 이동 명령은 이 컨트롤러가 내린다.
	FollowTarget = UGameplayStatics::GetPlayerPawn(this, 0);
}

void AD0701_FollowController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APawn* MyPawn = GetPawn();
	AActor* Target = FollowTarget.Get();
	if (!MyPawn || !Target)
	{
		FollowTarget = UGameplayStatics::GetPlayerPawn(this, 0);
		return;
	}
	if (!bFollow)
	{
		return;
	}
	// MoveToActor 는 목표가 움직이면 경로를 갱신하며 따라가고, AcceptanceRadius 안에 들어오면 끝난다.
	// 끝난 뒤 플레이어가 다시 멀어지면 새 요청을 건다.
	const float Distance = FVector::Dist2D(MyPawn->GetActorLocation(), Target->GetActorLocation());
	if (GetMoveStatus() == EPathFollowingStatus::Idle && Distance > AcceptanceRadius + 60.f)
	{
		MoveToActor(Target, AcceptanceRadius, true, true);
		++MoveRequests;
	}
}

// ============================================================ 데모

ADemo0701_AIController::ADemo0701_AIController()
{
	DemoId = TEXT("0701");
	Controls = TEXT("WASD 이동 · V 카메라 전환 · C : 따라오기 ON/OFF\n")
		TEXT("관찰: 빨간 적은 AIController 가 빙의해 MoveToActor 로 기둥을 돌아 따라오고,\n")
		TEXT("회색 적은 Auto Possess AI = Disabled 라 컨트롤러가 없어 가만히 서 있다");
	Showcase = TEXT("Auto Possess AI 로 빙의한 AIController 가 MoveToActor 로 플레이어 추적, 컨트롤러 없는 폰과 비교");
	bNeedsNavMesh = true;
	NavBoundsExtent = FVector(3000.f, 3000.f, 600.f);
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-600.f, 0.f, 100.f));
}

void ADemo0701_AIController::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 기둥: 실행 중 스폰 → Dynamic NavMesh 가 구멍을 뚫는다
	SpawnBlock(World, FVector(300.f, -150.f, 0.f), FVector(160.f, 160.f, 300.f), Colors::Wall);
	SpawnBlock(World, FVector(300.f, 250.f, 0.f), FVector(160.f, 160.f, 300.f), Colors::Wall);
	SpawnBlock(World, FVector(700.f, 50.f, 0.f), FVector(160.f, 400.f, 300.f), Colors::Wall);

	// 1) AIControllerClass = AD0701_FollowController, AutoPossessAI = PlacedInWorldOrSpawned (AD07Enemy 기본값)
	Follower = D07::SpawnEnemy(World, FVector(1300.f, -350.f, 0.f), 180.f, AD0701_FollowController::StaticClass(), Colors::Enemy);
	if (AD0701_FollowController* AIC = Follower ? Cast<AD0701_FollowController>(Follower->GetController()) : nullptr)
	{
		AIC->AcceptanceRadius = AcceptanceRadius;
	}
	Follower->SetLabel(TEXT("AIController 빙의 → MoveToActor"), Colors::Item);

	// 2) 같은 캐릭터지만 Auto Possess AI 를 끈다 → 컨트롤러가 생기지 않는다
	const FTransform Transform(FRotator(0.f, 180.f, 0.f), FVector(1300.f, 450.f, 98.f));
	Brainless = World->SpawnActorDeferred<AD07Enemy>(AD07Enemy::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	Brainless->AutoPossessAI = EAutoPossessAI::Disabled;
	Brainless->FinishSpawning(Transform);
	Brainless->SetBodyColor(Colors::Neutral);
	Brainless->SetMarkerColor(Colors::Dark, 0.f);
	Brainless->SetLabel(TEXT("Auto Possess AI: Disabled → 두뇌 없음"), FLinearColor(0.8f, 0.8f, 0.8f));

	OverviewCamera = D07::SetupOverviewCamera(this, FVector(-1500.f, 0.f, 1500.f), FRotator(-42.f, 0.f, 0.f));
}

void ADemo0701_AIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::V))
	{
		D07::ToggleOverviewCamera(this, OverviewCamera);
	}

	AD0701_FollowController* AIC = Follower ? Cast<AD0701_FollowController>(Follower->GetController()) : nullptr;
	if (!AIC)
	{
		SetStatus(TEXT("컨트롤러 없음"));
		return;
	}
	if (WasKeyPressed(EKeys::C))
	{
		AIC->bFollow = !AIC->bFollow;
		if (!AIC->bFollow)
		{
			AIC->StopMovement();
		}
	}
	D07::DrawCurrentPath(AIC, FColor(255, 200, 40));

	const APawn* Player = GetPlayerPawn();
	const float Distance = Player ? FVector::Dist2D(Player->GetActorLocation(), Follower->GetActorLocation()) : 0.f;
	const EPathFollowingStatus::Type Status = AIC->GetMoveStatus();
	const TCHAR* StatusName = Status == EPathFollowingStatus::Moving ? TEXT("Moving (따라가는 중)")
		: Status == EPathFollowingStatus::Idle ? TEXT("Idle (도착/대기)") : TEXT("Waiting/Paused");
	Follower->SetMarkerColor(Status == EPathFollowingStatus::Moving ? Colors::Accent : Colors::Ally);

	SetStatus(FString::Printf(TEXT("빨간 적: Controller = %s · 따라오기 %s\nMoveToActor 상태 %s · 거리 %.0fcm (AcceptanceRadius %.0f) · 요청 %d회\n회색 적: Controller = %s"),
		*AIC->GetClass()->GetName(), AIC->bFollow ? TEXT("ON") : TEXT("OFF"), StatusName, Distance, AcceptanceRadius, AIC->MoveRequests,
		Brainless && Brainless->GetController() ? *Brainless->GetController()->GetName() : TEXT("없음 (None)")));
}
