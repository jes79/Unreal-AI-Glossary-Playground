#pragma once

#include "CoreMinimal.h"
#include "GlossaryCharacter.h"
#include "D0106_BufferCharacter.generated.h"

/**
 * 0106 점프 버퍼링 캐릭터.
 * 공중에서 누른 점프(지금은 못 뜀)를 시각과 함께 기억해 두었다가,
 * Landed 이벤트에서 JumpBufferTime 이내였으면 바로 점프를 실행한다.
 */
UCLASS()
class GLOSSARYMOVEMENT_API AD0106_BufferCharacter : public AGlossaryCharacter
{
	GENERATED_BODY()

public:
	AD0106_BufferCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditAnywhere, Category = "Jump Buffer", meta = (ClampMin = 0))
	float JumpBufferTime = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Jump Buffer")
	bool bJumpBufferEnabled = true;

	/** 공중에서 누른 뒤 지난 시간 (-1 = 버퍼 비어 있음) */
	float GetBufferedAge() const;
	int32 GetBufferedJumpCount() const { return BufferedJumpCount; }
	int32 GetExpiredCount() const { return ExpiredCount; }
	float GetLastEarlyBy() const { return LastEarlyBy; }

protected:
	virtual void DoJumpStart() override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Landed 는 이동 처리 도중에 불리므로 같은 프레임에 Jump() 하면 입력이 지워진다 → 다음 Tick 에서 실행 */
	bool bJumpOnNextTick = false;

	/** 공중에서 점프를 누른 월드 시각 (-1 = 없음) */
	float BufferedPressTime = -1.f;
	int32 BufferedJumpCount = 0;
	int32 ExpiredCount = 0;
	float LastEarlyBy = 0.f;
};
