#include "D0115_Drone.h"

#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GlossaryKit.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "MovementKit.h"

AD0115_Drone::AD0115_Drone()
{
	PrimaryActorTick.bCanEverTick = true;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(60.f);
	Collision->SetCollisionProfileName(TEXT("Pawn"));
	RootComponent = Collision;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Collision);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GlossaryKit::InitShapeComponent(Body, GlossaryKit::EShape::Cube, FVector(90.f, 90.f, 30.f));

	// 프로펠러 4개 (Tick 에서 회전)
	const FVector RotorPos[] = { FVector(70.f, 70.f, 25.f), FVector(70.f, -70.f, 25.f), FVector(-70.f, 70.f, 25.f), FVector(-70.f, -70.f, 25.f) };
	for (int32 i = 0; i < 4; ++i)
	{
		UStaticMeshComponent* Rotor = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Rotor%d"), i));
		Rotor->SetupAttachment(Collision);
		Rotor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GlossaryKit::InitShapeComponent(Rotor, GlossaryKit::EShape::Cube, FVector(90.f, 10.f, 4.f));
		Rotor->SetRelativeLocation(RotorPos[i]);
		Rotors.Add(Rotor);
	}

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(Collision);
	CameraBoom->TargetArmLength = 350.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 8.f;
	CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 40.f));

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	Movement->MaxSpeed = 900.f;
	Movement->Acceleration = 2000.f;
	Movement->Deceleration = 1500.f;

	bUseControllerRotationYaw = true;   // 드론 앞쪽이 카메라 방향을 따라 돈다
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
}

void AD0115_Drone::BuildInputAssets()
{
	if (IMC_Drone)
	{
		return;
	}
	using namespace MovementKit;
	IA_Fly = MakeAction(this, TEXT("IA_Fly"), EInputActionValueType::Axis2D);
	IA_Lift = MakeAction(this, TEXT("IA_Lift"), EInputActionValueType::Axis1D);
	// 시점은 캐릭터와 같은 템플릿 액션(IA_MouseLook · IA_Look)을 그대로 바인딩한다 — IMC_MouseLook 이 계속 켜져 있으므로
	LookAction = GlossaryKit::LoadAsset<UInputAction>(TEXT("/Game/Input/Actions/IA_Look"));
	MouseLookAction = GlossaryKit::LoadAsset<UInputAction>(TEXT("/Game/Input/Actions/IA_MouseLook"));

	IMC_Drone = MakeContext(this, TEXT("IMC_Drone"));
	MapWASD(IMC_Drone, IA_Fly);
	MapStick(IMC_Drone, IA_Fly, EKeys::Gamepad_Left2D);
	// 상승/하강: Space = +1, Ctrl = -1 (Negate)
	IMC_Drone->MapKey(IA_Lift, EKeys::SpaceBar);
	IMC_Drone->MapKey(IA_Lift, EKeys::LeftControl).Modifiers.Add(NewObject<UInputModifierNegate>(IMC_Drone));
	IMC_Drone->MapKey(IA_Lift, EKeys::Gamepad_RightTrigger);
	IMC_Drone->MapKey(IA_Lift, EKeys::Gamepad_LeftTrigger).Modifiers.Add(NewObject<UInputModifierNegate>(IMC_Drone));
}

void AD0115_Drone::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	BuildInputAssets();

	// 빙의 해제: 이전 플레이어의 입력에서 IMC_Drone 제거
	if (APlayerController* Old = LastPC.Get(); Old && Old != Controller)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = MovementKit::GetInputSubsystem(Old))
		{
			Subsystem->RemoveMappingContext(IMC_Drone);
		}
		LastPC.Reset();
	}
	// 빙의: 새 플레이어 입력에 IMC_Drone 을 우선순위 1로 추가 (WASD·Space 를 캐릭터보다 먼저 가져감)
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = MovementKit::GetInputSubsystem(PC))
		{
			Subsystem->AddMappingContext(IMC_Drone, 1);
		}
		LastPC = PC;
	}
}

void AD0115_Drone::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	BuildInputAssets();
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		return;
	}
	Input->BindActionValueLambda(IA_Fly, ETriggerEvent::Triggered, [this](const FInputActionValue& Value)
	{
		const FVector2D Axis = Value.Get<FVector2D>();
		const FRotator Yaw(0.f, GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
		AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
	});
	Input->BindActionValueLambda(IA_Lift, ETriggerEvent::Triggered, [this](const FInputActionValue& Value)
	{
		AddMovementInput(FVector::UpVector, Value.Get<float>());
	});
	auto Look = [this](const FInputActionValue& Value)
	{
		const FVector2D Axis = Value.Get<FVector2D>();
		AddControllerYawInput(Axis.X);
		AddControllerPitchInput(Axis.Y);
	};
	if (LookAction) Input->BindActionValueLambda(LookAction, ETriggerEvent::Triggered, Look);
	if (MouseLookAction) Input->BindActionValueLambda(MouseLookAction, ETriggerEvent::Triggered, Look);
}

void AD0115_Drone::BeginPlay()
{
	Super::BeginPlay();
	GlossaryKit::SetColor(Body, GlossaryKit::Colors::Accent, 0.4f, 0.3f);
	for (UStaticMeshComponent* Rotor : Rotors)
	{
		GlossaryKit::SetColor(Rotor, GlossaryKit::Colors::Dark);
	}
}

void AD0115_Drone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// 조종 중일 때만 프로펠러가 빠르게 돈다
	const float Speed = Controller ? RotorSpeed : RotorSpeed * 0.1f;
	for (int32 i = 0; i < Rotors.Num(); ++i)
	{
		Rotors[i]->AddLocalRotation(FRotator(0.f, (i % 2 ? 1.f : -1.f) * Speed * DeltaSeconds, 0.f));
	}
	// 기울기 연출: 이동 방향으로 살짝 숙인다
	const FVector LocalVel = GetActorRotation().UnrotateVector(GetVelocity());
	Body->SetRelativeRotation(FRotator(-LocalVel.X * 0.015f, 0.f, LocalVel.Y * 0.015f));
}
