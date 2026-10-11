#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0105_CoyoteTime.generated.h"

/**
 * 0105 코요테 타임
 * 높은 발판 사이 간격을 두고, 끝에서 걸어 나간 뒤 0.1초 안에 누른 점프를 허용한다(AD0105_CoyoteCharacter).
 * C 로 ON/OFF, T 로 슬로 모션(타이밍 확인용).
 */
UCLASS()
class GLOSSARYMOVEMENT_API ADemo0105_CoyoteTime : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0105_CoyoteTime();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 요청 예문의 값. 시작할 때 캐릭터에 복사된다 */
	UPROPERTY(EditAnywhere, Category = "Coyote Time", meta = (ClampMin = 0))
	float CoyoteTime = 0.1f;

	/** T 키 슬로 모션 배율 */
	UPROPERTY(EditAnywhere, Category = "Coyote Time")
	float SlowMotion = 0.3f;

private:
	int32 SeenCoyote = 0;
	int32 SeenRejected = 0;
	bool bSlow = false;
};
