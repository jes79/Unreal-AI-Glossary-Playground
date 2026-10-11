#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0111_CameraLag.generated.h"

/**
 * 0111 Camera Lag
 * Spring Arm 의 Enable Camera Lag(Speed 5) 과 Camera Rotation Lag(Speed 10) 을 켜고 끈다.
 * R 로 원형 트랙 자동 달리기를 켜면 손을 떼고도 캐릭터가 화면 중앙에서 밀려나는 정도를 볼 수 있다.
 */
UCLASS()
class GLOSSARYMOVEMENT_API ADemo0111_CameraLag : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0111_CameraLag();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Camera Lag") bool bEnableCameraLag = true;
	UPROPERTY(EditAnywhere, Category = "Camera Lag") float CameraLagSpeed = 5.f;
	UPROPERTY(EditAnywhere, Category = "Camera Lag") bool bEnableCameraRotationLag = true;
	UPROPERTY(EditAnywhere, Category = "Camera Lag") float CameraRotationLagSpeed = 10.f;

	/** 자동 달리기 원형 트랙 */
	UPROPERTY(EditAnywhere, Category = "Camera Lag") FVector TrackCenter = FVector(600.f, 0.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Camera Lag") float TrackRadius = 600.f;

private:
	void Apply();
	bool bAutoRun = true;
	float TrackAngle = PI;   // 트랙 왼쪽(플레이어 시작 쪽)에서 출발
};
