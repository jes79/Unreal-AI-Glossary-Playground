#include "D0214_SportsCar.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"

UD0214_WheelFront::UD0214_WheelFront()
{
	AxleType = EAxleType::Front;
	bAffectedBySteering = true;      // 앞바퀴만 조향
	MaxSteerAngle = 40.f;
	WheelRadius = 39.f;
	WheelWidth = 35.f;
	FrictionForceMultiplier = 3.f;
	MaxBrakeTorque = 4500.f;
	MaxHandBrakeTorque = 6000.f;
}

UD0214_WheelRear::UD0214_WheelRear()
{
	AxleType = EAxleType::Rear;
	bAffectedByEngine = true;        // 후륜 구동
	bAffectedByHandbrake = true;
	MaxSteerAngle = 0.f;
	WheelRadius = 40.f;
	WheelWidth = 40.f;
	FrictionForceMultiplier = 4.f;
	SlipThreshold = 100.f;
	SkidThreshold = 100.f;
	MaxHandBrakeTorque = 6000.f;
}

AD0214_SportsCar::AD0214_SportsCar()
{
	// 템플릿 차량 메시 + 바퀴 회전 애님 BP (복사해 둔 /Game/Vehicles/SportsCar)
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CarMesh(TEXT("/Game/Vehicles/SportsCar/SKM_SportsCar"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> CarAnim(TEXT("/Game/Vehicles/SportsCar/ABP_SportsCar"));
	if (CarMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(CarMesh.Object);
	}
	if (CarAnim.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(CarAnim.Class);
	}
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(TEXT("Vehicle"));

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(GetMesh());
	SpringArm->TargetArmLength = 650.f;
	SpringArm->SocketOffset.Z = 150.f;
	SpringArm->bDoCollisionTest = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritRoll = false;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraRotationLagSpeed = 2.f;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	// ---------- 핵심: Chaos Wheeled Vehicle Movement 설정 ----------
	ChaosMovement = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());
	ChaosMovement->ChassisHeight = 144.f;
	ChaosMovement->DragCoefficient = 0.31f;
	ChaosMovement->bLegacyWheelFrictionPosition = true;

	// 바퀴 4개: 클래스(앞/뒤) + 스켈레탈 메시의 바퀴 뼈 이름
	ChaosMovement->WheelSetups.SetNum(4);
	const TCHAR* Bones[4] = { TEXT("Phys_Wheel_FL"), TEXT("Phys_Wheel_FR"), TEXT("Phys_Wheel_BL"), TEXT("Phys_Wheel_BR") };
	for (int32 i = 0; i < 4; ++i)
	{
		ChaosMovement->WheelSetups[i].WheelClass = i < 2 ? UD0214_WheelFront::StaticClass() : UD0214_WheelRear::StaticClass();
		ChaosMovement->WheelSetups[i].BoneName = FName(Bones[i]);
		ChaosMovement->WheelSetups[i].AdditionalOffset = FVector::ZeroVector;
	}

	// 엔진: 최대 토크 × 토크 커브(RPM 별 비율)
	ChaosMovement->EngineSetup.MaxTorque = 750.f;
	ChaosMovement->EngineSetup.MaxRPM = 7000.f;
	ChaosMovement->EngineSetup.EngineIdleRPM = 900.f;
	ChaosMovement->EngineSetup.EngineBrakeEffect = 0.2f;
	ChaosMovement->EngineSetup.EngineRevUpMOI = 5.f;
	ChaosMovement->EngineSetup.EngineRevDownRate = 600.f;
	FRichCurve* Torque = ChaosMovement->EngineSetup.TorqueCurve.GetRichCurve();
	Torque->Reset();
	Torque->AddKey(0.f, 400.f);
	Torque->AddKey(1890.f, 500.f);
	Torque->AddKey(5730.f, 400.f);
	Torque->AddKey(7000.f, 300.f);

	// 변속기: 자동 5단 + 자동 후진
	ChaosMovement->TransmissionSetup.bUseAutomaticGears = true;
	ChaosMovement->TransmissionSetup.bUseAutoReverse = true;
	ChaosMovement->TransmissionSetup.FinalRatio = 2.81f;
	ChaosMovement->TransmissionSetup.ChangeUpRPM = 6000.f;
	ChaosMovement->TransmissionSetup.ChangeDownRPM = 2000.f;
	ChaosMovement->TransmissionSetup.GearChangeTime = 0.2f;
	ChaosMovement->TransmissionSetup.TransmissionEfficiency = 0.9f;
	ChaosMovement->TransmissionSetup.ForwardGearRatios = { 4.25f, 2.52f, 1.66f, 1.22f, 1.f };
	ChaosMovement->TransmissionSetup.ReverseGearRatios = { 4.04f };

	// 조향: Ackermann, 속도가 빠를수록 덜 꺾이게
	ChaosMovement->SteeringSetup.SteeringType = ESteeringType::Ackermann;
	ChaosMovement->SteeringSetup.AngleRatio = 0.7f;
	FRichCurve* Steering = ChaosMovement->SteeringSetup.SteeringCurve.GetRichCurve();
	Steering->Reset();
	Steering->AddKey(0.f, 1.f);
	Steering->AddKey(40.f, 0.7f);
	Steering->AddKey(120.f, 0.6f);

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMC(TEXT("/Game/Input/IMC_Default"));
	static ConstructorHelpers::FObjectFinder<UInputAction> IAMove(TEXT("/Game/Input/Actions/IA_Move"));
	static ConstructorHelpers::FObjectFinder<UInputAction> IAJump(TEXT("/Game/Input/Actions/IA_Jump"));
	MappingContext = IMC.Object;
	MoveAction = IAMove.Object;
	HandbrakeAction = IAJump.Object;
}

void AD0214_SportsCar::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (MappingContext)
			{
				Subsystem->AddMappingContext(MappingContext, 0);
			}
		}
	}
}

void AD0214_SportsCar::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AD0214_SportsCar::Move);
			Input->BindAction(MoveAction, ETriggerEvent::Completed, this, &AD0214_SportsCar::MoveCompleted);
		}
		if (HandbrakeAction)
		{
			Input->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &AD0214_SportsCar::HandbrakeOn);
			Input->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &AD0214_SportsCar::HandbrakeOff);
		}
	}
}

void AD0214_SportsCar::Move(const FInputActionValue& Value)
{
	bAutoPilot = false;
	const FVector2D Axis = Value.Get<FVector2D>();
	// W/S → 스로틀/브레이크(정지 상태에서 S 를 누르면 자동 후진), A/D → 조향
	ChaosMovement->SetThrottleInput(FMath::Max(Axis.Y, 0.f));
	ChaosMovement->SetBrakeInput(FMath::Max(-Axis.Y, 0.f));
	ChaosMovement->SetSteeringInput(Axis.X);
}

void AD0214_SportsCar::MoveCompleted(const FInputActionValue& Value)
{
	ChaosMovement->SetThrottleInput(0.f);
	ChaosMovement->SetBrakeInput(0.f);
	ChaosMovement->SetSteeringInput(0.f);
}

void AD0214_SportsCar::HandbrakeOn()
{
	ChaosMovement->SetHandbrakeInput(true);
}

void AD0214_SportsCar::HandbrakeOff()
{
	ChaosMovement->SetHandbrakeInput(false);
}

void AD0214_SportsCar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 공중에서는 회전 감쇠를 키워 뒤집힘 방지 (템플릿과 같은 처리)
	GetMesh()->SetAngularDamping(ChaosMovement->IsMovingOnGround() ? 0.f : 3.f);

	// 카메라를 천천히 차 뒤쪽으로 되돌린다
	const float Yaw = FMath::FInterpTo(SpringArm->GetRelativeRotation().Yaw, 0.f, DeltaSeconds, 1.f);
	SpringArm->SetRelativeRotation(FRotator(0.f, Yaw, 0.f));

	if (bAutoPilot)
	{
		// 자동 운전: 가속하면서 천천히 S 자로 조향
		AutoTime += DeltaSeconds;
		ChaosMovement->SetThrottleInput(0.8f);
		ChaosMovement->SetBrakeInput(0.f);
		ChaosMovement->SetSteeringInput(FMath::Sin(AutoTime * 0.6f) * 0.5f);
	}
}
