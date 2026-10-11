#include "Demo0103_MappingContextSwitch.h"

#include "Components/StaticMeshComponent.h"
#include "D0103_VehicleCharacter.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"
#include "MovementKit.h"
#include "MovementSidePanel.h"

using namespace GlossaryKit;

ADemo0103_MappingContextSwitch::ADemo0103_MappingContextSwitch()
{
	DemoId = TEXT("0103");
	Controls = TEXT("걷기: WASD 이동 · Space 점프 · 차 옆에서 E 타기\n")
		TEXT("운전: W/S 액셀·후진 · A/D 핸들 · Space 경적 · E 내리기\n")
		TEXT("관찰: 같은 키가 IMC_Vehicle(우선순위 1)이 올라가면 다른 액션이 된다. 오른쪽 목록에서 컨텍스트가 생기고 사라진다");
	Showcase = TEXT("차에 타면 IMC_Vehicle 을 우선순위 1로 추가·내리면 제거, 같은 WASD/Space/E 가 운전·경적·내리기로 바뀜, 활성 컨텍스트 목록 표시");
	PlayerPawnClass = AD0103_VehicleCharacter::StaticClass();
}

AActor* ADemo0103_MappingContextSwitch::SpawnCar(const FVector& Location, float Yaw)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* NewCar = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
	USceneComponent* CarRoot = NewObject<USceneComponent>(NewCar, TEXT("CarRoot"));
	CarRoot->SetMobility(EComponentMobility::Movable);
	NewCar->SetRootComponent(CarRoot);
	CarRoot->RegisterComponent();
	NewCar->AddInstanceComponent(CarRoot);
	NewCar->SetActorLocationAndRotation(Location, FRotator(0.f, Yaw, 0.f));

	// 차체 (앞 = +X)
	AddShape(NewCar, EShape::Cube, FVector(0.f, 0.f, 60.f), FVector(360.f, 180.f, 70.f), Colors::Player);
	AddShape(NewCar, EShape::Cube, FVector(-20.f, 0.f, 120.f), FVector(180.f, 160.f, 60.f), Colors::Cyan);
	AddShape(NewCar, EShape::Cube, FVector(175.f, 0.f, 70.f), FVector(15.f, 150.f, 20.f), Colors::Item);   // 헤드라이트 쪽 표시
	for (const FVector2D& W : { FVector2D(110.f, 95.f), FVector2D(110.f, -95.f), FVector2D(-110.f, 95.f), FVector2D(-110.f, -95.f) })
	{
		AddShape(NewCar, EShape::Cylinder, FVector(W.X, W.Y, 35.f), FVector(70.f, 70.f, 30.f), Colors::Dark, nullptr, FRotator(0.f, 0.f, 90.f));
	}
	return NewCar;
}

void ADemo0103_MappingContextSwitch::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	Car = SpawnCar(FVector(0.f, 250.f, 0.f), 0.f);
	if (AD0103_VehicleCharacter* Character = Cast<AD0103_VehicleCharacter>(GetPlayerPawn()))
	{
		Character->SetVehicle(Car);
	}
	Prompt = SpawnLabel(World, TEXT("E : 타기"), FVector(0.f, 250.f, 260.f), Colors::Item, 22.f);

	// 운전해 볼 콘 코스
	for (int32 i = 0; i < 8; ++i)
	{
		SpawnShape(World, EShape::Cone, FVector(900.f + i * 350.f, (i % 2 == 0) ? 250.f : -250.f, 45.f), FVector(60.f, 60.f, 90.f), Colors::Accent);
	}
	SpawnLabel(World, TEXT("콘 코스 — 운전 조작(IMC_Vehicle)으로 지나가 보세요"), FVector(1500.f, 0.f, 220.f), FLinearColor::White, 18.f);

	Panel = UMovementSidePanel::Create(GetPC(), TEXT("활성 Input Mapping Context"), 460.f);
}

void ADemo0103_MappingContextSwitch::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Panel)
	{
		Panel->RemoveFromParent();
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0103_MappingContextSwitch::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AD0103_VehicleCharacter* Character = Cast<AD0103_VehicleCharacter>(GetPlayerPawn());
	if (!Character)
	{
		return;
	}
	const bool bDriving = Character->IsDriving();

	if (Prompt && Car)
	{
		Prompt->SetActorHiddenInGame(bDriving || !Character->IsNearVehicle());
		Prompt->SetActorLocation(Car->GetActorLocation() + FVector(0.f, 0.f, 260.f));
	}

	if (Panel)
	{
		const FString Contexts = MovementKit::DescribeContexts(GetPC(), {
			{ TEXT("IMC_Vehicle (실행 중 생성)"), Character->GetVehicleContext() },
			{ TEXT("IMC_Default (템플릿 /Game/Input)"), Character->GetDefaultContext() },
			{ TEXT("IMC_MouseLook (템플릿)"), Character->GetMouseContext() },
			{ TEXT("IMC_OnFoot (실행 중 생성)"), Character->GetOnFootContext() },
		});
		const FString Keys = bDriving
			? TEXT("W/S → IA_Drive (액셀)\nA/D → IA_Drive (핸들)\nSpace → IA_Horn\nE → IA_Exit")
			: TEXT("WASD → IA_Move\nSpace → IA_Jump\nE → IA_Enter");
		Panel->SetBody(FString::Printf(TEXT("%s\n\n지금 키의 의미 (우선순위가 높은 컨텍스트가 먼저)\n%s"), *Contexts, *Keys));
	}

	SetStatus(FString::Printf(TEXT("%s · 마지막 액션 %s\n%s"),
		bDriving ? TEXT("운전 중 (IMC_Vehicle 우선순위 1)") : TEXT("걷는 중"),
		Character->GetLastAction().IsEmpty() ? TEXT("-") : *Character->GetLastAction(),
		bDriving ? *FString::Printf(TEXT("Throttle %.2f · Steer %.2f · 속도 %.0f cm/s"), Character->GetThrottle(), Character->GetSteer(), Character->GetVelocity().Size2D())
		         : (Character->IsNearVehicle() ? TEXT("차 옆: E 를 누르면 탑니다") : TEXT("파란 차로 가세요"))));
}
