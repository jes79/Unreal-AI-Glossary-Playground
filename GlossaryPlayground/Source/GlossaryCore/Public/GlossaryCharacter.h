#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GlossaryCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/**
 * 대부분의 예제 레벨이 쓰는 3인칭 캐릭터: Manny + ABP_Unarmed + Spring Arm 카메라 + Enhanced Input.
 * 입력 에셋은 Third Person 템플릿과 같은 /Game/Input 의 IA_Move · IA_Look · IA_MouseLook · IA_Jump · IMC_Default · IMC_MouseLook.
 * 데모마다 다른 조작이 필요하면 상속해서 DoMove / DoJumpStart 등을 오버라이드한다.
 */
UCLASS()
class GLOSSARYCORE_API AGlossaryCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGlossaryCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(BlueprintCallable, Category = Input) virtual void DoMove(float Right, float Forward);
	UFUNCTION(BlueprintCallable, Category = Input) virtual void DoLook(float Yaw, float Pitch);
	UFUNCTION(BlueprintCallable, Category = Input) virtual void DoJumpStart();
	UFUNCTION(BlueprintCallable, Category = Input) virtual void DoJumpEnd();

	/** 이동 입력을 막는다 (UI 를 열었을 때 등) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input)
	bool bInputEnabled = true;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera) TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera) TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(EditAnywhere, Category = Input) TObjectPtr<UInputMappingContext> DefaultMappingContext;
	UPROPERTY(EditAnywhere, Category = Input) TObjectPtr<UInputMappingContext> MouseLookMappingContext;
	UPROPERTY(EditAnywhere, Category = Input) TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditAnywhere, Category = Input) TObjectPtr<UInputAction> LookAction;
	UPROPERTY(EditAnywhere, Category = Input) TObjectPtr<UInputAction> MouseLookAction;
	UPROPERTY(EditAnywhere, Category = Input) TObjectPtr<UInputAction> JumpAction;
};
