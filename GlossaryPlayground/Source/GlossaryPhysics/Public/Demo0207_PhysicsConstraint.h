#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0207_PhysicsConstraint.generated.h"

class ACameraActor;
class AStaticMeshActor;
class UPhysicsConstraintComponent;

/**
 * 0207 Physics Constraint
 * 문: Physics Constraint Component 로 문짝을 세로축 경첩에 묶고 Swing1 을 ±90° 로 제한 (선형 이동·Swing2·Twist 는 Locked).
 * 체인: 고리 8개를 Constraint 로 줄줄이 연결(선형 Locked, 회전 Free = 볼 조인트) → 흔들리는 체인.
 */
UCLASS()
class GLOSSARYPHYSICS_API ADemo0207_PhysicsConstraint : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0207_PhysicsConstraint();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 문 경첩 Swing 제한 각도 (±) */
	UPROPERTY(EditAnywhere, Category = "Physics Constraint", meta = (ClampMin = 0, ClampMax = 180))
	float DoorSwingLimit = 90.f;

	UPROPERTY(EditAnywhere, Category = "Physics Constraint", meta = (ClampMin = 2))
	int32 ChainLinks = 8;

	/** 문을 미는 속도 변화 (cm/s) */
	UPROPERTY(EditAnywhere, Category = "Physics Constraint")
	float DoorPush = 600.f;

private:
	UPhysicsConstraintComponent* MakeConstraint(const FVector& Location, const FRotator& Rotation, UPrimitiveComponent* A, UPrimitiveComponent* B);
	void ApplyDoorLimit();
	void PushDoor();

	UPROPERTY() TObjectPtr<ACameraActor> Camera;
	UPROPERTY() TObjectPtr<AStaticMeshActor> Door;
	UPROPERTY() TObjectPtr<UPhysicsConstraintComponent> DoorHinge;
	UPROPERTY() TArray<TObjectPtr<AStaticMeshActor>> Links;
	bool bLimitOn = true;
	float PushTimer = 0.f;
	float PushSign = 1.f;
	float MaxAngleSeen = 0.f;
	float DoorBaseYaw = 0.f;
};
