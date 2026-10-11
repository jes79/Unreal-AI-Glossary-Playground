#include "Demo0106_JumpBuffer.h"

#include "D0106_BufferCharacter.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"
#include "Kismet/GameplayStatics.h"

using namespace GlossaryKit;

ADemo0106_JumpBuffer::ADemo0106_JumpBuffer()
{
	DemoId = TEXT("0106");
	Controls = TEXT("WASD 이동 · Space 점프 · C : 점프 버퍼링 ON/OFF · T : 슬로 모션\n")
		TEXT("징검다리를 연속 점프로 건너며, 착지하기 조금 전에 Space 를 눌러 보세요\n")
		TEXT("관찰: ON 이면 착지 0.15초 전 입력도 착지 순간 점프(초록), OFF 면 입력이 사라짐(빨강)");
	Showcase = TEXT("착지 직전 0.15초 안에 누른 점프를 기억했다가 Landed 에서 실행, ON/OFF·슬로 모션 비교");
	PlayerPawnClass = AD0106_BufferCharacter::StaticClass();
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-700.f, 0.f, 100.f));
}

void ADemo0106_JumpBuffer::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 징검다리: 높이가 오르내리는 발판 (간격이 있어 연속 점프가 필요)
	const float Heights[] = { 40.f, 90.f, 140.f, 90.f, 140.f, 190.f, 140.f, 90.f, 40.f };
	for (int32 i = 0; i < UE_ARRAY_COUNT(Heights); ++i)
	{
		const FVector Base(-300.f + i * 330.f, 0.f, 0.f);
		SpawnBlock(World, Base, FVector(200.f, 320.f, Heights[i]), i % 2 == 0 ? Colors::Platform : Colors::Neutral);
	}
	SpawnLabel(World, TEXT("연속 점프 징검다리 → 착지 직전에 미리 Space"), FVector(600.f, 0.f, 380.f), Colors::Item, 18.f);

	if (AD0106_BufferCharacter* Character = Cast<AD0106_BufferCharacter>(GetPlayerPawn()))
	{
		Character->JumpBufferTime = JumpBufferTime;
		BufferLabel = SpawnLabel(World, TEXT(""), Character->GetActorLocation(), Colors::Cyan, 16.f);
	}
}

void ADemo0106_JumpBuffer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	Super::EndPlay(EndPlayReason);
}

void ADemo0106_JumpBuffer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AD0106_BufferCharacter* Character = Cast<AD0106_BufferCharacter>(GetPlayerPawn());
	if (!Character)
	{
		return;
	}
	if (WasKeyPressed(EKeys::C))
	{
		Character->bJumpBufferEnabled = !Character->bJumpBufferEnabled;
	}
	if (WasKeyPressed(EKeys::T))
	{
		bSlow = !bSlow;
		UGameplayStatics::SetGlobalTimeDilation(this, bSlow ? SlowMotion : 1.f);
	}

	// 버퍼에 들어 있는 입력을 머리 위에 표시
	const float Age = Character->GetBufferedAge();
	if (AGlossaryLabel* L = BufferLabel.Get())
	{
		L->SetActorLocation(Character->GetActorLocation() + FVector(0.f, 0.f, 130.f));
		L->SetActorHiddenInGame(Age < 0.f);
		if (Age >= 0.f)
		{
			const bool bValid = Character->bJumpBufferEnabled && Age <= Character->JumpBufferTime;
			L->SetText(FString::Printf(TEXT("버퍼: %.2f초 전 입력 %s"), Age, bValid ? TEXT("(유효)") : TEXT("(만료)")));
			L->SetColor(bValid ? Colors::Cyan : Colors::Enemy);
		}
	}

	if (Character->GetBufferedJumpCount() != SeenBuffered)
	{
		SeenBuffered = Character->GetBufferedJumpCount();
		if (AGlossaryLabel* L = SpawnLabel(GetWorld(), FString::Printf(TEXT("버퍼 점프! 착지 %.3f초 전 입력"), Character->GetLastEarlyBy()),
			Character->GetActorLocation() + FVector(0.f, 0.f, 60.f), Colors::Ally, 16.f))
		{
			L->SetLifeSpan(2.f);
		}
	}
	if (Character->GetExpiredCount() != SeenExpired)
	{
		SeenExpired = Character->GetExpiredCount();
		const FString Why = Character->bJumpBufferEnabled
			? FString::Printf(TEXT("너무 이름: %.2f초 전 (> %.2f초)"), Character->GetLastEarlyBy(), Character->JumpBufferTime)
			: FString::Printf(TEXT("OFF: %.2f초 전 입력이 사라짐"), Character->GetLastEarlyBy());
		if (AGlossaryLabel* L = SpawnLabel(GetWorld(), Why, Character->GetActorLocation() + FVector(0.f, 0.f, 60.f), Colors::Enemy, 16.f))
		{
			L->SetLifeSpan(2.f);
		}
	}

	SetStatus(FString::Printf(TEXT("점프 버퍼링 %s (%.2f초)%s · %s\n버퍼 점프 성공 %d · 버려진 입력 %d"),
		Character->bJumpBufferEnabled ? TEXT("ON") : TEXT("OFF"), Character->JumpBufferTime, bSlow ? TEXT(" · 슬로 모션") : TEXT(""),
		*Character->GetCharacterMovement()->GetMovementName(), SeenBuffered, SeenExpired));
}
