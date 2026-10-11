#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0110_SpringArm.generated.h"

/**
 * 0110 Spring Arm (카메라 붐)
 * 길이 400 의 Spring Arm 이 마우스(Control Rotation)로 돌고, 벽에 막히면 Do Collision Test 로 당겨진다.
 * 캐릭터 바로 뒤에 벽이 있는 좁은 골목에서 시작해 C 로 충돌 검사를 끄면 카메라가 벽 너머로 가서 화면이 가려진다.
 */
UCLASS()
class GLOSSARYMOVEMENT_API ADemo0110_SpringArm : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0110_SpringArm();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Spring Arm") float TargetArmLength = 400.f;
	UPROPERTY(EditAnywhere, Category = "Spring Arm") bool bDoCollisionTest = true;
	/** 충돌 검사에 쓰는 구 반지름 */
	UPROPERTY(EditAnywhere, Category = "Spring Arm") float ProbeSize = 12.f;

private:
	void Apply();
};
