#include "Demo0105_CoyoteTime.h"

#include "D0105_CoyoteCharacter.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"
#include "Kismet/GameplayStatics.h"

using namespace GlossaryKit;

namespace
{
	constexpr float PlatformHeight = 250.f;
}

ADemo0105_CoyoteTime::ADemo0105_CoyoteTime()
{
	DemoId = TEXT("0105");
	Controls = TEXT("WASD 이동 · Space 점프 · C : 코요테 타임 ON/OFF · T : 슬로 모션\n")
		TEXT("발판 끝(노란 선)을 지나 떨어지기 시작한 뒤 점프를 눌러 보세요\n")
		TEXT("관찰: ON 이면 떨어진 지 0.1초 안의 점프가 성공(초록), OFF 면 실패(빨강)");
	Showcase = TEXT("발판 끝에서 걸어 나간 뒤 0.1초 동안 점프 허용(OnMovementModeChanged 기록 + CanJumpInternal), ON/OFF·슬로 모션 비교");
	PlayerPawnClass = AD0105_CoyoteCharacter::StaticClass();
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-700.f, 0.f, PlatformHeight + 100.f));
}

void ADemo0105_CoyoteTime::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 높은 발판 3개, 사이 간격 380cm — 걸어서 떨어지기 쉬운 거리
	const float Starts[] = { -1000.f, 380.f, 1760.f };
	for (float X : Starts)
	{
		SpawnBlock(World, FVector(X + 500.f, 0.f, 0.f), FVector(1000.f, 700.f, PlatformHeight), Colors::Platform);
		// 발판 끝 표시 (노란 줄)
		SpawnBlock(World, FVector(X + 990.f, 0.f, PlatformHeight), FVector(20.f, 700.f, 2.f), Colors::Item);
	}
	SpawnLabel(World, TEXT("여기서 걸어 나간 뒤 0.1초 안에 Space"), FVector(0.f, 0.f, PlatformHeight + 230.f), Colors::Item, 18.f);

	if (AD0105_CoyoteCharacter* Character = Cast<AD0105_CoyoteCharacter>(GetPlayerPawn()))
	{
		Character->CoyoteTime = CoyoteTime;
	}
}

void ADemo0105_CoyoteTime::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	Super::EndPlay(EndPlayReason);
}

void ADemo0105_CoyoteTime::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AD0105_CoyoteCharacter* Character = Cast<AD0105_CoyoteCharacter>(GetPlayerPawn());
	if (!Character)
	{
		return;
	}
	if (WasKeyPressed(EKeys::C))
	{
		Character->bCoyoteTimeEnabled = !Character->bCoyoteTimeEnabled;
	}
	if (WasKeyPressed(EKeys::T))
	{
		bSlow = !bSlow;
		UGameplayStatics::SetGlobalTimeDilation(this, bSlow ? SlowMotion : 1.f);
	}

	// 성공·실패를 그 자리에 라벨로 남긴다
	if (Character->GetCoyoteJumpCount() != SeenCoyote)
	{
		SeenCoyote = Character->GetCoyoteJumpCount();
		if (AGlossaryLabel* L = SpawnLabel(GetWorld(), FString::Printf(TEXT("코요테 점프 성공! 떨어진 지 %.3f초"), Character->GetLastCoyoteDelay()),
			Character->GetActorLocation() + FVector(0.f, 0.f, 140.f), Colors::Ally, 18.f))
		{
			L->SetLifeSpan(3.f);
		}
	}
	if (Character->GetRejectedCount() != SeenRejected)
	{
		SeenRejected = Character->GetRejectedCount();
		const FString Why = Character->bCoyoteTimeEnabled
			? FString::Printf(TEXT("늦음: %.3f초 > %.1f초"), Character->GetLastRejectedDelay(), Character->CoyoteTime)
			: FString::Printf(TEXT("OFF: %.3f초 늦게 눌러 실패"), Character->GetLastRejectedDelay());
		if (AGlossaryLabel* L = SpawnLabel(GetWorld(), Why, Character->GetActorLocation() + FVector(0.f, 0.f, 140.f), Colors::Enemy, 18.f))
		{
			L->SetLifeSpan(3.f);
		}
	}

	// 플랫폼에서 떨어지면 시작 위치로
	if (Character->GetActorLocation().Z < PlatformHeight - 120.f && Character->GetCharacterMovement()->IsMovingOnGround())
	{
		Character->SetActorLocation(FVector(Character->GetActorLocation().X < 380.f ? -700.f : 600.f, 0.f, PlatformHeight + 100.f), false, nullptr, ETeleportType::TeleportPhysics);
	}

	const float Since = Character->GetTimeSinceWalkOff();
	const FString Window = Since < 0.f ? FString(TEXT("발판 위 / 점프 중"))
		: (Since <= Character->CoyoteTime && Character->bCoyoteTimeEnabled
			? FString::Printf(TEXT("걸어 나감 %.3f초 — 아직 점프 가능!"), Since)
			: FString::Printf(TEXT("걸어 나감 %.3f초 — 창 닫힘"), Since));
	SetStatus(FString::Printf(TEXT("코요테 타임 %s (%.1f초)%s\n%s · 성공 %d · 실패 %d"),
		Character->bCoyoteTimeEnabled ? TEXT("ON") : TEXT("OFF"), Character->CoyoteTime, bSlow ? TEXT(" · 슬로 모션") : TEXT(""),
		*Window, SeenCoyote, SeenRejected));
}
