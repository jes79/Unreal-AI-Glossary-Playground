#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0214_ChaosVehicle.generated.h"

/**
 * 0214 Chaos Vehicle
 * 플레이어 폰 = AD0214_SportsCar (AWheeledVehiclePawn + ChaosWheeledVehicleMovementComponent, 4륜).
 * WASD 로 운전, Space 핸드브레이크. 콘 슬라럼·점프대·부딪힐 상자를 놓는다.
 */
UCLASS()
class GLOSSARYPHYSICS_API ADemo0214_ChaosVehicle : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0214_ChaosVehicle();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
};
