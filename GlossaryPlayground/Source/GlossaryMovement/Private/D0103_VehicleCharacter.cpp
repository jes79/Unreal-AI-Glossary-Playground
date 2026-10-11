#include "D0103_VehicleCharacter.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputTriggers.h"
#include "MovementKit.h"

AD0103_VehicleCharacter::AD0103_VehicleCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
}

void AD0103_VehicleCharacter::BuildInputAssets()
{
	if (IMC_Vehicle)
	{
		return;
	}
	using namespace MovementKit;
	IA_Enter = MakeAction(this, TEXT("IA_Enter"), EInputActionValueType::Boolean);
	IA_Drive = MakeAction(this, TEXT("IA_Drive"), EInputActionValueType::Axis2D);
	IA_Exit = MakeAction(this, TEXT("IA_Exit"), EInputActionValueType::Boolean);
	IA_Horn = MakeAction(this, TEXT("IA_Horn"), EInputActionValueType::Boolean);

	// 걸을 때만 쓰는 컨텍스트: E = 타기
	IMC_OnFoot = MakeContext(this, TEXT("IMC_OnFoot"));
	IMC_OnFoot->MapKey(IA_Enter, EKeys::E).Triggers.Add(NewObject<UInputTriggerPressed>(IMC_OnFoot));
	IMC_OnFoot->MapKey(IA_Enter, EKeys::Gamepad_FaceButton_Top).Triggers.Add(NewObject<UInputTriggerPressed>(IMC_OnFoot));

	// 차량 컨텍스트: 걷기와 "같은 키"를 다른 액션에 연결한다.
	// 우선순위가 높으면 그 키를 먼저 가져가므로(bConsumeInput) 아래 IMC_Default 의 IA_Move·IA_Jump 는 발동하지 않는다.
	IMC_Vehicle = MakeContext(this, TEXT("IMC_Vehicle"));
	MapWASD(IMC_Vehicle, IA_Drive);                              // W/S = 액셀·후진, A/D = 핸들
	MapStick(IMC_Vehicle, IA_Drive, EKeys::Gamepad_Left2D);
	IMC_Vehicle->MapKey(IA_Exit, EKeys::E).Triggers.Add(NewObject<UInputTriggerPressed>(IMC_Vehicle));
	IMC_Vehicle->MapKey(IA_Exit, EKeys::Gamepad_FaceButton_Top).Triggers.Add(NewObject<UInputTriggerPressed>(IMC_Vehicle));
	IMC_Vehicle->MapKey(IA_Horn, EKeys::SpaceBar).Triggers.Add(NewObject<UInputTriggerPressed>(IMC_Vehicle));   // 걸을 땐 점프였던 키
	IMC_Vehicle->MapKey(IA_Horn, EKeys::Gamepad_FaceButton_Bottom).Triggers.Add(NewObject<UInputTriggerPressed>(IMC_Vehicle));
}

void AD0103_VehicleCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();   // 템플릿 IMC_Default · IMC_MouseLook (우선순위 0)
	BuildInputAssets();
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = MovementKit::GetInputSubsystem(Cast<APlayerController>(Controller)))
	{
		Subsystem->AddMappingContext(IMC_OnFoot, 0);
	}
}

void AD0103_VehicleCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	BuildInputAssets();
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		return;
	}
	Input->BindAction(IA_Enter, ETriggerEvent::Triggered, this, &AD0103_VehicleCharacter::EnterVehicle);
	Input->BindAction(IA_Exit, ETriggerEvent::Triggered, this, &AD0103_VehicleCharacter::ExitVehicle);
	Input->BindAction(IA_Horn, ETriggerEvent::Triggered, this, &AD0103_VehicleCharacter::Honk);
	Input->BindActionValueLambda(IA_Drive, ETriggerEvent::Triggered, [this](const FInputActionValue& Value)
	{
		const FVector2D Axis = Value.Get<FVector2D>();
		Steer = Axis.X;
		Throttle = Axis.Y;
		LastAction = TEXT("IA_Drive");
	});
	Input->BindActionValueLambda(IA_Drive, ETriggerEvent::Completed, [this](const FInputActionValue&)
	{
		Steer = 0.f;
		Throttle = 0.f;
	});
}

bool AD0103_VehicleCharacter::IsNearVehicle() const
{
	const AActor* Car = Vehicle.Get();
	return Car && FVector::Dist2D(Car->GetActorLocation(), GetActorLocation()) <= EnterDistance;
}

void AD0103_VehicleCharacter::EnterVehicle()
{
	LastAction = TEXT("IA_Enter");
	AActor* Car = Vehicle.Get();
	if (bDriving || !Car || !IsNearVehicle())
	{
		return;
	}
	APlayerController* PC = Cast<APlayerController>(Controller);
	UEnhancedInputLocalPlayerSubsystem* Subsystem = MovementKit::GetInputSubsystem(PC);
	if (!Subsystem)
	{
		return;
	}

	// ★ 핵심: 차에 타면 IMC_Vehicle 을 우선순위 1로 추가
	Subsystem->AddMappingContext(IMC_Vehicle, VehicleContextPriority);
	bDriving = true;

	// 캐릭터를 차 자리로 옮기고 차를 몸에 붙인다 (모습은 차, 이동은 CharacterMovement)
	SetActorLocationAndRotation(Car->GetActorLocation() + FVector(0.f, 0.f, 100.f), FRotator(0.f, Car->GetActorRotation().Yaw, 0.f));
	Car->SetActorEnableCollision(false);
	Car->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
	Car->SetActorRelativeLocation(FVector(0.f, 0.f, -96.f));
	Car->SetActorRelativeRotation(FRotator::ZeroRotator);
	GetMesh()->SetVisibility(false);

	UCharacterMovementComponent* Move = GetCharacterMovement();
	SavedWalkSpeed = Move->MaxWalkSpeed;
	Move->MaxWalkSpeed = DriveSpeed;
	Move->bOrientRotationToMovement = false;
	GetCameraBoom()->TargetArmLength = 750.f;
}

void AD0103_VehicleCharacter::ExitVehicle()
{
	LastAction = TEXT("IA_Exit");
	if (!bDriving)
	{
		return;
	}
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = MovementKit::GetInputSubsystem(PC))
	{
		// ★ 핵심: 내리면 IMC_Vehicle 제거 → WASD·Space·E 가 다시 걷기 액션으로
		Subsystem->RemoveMappingContext(IMC_Vehicle);
	}
	bDriving = false;
	Throttle = Steer = 0.f;

	if (AActor* Car = Vehicle.Get())
	{
		Car->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Car->SetActorEnableCollision(true);
		// 차 옆으로 내린다
		SetActorLocation(GetActorLocation() + GetActorRightVector() * 260.f, false, nullptr, ETeleportType::TeleportPhysics);
	}
	GetMesh()->SetVisibility(true);
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = SavedWalkSpeed;
	Move->bOrientRotationToMovement = true;
	GetCameraBoom()->TargetArmLength = 400.f;
}

void AD0103_VehicleCharacter::Honk()
{
	LastAction = TEXT("IA_Horn");
	if (AGlossaryLabel* Label = GlossaryKit::SpawnLabel(GetWorld(), TEXT("빵빵! (Space = IA_Horn)"), GetActorLocation() + FVector(0.f, 0.f, 200.f), GlossaryKit::Colors::Item, 24.f))
	{
		Label->SetLifeSpan(0.8f);
	}
}

void AD0103_VehicleCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bDriving)
	{
		return;
	}
	// 자동차식 조작: A/D 는 옆걸음이 아니라 회전, W/S 는 앞뒤
	const float Direction = Throttle < -0.1f ? -1.f : 1.f;
	if (FMath::Abs(Throttle) > 0.05f)
	{
		AddActorWorldRotation(FRotator(0.f, Steer * SteerRate * Direction * DeltaSeconds, 0.f));
	}
	AddMovementInput(GetActorForwardVector(), Throttle);
}
