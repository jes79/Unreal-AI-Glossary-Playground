#include "D0109_WallJumpCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

AD0109_WallJumpCharacter::AD0109_WallJumpCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	GetCharacterMovement()->AirControl = 0.6f;   // 벽 사이를 오가기 쉽게
}

bool AD0109_WallJumpCharacter::FindWall(FVector& OutNormal) const
{
	const float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	const FVector Start = GetActorLocation();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WallCheck), false, this);
	float BestDistance = TNumericLimits<float>::Max();
	for (int32 i = 0; i < 8; ++i)
	{
		const FVector Dir = FRotator(0.f, i * 45.f, 0.f).Vector();
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Dir * (Radius + WallCheckDistance), ECC_Visibility, Params)
			&& FMath::Abs(Hit.ImpactNormal.Z) < 0.3f        // 거의 수직인 면만 벽으로 인정
			&& Hit.Distance < BestDistance)
		{
			BestDistance = Hit.Distance;
			OutNormal = Hit.ImpactNormal.GetSafeNormal2D();
		}
	}
	return BestDistance < TNumericLimits<float>::Max();
}

void AD0109_WallJumpCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	IgnoreWallTime = FMath::Max(0.f, IgnoreWallTime - DeltaSeconds);

	UCharacterMovementComponent* Move = GetCharacterMovement();
	bOnWall = bWallJumpEnabled && IgnoreWallTime <= 0.f && Move->IsFalling() && FindWall(WallNormal);
	bWallSliding = false;
	if (bOnWall && Move->Velocity.Z < -WallSlideSpeed)
	{
		// ★ 벽 슬라이드: 낙하 속도를 제한 (벽에 기대 천천히 미끄러짐)
		Move->Velocity.Z = -WallSlideSpeed;
		bWallSliding = true;
	}
	if (bOnWall)
	{
		// 벽을 등지고 바라보게 (다음 점프 방향이 보이도록)
		SetActorRotation(FRotator(0.f, WallNormal.Rotation().Yaw, 0.f));
	}
}

void AD0109_WallJumpCharacter::DoJumpStart()
{
	if (bOnWall && bInputEnabled)
	{
		// ★ 벽 점프: 벽 반대쪽(법선) + 위로 발사. XY·Z 모두 덮어써서 매번 같은 궤적
		LaunchCharacter(WallNormal * WallJumpHorizontal + FVector::UpVector * WallJumpVertical, true, true);
		IgnoreWallTime = 0.15f;   // 떨어지자마자 같은 벽에 다시 붙지 않게
		bOnWall = false;
		++WallJumpCount;
		return;
	}
	Super::DoJumpStart();
}
