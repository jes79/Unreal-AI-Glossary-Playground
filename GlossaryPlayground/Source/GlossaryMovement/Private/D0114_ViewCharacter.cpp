#include "D0114_ViewCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

AD0114_ViewCharacter::AD0114_ViewCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void AD0114_ViewCharacter::SetView(ED0114View NewView)
{
	View = NewView;
	USpringArmComponent* Arm = GetCameraBoom();
	UCameraComponent* Camera = GetFollowCamera();
	UCharacterMovementComponent* Move = GetCharacterMovement();

	// 먼저 공통 기본값으로 되돌린 뒤 시점별로 덮어쓴다
	Arm->SetUsingAbsoluteRotation(false);
	Arm->SetRelativeLocation(FVector::ZeroVector);
	Arm->bUsePawnControlRotation = true;
	Arm->bDoCollisionTest = true;
	Arm->bEnableCameraLag = false;
	Camera->SetFieldOfView(90.f);
	bUseControllerRotationYaw = false;
	Move->bOrientRotationToMovement = true;
	Move->SetPlaneConstraintEnabled(false);
	GetMesh()->SetOwnerNoSee(false);

	switch (View)
	{
	case ED0114View::TPS:
		// 3인칭: 팔 길이 400, 마우스가 Control Rotation 을 돌리고 팔이 따라 돈다
		Arm->TargetArmLength = TPSArmLength;
		break;

	case ED0114View::FPS:
		// 1인칭: 팔 길이 0 + 눈 높이, 몸이 마우스 Yaw 를 따라 돈다. 내 몸은 내 카메라에서 숨긴다
		Arm->TargetArmLength = 0.f;
		Arm->SetRelativeLocation(FVector(0.f, 0.f, FPSEyeHeight));
		Arm->bDoCollisionTest = false;
		bUseControllerRotationYaw = true;
		Move->bOrientRotationToMovement = false;
		GetMesh()->SetOwnerNoSee(true);
		Camera->SetFieldOfView(100.f);
		break;

	case ED0114View::TopDown:
		// 탑다운: 팔을 월드 기준 고정 각도로(Absolute Rotation), Control Rotation 을 따르지 않는다
		Arm->SetUsingAbsoluteRotation(true);
		Arm->SetWorldRotation(TopDownRotation);
		Arm->TargetArmLength = TopDownArmLength;
		Arm->bUsePawnControlRotation = false;
		Arm->bDoCollisionTest = false;
		Arm->bEnableCameraLag = true;   // 클릭 이동을 부드럽게 따라가도록
		break;

	case ED0114View::SideScroller:
		// 횡스크롤: 옆(+Y 쪽)에서 고정 카메라, 이동은 X-Z 평면으로 제한
		Arm->SetUsingAbsoluteRotation(true);
		Arm->SetWorldRotation(FRotator(-10.f, -90.f, 0.f));   // -Y 를 바라봄 → 화면 오른쪽 = 월드 +X
		Arm->TargetArmLength = SideArmLength;
		Arm->bUsePawnControlRotation = false;
		Arm->bDoCollisionTest = false;
		Move->SetPlaneConstraintNormal(FVector(0.f, 1.f, 0.f));
		Move->SetPlaneConstraintEnabled(true);
		break;
	}
}

void AD0114_ViewCharacter::DoMove(float Right, float Forward)
{
	if (!Controller || !bInputEnabled)
	{
		return;
	}
	switch (View)
	{
	case ED0114View::TopDown:
		// 화면 위 = 월드 +X, 화면 오른쪽 = 월드 +Y (카메라 Yaw 0 고정)
		AddMovementInput(FVector::ForwardVector, Forward);
		AddMovementInput(FVector::RightVector, Right);
		break;
	case ED0114View::SideScroller:
		// A/D 만 쓴다: 화면 오른쪽 = 월드 +X
		AddMovementInput(FVector::ForwardVector, Right);
		break;
	default:
		Super::DoMove(Right, Forward);
		break;
	}
}

void AD0114_ViewCharacter::DoLook(float Yaw, float Pitch)
{
	// 탑다운·횡스크롤은 카메라가 고정이므로 마우스 시점 회전을 무시
	if (View == ED0114View::TPS || View == ED0114View::FPS)
	{
		Super::DoLook(Yaw, Pitch);
	}
}
