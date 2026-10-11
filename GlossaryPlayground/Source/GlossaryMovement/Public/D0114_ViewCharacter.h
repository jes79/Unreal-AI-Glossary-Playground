#pragma once

#include "CoreMinimal.h"
#include "GlossaryCharacter.h"
#include "D0114_ViewCharacter.generated.h"

UENUM()
enum class ED0114View : uint8
{
	TPS,            // 3인칭: 등 뒤 Spring Arm, 마우스로 궤도 회전
	FPS,            // 1인칭: 눈 위치 카메라, 몸이 Control Rotation 을 따라 회전
	TopDown,        // 탑다운: 고정 각도로 내려다봄, 클릭 이동
	SideScroller,   // 횡스크롤: 옆에서 고정, X-Z 평면으로 이동 제한
};

/**
 * 0114 시점 전환 캐릭터. 같은 캐릭터의 Spring Arm·카메라·이동 설정만 바꿔 네 가지 시점을 만든다.
 */
UCLASS()
class GLOSSARYMOVEMENT_API AD0114_ViewCharacter : public AGlossaryCharacter
{
	GENERATED_BODY()

public:
	AD0114_ViewCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void SetView(ED0114View NewView);
	ED0114View GetView() const { return View; }

	virtual void DoMove(float Right, float Forward) override;
	virtual void DoLook(float Yaw, float Pitch) override;

	UPROPERTY(EditAnywhere, Category = "Camera Views") float TPSArmLength = 400.f;
	UPROPERTY(EditAnywhere, Category = "Camera Views") float TopDownArmLength = 1400.f;
	UPROPERTY(EditAnywhere, Category = "Camera Views") FRotator TopDownRotation = FRotator(-60.f, 0.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Camera Views") float SideArmLength = 1000.f;
	UPROPERTY(EditAnywhere, Category = "Camera Views") float FPSEyeHeight = 70.f;

protected:
	ED0114View View = ED0114View::TPS;
};
