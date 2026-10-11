#pragma once

#include "CoreMinimal.h"
#include "ChaosVehicleWheel.h"
#include "WheeledVehiclePawn.h"
#include "D0214_SportsCar.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class UChaosWheeledVehicleMovementComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
struct FInputActionValue;

/** 0214 앞바퀴: 조향(40°) + 브레이크 */
UCLASS()
class GLOSSARYPHYSICS_API UD0214_WheelFront : public UChaosVehicleWheel
{
	GENERATED_BODY()
public:
	UD0214_WheelFront();
};

/** 0214 뒷바퀴: 구동(엔진) + 핸드브레이크 */
UCLASS()
class GLOSSARYPHYSICS_API UD0214_WheelRear : public UChaosVehicleWheel
{
	GENERATED_BODY()
public:
	UD0214_WheelRear();
};

/**
 * 0214 4륜 차량: AWheeledVehiclePawn + ChaosWheeledVehicleMovementComponent.
 * 메시는 엔진 Vehicle 템플릿의 SKM_SportsCar(바퀴 뼈 Phys_Wheel_FL/FR/BL/BR)와 ABP_SportsCar(바퀴 회전 애님).
 * 입력은 템플릿 /Game/Input 의 IA_Move(WASD) · IA_Jump(Space = 핸드브레이크).
 */
UCLASS()
class GLOSSARYPHYSICS_API AD0214_SportsCar : public AWheeledVehiclePawn
{
	GENERATED_BODY()

public:
	AD0214_SportsCar();

	UChaosWheeledVehicleMovementComponent* GetChaosMovement() const { return ChaosMovement; }

	/** 데모 셧(자동 스크린샷)용 자동 운전 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Vehicle)
	bool bAutoPilot = false;

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;

	void Move(const FInputActionValue& Value);
	void MoveCompleted(const FInputActionValue& Value);
	void HandbrakeOn();
	void HandbrakeOff();

	UPROPERTY(VisibleAnywhere, Category = Camera) TObjectPtr<USpringArmComponent> SpringArm;
	UPROPERTY(VisibleAnywhere, Category = Camera) TObjectPtr<UCameraComponent> Camera;
	UPROPERTY() TObjectPtr<UChaosWheeledVehicleMovementComponent> ChaosMovement;

	/** 보이는 차체: 복사한 SKM_SportsCar 가 이 프로젝트에서 렌더되지 않아 같은 모양의 SM_SportsCar 를 물리 메시에 붙인다 */
	UPROPERTY(VisibleAnywhere, Category = Vehicle) TObjectPtr<UStaticMeshComponent> BodyVisual;

	UPROPERTY(EditAnywhere, Category = Input) TObjectPtr<UInputMappingContext> MappingContext;
	UPROPERTY(EditAnywhere, Category = Input) TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditAnywhere, Category = Input) TObjectPtr<UInputAction> HandbrakeAction;

private:
	float AutoTime = 0.f;
};
