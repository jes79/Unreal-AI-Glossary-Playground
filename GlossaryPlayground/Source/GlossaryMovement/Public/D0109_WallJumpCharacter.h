#pragma once

#include "CoreMinimal.h"
#include "GlossaryCharacter.h"
#include "D0109_WallJumpCharacter.generated.h"

/**
 * 0109 벽 점프 / 벽 슬라이드 캐릭터.
 * 공중에서 벽에 닿아 있으면 낙하 속도를 WallSlideSpeed 로 제한하고(슬라이드),
 * 그 상태에서 점프하면 벽 법선(반대쪽) + 위로 LaunchCharacter 한다.
 */
UCLASS()
class GLOSSARYMOVEMENT_API AD0109_WallJumpCharacter : public AGlossaryCharacter
{
	GENERATED_BODY()

public:
	AD0109_WallJumpCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 벽에 붙어 있을 때 최대 낙하 속도 (cm/s) */
	UPROPERTY(EditAnywhere, Category = "Wall Jump") float WallSlideSpeed = 150.f;
	/** 벽 점프: 벽 반대쪽으로 / 위로 */
	UPROPERTY(EditAnywhere, Category = "Wall Jump") float WallJumpHorizontal = 750.f;
	UPROPERTY(EditAnywhere, Category = "Wall Jump") float WallJumpVertical = 800.f;
	/** 캡슐 반경 밖으로 이만큼 안에 벽이 있으면 "붙었다"로 본다 */
	UPROPERTY(EditAnywhere, Category = "Wall Jump") float WallCheckDistance = 20.f;
	UPROPERTY(EditAnywhere, Category = "Wall Jump") bool bWallJumpEnabled = true;

	bool IsOnWall() const { return bOnWall; }
	bool IsWallSliding() const { return bWallSliding; }
	FVector GetWallNormal() const { return WallNormal; }
	int32 GetWallJumpCount() const { return WallJumpCount; }

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void DoJumpStart() override;

	/** 캡슐 둘레 8방향으로 짧게 쏴서 세워진 벽을 찾는다 */
	bool FindWall(FVector& OutNormal) const;

	bool bOnWall = false;
	bool bWallSliding = false;
	FVector WallNormal = FVector::ZeroVector;
	float IgnoreWallTime = 0.f;
	int32 WallJumpCount = 0;
};
