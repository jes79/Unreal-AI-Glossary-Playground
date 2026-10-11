#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0106_JumpBuffer.generated.h"

class AGlossaryLabel;

/**
 * 0106 점프 버퍼링
 * 착지 직전(0.15초 이내)에 누른 점프를 기억했다가 Landed 에서 실행한다(AD0106_BufferCharacter).
 * 징검다리 발판을 연속 점프로 건너며 C 로 ON/OFF, T 로 슬로 모션.
 */
UCLASS()
class GLOSSARYMOVEMENT_API ADemo0106_JumpBuffer : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0106_JumpBuffer();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "Jump Buffer", meta = (ClampMin = 0))
	float JumpBufferTime = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Jump Buffer")
	float SlowMotion = 0.3f;

private:
	TWeakObjectPtr<AGlossaryLabel> BufferLabel;
	int32 SeenBuffered = 0;
	int32 SeenExpired = 0;
	bool bSlow = false;
};
