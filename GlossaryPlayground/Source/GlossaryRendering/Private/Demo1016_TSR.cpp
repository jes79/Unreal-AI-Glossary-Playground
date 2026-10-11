#include "Demo1016_TSR.h"

#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo1016_TSR::ADemo1016_TSR()
{
	DemoId = TEXT("1016");
	Controls = TEXT("1 : TSR   2 : TAA   3 : FXAA   4 : 안티에일리어싱 없음   5 : Screen Percentage 100 ↔ 67\n")
		TEXT("WASD·QE·←→↑↓ 카메라\n")
		TEXT("관찰: 67% 로 그려도 TSR 은 가는 철사·먼 울타리를 거의 100% 처럼 복원하고 GPU 시간은 줄어든다.\n")
		TEXT("FXAA·없음 에서는 67% 가 흐리거나 계단·깜빡임이 심하다.");
	Showcase = TEXT("가는 철사·먼 울타리·회전 날개, r.AntiAliasingMethod TSR/TAA/FXAA/없음 + r.ScreenPercentage 67 화질·GPU 비교");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Day;
}

void ADemo1016_TSR::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 1) 비스듬한 가는 철사 (계단 현상이 잘 보인다)
	for (int32 i = 0; i < 14; ++i)
	{
		SpawnShape(World, EShape::Cylinder, FVector(900.f, -350.f + i * 40.f, 150.f), FVector(1.5f, 1.5f, 260.f), Colors::Dark, FRotator(0.f, 0.f, 20.f + i * 3.f));
	}
	// 2) 멀리 이어지는 울타리 (먼 곳의 얇은 형상 = 깜빡임)
	for (int32 i = 0; i < 120; ++i)
	{
		SpawnShape(World, EShape::Cube, FVector(800.f + i * 60.f, 700.f, 60.f), FVector(4.f, 4.f, 120.f), Colors::Neutral);
	}
	SpawnShape(World, EShape::Cube, FVector(800.f + 3600.f, 700.f, 110.f), FVector(7200.f, 3.f, 3.f), Colors::Neutral);
	// 3) 회전하는 가는 날개 (움직이는 물체의 잔상 = 시간 누적 AA 의 약점)
	Fan = NewObject<USceneComponent>(this);
	Fan->SetupAttachment(RootComponent);
	Fan->SetRelativeLocation(FVector(700.f, 100.f, 250.f));
	Fan->RegisterComponent();
	for (int32 i = 0; i < 6; ++i)
	{
		AddShape(this, EShape::Cube, FVector(0.f, 0.f, 0.f), FVector(4.f, 400.f, 12.f), Colors::Enemy, Fan, FRotator(0.f, 0.f, i * 30.f));
	}
	// 4) 체크무늬 판 (고주파 무늬)
	for (int32 x = 0; x < 16; ++x)
	{
		for (int32 y = 0; y < 16; ++y)
		{
			if ((x + y) % 2 == 0)
			{
				SpawnShape(World, EShape::Cube, FVector(1500.f + x * 25.f, -800.f + y * 25.f, 1.f), FVector(25.f, 25.f, 2.f), FLinearColor::White);
			}
		}
	}

	Camera = RenderKit::SpawnLookAtCamera(this, FVector(0.f, 0.f, 260.f), FVector(800.f, 50.f, 180.f), 75.f);
	Apply();
}

void ADemo1016_TSR::Apply()
{
	// 핵심 두 줄: 안티에일리어싱 방식과 내부 렌더 해상도 비율
	CVars.Set(TEXT("r.AntiAliasingMethod"), AntiAliasingMethod);
	CVars.Set(TEXT("r.ScreenPercentage"), ScreenPercentage);
}

void ADemo1016_TSR::EndPlay(const EEndPlayReason::Type Reason)
{
	CVars.RestoreAll();
	Super::EndPlay(Reason);
}

void ADemo1016_TSR::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One))   { AntiAliasingMethod = 4; Apply(); }
	if (WasKeyPressed(EKeys::Two))   { AntiAliasingMethod = 2; Apply(); }
	if (WasKeyPressed(EKeys::Three)) { AntiAliasingMethod = 1; Apply(); }
	if (WasKeyPressed(EKeys::Four))  { AntiAliasingMethod = 0; Apply(); }
	if (WasKeyPressed(EKeys::Five))  { ScreenPercentage = ScreenPercentage < 99.f ? 100.f : 67.f; Apply(); }
	RenderKit::TickFlyCamera(this, Camera, DeltaSeconds, 400.f);
	if (Fan)
	{
		Fan->AddLocalRotation(FRotator(0.f, 0.f, 90.f * DeltaSeconds));
	}

	int32 SX = 1600, SY = 900;
	if (APlayerController* PC = GetPC())
	{
		PC->GetViewportSize(SX, SY);
	}
	const TCHAR* Names[] = {TEXT("없음"), TEXT("FXAA"), TEXT("TAA"), TEXT("MSAA"), TEXT("TSR")};
	Stats.Update(DeltaSeconds);
	SetStatus(FString::Printf(TEXT("Anti-Aliasing %s · Screen Percentage %.0f%% → 내부 %d×%d 를 %d×%d 로 업스케일\n%s"),
		Names[FMath::Clamp(AntiAliasingMethod, 0, 4)], ScreenPercentage,
		FMath::RoundToInt(SX * ScreenPercentage / 100.f), FMath::RoundToInt(SY * ScreenPercentage / 100.f), SX, SY, *Stats.ToString(false)));
}
