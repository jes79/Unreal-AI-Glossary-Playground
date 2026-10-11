#include "Demo0525_MotionBlur.h"

#include "Camera/CameraActor.h"
#include "EffectsDemoKit.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

namespace
{
	constexpr float TrackRadius = 2000.f;
}

ADemo0525_MotionBlur::ADemo0525_MotionBlur()
{
	DemoId = TEXT("0525");
	Controls = TEXT("M : Motion Blur ON/OFF (게임 옵션의 '모션 블러' 토글과 같은 역할)   [ / ] : 차 속도 -/+\n")
		TEXT("관찰: 카메라가 시속 108km 로 달리는 차를 따라가므로 차는 또렷하고 옆을 지나가는 기둥이 길게 번진다.\n")
		TEXT("Motion Blur Amount 0.5 = 셔터가 프레임 시간의 절반 동안 열린 것과 같은 번짐");
	Showcase = TEXT("빠르게 달리는 차를 따라가는 카메라, Motion Blur Amount 0.5 로 배경이 흐르는 속도감, 옵션처럼 ON/OFF");
	PlayerPawnClass = nullptr;
}

void ADemo0525_MotionBlur::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 원형 트랙 양옆에 기둥 (지나가는 속도가 보이도록 촘촘히)
	for (int32 i = 0; i < 72; ++i)
	{
		const float A = i * 5.f * PI / 180.f;
		const FLinearColor Color = i % 2 == 0 ? Colors::Item : Colors::Enemy;
		SpawnBlock(World, FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * (TrackRadius + 400.f), FVector(60.f, 60.f, 300.f), Color);
		SpawnBlock(World, FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * (TrackRadius - 400.f), FVector(60.f, 60.f, 200.f), Colors::Wall);
	}

	// 차: 몸통 + 지붕 (한 액터에 묶음)
	AStaticMeshActor* Body = SpawnShape(World, EShape::Cube, FVector::ZeroVector, FVector(420.f, 180.f, 70.f), Colors::Enemy);
	AddShape(Body, EShape::Cube, FVector(-30.f, 0.f, 60.f), FVector(220.f, 160.f, 60.f), Colors::Dark);
	for (int32 i = 0; i < 4; ++i)
	{
		AddShape(Body, EShape::Cylinder, FVector(i < 2 ? 140.f : -140.f, i % 2 == 0 ? 95.f : -95.f, -30.f), FVector(70.f, 70.f, 30.f), Colors::Dark, nullptr, FRotator(0.f, 0.f, 90.f));
	}
	Car = Body;

	ChaseCamera = EffectsKit::SetupFixedCamera(this, FVector(0.f, 0.f, 300.f), FVector::ZeroVector, 75.f);

	Volume = EffectsKit::SpawnUnboundPostProcess(World);
	Volume->Settings.bOverride_MotionBlurAmount = true;
	Volume->Settings.bOverride_MotionBlurMax = true;
	Volume->Settings.MotionBlurAmount = MotionBlurAmount;
	Volume->Settings.MotionBlurMax = 10.f;   // 화면 폭 대비 최대 번짐 %
}

void ADemo0525_MotionBlur::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::M)) bBlurOn = !bBlurOn;
	if (WasKeyPressed(EKeys::LeftBracket))  CarSpeed = FMath::Max(500.f, CarSpeed - 500.f);
	if (WasKeyPressed(EKeys::RightBracket)) CarSpeed = FMath::Min(6000.f, CarSpeed + 500.f);

	Angle += CarSpeed / TrackRadius * DeltaSeconds;
	const FVector Dir(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
	const FVector Forward(-FMath::Sin(Angle), FMath::Cos(Angle), 0.f);
	const FVector CarPos = Dir * TrackRadius + FVector(0.f, 0.f, 60.f);
	if (Car)
	{
		Car->SetActorLocationAndRotation(CarPos, Forward.Rotation());
	}
	if (ChaseCamera)
	{
		// 차 뒤쪽 바깥에서 따라가며 차를 본다
		const FVector CamPos = CarPos - Forward * 600.f + Dir * 350.f + FVector(0.f, 0.f, 220.f);
		ChaseCamera->SetActorLocationAndRotation(CamPos, (CarPos + Forward * 200.f - CamPos).Rotation());
	}
	if (Volume)
	{
		Volume->Settings.MotionBlurAmount = bBlurOn ? MotionBlurAmount : 0.f;   // 옵션 OFF = Amount 0
	}
	SetStatus(FString::Printf(TEXT("Motion Blur %s · Amount %.1f · 차 속도 %.0f cm/s (시속 %.0f km)"),
		bBlurOn ? TEXT("ON") : TEXT("OFF"), bBlurOn ? MotionBlurAmount : 0.f, CarSpeed, CarSpeed * 0.036f));
}
