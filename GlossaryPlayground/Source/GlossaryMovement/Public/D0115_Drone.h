#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "D0115_Drone.generated.h"

class UCameraComponent;
class UFloatingPawnMovement;
class UInputAction;
class UInputMappingContext;
class USphereComponent;
class USpringArmComponent;
class UStaticMeshComponent;

/**
 * 0115 Possess 데모의 드론 Pawn: 날아다니는 폰(FloatingPawnMovement) + 자기 카메라 + 자기 입력(IMC_Drone).
 * 컨트롤러가 바뀌면 NotifyControllerChanged 에서 IMC_Drone 을 추가/제거한다.
 */
UCLASS()
class GLOSSARYMOVEMENT_API AD0115_Drone : public APawn
{
	GENERATED_BODY()

public:
	AD0115_Drone();

	UInputMappingContext* GetDroneContext() const { return IMC_Drone; }

	UPROPERTY(EditAnywhere, Category = "Drone") float RotorSpeed = 1440.f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;

	void BuildInputAssets();

	UPROPERTY(VisibleAnywhere, Category = "Drone") TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere, Category = "Drone") TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere, Category = "Drone") TArray<TObjectPtr<UStaticMeshComponent>> Rotors;
	UPROPERTY(VisibleAnywhere, Category = "Drone") TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, Category = "Drone") TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere, Category = "Drone") TObjectPtr<UFloatingPawnMovement> Movement;

	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Fly;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Lift;
	/** 템플릿 /Game/Input/Actions/IA_Look · IA_MouseLook */
	UPROPERTY(Transient) TObjectPtr<UInputAction> LookAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> MouseLookAction;
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> IMC_Drone;

	/** 마지막으로 이 드론을 조종했던 플레이어 컨트롤러 (Unpossess 때 IMC 를 빼기 위해) */
	TWeakObjectPtr<APlayerController> LastPC;
};
