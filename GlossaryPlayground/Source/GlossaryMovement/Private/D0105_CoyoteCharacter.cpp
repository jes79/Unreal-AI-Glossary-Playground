#include "D0105_CoyoteCharacter.h"

#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

AD0105_CoyoteCharacter::AD0105_CoyoteCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

float AD0105_CoyoteCharacter::GetTimeSinceWalkOff() const
{
	return WalkOffTime >= 0.f ? GetWorld()->GetTimeSeconds() - WalkOffTime : -1.f;
}

void AD0105_CoyoteCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	const UCharacterMovementComponent* Move = GetCharacterMovement();
	if (Move->MovementMode == MOVE_Falling && PrevMovementMode == MOVE_Walking && !bPressedJump)
	{
		// 점프 버튼 없이 Falling → 발판 끝에서 걸어 나간 것. 이 시각을 기록한다.
		WalkOffTime = GetWorld()->GetTimeSeconds();
	}
	else if (Move->MovementMode != MOVE_Falling)
	{
		WalkOffTime = -1.f;   // 착지하면 초기화
	}
}

bool AD0105_CoyoteCharacter::CanJumpInternal_Implementation() const
{
	// 기본 규칙(땅 위 또는 남은 점프 횟수)으로 되면 그대로
	if (Super::CanJumpInternal_Implementation())
	{
		return true;
	}
	// 코요테 타임: 걸어 나간 지 CoyoteTime 초 이내면 공중이어도 첫 점프 허용
	const float Since = GetTimeSinceWalkOff();
	return bCoyoteTimeEnabled && Since >= 0.f && Since <= CoyoteTime;
}

void AD0105_CoyoteCharacter::DoJumpStart()
{
	const float Since = GetTimeSinceWalkOff();
	bLastJumpWasCoyote = false;
	if (Since >= 0.f)
	{
		if (bCoyoteTimeEnabled && Since <= CoyoteTime)
		{
			bLastJumpWasCoyote = true;
			LastCoyoteDelay = Since;
			++CoyoteJumpCount;
		}
		else
		{
			LastRejectedDelay = Since;   // 너무 늦었거나 OFF → 점프 안 됨
			++RejectedCount;
		}
	}
	Super::DoJumpStart();
}

void AD0105_CoyoteCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();
	WalkOffTime = -1.f;   // 코요테 점프는 한 번만
}
