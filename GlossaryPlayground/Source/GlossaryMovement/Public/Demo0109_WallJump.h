#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0109_WallJump.generated.h"

class AGlossaryLabel;

/**
 * 0109 벽 점프 / 벽 슬라이드
 * 마주 보는 두 벽 사이(수직 통로)에서 벽에 붙어 천천히 미끄러지고, 점프로 반대쪽 벽으로 튀어 오른다(AD0109_WallJumpCharacter).
 */
UCLASS()
class GLOSSARYMOVEMENT_API ADemo0109_WallJump : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0109_WallJump();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	TWeakObjectPtr<AGlossaryLabel> HeadLabel;
	TWeakObjectPtr<AActor> NormalArrow;
	float BestHeight = 0.f;
};
