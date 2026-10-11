#include "GlossaryCharacter.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"

AGlossaryCharacter::AGlossaryCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 500.f, 0.f);
	Move->JumpZVelocity = 500.f;
	Move->AirControl = 0.35f;
	Move->MaxWalkSpeed = 500.f;
	Move->MinAnalogWalkSpeed = 20.f;
	Move->BrakingDecelerationWalking = 2000.f;
	Move->BrakingDecelerationFalling = 1500.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Manny + 템플릿 애니메이션 블루프린트
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> MannyAnim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	if (MannyMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MannyMesh.Object);
		GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));
	}
	if (MannyAnim.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(MannyAnim.Class);
	}

	// Third Person 템플릿과 같은 Enhanced Input 에셋
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMCDefault(TEXT("/Game/Input/IMC_Default"));
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMCMouse(TEXT("/Game/Input/IMC_MouseLook"));
	static ConstructorHelpers::FObjectFinder<UInputAction> IAMove(TEXT("/Game/Input/Actions/IA_Move"));
	static ConstructorHelpers::FObjectFinder<UInputAction> IALook(TEXT("/Game/Input/Actions/IA_Look"));
	static ConstructorHelpers::FObjectFinder<UInputAction> IAMouseLook(TEXT("/Game/Input/Actions/IA_MouseLook"));
	static ConstructorHelpers::FObjectFinder<UInputAction> IAJump(TEXT("/Game/Input/Actions/IA_Jump"));
	DefaultMappingContext = IMCDefault.Object;
	MouseLookMappingContext = IMCMouse.Object;
	MoveAction = IAMove.Object;
	LookAction = IALook.Object;
	MouseLookAction = IAMouseLook.Object;
	JumpAction = IAJump.Object;
}

void AGlossaryCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AGlossaryCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// 빙의(Possess)될 때마다 매핑 컨텍스트를 다시 등록 — 0115 Possess 데모에서 중요
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext) Subsystem->AddMappingContext(DefaultMappingContext, 0);
			if (MouseLookMappingContext) Subsystem->AddMappingContext(MouseLookMappingContext, 0);
		}
	}
}

void AGlossaryCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (JumpAction)
		{
			Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AGlossaryCharacter::DoJumpStart);
			Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &AGlossaryCharacter::DoJumpEnd);
		}
		if (MoveAction) Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGlossaryCharacter::Move);
		if (LookAction) Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGlossaryCharacter::Look);
		if (MouseLookAction) Input->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AGlossaryCharacter::Look);
	}
}

void AGlossaryCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	DoMove(Axis.X, Axis.Y);
}

void AGlossaryCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	DoLook(Axis.X, Axis.Y);
}

void AGlossaryCharacter::DoMove(float Right, float Forward)
{
	if (!Controller || !bInputEnabled)
	{
		return;
	}
	// 카메라(Control Rotation)의 Yaw 기준으로 앞/오른쪽
	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Forward);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Right);
}

void AGlossaryCharacter::DoLook(float Yaw, float Pitch)
{
	if (Controller)
	{
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AGlossaryCharacter::DoJumpStart()
{
	if (bInputEnabled)
	{
		Jump();
	}
}

void AGlossaryCharacter::DoJumpEnd()
{
	StopJumping();
}
