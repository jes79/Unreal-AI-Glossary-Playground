#pragma once

#include "CoreMinimal.h"
#include "GlossaryCharacter.h"
#include "D0103_VehicleCharacter.generated.h"

struct FInputActionInstance;

/**
 * 0103 Input Mapping Context 전환 데모용 캐릭터.
 * 걸을 때: 템플릿 IMC_Default(WASD=IA_Move) + IMC_OnFoot(E=IA_Enter)
 * 차에 타면: IMC_Vehicle 을 우선순위 1로 추가 → 같은 WASD·E 키가 IA_Drive·IA_Exit 로 바뀐다 (높은 우선순위가 키를 가져감)
 * 내리면: IMC_Vehicle 제거 → 원래 조작으로 돌아온다.
 */
UCLASS()
class GLOSSARYMOVEMENT_API AD0103_VehicleCharacter : public AGlossaryCharacter
{
	GENERATED_BODY()

public:
	AD0103_VehicleCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 데모가 레벨에 놓은 차 */
	void SetVehicle(AActor* InVehicle) { Vehicle = InVehicle; }
	bool IsDriving() const { return bDriving; }
	bool IsNearVehicle() const;

	UInputMappingContext* GetVehicleContext() const { return IMC_Vehicle; }
	UInputMappingContext* GetOnFootContext() const { return IMC_OnFoot; }
	UInputMappingContext* GetDefaultContext() const { return DefaultMappingContext; }
	UInputMappingContext* GetMouseContext() const { return MouseLookMappingContext; }
	const FString& GetLastAction() const { return LastAction; }
	float GetThrottle() const { return Throttle; }
	float GetSteer() const { return Steer; }

	/** 차에 타면 IMC_Vehicle 을 이 우선순위로 추가한다 */
	UPROPERTY(EditAnywhere, Category = "Mapping Context")
	int32 VehicleContextPriority = 1;

	UPROPERTY(EditAnywhere, Category = "Mapping Context")
	float EnterDistance = 450.f;

	UPROPERTY(EditAnywhere, Category = "Vehicle")
	float DriveSpeed = 1400.f;

	UPROPERTY(EditAnywhere, Category = "Vehicle")
	float SteerRate = 90.f;

protected:
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;

	void BuildInputAssets();
	void EnterVehicle();
	void ExitVehicle();
	void Honk();

	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Enter;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Drive;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Exit;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Horn;
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> IMC_OnFoot;
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> IMC_Vehicle;

	TWeakObjectPtr<AActor> Vehicle;
	bool bDriving = false;
	float Throttle = 0.f;
	float Steer = 0.f;
	FString LastAction;
	float SavedWalkSpeed = 500.f;
};
