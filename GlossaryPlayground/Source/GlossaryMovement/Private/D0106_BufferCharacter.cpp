#include "D0106_BufferCharacter.h"

#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

AD0106_BufferCharacter::AD0106_BufferCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

float AD0106_BufferCharacter::GetBufferedAge() const
{
	return BufferedPressTime >= 0.f ? GetWorld()->GetTimeSeconds() - BufferedPressTime : -1.f;
}

void AD0106_BufferCharacter::DoJumpStart()
{
	if (GetCharacterMovement()->IsFalling() && !CanJump())
	{
		// 지금은 뛸 수 없다 → 누른 시각만 기억한다(버퍼)
		BufferedPressTime = GetWorld()->GetTimeSeconds();
	}
	Super::DoJumpStart();
}

void AD0106_BufferCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	const float Age = GetBufferedAge();
	BufferedPressTime = -1.f;
	if (Age < 0.f)
	{
		return;
	}
	if (bJumpBufferEnabled && Age <= JumpBufferTime)
	{
		// 착지하자마자 버퍼된 점프 실행 — 플레이어는 "눌렀는데 씹혔다"는 느낌을 받지 않는다
		LastEarlyBy = Age;
		++BufferedJumpCount;
		bJumpOnNextTick = true;
	}
	else
	{
		LastEarlyBy = Age;
		++ExpiredCount;   // 너무 일찍 눌렀거나 OFF → 버려짐
	}
}

void AD0106_BufferCharacter::Tick(float DeltaSeconds)
{
	// 액터 Tick 은 CharacterMovement Tick 보다 먼저 돈다 → 여기서 Jump() 하면 이번 프레임 이동에서 바로 뛴다
	if (bJumpOnNextTick)
	{
		bJumpOnNextTick = false;
		Jump();
	}
	Super::Tick(DeltaSeconds);
}
