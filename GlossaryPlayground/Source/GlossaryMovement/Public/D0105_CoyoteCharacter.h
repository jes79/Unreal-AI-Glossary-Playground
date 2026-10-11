#pragma once

#include "CoreMinimal.h"
#include "GlossaryCharacter.h"
#include "D0105_CoyoteCharacter.generated.h"

/**
 * 0105 코요테 타임 캐릭터.
 * OnMovementModeChanged 에서 "점프 없이 Falling 이 된 시각"을 기록하고,
 * CanJumpInternal 을 덮어써서 그 뒤 CoyoteTime 초 동안은 공중에서도 첫 점프를 허용한다.
 */
UCLASS()
class GLOSSARYMOVEMENT_API AD0105_CoyoteCharacter : public AGlossaryCharacter
{
	GENERATED_BODY()

public:
	AD0105_CoyoteCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditAnywhere, Category = "Coyote Time", meta = (ClampMin = 0))
	float CoyoteTime = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Coyote Time")
	bool bCoyoteTimeEnabled = true;

	/** 발판에서 걸어 나간 뒤 지난 시간 (걸어 나간 상태가 아니면 -1) */
	float GetTimeSinceWalkOff() const;
	/** 마지막 점프가 코요테 타임 덕분이었는지와 그때의 낙하 경과 시간 */
	bool WasLastJumpCoyote() const { return bLastJumpWasCoyote; }
	float GetLastCoyoteDelay() const { return LastCoyoteDelay; }
	/** 점프를 눌렀지만 거절된 마지막 시점의 낙하 경과 시간 (-1 이면 없음) */
	float GetLastRejectedDelay() const { return LastRejectedDelay; }
	int32 GetCoyoteJumpCount() const { return CoyoteJumpCount; }
	int32 GetRejectedCount() const { return RejectedCount; }

protected:
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;
	virtual bool CanJumpInternal_Implementation() const override;
	virtual void OnJumped_Implementation() override;
	virtual void DoJumpStart() override;

	/** 걸어 나가 Falling 이 된 월드 시각 (-1 = 해당 없음) */
	float WalkOffTime = -1.f;
	bool bLastJumpWasCoyote = false;
	float LastCoyoteDelay = 0.f;
	float LastRejectedDelay = -1.f;
	int32 CoyoteJumpCount = 0;
	int32 RejectedCount = 0;
};
