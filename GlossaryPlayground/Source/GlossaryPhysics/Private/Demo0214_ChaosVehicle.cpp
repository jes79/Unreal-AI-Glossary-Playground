#include "Demo0214_ChaosVehicle.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "D0214_SportsCar.h"
#include "Components/SkeletalMeshComponent.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0214_ChaosVehicle::ADemo0214_ChaosVehicle()
{
	DemoId = TEXT("0214");
	Controls = TEXT("W 가속 · S 브레이크/후진 · A/D 조향 · Space 핸드브레이크\n")
		TEXT("1 : 자동 운전 ON/OFF (WASD 를 누르면 꺼짐) · R : 차 제자리로\n")
		TEXT("관찰: 엔진 토크 → 자동 변속 → 뒷바퀴 구동, 앞바퀴만 조향. 서스펜션이 점프대·상자에 반응");
	Showcase = TEXT("ChaosWheeledVehicleMovementComponent 4륜 스포츠카(템플릿 메시)를 WASD 로 운전, 속도·기어·RPM 표시");
	PlayerPawnClass = AD0214_SportsCar::StaticClass();
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-2000.f, 0.f, 60.f));
	Environment.FloorSize = 20000.f;
}

void ADemo0214_ChaosVehicle::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 콘 슬라럼
	for (int32 i = 0; i < 8; ++i)
	{
		SpawnShape(World, EShape::Cone, FVector(-800.f + i * 450.f, (i % 2 == 0) ? 250.f : -250.f, 40.f), FVector(50.f, 50.f, 80.f), Colors::Accent);
	}
	// 점프대 (경사 12°)
	SpawnShape(World, EShape::Cube, FVector(3300.f, -1000.f, 60.f), FVector(800.f, 500.f, 30.f), Colors::Platform, FRotator(12.f, 0.f, 0.f));
	// 부딪힐 물리 상자 더미
	for (int32 Z = 0; Z < 3; ++Z)
	{
		for (int32 Y = 0; Y < 4 - Z; ++Y)
		{
			SpawnShape(World, EShape::Cube, FVector(3200.f, 900.f + (Y - (3 - Z) * 0.5f) * 105.f, 50.f + Z * 102.f), FVector(100.f),
				FLinearColor::LerpUsingHSV(Colors::Player, Colors::Cyan, Z / 2.f), FRotator::ZeroRotator, true);
		}
	}
	SpawnLabel(World, TEXT("콘 슬라럼"), FVector(-800.f, 0.f, 220.f), Colors::Item, 20.f);
	SpawnLabel(World, TEXT("점프대"), FVector(3300.f, -1000.f, 300.f), Colors::Item, 20.f);
	SpawnLabel(World, TEXT("물리 상자"), FVector(3200.f, 900.f, 420.f), Colors::Item, 20.f);
}

void ADemo0214_ChaosVehicle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AD0214_SportsCar* Car = Cast<AD0214_SportsCar>(GetPlayerPawn());
	if (!Car)
	{
		SetStatus(TEXT("차량 폰이 없습니다 (/Game/Vehicles 콘텐츠 확인)"));
		return;
	}
	if (WasKeyPressed(EKeys::One))
	{
		Car->bAutoPilot = !Car->bAutoPilot;
	}
	if (WasKeyPressed(EKeys::R))
	{
		// 뒤집혔을 때 복구: 텔레포트 + 속도 초기화
		Car->GetMesh()->SetWorldLocationAndRotation(PlayerStartTransform.GetLocation() + FVector(0.f, 0.f, 50.f), FRotator::ZeroRotator, false, nullptr, ETeleportType::ResetPhysics);
		Car->GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Car->GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}

	UChaosWheeledVehicleMovementComponent* Move = Car->GetChaosMovement();
	int32 WheelsOnGround = 0;
	for (int32 i = 0; i < Move->GetNumWheels(); ++i)
	{
		if (Move->GetWheelState(i).bInContact)
		{
			++WheelsOnGround;
		}
	}
	SetStatus(FString::Printf(TEXT("속도 %.0f km/h · 기어 %d · 엔진 %.0f RPM\n접지 바퀴 %d/4 · 자동 운전 %s"),
		Move->GetForwardSpeed() * 0.036f, Move->GetCurrentGear(), Move->GetEngineRotationSpeed(),
		WheelsOnGround, Car->bAutoPilot ? TEXT("ON") : TEXT("OFF")));
}
